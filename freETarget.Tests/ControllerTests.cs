using freETarget.Properties;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Xunit;

namespace freETarget.Tests {

    class FakeDialogs : IUserDialogs {
        public List<string> errors = new List<string>();
        public List<string> warnings = new List<string>();
        public List<string> questions = new List<string>();
        public bool yesNoAnswer = false;
        public bool? yesNoCancelAnswer = false;

        public void showError(string text, string title) { errors.Add(text); }
        public void showWarning(string text, string title) { warnings.Add(text); }
        public bool confirm(string text, string title) { questions.Add(text); return true; }
        public bool askYesNo(string text, string title) { questions.Add(text); return yesNoAnswer; }
        public bool? askYesNoCancel(string text, string title) { questions.Add(text); return yesNoCancelAnswer; }
    }

    /** a target that is driven by the test */
    class FakeComm : comms.aCommModule {
        public override event CommEventHandler CommDataReceivedEvent;
        public override event CommEventHandler CommDisconnectedEvent;
        public List<string> sent = new List<string>();
        public bool isOpen;
        public Exception openError;

        public override void open(comms.OpenParams value) {
            if (openError != null) {
                throw openError;
            }
            isOpen = true;
        }
        public override void close() { isOpen = false; }
        public override void sendData(string text) { sent.Add(text); }
        public override string getCommInfo() { return "FAKE"; }

        public void receive(string text) { CommDataReceivedEvent?.Invoke(this, new comms.CommEventArgs(text)); }
        public void drop() { CommDisconnectedEvent?.Invoke(this, new comms.CommEventArgs("Disconnected!")); }
    }

    public class ControllerTests : IDisposable {

        private readonly TempDir dir = new TempDir();
        private readonly FakeDialogs dialogs = new FakeDialogs();
        private readonly List<MainController> controllers = new List<MainController>();
        private FakeComm comm;

        private readonly List<Shot> shotsSeen = new List<Shot>();
        private int sessionsStarted;
        private int shotsCleared;

        public ControllerTests() {
            Make.resetSettings();
        }

        public void Dispose() {
            foreach (MainController c in controllers) {
                c.Dispose();
            }
            Make.resetSettings();
            dir.Dispose();
        }

        private MainController start(bool withName = true) {
            MainController controller = new MainController(dialogs, action => action());
            controllers.Add(controller);
            controller.connectDelay = 0;
            controller.versionDelay = 0;
            controller.commFactory = host => comm = new FakeComm();
            controller.ShotAdded += shot => shotsSeen.Add(shot);
            controller.SessionStarted += () => sessionsStarted++;
            controller.ShotsCleared += () => shotsCleared++;

            Assert.Null(controller.init(4, "vTest", Path.Combine(dir.path, "database"), Path.Combine(dir.path, "log")));
            Settings.Default.portName = "COM1";
            if (withName) {
                Settings.Default.name = "tester";
            }
            controller.startDefaultSession();
            return controller;
        }

        private async Task<MainController> connected() {
            MainController controller = start();
            await controller.connect();
            comm.receive("freETarget 5.2.1\r\n");
            Assert.Equal(MainController.Status.CONNECTED, controller.currentStatus);
            return controller;
        }

        private static string shotMessage(int count, decimal x, decimal y) {
            return "{\"shot\":" + count + ", \"miss\":0, \"x\":" + x.ToString(System.Globalization.CultureInfo.InvariantCulture) + ", \"y\":" + y.ToString(System.Globalization.CultureInfo.InvariantCulture) + ", \"r\":0.00, \"a\":0.00}\r\n";
        }

        [Fact]
        public void first_start_creates_database_log_and_the_default_session() {
            MainController controller = start();

            Assert.True(File.Exists(Path.Combine(dir.path, "database", "Storage.db")));
            Assert.True(File.Exists(controller.logger.logFile));
            Assert.Equal(6, controller.eventManager.getActiveEventsList().Count);
            Assert.Equal("Pistol Practice", controller.currentSession.eventType.Name);
            Assert.Equal("Pistol Practice", controller.selectedEvent.Name);
            Assert.Equal(1, sessionsStarted);
            Assert.Equal(MainController.Status.NOT_CONNECTED, controller.currentStatus);
            Assert.Empty(dialogs.questions);
            Assert.Empty(dialogs.errors);
        }

        [Fact]
        public async Task connecting_needs_a_shooter_name() {
            MainController controller = start(withName: false);

            await controller.connect();

            Assert.Equal(MainController.Status.NOT_CONNECTED, controller.currentStatus);
            Assert.Contains("No shooter name entered", dialogs.warnings.Single());
            Assert.Null(comm);
        }

