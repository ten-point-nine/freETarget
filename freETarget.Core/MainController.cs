using freETarget.Properties;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace freETarget {

    /** the questions and messages the controller needs to put in front of the user */
    public interface IUserDialogs {
        void showError(string text, string title);
        void showWarning(string text, string title);
        /** OK / Cancel question. true for OK */
        bool confirm(string text, string title);
        bool askYesNo(string text, string title);
        /** true for yes, false for no, null for cancel */
        bool? askYesNoCancel(string text, string title);
    }

    /**
     * everything the main window does that is not drawing: startup, connection to the target, the running session,
     * handling of what the target sends, rapid fire series, recovery after a crash.
     * the UI calls the methods and listens to the events. all methods must be called on the UI thread;
     * what arrives from the target on other threads is brought to the UI thread before anything changes, so the events are raised there too
     */
    public class MainController : IAppHost, IDisposable {

        public enum Status {
            NOT_CONNECTED,
            CONECTING,
            CONNECTED,
            LOADED      //a saved session is displayed
        }

        public const string Arduino = "Arduino";
        public const string ESP32 = "ESP32";

        private const string saveQuestion = "Current session is unsaved. Do you want to save it?";

        private readonly IUserDialogs dialogs;
        private readonly Action<Action> uiThread;

        public Logger logger { get; private set; }
        public StorageController storage { get; private set; }
        public EventManager eventManager { get; private set; }

        public Status currentStatus { get; private set; } = Status.NOT_CONNECTED;
        public Session currentSession { get; private set; }
        /** the event of the current session, or the one the next session will get */
        public Event selectedEvent { get; private set; }
        public string statusText { get; private set; } = "";
        /** a TCP connection dropped and is being reopened */
        public bool connectionLost { get; private set; }
        /** the settings that the target reported last. null when not connected */
        public Echo lastEcho { get; private set; }
        public bool rapidFireRunning { get; private set; }

        public decimal calibrationX = 0;
        public decimal calibrationY = 0;
        public decimal calibrationAngle = 0;

        public comms.aCommModule commModule { get; private set; }

        /** creates the comm module for the protocol in the settings. replaceable for tests */
        public Func<IAppHost, comms.aCommModule> commFactory = host => {
            if (Settings.Default.CommProtocol == "USB") {
                return new comms.USB(host);
            } else {
                return new comms.TCP(host);
            }
        };

        /** miliseconds to wait after the first text from the target before the version is requested, and after that request */
        public int connectDelay = 500;
        public int versionDelay = 100;
        public int reconnectInterval = 2000;

        /** connection status, status text or the rapid fire state changed */
        public event Action StatusChanged;
        /** a new session started. everything that shows shots must be cleared */
        public event Action SessionStarted;
        /** a saved session is shown for viewing, or was put away again. everything that shows the session must be filled again */
        public event Action SessionLoaded;
        /** a saved session was loaded to be continued. everything that shows the session must be filled again */
        public event Action SessionResumed;
        /** the shots of the session were removed */
        public event Action ShotsCleared;
        public event Action<Shot> ShotAdded;
        /** the series to display or the statistics changed */
        public event Action ShotListChanged;
        /** a line for the message area. true to also show it as a hint over the target */
        public event Action<string, bool> MessageDisplayed;
        /** raw text from the target */
        public event Action<string> DataReceived;
        /** the text of the terminal window changed */
        public event Action TerminalChanged;
        public event Action EchoReceived;
        /** events were added or changed, or other events are active now */
        public event Action EventsChanged;
        /** the scores of the shots were computed again (calibration). everything that shows shots must be filled again */
        public event Action ShotsRecalculated;
        /** the calibration values changed */
        public event Action CalibrationChanged;

        private readonly MessageReader messageReader = new MessageReader();
        private readonly System.Text.StringBuilder terminal = new System.Text.StringBuilder();
        private const int terminalLimit = 200000; //characters kept for the terminal window
        private RapidFireSequencer rapidFire;
        private Timer rapidFireTimer;
        private Timer reconnectTimer;
        private bool reconnecting;
        private bool handshakeStarted;
        private bool seriesSelected = false;

        private string recoverySessionName = "";
        private string recoverySessionTime = "";
        private List<string> recoveryLines;

        public MainController(IUserDialogs dialogs, Action<Action> uiThread) {
            this.dialogs = dialogs;
            this.uiThread = uiThread;
        }

        //---------------------------------------------------------------- startup

        /**
         * opens the log and the database, loads settings and events.
         * returns null, or the reason why the application cannot run
         */
        public string init(int requiredDatabaseVersion, string versionText, string databaseDirectory = null, string logDirectory = null) {
            logger = new Logger(logDirectory);
            if (Settings.Default.fileLogging) {
                displayMessage("Log location: " + logger.logFile, false);
            }

            bool cleanEnd = logger.logFile == null || SessionRecovery.wasCleanShutdown(logger.logFile);

            log(SessionRecovery.StartupMarker + "\n\nStarting freETarget - " + versionText + " ... ");

            storage = databaseDirectory == null ? new StorageController(this) : new StorageController(this, databaseDirectory);
            string testDB = storage.checkDB(requiredDatabaseVersion);
            if (testDB != null) {
                return "Database check failed. Please check your installation. " + Environment.NewLine + Environment.NewLine + testDB;
            }

            SettingsStore.init(storage, Settings.Default);
            log("Loading settings from DB");
            SettingsStore.load(storage, Settings.Default);

            eventManager = new EventManager(storage);
            if (storage.checkEvents() == 0) {
                log("Empty events table. Creating default events...");
                eventManager.initializeEvents();
            }
            eventManager.setEventsList(storage.loadEvents());
            eventManager.setActiveEventsList(storage.loadActiveEventsIDs());

            List<string> names = new List<string>();
            foreach (Event e in eventManager.getEventsList()) {
                names.Add(e.ToString());
            }
            log("Events loaded: " + string.Join(" | ", names));

            this.calibrationX = Settings.Default.calibrationX;
            this.calibrationY = Settings.Default.calibrationY;
            this.calibrationAngle = Settings.Default.calibrationAngle;

            if (cleanEnd == false) {
                askForRecovery();
            }

            return null;
        }

        private void askForRecovery() {
            bool recover = dialogs.askYesNo("freETarget has detected that the program did not shutdown corectly."
                    + Environment.NewLine + Environment.NewLine
                    + "Do you want to try to recover the shots of the last session from the log?"
                    + Environment.NewLine + Environment.NewLine
                    + "Recovery will happen after the PC client connects to the target."
                   , "Abnormal shutdown detected");

            if (recover) {
                RecoveredSession recovered = SessionRecovery.read(logger.logFile);
                if (recovered.sessionName == "") {
                    log("No session to recover");
                    return;
                }
                recoverySessionTime = recovered.sessionTime;
                recoverySessionName = recovered.sessionName;
                recoveryLines = recovered.lines;

                log("Recovering session '" + recovered.sessionName + "' that started at <" + recoverySessionTime + "> ... after connect");
            }
        }

        /** starts a session of the default event of the settings. called once when the main window opens */
        public void startDefaultSession() {
            Event ev = eventManager.findEventByName(Settings.Default.defaultTarget.Trim());

            if (ev == null) {
                dialogs.showError("Default event is not available: " + Settings.Default.defaultTarget.Trim(), "Configuration error");
                return;
            }

            selectEvent(ev);
        }

        //---------------------------------------------------------------- IAppHost

        public void log(string s) {
            logger.log(s);
        }

        public void displayMessage(string text, bool showToolTip) {
            Console.WriteLine(text);
            uiThread(() => MessageDisplayed?.Invoke(text, showToolTip));
        }

        public void showError(string text, string title) {
            dialogs.showError(text, title);
        }

        public void showWarning(string text, string title) {
            dialogs.showWarning(text, title);
        }

        public bool confirm(string text, string title) {
            return dialogs.confirm(text, title);
        }

        public Event findEventByID(long id) {
            return eventManager.findEventByID(id);
        }

        //---------------------------------------------------------------- settings

        public void saveSettings() {
            SettingsStore.save(storage, Settings.Default);
        }

        //---------------------------------------------------------------- events

        /**
         * saves an edited event: a new one if existing is null, otherwise the changes to that event.
         * returns the problems with the fields, or an empty text if it was saved
         */
        public string saveEvent(EventDraft draft, Event existing) {
            string errors = draft.validate();
            if (errors.Length > 0) {
                return errors;
            }

            if (existing != null) {
                draft.applyTo(existing);
                storage.updateEvent(existing);
            } else {
                storage.storeEvent(draft.create());
            }
            reloadEvents();
            return "";
        }

        /** which events are offered in the main window, in this order */
        public void setActiveEvents(List<Event> events) {
            storage.updateActiveEvents(events);
            reloadEvents();
        }

        //reads the events from the database again and puts the idle session on the current data of its event
        private void reloadEvents() {
            eventManager.setEventsList(storage.loadEvents());
            eventManager.setActiveEventsList(storage.loadActiveEventsIDs());
            EventsChanged?.Invoke();

            if (currentSession != null && currentSession.Shots.Count == 0 && currentStatus == Status.NOT_CONNECTED && selectedEvent != null) {
                Event current = eventManager.findEventByID(selectedEvent.ID);
                if (current == null || !eventManager.getActiveEventsList().Contains(current)) {
                    List<Event> active = eventManager.getActiveEventsList();
                    current = active.Count > 0 ? active[0] : current;
                }
                if (current != null) {
                    selectEvent(current);
                }
            }
        }

        public bool isCalibrated() {
            return calibrationX != 0 || calibrationY != 0 || calibrationAngle != 0;
        }

        /**
         * moves and turns the shots: the calibration is changed by these amounts (milimeters and degrees)
         * and all shots of the session are scored again. applies to the shots that follow too
         */
        public void calibrate(decimal dx, decimal dy, decimal dAngle) {
            setCalibration(calibrationX + dx, calibrationY + dy, calibrationAngle + dAngle);
        }

        public void resetCalibration() {
            setCalibration(0, 0, 0);
        }

        private void setCalibration(decimal x, decimal y, decimal angle) {
            calibrationX = x;
            calibrationY = y;
            calibrationAngle = angle;

            if (currentSession != null) {
                int sessionScore = 0;
                decimal sessionDecimalScore = 0;
                int sessionInnerTens = 0;

                foreach (Shot shot in getShots()) {
                    shot.calibrationX = calibrationX;
                    shot.calibrationY = calibrationY;
                    shot.calibrationAngle = calibrationAngle;
                    shot.computeScore(currentSession.getTarget());

                    sessionScore += shot.score;
                    sessionDecimalScore += shot.decimalScore;
                    if (shot.innerTen) {
                        sessionInnerTens++;
                    }
                }

                currentSession.score = sessionScore;
                currentSession.decimalScore = sessionDecimalScore;
                currentSession.innerX = sessionInnerTens;
                updateStatistics();
            }

            CalibrationChanged?.Invoke();
            ShotsRecalculated?.Invoke();
        }

        public void saveCalibration() {
            Settings.Default.calibrationX = calibrationX;
            Settings.Default.calibrationY = calibrationY;
            Settings.Default.calibrationAngle = calibrationAngle;
            saveSettings();
        }

        //---------------------------------------------------------------- connection

        private void setStatus(Status status, string text) {
            currentStatus = status;
            if (text != null) {
                statusText = text;
            }
            StatusChanged?.Invoke();
        }

        /** what the connect button does: connects, or closes the session and disconnects */
        public Task toggleConnection() {
            if (currentStatus == Status.NOT_CONNECTED || currentStatus == Status.LOADED) {
                return connect();
            } else {
                if (!clearShots()) {
                    disconnect();
                }
                return Task.CompletedTask;
            }
        }

        private bool checkConnectionSettings() {
            if (Settings.Default.CommProtocol == "USB") {
                if (Settings.Default.portName == null || Settings.Default.portName.Trim() == "") {
                    dialogs.showWarning("No COM port selected. Please go to the Settings dialog and select a port.", "Cannot connect");
                    return false;
                }
            } else {
                //TCP
                if (Settings.Default.TcpIP == null || Settings.Default.TcpIP.Trim() == "") {
                    dialogs.showWarning("No TCP IP address entered. Please go to the Settings dialog and enter an IP address.", "Cannot connect");
                    return false;
                }

                if (Settings.Default.TcpPort <= 0) {
                    dialogs.showWarning("No TCP port entered. Please go to the Settings dialog and enter an TCP port.", "Cannot connect");
                    return false;
                }
            }

            if (Settings.Default.name == null || Settings.Default.name.Trim() == "") {
                dialogs.showWarning("No shooter name entered. Please go to the Settings dialog and enter a name.", "Cannot connect");
                return false;
            }
            return true;
        }

        private comms.OpenParams getOpenParams() {
            if (Settings.Default.CommProtocol == "USB") {
                comms.UsbOpenParams para = new comms.UsbOpenParams();
                para.portName = Settings.Default.portName;
                para.baudRate = Settings.Default.baudRate;
                para.dataBits = 8;
                para.dtrEnable = true; //both ESP and Arduino work with the same settings
                para.rtsEnable = true;
                return para;
            } else {
                comms.TcpOpenParams para = new comms.TcpOpenParams();
                para.IP = Settings.Default.TcpIP;
                para.port = Settings.Default.TcpPort;
                return para;
            }
        }

        /**
         * opens the connection. the target sends a text "freETarget VX.X" on connect, the connection is established when that arrives.
         * opening can take a while (network), so it does not block the UI: the returned task ends when the port is open or failed
         */
        public async Task connect() {
            if (currentStatus != Status.NOT_CONNECTED && currentStatus != Status.LOADED) {
                return;
            }
            if (!checkConnectionSettings()) {
                return;
            }
            //a saved session that is shown for viewing makes room for the new session
            unloadJournalSession();

            comms.aCommModule module = commFactory(this);
            module.CommDataReceivedEvent += onDataReceived;
            module.CommDisconnectedEvent += onDisconnected;
            this.commModule = module;
            comms.OpenParams para = getOpenParams();

            handshakeStarted = false;
            setStatus(Status.CONECTING, "Connecting...");

            try {
                await Task.Run(() => module.open(para));
            } catch (Exception ex) {
                if (this.commModule != module) {
                    return; //cancelled in the meantime
                }
                log("Error connecting: " + ex.Message);
                this.commModule = null;
                setStatus(Status.NOT_CONNECTED, "Error opening comms: " + ex.Message);
                dialogs.showError("Failed to connect to the target", "Connecting error");
            }
        }

        //called by the comm module on one of its threads
        private void onDataReceived(object sender, comms.CommEventArgs e) {
            if (currentStatus == Status.CONECTING && !handshakeStarted) {
                //first incoming text from target after port open is "freETarget VX.x" - use this to confirm connection
                handshakeStarted = true;
                string greeting = e.Text.Trim().Replace('\0', ' ');
                Thread.Sleep(connectDelay);
                uiThread(() => connectDone(sender, greeting));
            }
            uiThread(() => handleData(sender, e.Text));
        }

        /** received connection text from target. connection established */
        private void connectDone(object sender, string target) {
            if (sender != commModule || currentStatus != Status.CONECTING) {
                return; //disconnected in the meantime
            }

            send("{\"VERSION\":7}");
            Thread.Sleep(versionDelay);

            String t = target;
            if (t.IndexOf(Environment.NewLine, StringComparison.Ordinal) > -1) {
                t = t.Substring(0, t.IndexOf(Environment.NewLine, StringComparison.Ordinal));
            }
            t = t.Trim();
            connectionLost = false;
            setStatus(Status.CONNECTED, "Connected to " + t + " on " + commModule.getCommInfo());
            log("****************\nConnected to " + t + " on " + commModule.getCommInfo());
            displayMessage("Connected to " + t, false);

            initNewSession();

            if (recoveryLines != null) {
                //there are shots to recover
                log("Recovering shots...");
                List<string> lines = recoveryLines;
                string time = recoverySessionTime;
                recoveryLines = null;
                recoverySessionName = "";
                recoverySessionTime = "";

                foreach (string line in lines) {
                    handleData(commModule, line);
                }

                currentSession.startTime = DateTime.ParseExact(time, Logger.TimestampFormat, CultureInfo.InvariantCulture);
                currentSession.continueSession();
            }
        }

        private void handleData(object sender, string text) {
            if (sender != commModule) {
                return; //from a connection that was closed in the meantime
            }

            string indata = text.Trim().Replace('\0', ' ');

            DataReceived?.Invoke(indata);
            appendToTerminal(indata);
            log(indata);

            if (currentSession == null) {
                return;
            }

            messageReader.append(indata);

            //there might be more than one complete message in there
            string message;
            while ((message = messageReader.next()) != null) {
                switch (TargetMessage.classify(message)) {
                    case TargetMessage.Kind.Shot:
                        Shot shot = TargetMessage.parseShot(message, calibrationX, calibrationY, calibrationAngle, currentSession.getTarget());
                        if (shot.count >= 0) {
                            handleShot(shot, message);
                        } else {
                            handleEcho(message);
                        }
                        break;
                    case TargetMessage.Kind.BadShot:
                        displayMessage("Bad shot reported by the target: " + message, false);
                        handleEcho(message);
                        break;
                    default:
                        handleEcho(message);
                        break;
                }
            }
        }

        private void handleShot(Shot shot, string message) {
            if (shot.miss == true) {
                if (Settings.Default.ignoreMiss == true) {
                    return;
                }
                currentSession.addShot(shot);
                displayMessage(message, false);
            } else {
                currentSession.addShot(shot);
                displayMessage(message + " ---- " + "Computed shot X:" + shot.getX() + " Y:" + shot.getY() + " R:" + shot.radius + " A:" + shot.angle, false);
                logger.log("{\"shot\":" + shot.count + " \"x\":" + shot.getX().ToString(CultureInfo.InvariantCulture) + " \"y\":" + shot.getY().ToString(CultureInfo.InvariantCulture)
                    + " \"score\":" + shot.score + " \"decimal\":" + shot.decimalScore.ToString(CultureInfo.InvariantCulture) + "}", logger.shotFile, true);
            }

            updateStatistics();
            ShotAdded?.Invoke(shot);
            new VirtualRO(currentSession).speakShot(shot);
        }

        private void handleEcho(string message) {
            lastEcho = Echo.parseJson(message);
            log("Echo received =   " + (lastEcho != null ? lastEcho.ToString() : "null"));

            //if target has OFFSET values, override local calibration -> set it to 0
            if (lastEcho != null) {
                if (lastEcho.X_OFFSET != decimal.Zero || lastEcho.Y_OFFSET != decimal.Zero) {
                    this.calibrationX = 0;
                    this.calibrationY = 0;
                    log("ECHO returned non-zero OFFSETS  X:" + lastEcho.X_OFFSET + " Y:" + lastEcho.Y_OFFSET + ". Reseting local calibration to zero... ");
                    CalibrationChanged?.Invoke();
                }
            }
            EchoReceived?.Invoke();
        }

        /** sends a command to the target and logs it */
        public void send(string command) {
            if (commModule != null) {
                commModule.sendData(command);
                log("Sending: " + command);
                appendToTerminal("> " + command);
            }
        }

        //---------------------------------------------------------------- terminal

        /**
         * everything the target sent since the application started, and the commands that were sent to it (marked with ">").
         * the oldest text is dropped when it gets too long
         */
        public string terminalText {
            get { return terminal.ToString(); }
        }

        private void appendToTerminal(string text) {
            if (string.IsNullOrEmpty(text)) {
                return;
            }
            terminal.Append(text).Append(Environment.NewLine);
            if (terminal.Length > terminalLimit) {
                terminal.Remove(0, terminal.Length - terminalLimit / 2);
            }
            TerminalChanged?.Invoke();
        }

        /**
         * sends a command that sets one value in the target, for example LED_BRIGHT 50.
         * returns null, or why it was not sent
         */
        public string sendCommand(string name, string parameter) {
            if (commModule == null || currentStatus != Status.CONNECTED) {
                return "The target is not connected";
            }
            if (string.IsNullOrWhiteSpace(name)) {
                return "No command selected";
            }
            if (string.IsNullOrWhiteSpace(parameter)) {
                return "Cannot send empty value";
            }
            send("{\"" + name.Trim() + "\":" + parameter.Trim() + "}");
            return null;
        }

        /** sends a text to the target exactly as it is. returns null, or why it was not sent */
        public string sendRaw(string text) {
            if (commModule == null || currentStatus != Status.CONNECTED) {
                return "The target is not connected";
            }
            if (string.IsNullOrEmpty(text)) {
                return "Nothing to send";
            }
            send(text);
            return null;
        }

        /** asks the target for its settings. the answer arrives as lastEcho with the EchoReceived event */
        public void requestEcho() {
            lastEcho = null;
            send("{\"ECHO\":0}");
        }

        /**
         * stores the settings in the target, which reports all its settings back afterwards.
         * returns null, or why the settings cannot be sent
         */
        public string applyTargetSettings(TargetSettings settings) {
            if (commModule == null || currentStatus != Status.CONNECTED) {
                return "The target is not connected";
            }
            string error = settings.validate();
            if (error != null) {
                return error;
            }
            send(settings.toCommand());
            return null;
        }

        //called by the TCP module on one of its threads when the connection drops
        private void onDisconnected(object sender, comms.CommEventArgs e) {
            uiThread(() => {
                if (sender != commModule) {
                    return;
                }
                connectionLost = true;
                StatusChanged?.Invoke();
                if (reconnectTimer == null) {
                    reconnectTimer = new Timer(_ => uiThread(reconnect), null, reconnectInterval, reconnectInterval);
                }
            });
        }

        private async void reconnect() {
            comms.aCommModule module = commModule;
            if (module == null || !connectionLost || reconnecting) {
                return;
            }
            reconnecting = true;
            try {
                log("Attempting reconnect...");
                comms.OpenParams para = getOpenParams();
                await Task.Run(() => module.open(para));
                if (module != commModule) {
                    module.close(); //disconnected by the user while reconnecting
                    return;
                }
                log("Reconnect succesfull.");
                stopReconnect();
                connectionLost = false;
                StatusChanged?.Invoke();
            } catch (Exception) {
                log("Reconnect failed.");
            } finally {
                reconnecting = false;
            }
        }

        private void stopReconnect() {
            if (reconnectTimer != null) {
                reconnectTimer.Dispose();
                reconnectTimer = null;
            }
        }

        public void disconnect() {
            comms.aCommModule module = commModule;
            commModule = null;
            stopReconnect();
            stopRapidFire();
            if (module != null) {
                try {
                    module.close();
                } catch (Exception ex) {
                    log("Error closing the connection: " + ex.Message);
                }
            }

            displayMessage("Disconnected", false);
            log("*********\nDisconnected.");
            connectionLost = false;
            lastEcho = null;
            setStatus(Status.NOT_CONNECTED, "Disconnected");
        }

        //---------------------------------------------------------------- session

        /** starts a session of this event. the running session is saved first if it has shots */
        public void selectEvent(Event ev) {
            selectedEvent = ev;
            if (currentStatus == Status.LOADED) {
                //the saved session that was shown is put away
                currentStatus = Status.NOT_CONNECTED;
                statusText = "";
            }
            initNewSession();
        }

        private void saveSession() {
            if (currentSession.id == 0) {
                storage.storeSession(currentSession, true);
                displayMessage("Session saved", true);
            } else {
                storage.updateSession(currentSession);
                displayMessage("Session " + currentSession.id + " updated", true);
            }
        }

        private void initNewSession() {
            if (currentSession != null && currentSession.Shots.Count > 0 && currentStatus != Status.LOADED) {
                saveSession();
            }

            if (recoverySessionName != "") {
                //the session to recover decides the event
                string sessionName = recoverySessionName.Trim();
                foreach (Event active in eventManager.getActiveEventsList()) {
                    if (sessionName.Contains(active.Name.Trim())) {
                        selectedEvent = active;
                        break;
                    }
                }
            }

            Event ev = selectedEvent;
            if (ev == null) {
                return;
            }

            this.log("       Starting new session with name: " + ev.Name);

            stopRapidFire();
            seriesSelected = false;
            currentSession = Session.createNewSession(ev, Settings.Default.name);
            currentSession.start();
            this.log(SessionRecovery.NewSessionMarker + " '" + currentSession.ToString() + "' started ###");
            if (commModule != null) { // there is a default session starting before the connection to the target is done
                send("{\"ATHLETE\":\"" + currentSession.user + "\", \"EVENT\":\"" + currentSession.ToString() + "\", \"TARGET_NAME\":\"" + currentSession.targetType + "\"}");
            }

            SessionStarted?.Invoke();
        }

        /**
         * ends the current session after asking whether to save it, and empties it.
         * returns true if the user cancelled and the session stays as it is
         */
        public bool clearShots() {
            if (currentSession == null) {
                return false;
            }

            if (currentSession.Shots.Count > 0 && currentStatus != Status.LOADED) {
                bool? save = dialogs.askYesNoCancel(saveQuestion
                    + Environment.NewLine + Environment.NewLine
                    + "'Yes' and 'No' will close the session."
                    + Environment.NewLine + "'Cancel' will keep the current session alive.", "Save session");

                if (save == true) {
                    saveSession();
                } else if (save == null) {
                    return true;
                }
            }

            seriesSelected = false;
            currentSession.clear();
            ShotsCleared?.Invoke();
            return false;
        }

        /** a saved session can be continued while connected, as long as the new session has no shots */
        public bool canResume() {
            return currentStatus == Status.CONNECTED && currentSession != null && currentSession.Shots.Count == 0;
        }

        /** the saved sessions of the current shooter in the current event, newest first */
        public List<ListBoxSessionItem> getResumableSessions() {
            if (currentSession == null || currentSession.user == null) {
                return new List<ListBoxSessionItem>();
            }
            return storage.findSessionsForUser(currentSession.user, currentSession.eventType);
        }

        /**
         * continues a saved session: it replaces the empty current session, new shots are added to it and saving updates it.
         * returns false if the session could not be loaded
         */
        public bool resumeSession(long id) {
            if (!canResume()) {
                return false;
            }

            Session session = storage.findSession(id);
            if (session == null) {
                return false; //the reason was shown already
            }
            session.repopulateSeries();

            stopRapidFire();
            seriesSelected = false;
            currentSession = session;
            selectedEvent = session.eventType;

            if (session.minutes > 0) {
                session.endTime = session.startTime.AddMinutes(session.minutes);
            } else {
                session.endTime = DateTime.MinValue;
            }

            updateStatistics();
            SessionResumed?.Invoke();

            displayMessage("Session " + session.id + " resumed", false);
            log("Session " + session.id + " resumed");
            return true;
        }

        /** all shots of the session */
        public List<Shot> getShots() {
            if (currentStatus == Status.LOADED) {
                return currentSession.LoadedShots;
            } else {
                return currentSession.Shots;
            }
        }

        /** the shots to draw on the target: all of them or the current series */
        public List<Shot> getShotList() {
            if (Settings.Default.OnlySeries || seriesSelected) {
                return currentSession.CurrentSeries;
            } else {
                return getShots();
            }
        }

        /** shows only one series on the target. -1 goes back to what the settings say */
        public void selectSeries(int index) {
            if (index >= 0 && index < currentSession.AllSeries.Count) {
                seriesSelected = true;
                currentSession.CurrentSeries = currentSession.AllSeries[index];
            } else {
                seriesSelected = false;
                if (currentSession.AllSeries.Count > 0) {
                    currentSession.CurrentSeries = currentSession.AllSeries[currentSession.AllSeries.Count - 1];
                }
            }
            updateStatistics();
            ShotListChanged?.Invoke();
        }

        /** mean point of impact, mean radius and group size exist from the second shot on */
        public bool hasStatistics() {
            return currentSession != null && getShotList().Count > 1;
        }

        private void updateStatistics() {
            if (hasStatistics()) {
                currentSession.calculateMeanValuesAndGroupSize(getShotList());
            }
        }

        //---------------------------------------------------------------- journal

        /**
         * the journal shows saved sessions, which needs the main window free: the running session is closed (after asking
         * whether to save it) and the target disconnected. returns false if the user wants to keep the session
         */
        public bool prepareForJournal() {
            if (currentStatus == Status.CONNECTED || currentStatus == Status.CONECTING) {
                if (clearShots()) {
                    return false;
                }
                disconnect();
            }
            return true;
        }

        /** shows a saved session in the main window, for viewing only */
        public void loadSessionFromJournal(Session session) {
            if (currentStatus == Status.CONNECTED || currentStatus == Status.CONECTING) {
                return;
            }
            if (currentStatus == Status.LOADED && currentSession != null && currentSession.id == session.id) {
                displayMessage("Session " + session.id + " already loaded", true);
                return;
            }

            stopRapidFire();
            seriesSelected = false;
            currentSession = session;
            selectedEvent = session.eventType;
            session.AllSeries.Clear();
            session.LoadedShots.Clear();
            foreach (Shot s in session.Shots) {
                session.addLoadedShot(s);
            }

            currentStatus = Status.LOADED;
            statusText = "Session " + session.id + " of " + session.user + " loaded from the journal";
            updateStatistics();
            SessionLoaded?.Invoke();
            StatusChanged?.Invoke();
            displayMessage("Session " + session.id + " loaded", false);
        }

        /**
         * puts the saved session away and goes back to an empty session of the default event.
         * the session stays on the screen after the journal is closed; this happens when the user connects to a target
         */
        public void unloadJournalSession() {
            if (currentStatus != Status.LOADED) {
                return;
            }
            currentStatus = Status.NOT_CONNECTED;
            statusText = "";
            currentSession = null;
            startDefaultSession();
            StatusChanged?.Invoke();
        }

        /** writes the PDF report of a saved session and returns the name of the file. null if it failed */
        public string exportSavedSessionPdf(long id, string folder = null) {
            if (folder == null) {
                folder = PDFGenerator.getFolder();
            }
            try {
                Directory.CreateDirectory(folder);
            } catch (Exception ex) {
                log(ex.Message);
                dialogs.showError("Path " + folder + " not valid. Cannot save PDF", "Error saving PDF");
                return null;
            }

            //from the database, not the session that is shown: the report builds its own series
            Session session = storage.findSession(id);
            if (session == null) {
                return null;
            }
            try {
                string file = PDFGenerator.generateAndSavePDF(session, folder, null, RtfText.toPlainText(session.diaryEntry));
                log("PDF saved: " + file);
                displayMessage("PDF saved: " + file, false);
                return file;
            } catch (Exception ex) {
                log("Error saving pdf " + ex.ToString());
                dialogs.showError("Error saving pdf!  " + Environment.NewLine + ex.Message, "PDF Export Error");
                return null;
            }
        }

        /** writes a saved session to the export folder. returns the name of the file, or null if it failed */
        public string exportSessionFile(Session session, string folder = null) {
            try {
                string file = SessionFile.export(session, folder ?? SessionFile.getExportFolder());
                log("Session " + session.id + " exported to " + file);
                displayMessage("Session " + session.id + " exported to " + file, false);
                return file;
            } catch (Exception ex) {
                log("Error exporting session: " + session.id + Environment.NewLine + ex.Message);
                dialogs.showError("The session could not be exported: " + ex.Message, "Export");
                return null;
            }
        }

        /** reads a session from an export file and stores it. returns false if that was not possible, the reason is shown */
        public bool importSessionFile(string file) {
            string error;
            Session session;
            try {
                session = SessionFile.fromText(File.ReadAllText(file), eventManager, out error);
            } catch (Exception ex) {
                session = null;
                error = "The file cannot be read: " + ex.Message;
            }
            if (session == null) {
                dialogs.showError(error, "Error loading session");
                return false;
            }

            storage.storeSession(session, false);
            log("Session from file " + file + " imported succesfully.");
            displayMessage("Session from file " + file + " imported succesfully.", false);
            return true;
        }

        //---------------------------------------------------------------- report

        /**
         * writes the PDF report of the session that is on the screen and returns the name of the file. null if there is nothing to report or it failed.
         * the session itself is not saved or changed by this
         */
        public string exportPdf(string folder = null) {
            if (currentSession == null || getShots().Count == 0) {
                dialogs.showWarning("There are no shots in the session yet.", "PDF export");
                return null;
            }

            if (folder == null) {
                folder = PDFGenerator.getFolder();
            }

            try {
                Directory.CreateDirectory(folder);
            } catch (Exception ex) {
                log(ex.Message);
                dialogs.showError("Path " + folder + " not valid. Cannot save PDF", "Error saving PDF");
                return null;
            }

            try {
                //a copy: the report builds its own series and needs the statistics that saving computes
                Session copy = currentSession.copy();
                if (currentStatus != Status.LOADED) {
                    copy.prepareForSaving();
                }

                string file = PDFGenerator.generateAndSavePDF(copy, folder, null, RtfText.toPlainText(copy.diaryEntry));
                log("PDF saved: " + file);
                displayMessage("PDF saved: " + file, true);
                return file;
            } catch (Exception ex) {
                log("Error saving pdf " + ex.ToString());
                dialogs.showError("Error saving pdf!  " + Environment.NewLine + ex.Message, "PDF Export Error");
                return null;
            }
        }

        //---------------------------------------------------------------- rapid fire

        /** starts a rapid fire series: load time first, then the commands for the target */
        public void startRapidFire() {
            if (currentSession == null || !currentSession.eventType.RapidFire || rapidFireRunning) {
                return;
            }
            currentSession.resetVRO();

            rapidFire = new RapidFireSequencer(currentSession.eventType);
            int interval = rapidFire.start();
            rapidFireRunning = true;
            currentSession.RFseriesActive = true;
            rapidFireTimer = new Timer(_ => uiThread(rapidFireTick), null, interval, Timeout.Infinite);
            StatusChanged?.Invoke();
        }

        internal void rapidFireTick() {
            if (!rapidFireRunning) {
                return;
            }
            RapidFireStep step = rapidFire.tick();

            if (step.misconfigured) {
                log("rapid fire misconfiguration");
            }

            if (step.command != null) {
                send(step.command);
            }

            if (step.finished) {
                stopRapidFire();
            } else if (rapidFireTimer != null) {
                if (step.nextInterval > 0) {
                    rapidFireInterval = step.nextInterval;
                }
                rapidFireTimer.Change(rapidFireInterval, Timeout.Infinite);
            }
        }

        private int rapidFireInterval = Timeout.Infinite;

        private void stopRapidFire() {
            if (rapidFireTimer != null) {
                rapidFireTimer.Dispose();
                rapidFireTimer = null;
            }
            if (rapidFireRunning) {
                rapidFireRunning = false;
                if (currentSession != null) {
                    currentSession.RFseriesActive = false;
                }
                StatusChanged?.Invoke();
            }
        }

        //---------------------------------------------------------------- shutdown

        /**
         * called when the main window is about to close.
         * returns false if the user wants to keep the session, the window must stay open then
         */
        public bool shutdown() {
            if (currentStatus == Status.CONNECTED || currentStatus == Status.CONECTING) {
                if (clearShots()) {
                    return false;
                }
                disconnect();
            }

            log(SessionRecovery.NormalShutdownMarker);
            return true;
        }

        public void Dispose() {
            stopReconnect();
            stopRapidFire();
            if (commModule != null) {
                try {
                    commModule.close();
                } catch (Exception) {
                }
                commModule = null;
            }
        }
    }
}