        [Fact]
        public async Task connection_is_established_when_the_target_answers() {
            MainController controller = start();
            List<MainController.Status> statuses = new List<MainController.Status>();
            controller.StatusChanged += () => statuses.Add(controller.currentStatus);

            await controller.connect();
            Assert.Equal(MainController.Status.CONECTING, controller.currentStatus);
            Assert.True(comm.isOpen);

            comm.receive("freETarget 5.2.1\r\nmore text");

            Assert.Equal(MainController.Status.CONNECTED, controller.currentStatus);
            Assert.Equal("Connected to freETarget 5.2.1 on FAKE", controller.statusText);
            Assert.Equal("{\"VERSION\":7}", comm.sent[0]);
            Assert.Equal("{\"ATHLETE\":\"tester\", \"EVENT\":\"Pistol Practice\", \"TARGET_NAME\":\"freETarget.targets.AirPistol\"}", comm.sent[1]);
            Assert.Equal(new[] { MainController.Status.CONECTING, MainController.Status.CONNECTED }, statuses);
            Assert.Equal(2, sessionsStarted); //a fresh session starts with the connection
        }

        [Fact]
        public async Task failed_connection_is_reported() {
            MainController controller = start();
            controller.commFactory = host => comm = new FakeComm { openError = new IOException("port is gone") };

            await controller.connect();

            Assert.Equal(MainController.Status.NOT_CONNECTED, controller.currentStatus);
            Assert.Contains("port is gone", controller.statusText);
            Assert.Single(dialogs.errors);
            Assert.Null(controller.commModule);
        }

        [Fact]
        public async Task shots_from_the_target_are_scored_and_added_to_the_session() {
            MainController controller = await connected();

            comm.receive(shotMessage(1, 0, 0));
            comm.receive("{\"shot\":2, \"miss\":0, \"x\":8.00, ");    //a message can arrive in pieces
            comm.receive("\"y\":0.00}\r\n" + shotMessage(3, 0, 16));

            Assert.Equal(new[] { 10.9m, 10.0m, 9.0m }, shotsSeen.Select(s => s.decimalScore));
            Assert.Equal(29, controller.currentSession.score);
            Assert.Equal(3, controller.getShotList().Count);
            Assert.True(controller.hasStatistics());
            Assert.Contains("\"score\":9", File.ReadAllText(controller.logger.shotFile));
        }

        [Fact]
        public async Task misses_can_be_ignored() {
            MainController controller = await connected();

            comm.receive("{\"shot\":1, \"miss\":1, \"x\":0, \"y\":0}");
            Settings.Default.ignoreMiss = true;
            comm.receive("{\"shot\":2, \"miss\":1, \"x\":0, \"y\":0}");

            Assert.Single(controller.currentSession.Shots);
            Assert.True(shotsSeen.Single().miss);
        }

        [Fact]
        public async Task calibration_is_applied_to_incoming_shots() {
            MainController controller = await connected();
            controller.calibrationX = 8;

            comm.receive(shotMessage(1, 0, 0));

            Assert.Equal(10.0m, shotsSeen.Single().decimalScore);
        }

        private const string echo = "{\"NAME\":\"DOC\", \"NAME_ID\":11, \"LED_BRIGHT\":50, \"PAPER_TIME\":0, \"STEP_TIME\":30, \"STEP_COUNT\":200, \"SENSOR\":230.5, "
            + "\"X_OFFSET\":0, \"Y_OFFSET\":0, \"Z_OFFSET\":13, \"NORTH_X\":1, \"NORTH_Y\":-2, \"EAST_X\":3, \"EAST_Y\":4, \"SOUTH_X\":5, \"SOUTH_Y\":6, \"WEST_X\":7, \"WEST_Y\":8}";

        [Fact]
        public async Task calibration_moves_the_shots_already_fired() {
            MainController controller = await connected();
            comm.receive(shotMessage(1, 0, 0));    //10.9
            comm.receive(shotMessage(2, -8, 0));   //10.0
            int recalculated = 0;
            controller.ShotsRecalculated += () => recalculated++;

            controller.calibrate(8, 0, 0); //everything 8mm to the right

            Assert.Equal(new[] { 10.0m, 10.9m }, controller.currentSession.Shots.Select(s => s.decimalScore));
            Assert.Equal(20.9m, controller.currentSession.decimalScore);
            Assert.Equal(1, controller.currentSession.innerX);
            Assert.Equal(1, recalculated);
            Assert.True(controller.isCalibrated());

            comm.receive(shotMessage(3, -8, 0));   //the following shots are calibrated too
            Assert.Equal(10.9m, controller.currentSession.Shots[2].decimalScore);

            controller.calibrate(0, 0, 90);        //a quarter turn does not change the distance to the center
            Assert.Equal(31.8m, controller.currentSession.decimalScore);
            Assert.Equal(90m, controller.calibrationAngle);

            controller.resetCalibration();
            Assert.Equal(new[] { 10.9m, 10.0m, 10.0m }, controller.currentSession.Shots.Select(s => s.decimalScore));
            Assert.Equal(30, controller.currentSession.score);
            Assert.False(controller.isCalibrated());
        }

        [Fact]
        public async Task calibration_is_kept_for_the_next_start() {
            MainController controller = await connected();
            controller.calibrate(1.5m, -0.5m, 2);
            controller.saveCalibration();
            controller.shutdown();

            MainController next = start();

            Assert.Equal(1.5m, next.calibrationX);
            Assert.Equal(-0.5m, next.calibrationY);
            Assert.Equal(2m, next.calibrationAngle);
        }

        [Fact]
        public async Task settings_of_the_target_are_read_changed_and_sent_back() {
            MainController controller = await connected();

            controller.requestEcho();
            Assert.Null(controller.lastEcho);
            Assert.Equal("{\"ECHO\":0}", comm.sent.Last());
            comm.receive(echo);

            TargetSettings settings = TargetSettings.fromEcho(controller.lastEcho);
            Assert.Equal("DOC", TargetSettings.TargetNames[settings.nameId]);
            Assert.Equal(50, settings.ledBrightness);
            Assert.False(settings.isDirectCurrent());
            Assert.Equal("230.5", settings.sensorDiameter);
            Assert.Equal("-2", settings.northY);

            settings.nameId = 12;
            settings.xOffset = "1.5";
            settings.paperTime = "500";
            Assert.Null(controller.applyTargetSettings(settings));

            Assert.Equal("{\"SENSOR\":230.5, \"X_OFFSET\":1.5, \"Y_OFFSET\":0, \"Z_OFFSET\":13, \"PAPER_TIME\":500, \"STEP_TIME\":0, \"STEP_COUNT\":0, "
                + "\"NORTH_X\":1, \"NORTH_Y\":-2, \"EAST_X\":3, \"EAST_Y\":4, \"SOUTH_X\":5, \"SOUTH_Y\":6, \"WEST_X\":7, \"WEST_Y\":8, \"LED_BRIGHT\":50, \"NAME_ID\":12, \"ECHO\":9 }", comm.sent.Last());
        }

        [Theory]
        [InlineData("1,5", "X offset is not a number")]   //decimal comma
        [InlineData("abc", "X offset is not a number")]
        [InlineData("", "X offset is not a number")]
        public async Task wrong_target_settings_are_not_sent(string xOffset, string message) {
            MainController controller = await connected();
            comm.sent.Clear();
            TargetSettings settings = new TargetSettings { xOffset = xOffset };

            Assert.Equal(message, controller.applyTargetSettings(settings));
            Assert.Empty(comm.sent);

            Assert.Equal("Stepper steps must be a positive number", new TargetSettings { stepCount = "-1" }.validate());
            Assert.Equal("Offset for West Y sensor is not a number", new TargetSettings { westY = "1.5" }.validate());
            Assert.Null(new TargetSettings().validate());
        }

        [Fact]
        public async Task offsets_stored_in_the_target_replace_the_local_calibration() {
            MainController controller = await connected();
            controller.calibrate(2, 3, 0);

            comm.receive(echo.Replace("\"X_OFFSET\":0", "\"X_OFFSET\":1.5"));

            Assert.Equal(0m, controller.calibrationX);
            Assert.Equal(0m, controller.calibrationY);
        }

        [Fact]
        public async Task answer_to_a_command_is_kept_as_echo() {
            MainController controller = await connected();
            int echoes = 0;
            controller.EchoReceived += () => echoes++;

            controller.send("{\"ECHO\":0}");
            comm.receive("{\"NAME\":\"TARGET 1\", \"ANGLE\":45, \"X_OFFSET\":0, \"Y_OFFSET\":0}");

            Assert.Equal("{\"ECHO\":0}", comm.sent.Last());
            Assert.Equal(1, echoes);
            Assert.Empty(shotsSeen);
        }

        [Fact]
        public async Task changing_the_event_saves_the_running_session() {
            MainController controller = await connected();
            comm.receive(shotMessage(1, 0, 0));
            Event practice = controller.selectedEvent;

            controller.selectEvent(controller.eventManager.findEventByName("Rifle Match"));

            Assert.Empty(dialogs.questions); //saved without asking
            Assert.Single(controller.storage.findSessionsForUser("tester", practice));
            Assert.Equal("Rifle Match", controller.currentSession.eventType.Name);
            Assert.Empty(controller.currentSession.Shots);
            Assert.Contains("\"EVENT\":\"Rifle Match\"", comm.sent.Last());
        }

        [Theory]
        [InlineData(null, MainController.Status.CONNECTED, 0, 1)]       //cancel: nothing happens
        [InlineData(false, MainController.Status.NOT_CONNECTED, 0, 0)]  //no: session is thrown away
        [InlineData(true, MainController.Status.NOT_CONNECTED, 1, 0)]   //yes: session is saved
        public async Task disconnecting_asks_what_to_do_with_the_session(bool? answer, MainController.Status status, int saved, int shotsLeft) {
            MainController controller = await connected();
            comm.receive(shotMessage(1, 0, 0));
            dialogs.yesNoCancelAnswer = answer;
            FakeComm target = comm;

            await controller.toggleConnection();

            Assert.Single(dialogs.questions);
            Assert.Equal(status, controller.currentStatus);
            Assert.Equal(saved, controller.storage.findSessionsForUser("tester", controller.selectedEvent).Count);
            Assert.Equal(shotsLeft, controller.currentSession.Shots.Count);
            Assert.Equal(answer == null, target.isOpen);
            Assert.Equal(answer == null ? 0 : 1, shotsCleared);
        }

        [Fact]
        public async Task text_from_a_closed_connection_is_ignored() {
            MainController controller = await connected();
            FakeComm old = comm;
            controller.disconnect();

            old.receive(shotMessage(1, 0, 0));

            Assert.Empty(shotsSeen);
        }

        [Fact]
        public async Task one_series_can_be_shown_alone() {
            MainController controller = await connected();
            for (int i = 1; i <= 12; i++) {
                comm.receive(shotMessage(i, i, 0));
            }

            controller.selectSeries(0);
            Assert.Equal(10, controller.getShotList().Count);
            Assert.Equal(9m, controller.currentSession.groupSize);

            controller.selectSeries(-1);
            Assert.Equal(12, controller.getShotList().Count);
            Assert.Equal(11m, controller.currentSession.groupSize);

            Settings.Default.OnlySeries = true;
            Assert.Equal(2, controller.getShotList().Count);
        }

        [Fact]
        public async Task rapid_fire_series_sends_its_commands_to_the_target() {
            MainController controller = await connected();
            controller.storage.storeEvent(Make.rapidFireEvent(5, 8, -1, 7, 60));
            Event rapid = controller.storage.loadEvents().Single(e => e.RapidFire);
            controller.selectEvent(rapid);
            comm.sent.Clear();

            controller.startRapidFire();
            Assert.True(controller.rapidFireRunning);
            Assert.True(controller.currentSession.RFseriesActive);

            controller.rapidFireTick(); //load time over
            controller.rapidFireTick(); //series over
            controller.rapidFireTick(); //cooldown over

            Assert.Equal(new[] { "{\"RAPID_COUNT\":5, \"RAPID_WAIT\":7, \"RAPID_TIME\":8, \"RAPID_ENABLE\":1 }", "{\"RAPID_ENABLE\":0 }" }, comm.sent);
            Assert.False(controller.rapidFireRunning);
            Assert.False(controller.currentSession.RFseriesActive);
        }

        [Fact]
        public async Task session_is_recovered_after_a_crash() {
            MainController crashed = await connected();
            crashed.selectEvent(crashed.eventManager.findEventByName("Rifle Practice"));
            comm.receive(shotMessage(1, 0, 0));
            comm.receive(shotMessage(2, 8, 0));
            DateTime started = crashed.currentSession.startTime;
            crashed.Dispose(); //no shutdown
            shotsSeen.Clear();

            dialogs.yesNoAnswer = true;
            MainController controller = start();
            Assert.Contains("did not shutdown corectly", dialogs.questions.Single());
            Assert.Empty(shotsSeen); //recovery happens after the connection
            await controller.connect();
            comm.receive("freETarget 5.2.1\r\n");

            Assert.Equal("Rifle Practice", controller.currentSession.eventType.Name);
            Assert.Equal(new[] { 1, 2 }, controller.currentSession.Shots.Select(s => s.count));
            Assert.Equal(2, shotsSeen.Count);
            Assert.True(Math.Abs((controller.currentSession.startTime - started).TotalSeconds) < 1);

            //the next start is normal again
            Assert.True(controller.shutdown());
            dialogs.questions.Clear();
            start();
            Assert.Empty(dialogs.questions);
        }

        [Fact]
        public async Task report_of_the_running_session_is_written_without_disturbing_it() {
            MainController controller = await connected();
            controller.selectEvent(controller.eventManager.findEventByName("Pistol Match"));
            string folder = Path.Combine(dir.path, "pdf");

            Assert.Null(controller.exportPdf(folder)); //nothing to report yet
            Assert.Single(dialogs.warnings);

            for (int i = 1; i <= 12; i++) {
                comm.receive(shotMessage(i, i, -i));
            }
            DateTime end = controller.currentSession.endTime;

            string file = controller.exportPdf(folder);

            Assert.True(File.Exists(file));
            Assert.StartsWith(folder, file);
            Assert.Empty(dialogs.errors);
            Assert.Equal(end, controller.currentSession.endTime); //the match clock keeps running
            Assert.Equal(2, controller.currentSession.AllSeries.Count);
            Assert.Equal(0, controller.currentSession.id);
            Assert.Empty(controller.storage.findSessionsForUser("tester", controller.selectedEvent));

            comm.receive(shotMessage(13, 0, 0));
            Assert.Equal(13, controller.currentSession.Shots.Count);
        }

        [Fact]
        public async Task saved_session_can_be_resumed_and_is_updated_not_duplicated() {
            MainController controller = await connected();
            Event practice = controller.selectedEvent;
            Assert.True(controller.canResume());
            Assert.Empty(controller.getResumableSessions());

            comm.receive(shotMessage(1, 0, 0));
            comm.receive(shotMessage(2, 8, 0));
            Assert.False(controller.canResume()); //the session has shots
            controller.selectEvent(controller.eventManager.findEventByName("Rifle Match")); //saves it
            Assert.Empty(controller.getResumableSessions()); //sessions of the other event are not offered
            controller.selectEvent(practice);

            ListBoxSessionItem saved = controller.getResumableSessions().Single();
            int resumed = 0;
            controller.SessionResumed += () => resumed++;
            Assert.True(controller.resumeSession(saved.id));

            Assert.Equal(1, resumed);
            Assert.Equal(saved.id, controller.currentSession.id);
            Assert.Equal(new[] { 10.9m, 10.0m }, controller.getShotList().Select(s => s.decimalScore));
            Assert.Equal(20, controller.currentSession.score);
            Assert.True(controller.hasStatistics());
            Assert.Empty(dialogs.errors);

            comm.receive(shotMessage(3, 0, 16));
            Assert.Equal(new[] { 0, 1, 2 }, controller.currentSession.Shots.Select(s => s.index));
            Assert.Equal(29, controller.currentSession.score);
            Assert.Single(controller.currentSession.AllSeries);

            controller.selectEvent(controller.eventManager.findEventByName("Rifle Match")); //saves again
            ListBoxSessionItem again = controller.storage.findSessionsForUser("tester", practice).Single();
            Assert.Equal(saved.id, again.id);
            Assert.Equal(3, controller.storage.findSession(again.id).Shots.Count);
        }

        [Fact]
        public async Task resuming_needs_a_connection_and_an_existing_session() {
            MainController controller = start();
            Assert.False(controller.canResume());
            Assert.False(controller.resumeSession(1));

            await controller.connect();
            comm.receive("freETarget 5.2.1\r\n");
            Assert.False(controller.resumeSession(999)); //no such session
            Assert.Equal(0, controller.currentSession.id);
        }

        [Fact]
        public void event_is_created_changed_and_made_active() {
            MainController controller = start();
            int changes = 0;
            controller.EventsChanged += () => changes++;

            EventDraft draft = new EventDraft {
                name = "Club Final", type = Event.EventType.Final, decimalScoring = true, numberOfShots = "24", target = "freETarget.targets.AirRifle", caliber = "4.5",
                minutes = "-1", color = "Purple", shotsPerSeries = "5", seriesSeconds = "250", shotsInSeries = "10", singleShotSeconds = "50", shotsInSingleSeries = "2"
            };
            Assert.Equal("", controller.saveEvent(draft, null));

            Event created = controller.eventManager.findEventByName("Club Final");
            Assert.NotNull(created);
            Assert.IsType<targets.AirRifle>(created.Target);
            Assert.Equal(System.Drawing.Color.Purple, created.TabColor);
            Assert.Equal(5, created.Final_NumberOfShotPerSeries);
            Assert.Equal(-1, created.RF_LoadTime); //not a rapid fire event
            Assert.DoesNotContain(created, controller.eventManager.getActiveEventsList());

            //change it into a rapid fire practice
            EventDraft change = EventDraft.from(created);
            change.type = Event.EventType.Practice;
            change.rapidFire = true;
            change.rfNumberOfShots = "5";
            change.rfTimePerSerie = "8";
            change.rfTimePerShot = "-1";
            change.rfPauseTime = "7.5";
            change.rfLoadTime = "60";
            Assert.Equal("", controller.saveEvent(change, created));

            Event changed = controller.eventManager.findEventByName("Club Final");
            Assert.Equal(created.ID, changed.ID);
            Assert.Equal(Event.EventType.Practice, changed.Type);
            Assert.Equal(-1, changed.NumberOfShots);              //a practice has no number of shots
            Assert.Equal(-1, changed.Final_NumberOfShotPerSeries);
            Assert.True(changed.RapidFire);
            Assert.Equal(7.5m, changed.RF_TimeBetweenShots);

            //active as the only event: the idle session moves to it
            controller.setActiveEvents(new List<Event> { changed });
            Assert.Equal(new[] { "Club Final" }, controller.eventManager.getActiveEventsList().Select(e => e.Name));
            Assert.Equal("Club Final", controller.currentSession.eventType.Name);
            Assert.Equal(3, changes);
        }

        [Fact]
        public void event_with_wrong_fields_is_not_saved() {
            MainController controller = start();
            EventDraft draft = new EventDraft { name = "", type = Event.EventType.Match, numberOfShots = "sixty", caliber = "4,5", minutes = "75", color = "Gold" };

            string errors = controller.saveEvent(draft, null);

            Assert.Contains("'Name' is required", errors);
            Assert.Contains("'Number of Shots' field is not a number", errors);
            Assert.Contains("'Caliber' field is not a decimal", errors);
            Assert.Contains("Please select a 'Target'", errors);
            Assert.DoesNotContain("Rapid Fire", errors);   //not used by this event
            Assert.DoesNotContain("Series", errors);
            Assert.Equal(6, controller.eventManager.getEventsList().Count);
        }

        [Fact]
        public void every_default_event_survives_being_edited_unchanged() {
            MainController controller = start();
            foreach (Event ev in controller.eventManager.getEventsList().ToList()) {
                EventDraft draft = EventDraft.from(ev);
                Assert.Equal("", draft.validate());
                Event copy = draft.create();
                Assert.Equal(ev.Name, copy.Name);
                Assert.Equal(ev.Type, copy.Type);
                Assert.Equal(ev.NumberOfShots, copy.NumberOfShots);
                Assert.Equal(ev.Minutes, copy.Minutes);
                Assert.Equal(ev.Target.getName(), copy.Target.getName());
                Assert.Equal(ev.TabColor, copy.TabColor);
                Assert.Equal(ev.Final_SeriesSeconds, copy.Final_SeriesSeconds);
            }
            Assert.Contains("Moccasin", NamedColors.All);
            Assert.DoesNotContain("Transparent", NamedColors.All);
        }

        private async Task<(MainController, long)> withSavedSession() {
            MainController controller = await connected();
            comm.receive(shotMessage(1, 0, 0));
            comm.receive(shotMessage(2, 8, 0));
            comm.receive(shotMessage(3, 0, 16));
            Event practice = controller.selectedEvent;
            controller.selectEvent(controller.eventManager.findEventByName("Rifle Match")); //saves
            controller.selectEvent(practice);
            return (controller, controller.getResumableSessions().Single().id);
        }

        [Fact]
        public async Task journal_closes_the_running_session_and_shows_a_saved_one() {
            (MainController controller, long id) = await withSavedSession();
            comm.receive(shotMessage(1, 1, 1));

            dialogs.yesNoCancelAnswer = null; //cancel: the session goes on, no journal
            Assert.False(controller.prepareForJournal());
            Assert.Equal(MainController.Status.CONNECTED, controller.currentStatus);

            dialogs.yesNoCancelAnswer = false;
            Assert.True(controller.prepareForJournal());
            Assert.Equal(MainController.Status.NOT_CONNECTED, controller.currentStatus);

            int loaded = 0;
            controller.SessionLoaded += () => loaded++;
            Session saved = controller.storage.findSession(id);
            controller.loadSessionFromJournal(saved);

            Assert.Equal(MainController.Status.LOADED, controller.currentStatus);
            Assert.Equal(1, loaded);
            Assert.Equal(3, controller.getShots().Count);
            Assert.Equal(3, controller.getShotList().Count);
            Assert.Single(saved.AllSeries);
            Assert.True(controller.hasStatistics());
            Assert.Contains("loaded", controller.statusText);

            controller.loadSessionFromJournal(saved); //again: nothing happens
            Assert.Equal(1, loaded);
            Assert.Equal(3, controller.getShots().Count);

            //connecting puts the viewed session away without saving it again or asking about it
            dialogs.questions.Clear();
            await controller.toggleConnection();
            Assert.Equal(MainController.Status.CONECTING, controller.currentStatus);
            comm.receive("freETarget 5.2.1\r\n");
            Assert.Equal(MainController.Status.CONNECTED, controller.currentStatus);
            Assert.Empty(controller.currentSession.Shots);
            Assert.Equal(0, controller.currentSession.id);
            Assert.Empty(dialogs.questions);
            Assert.Single(controller.storage.findSessionsForUser("tester", saved.eventType));

            comm.receive(shotMessage(1, 0, 0));
            Assert.Single(controller.getShots());
        }

        [Fact]
        public async Task viewed_session_is_put_away_by_choosing_an_event_or_closing() {
            (MainController controller, long id) = await withSavedSession();
            controller.prepareForJournal();
            Event practice = controller.selectedEvent;
            controller.loadSessionFromJournal(controller.storage.findSession(id));

            controller.selectEvent(controller.eventManager.findEventByName("Rifle Match"));
            Assert.Equal(MainController.Status.NOT_CONNECTED, controller.currentStatus);
            Assert.Equal("Rifle Match", controller.currentSession.eventType.Name);
            Assert.Empty(controller.currentSession.Shots);

            controller.loadSessionFromJournal(controller.storage.findSession(id));
            dialogs.questions.Clear();
            Assert.True(controller.shutdown());
            Assert.Empty(dialogs.questions);
            Assert.Single(controller.storage.findSessionsForUser("tester", practice));
        }

        [Fact]
        public async Task saved_session_is_exported_imported_and_printed() {
            (MainController controller, long id) = await withSavedSession();
            controller.prepareForJournal();
            controller.storage.updateDiary(id, RtfText.fromPlainText("windy ~ cold"));
            Session saved = controller.storage.findSession(id);
            string folder = Path.Combine(dir.path, "export");

            string file = controller.exportSessionFile(saved, folder);

            Assert.Equal(Path.Combine(folder, "freETargetSession_tester-Pistol Practice_" + id + ".exp"), file);
            string[] csv = File.ReadAllLines(Path.ChangeExtension(file, ".csv"));
            Assert.Equal(3, csv.Length);
            Assert.EndsWith(",8,0,10.0", csv[1]);

            //import on a "different computer": the same database, where the session was deleted
            controller.storage.deleteSession(id);
            Assert.True(controller.importSessionFile(file));
            ListBoxSessionItem imported = controller.storage.findSessionsForUser("tester", saved.eventType).Single();
            Session again = controller.storage.findSession(imported.id);
            Assert.Equal(saved.score, again.score);
            Assert.Equal(saved.startTime, again.startTime);
            Assert.Equal(new[] { 10.9m, 10.0m, 9.0m }, again.Shots.Select(s => s.decimalScore));
            Assert.Equal("windy ~ cold", RtfText.toPlainText(again.diaryEntry));
            Assert.Empty(dialogs.errors);

            //the report of the saved session
            string pdf = controller.exportSavedSessionPdf(imported.id, Path.Combine(dir.path, "pdf"));
            Assert.EndsWith("Session " + imported.id + ".pdf", pdf);
            Assert.True(new FileInfo(pdf).Length > 5000);

            //a changed file is refused
            File.WriteAllText(file, File.ReadAllText(file).Replace("tester~29~", "tester~30~"));
            Assert.False(controller.importSessionFile(file));
            Assert.Contains("MD5 check failed", dialogs.errors.Last());
            File.WriteAllText(file, "something else");
            Assert.False(controller.importSessionFile(file));
            Assert.Single(controller.storage.findSessionsForUser("tester", saved.eventType));
        }

        [Fact]
        public void statistics_show_the_sessions_from_oldest_to_newest() {
            List<decimal> newestFirst = new List<decimal>();
            for (int i = 12; i >= 1; i--) {
                newestFirst.Add(i); //session 12 is the newest
            }

            StatisticsSeries series = StatisticsSeries.from(newestFirst);

            Assert.Equal(12, series.values.Count);
            Assert.Equal(1, series.values[0]);
            Assert.Equal(12, series.values[11]);
            Assert.Equal(7.5, series.lastTenAverage); //sessions 3 to 12
            Assert.Equal(1, series.min);
            Assert.Equal(12, series.max);
            Assert.Empty(StatisticsSeries.from(new List<decimal>()).values);
        }

        [Fact]
        public async Task terminal_collects_what_the_target_says_and_sends_commands() {
            MainController controller = await connected();
            int changes = 0;
            controller.TerminalChanged += () => changes++;
            comm.sent.Clear();

            comm.receive("some text of the target\r\n");
            Assert.Null(controller.sendCommand("LED_BRIGHT", " 75 "));
            Assert.Null(controller.sendRaw("!"));

            Assert.Equal(new[] { "{\"LED_BRIGHT\":75}", "!" }, comm.sent);
            Assert.Contains("freETarget 5.2.1", controller.terminalText); //from the moment of the connection
            Assert.Contains("some text of the target" + Environment.NewLine + "> {\"LED_BRIGHT\":75}" + Environment.NewLine + "> !", controller.terminalText);
            Assert.Equal(3, changes);

            Assert.Equal("Cannot send empty value", controller.sendCommand("LED_BRIGHT", " "));
            Assert.Equal("Nothing to send", controller.sendRaw(""));
            controller.disconnect();
            Assert.Equal("The target is not connected", controller.sendCommand("LED_BRIGHT", "1"));
            Assert.Equal("The target is not connected", controller.sendRaw("!"));
            Assert.Equal(2, comm.sent.Count);

            Assert.Equal(34, FirmwareCommand.All.Count);
            Assert.Equal(FirmwareCommand.All.Count, FirmwareCommand.All.Select(c => c.command).Distinct().Count());
        }

        [Fact]
        public async Task terminal_drops_the_oldest_text_when_it_gets_too_long() {
            MainController controller = await connected();
            string line = new string('x', 1000);

            for (int i = 0; i < 450; i++) {
                comm.receive(line);
            }
            comm.receive("the end");

            Assert.True(controller.terminalText.Length <= 200000);
            Assert.DoesNotContain("freETarget 5.2.1", controller.terminalText);
            Assert.EndsWith("the end" + Environment.NewLine, controller.terminalText);
        }

        [Fact]
        public async Task closing_the_application_can_be_cancelled_to_keep_the_session() {
            MainController controller = await connected();
            comm.receive(shotMessage(1, 0, 0));

            dialogs.yesNoCancelAnswer = null;
            Assert.False(controller.shutdown());
            Assert.Equal(MainController.Status.CONNECTED, controller.currentStatus);

            dialogs.yesNoCancelAnswer = true;
            Assert.True(controller.shutdown());
            Assert.Equal(MainController.Status.NOT_CONNECTED, controller.currentStatus);
            Assert.True(SessionRecovery.wasCleanShutdown(controller.logger.logFile));
        }

        [Fact]
        public async Task shots_arrive_over_a_real_network_connection() {
            TcpListener server = new TcpListener(IPAddress.Loopback, 0);
            server.Start();
            try {
                object uiLock = new object();
                MainController controller = new MainController(dialogs, action => { lock (uiLock) { action(); } });
                controllers.Add(controller);
                controller.connectDelay = 0;
                controller.versionDelay = 0;
                controller.reconnectInterval = 100;
                SemaphoreSlim shotArrived = new SemaphoreSlim(0);
                controller.ShotAdded += shot => { shotsSeen.Add(shot); shotArrived.Release(); };
                Assert.Null(controller.init(4, "vTest", Path.Combine(dir.path, "database"), Path.Combine(dir.path, "log")));
                Settings.Default.name = "tester";
                Settings.Default.CommProtocol = "TCP";
                Settings.Default.TcpIP = "127.0.0.1";
                Settings.Default.TcpPort = ((IPEndPoint)server.LocalEndpoint).Port;
                controller.startDefaultSession();

                Task connecting;
                lock (uiLock) {
                    connecting = controller.connect();
                }
                TcpClient target = await server.AcceptTcpClientAsync();
                await connecting;
                NetworkStream stream = target.GetStream();
                byte[] hello = Encoding.ASCII.GetBytes("freETarget 5.2.1\r\n");
                stream.Write(hello, 0, hello.Length);

                //the session start arrives at the target
                byte[] buffer = new byte[1024];
                StringBuilder received = new StringBuilder();
                target.ReceiveTimeout = 5000;
                while (!received.ToString().Contains("TARGET_NAME")) {
                    received.Append(Encoding.ASCII.GetString(buffer, 0, stream.Read(buffer, 0, buffer.Length)));
                }
                Assert.StartsWith("{\"VERSION\":7}", received.ToString());

                byte[] shot = Encoding.ASCII.GetBytes(shotMessage(1, 8, 0));
                stream.Write(shot, 0, shot.Length);
                Assert.True(await shotArrived.WaitAsync(5000), "the shot did not arrive");
                Assert.Equal(10.0m, shotsSeen.Single().decimalScore);

                //the target drops the connection and comes back: the application reconnects by itself
                target.Close();
                DateTime limit = DateTime.Now.AddSeconds(5);
                while (!controller.connectionLost && DateTime.Now < limit) {
                    await Task.Delay(20);
                }
                Assert.True(controller.connectionLost, "the lost connection was not noticed");

                TcpClient again = await server.AcceptTcpClientAsync().WaitAsync(TimeSpan.FromSeconds(5));
                limit = DateTime.Now.AddSeconds(5);
                while (controller.connectionLost && DateTime.Now < limit) {
                    await Task.Delay(20);
                }
                Assert.False(controller.connectionLost);
                shot = Encoding.ASCII.GetBytes(shotMessage(2, 0, 16));
                again.GetStream().Write(shot, 0, shot.Length);
                Assert.True(await shotArrived.WaitAsync(5000), "no shot after the reconnect");
                Assert.Equal(2, controller.currentSession.Shots.Count);

                lock (uiLock) {
                    controller.disconnect();
                }
                again.Close();
            } finally {
                server.Stop();
            }
        }
    }
}
