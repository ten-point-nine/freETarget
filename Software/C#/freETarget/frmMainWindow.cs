﻿using freETarget.Properties;
using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Data;
using System.Deployment.Internal;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Globalization;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.Remoting.Messaging;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using System.Reflection;

namespace freETarget {
    public partial class frmMainWindow : Form {

        /*
         * non-UI related variables
        */

        public const uint ES_CONTINUOUS = 0x80000000;
        public const uint ES_SYSTEM_REQUIRED = 0x00000001;
        public const uint ES_DISPLAY_REQUIRED = 0x00000002;

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern uint SetThreadExecutionState([In] uint esFlags);

        public enum Status {
            NOT_CONNECTED,
            CONECTING,
            CONNECTED,
            LOADED
        }
        private Status currentStatus = Status.NOT_CONNECTED;

        public const String FET_SSID_PREFIX = "FET-";

        public const String Arduino = "Arduino";
        public const String ESP32 = "ESP32";

        private string incomingJSON = "";               // Cumulative serial message


        public decimal calibrationX = 0;
        public decimal calibrationY = 0;
        public decimal calibrationAngle = 0;

        //accumulator of ALL incoming text from the arduino. it will all be displayed in the arduino window
        public StringBuilderWrapper output = new StringBuilderWrapper();

        private Session currentSession;

        private String recoverySessionName="";
        private String recoverySessionTime="";
        private List<String> recoveryLines;

        public StorageController storage;
        public EventManager eventManager;


        private String logFile = "";
        private String shotFile = "";

        /*
         *  UI related variables
         */

        private delegate void SafeCallDelegate(Shot shot);
        private delegate void SafeCallDelegate2(string text);
        private delegate void SafeCallDelegate3();
        private delegate void SafeCallDelegate4(string text, bool flag);

        private bool gridSeriesSelected = false;

        public comms.aCommModule commModule;

        public Echo lastEcho = null;

        private DateTime tooltipDisplayTime = DateTime.Now;

        private System.Windows.Forms.Timer reconnectTimer;

        private int RFduelCounter = 0;

        private const int RFcooldown = 3; //seconds

        public frmMainWindow() {
            InitializeComponent();
            initLog();

            this.reconnectTimer = new System.Windows.Forms.Timer();
            this.reconnectTimer.Interval = 2000; //reconnect time interval set to 2 seconds
            this.reconnectTimer.Tick += new System.EventHandler(this.reconnectTimer_Tick);

            bool cleanEnd = checkLogForCleanDisconnect();

            System.Reflection.Assembly assembly = System.Reflection.Assembly.GetExecutingAssembly();
            string v = "v" + assembly.GetName().Version.Major + "." + assembly.GetName().Version.Minor + "." + assembly.GetName().Version.Build;
            log("------------------------------------------------------------------------\n\nStarting freETarget - " + v + " ... ");
            statusVersion.Text = v;

            storage = new StorageController(this);
            //check DB
            String testDB = storage.checkDB(assembly.GetName().Version.Major);
            if (testDB != null) {
                MessageBox.Show("Database check failed. Please check your installation. " + Environment.NewLine + Environment.NewLine + testDB + Environment.NewLine + Environment.NewLine + "The application will now exit!", "Database problem", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Environment.Exit(0);
            }

            initSettingsDB();
            loadSettingsFromDB();

            eventManager = new EventManager(storage);

            //load events
            long count = storage.checkEvents();
            if (count == 0) {
                log("Empty events table. Creating default events...");
                eventManager.initializeEvents();
                Thread.Sleep(1000);
            }
            eventManager.setEventsList(storage.loadEvents());
            eventManager.setActiveEventsList(storage.loadActiveEventsIDs());

            String events = "";
            foreach (Event e in eventManager.getEventsList()) {
                events += e.ToString() + " | ";
            }
            log("Events loaded: " + events.Substring(0, events.Length - 2));

            toolTipButtons.SetToolTip(btnArduino, "Terminal " + Settings.Default.Board);
            if (Settings.Default.Board == Arduino) {
                btnArduino.Image = imgListBoards.Images[0];
            } else {
                btnArduino.Image = imgListBoards.Images[1];
            }

            this.calibrationX = Settings.Default.calibrationX;
            this.calibrationY = Settings.Default.calibrationY;
            this.calibrationAngle = Settings.Default.calibrationAngle;

            if (calibrationX == 0 && calibrationY == 0 && calibrationAngle == 0) {
                btnCalibration.BackColor = this.BackColor;
            } else {
                btnCalibration.BackColor = Settings.Default.targetColor;
            }

            if (Properties.Settings.Default.targetDistance != 100) {
                btnConfig.BackColor = Properties.Settings.Default.targetColor;
            } else {
                btnConfig.BackColor = this.BackColor;
            }

            toolTipButtons.SetToolTip(btnCalibration, "Calibration - X: " + calibrationX + " Y: " + calibrationY + " Angle: " + calibrationAngle);

            toolTipButtons.SetToolTip(btnConfig, "App Config - Target distance percent: " + Properties.Settings.Default.targetDistance.ToString(CultureInfo.InvariantCulture));

            initBreakdownChart();

            digitalClock.segments[1].ColonShow = true;
            digitalClock.segments[3].ColonShow = true;
            digitalClock.segments[1].ColonOn = true;
            digitalClock.segments[3].ColonOn = true;
            digitalClock.ResizeSegments();

            loadEventsOnTabs();

            if (cleanEnd == false) {
                recoverySessionName = recovery();
            }

        }

        private void initLog() {
            //init log location
            string logDirectory = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments) + @"\freETarget\log\";
            if (!Directory.Exists(logDirectory)) {
                try {
                    Directory.CreateDirectory(logDirectory);
                } catch (Exception ex) {
                    Console.WriteLine(ex.Message);

                    //if there is no write permission at exe location, write log in C:\Users\<user>\AppData\Roaming\freETarget\log
                    logDirectory = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + @"\freETarget\log\";
                    Directory.CreateDirectory(logDirectory);
                }
            }

            string logfilename = "freETarget." + DateTime.Now.ToString("yyyy.MM.dd") + ".log";
            if (!File.Exists(logDirectory + logfilename)) {
                FileStream log = null;
                try {
                    log = File.Create(logDirectory + logfilename);
                    logFile = logDirectory + logfilename;
                } catch (Exception ex) {
                    Console.WriteLine(ex.Message);
                    logFile = null;
                } finally {
                    log.Close();
                }

            } else {
                logFile = logDirectory + logfilename;
            }

            if (Properties.Settings.Default.fileLogging) {
                displayMessage("Log location: " + logFile, false);
            }

            //init shotfile - last shot is logged in this file, with X,Y coordinates and calculated score plus decimalscore
            if (!File.Exists(logDirectory + "Shot.log")) {
                FileStream shot = null;
                try {
                    shot = File.Create(logDirectory + "Shot.log");
                    shotFile = logDirectory + "Shot.log";
                } catch (Exception ex) {
                    Console.WriteLine(ex.Message);
                    shotFile = null;
                } finally {
                    shot.Close();
                }
            } else {
                shotFile = logDirectory + "Shot.log";
            }
        }

        public void log(string s) {
            log(s, this.logFile);
        }

        public void log(string s, string logFile) {
            log(s, logFile, false);
        }

        public void log(string s, string logFile, bool truncate) {
            if (s == null || s == "" || logFile == null) {
                return;
            }

            FileMode mode = FileMode.Append;
            if (truncate) {
                mode = FileMode.Truncate;
            }

            if (Properties.Settings.Default.fileLogging) { //log enabled from settings

                try {
                    //Opens a new file stream which allows asynchronous reading and writing
                    using (StreamWriter sw = new StreamWriter(new FileStream(logFile, mode, FileAccess.Write, FileShare.ReadWrite))) {
                        sw.WriteLine(DateTime.Now.ToString("yyyy.MM.dd HH:mm:ss:fff",CultureInfo.InvariantCulture) + " | " + s.Trim());
                    }
                } catch (Exception ex) {
                    //oh well...
                    Console.WriteLine("Error logging (" + logFile + "): " + ex.Message);
                }
            }
        }

        //called once at application start
        private void frmMainWindow_Load(object sender, EventArgs e) {
            initDefaultSession();
            this.WindowState = FormWindowState.Maximized;
        }

        private void initDefaultSession() {
            Event ev = eventManager.findEventByName(freETarget.Properties.Settings.Default.defaultTarget.Trim());

            if (ev == null) {
                MessageBox.Show("Default event is not available: " + Settings.Default.defaultTarget.Trim(), "Configuration error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                //Application.Exit();
                return;
            }

            foreach (TabPage tab in tcSessionType.TabPages) {
                if (ev.Name.Contains(tab.Text.Trim())) {
                    tcSessionType.SelectedTab = tab;
                    break;
                }
            }

            initNewSession();
        }

        private void initBreakdownChart() {
            Series series = chartBreakdown.Series[0];
            series.Points.AddXY("X", 0);
            series.Points.AddXY("10", 0);
            series.Points.AddXY("9", 0);
            series.Points.AddXY("8", 0);
            series.Points.AddXY("7", 0);
            series.Points.AddXY("6", 0);
            series.Points.AddXY("5", 0);
            series.Points.AddXY("4", 0);
            series.Points.AddXY("3", 0);
            series.Points.AddXY("2", 0);
            series.Points.AddXY("1", 0);
            series.Points.AddXY("0", 0);
            chartBreakdown.Update();
        }

        /*---------------------------------------------------------------------
         * 
         * serialPort_DataReceived()
         * 
         * Serial port received handler
         * 
         *---------------------------------------------------------------------
         * 
         * This function is called when the target emits a shot message.
         * 
         * Shot messages are a JSON packet of the form { message } <newline>
         * This function accumulates the incoming messages until a newline has
         * been received and then parses the message.
         * 
         * IMPORTANT
         * It is possible for this function to be called with a partial JSON
         * message thus the incoming messages are concatinated until the
         * newline arrives indicating a complete message ready to be parsed
         * 
         *--------------------------------------------------------------------*/
        private void DataReceived(object sender, comms.CommEventArgs e) {
            Application.DoEvents();
            //received data from serial port
            //SerialPort sp = (SerialPort)sender;
            string indata = e.Text.Trim().Replace('\0',' ');

            //first incoming text from target after port open is "freETarget VX.x" - use this to confirm connection
            if (/*indata.Contains("freETarget") &&*/ currentStatus == Status.CONECTING) {
                var d = new SafeCallDelegate2(connectDone); //confirm connect
                this.Invoke(d, new object[] { indata.Trim() });
            }

            output.add(indata);
            log(indata, logFile);

            //Console.WriteLine(Environment.NewLine + "Received: " + indata.Trim());
            incomingJSON += indata;                         // Accumulate the input

            int indexOpenBracket = incomingJSON.IndexOf('{');
            if (indexOpenBracket > -1) {
                int indexClosedBracket = incomingJSON.IndexOf('}', indexOpenBracket);

                if (indexClosedBracket > -1) {

                    String message = incomingJSON.Substring(indexOpenBracket + 1, indexClosedBracket - indexOpenBracket - 1);
                    incomingJSON = incomingJSON.Substring(indexClosedBracket + 1);

                    //Console.WriteLine("Complete json message: " + message);

                    //parse json message. might be a shot data or a test message
                    Shot shot = parseJson(message);

                    if (shot != null && shot.count >= 0) {
                        if (shot.miss == true) {
                            if (Settings.Default.ignoreMiss == true) {
                                //do nothing. ignore shot
                            } else {
                                currentSession.addShot(shot);

                                displayMessage(message, false);
                                displayShotData(shot);
                                VirtualRO vro = new VirtualRO(currentSession);
                                vro.speakShot(shot);
                            }
                        } else {
                            currentSession.addShot(shot);

                            displayMessage(message + " ---- " + "Computed shot X:" + shot.getX() + " Y:" + shot.getY() + " R:" + shot.radius + " A:" + shot.angle, false);
                            displayShotData(shot);
                            log("{\"shot\":" + shot.count + " \"x\":" + shot.getX().ToString(CultureInfo.InvariantCulture) + " \"y\":" + shot.getY().ToString(CultureInfo.InvariantCulture) + " \"score\":" + shot.score + " \"decimal\":" + shot.decimalScore.ToString(CultureInfo.InvariantCulture) + "}", shotFile, true);

                            VirtualRO vro = new VirtualRO(currentSession);
                            vro.speakShot(shot);

                            var d = new SafeCallDelegate3(targetRefresh); //draw shot
                            this.Invoke(d);

                            btnResume.BeginInvoke((Action)delegate ()
                            {
                                btnResume.Enabled = false;
                            });
                        }

                        if (incomingJSON.IndexOf("}") != -1) {

                            comms.CommEventArgs e2 = new comms.CommEventArgs("");
                            DataReceived(sender, e2); //call the event again to parse the remains. maybe there is another full message in there
                        }
                    } else {
                        lastEcho = Echo.parseJson(message);
                        log("Echo received =   " + (lastEcho != null ? lastEcho.ToString() : "null"));

                        //if target has OFFSET values, override local calibration -> set it to 0
                        if (lastEcho != null) {
                            if (lastEcho.X_OFFSET != decimal.Zero || lastEcho.Y_OFFSET != decimal.Zero) {
                                this.calibrationX = 0;
                                this.calibrationY = 0;
                                btnCalibration.BackColor = this.BackColor;
                                log("ECHO returned non-zero OFFSETS  X:" + lastEcho.X_OFFSET + " Y:" + lastEcho.Y_OFFSET + ". Reseting local calibration to zero... ");
                            }
                        }

                        if (incomingJSON.IndexOf("}") != -1) {

                            comms.CommEventArgs e2 = new comms.CommEventArgs("");
                            DataReceived(sender, e2); //call the event again to parse the remains. maybe there is another full message in there
                        }
                    }
                }
            }

        }

        private void btnConnect_Click(object sender, EventArgs e) {
            if (currentStatus == Status.NOT_CONNECTED) {

                if (Properties.Settings.Default.CommProtocol == "USB") {
                    if (Properties.Settings.Default.portName == null || Properties.Settings.Default.portName.Trim() == "") {
                        MessageBox.Show("No COM port selected. Please go to the Settings dialog and select a port.", "Cannot connect", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                        return;
                    }
                } else {
                    //TCP
                    if (Properties.Settings.Default.TcpIP == null || Properties.Settings.Default.TcpIP.Trim() == "") {
                        MessageBox.Show("No TCP IP address entered. Please go to the Settings dialog and enter an IP address.", "Cannot connect", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                        return;
                    }

                    if (Properties.Settings.Default.TcpPort <= 0) {
                        MessageBox.Show("No TCP port entered. Please go to the Settings dialog and enter an TCP port.", "Cannot connect", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                        return;
                    }
                }

                if (Properties.Settings.Default.name == null || Properties.Settings.Default.name.Trim() == "") {
                    MessageBox.Show("No shooter name entered. Please go to the Settings dialog and enter a name.", "Cannot connect", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                    return;
                }

                //read settings to determine the appropriate comm module
                if (Settings.Default.CommProtocol == "USB") {
                    this.commModule = new comms.USB(this);
                } else {
                    //TCP
                    this.commModule = new comms.TCP(this);
                }
                this.commModule.CommDataReceivedEvent += new comms.aCommModule.CommEventHandler(this.DataReceived);
                this.commModule.CommDisconnectedEvent += new comms.aCommModule.CommEventHandler(this.Disconnected);

                //use parameters for the selected comm
                comms.OpenParams para;
                if (Settings.Default.CommProtocol == "USB") {
                    para = new comms.UsbOpenParams();
                    ((comms.UsbOpenParams)para).portName = Properties.Settings.Default.portName;
                    ((comms.UsbOpenParams)para).baudRate = Properties.Settings.Default.baudRate;
                    ((comms.UsbOpenParams)para).dataBits = 8;
                    if (Properties.Settings.Default.Board == Arduino) {
                        ((comms.UsbOpenParams)para).dtrEnable = true; //seems like both ESP and Arduino work with the same settings
                        ((comms.UsbOpenParams)para).rtsEnable = true; //but this might change in the future
                    } else {
                        ((comms.UsbOpenParams)para).dtrEnable = true;
                        ((comms.UsbOpenParams)para).rtsEnable = true;
                    }
                } else {
                    para = new comms.TcpOpenParams();
                    ((comms.TcpOpenParams)para).IP = Settings.Default.TcpIP;
                    ((comms.TcpOpenParams)para).port = Settings.Default.TcpPort;
                    String ssid = getWifiSSID();
                    if (!ssid.StartsWith(FET_SSID_PREFIX)) {
                        log("Current Wi-Fi network(" + ssid + ") is not a freETarget SSID.");
                    }

                }





                try {

                    //open port. the target (arduino) will send a text "freETarget VX.X" on connect. 
                    //software will wait for this text to confirm succesfull connection
                    statusText.Text = "Connecting...";
                    btnConnect.Text = "Cancel";
                    currentStatus = Status.CONECTING;
                    Application.DoEvents();
                    commModule.open(para);

                } catch (Exception ex) {
                    log("Error connecting: " + ex.Message);
                    statusText.Text = "Error opening comms: " + ex.Message;
                    btnConnect.Text = "Connect";
                    currentStatus = Status.NOT_CONNECTED;
                    MessageBox.Show("Failed to connect to the target", "Connecting error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }

            } else {
                if (!clearShots()) {
                    disconnect();
                }
            }


        }

        /**
         * received connection text from target. connection established
         */
        private void connectDone(String target) {

            timer.Enabled = true;

            Thread.Sleep(500);
            commModule.sendData("{\"VERSION\":7}");
            log("Sending: " + "{\"VERSION\":7}");
            Thread.Sleep(100);


            btnConnect.Text = "Disconnect";
            currentStatus = Status.CONNECTED;
            if (Settings.Default.CommProtocol == "TCP") {
                connectionStatus.BackColor = Color.Green;
            }

            String t = target;
            if (t.IndexOf(Environment.NewLine) > -1) {
                t = t.Substring(0, t.IndexOf(Environment.NewLine));
            }
            t = t.Trim();
            statusText.Text = "Connected to " + t + " on " + commModule.getCommInfo();
            log("****************\nConnected to " + t + " on " + commModule.getCommInfo());
            displayMessage("Connected to " + t, false);

            Application.DoEvents();

            btnConnect.ImageKey = "disconnect";
            shotsList.Enabled = true;
            btnCalibration.Enabled = true;
            btnArduino.Enabled = true;
            btnTargetSettings.Enabled = true;
            btnUpload.Enabled = false;
            btnResume.Enabled = true;
            trkZoom.Enabled = true;
            tcSessionType.Enabled = true;
            tcSessionType.Refresh();

            SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED); //disable screensaver while connected

            initNewSession();
            targetRefresh();

            if (recoveryLines!=null) {
                //there are shots to recover
                log("Recovering shots...");
                foreach (string line in recoveryLines) {
                    comms.CommEventArgs e2 = new comms.CommEventArgs(line);
                    DataReceived(null, e2);
                }

                currentSession.startTime = DateTime.ParseExact(recoverySessionTime, "yyyy.MM.dd HH:mm:ss:fff", CultureInfo.InvariantCulture);
                currentSession.continueSession();

                recoveryLines = null;
                recoverySessionName = "";
                recoverySessionTime = "";
            }
        }

        //output messages
        public void displayMessage(string text, bool showToolTip) {
            if (txtOutput.InvokeRequired) {
                var d = new SafeCallDelegate4(displayMessage);
                txtOutput.Invoke(d, new object[] { text, showToolTip });
                return;
            } else {
                Console.WriteLine(text);
                txtOutput.AppendText(text);
                txtOutput.AppendText(Environment.NewLine);
                if (showToolTip) {
                    this.Focus();
                    toolTip.Show(text, imgTarget, 2000);
                    tooltipDisplayTime = DateTime.Now;
                }
            }
        }


        //write shot data
        private void displayShotData(Shot shot) {
            //special code for UI thread safety
            if (txtOutput.InvokeRequired) {
                var d = new SafeCallDelegate(displayShotData);
                txtOutput.Invoke(d, new object[] { shot });
                return;
            } else {
                string inner = "";
                if (shot.innerTen) {
                    inner = " *";
                }

                //write to total textbox
                if (Properties.Settings.Default.showScoring) {
                    txtTotal.Text = getShots().Count + ": " + currentSession.score.ToString() + " (" + currentSession.decimalScore.ToString(CultureInfo.InvariantCulture) + ") -" + currentSession.innerX.ToString() + "x";
                } else {
                    txtTotal.Text = "XXX";
                }

                //write to last shot textbox
                string lastShot = shot.decimalScore.ToString(CultureInfo.InvariantCulture);
                if (Properties.Settings.Default.showScoring) {
                    txtLastShot.Text = lastShot + inner;
                } else {
                    txtLastShot.Text = "XX";
                }

                drawArrow(shot);

                //write score to listview
                ListViewItem item = new ListViewItem(new string[] { "" }, (shot.index + 1).ToString());
                item.UseItemStyleForSubItems = false;
                ListViewItem.ListViewSubItem countItem = item.SubItems.Add((shot.index + 1).ToString());
                countItem.Font = new Font("MS Sans Serif", 7f, FontStyle.Italic | FontStyle.Bold);
                if (Properties.Settings.Default.showScoring) {
                    ListViewItem.ListViewSubItem scoreItem = item.SubItems.Add(shot.score.ToString());
                    scoreItem.Font = new Font("MS Sans Serif", 9.75f, FontStyle.Bold);
                    ListViewItem.ListViewSubItem decimalItem = item.SubItems.Add(shot.decimalScore.ToString(CultureInfo.InvariantCulture) + inner);
                    decimalItem.Font = new Font("MS Sans Serif", 9.75f, FontStyle.Bold);
                } else {
                    ListViewItem.ListViewSubItem scoreItem = item.SubItems.Add("XX");
                    scoreItem.Font = new Font("MS Sans Serif", 9.75f, FontStyle.Bold);
                    ListViewItem.ListViewSubItem decimalItem = item.SubItems.Add("XX.X");
                    decimalItem.Font = new Font("MS Sans Serif", 9.75f, FontStyle.Bold);
                }


                shotsList.Items.Add(item);
                shotsList.EnsureVisible(shotsList.Items.Count - 1);

                displayShotStatistics(getShotList());
                writeToGrid(shot);
                if (Properties.Settings.Default.showScoring) {
                    fillBreakdownChart(getShots());
                }

            }
        }

        private void displayShotStatistics(List<Shot> shotList) {
            if (shotList.Count > 1) {
                currentSession.calculateMeanValuesAndGroupSize(shotList);

                txtMeanRadius.Text = Math.Round(currentSession.rbar, 1).ToString();
                txtWindage.Text = Math.Round(Math.Abs(currentSession.xbar), 1).ToString();
                txtElevation.Text = Math.Round(Math.Abs(currentSession.ybar), 1).ToString();
                drawWindageAndElevationArrows(currentSession.xbar, currentSession.ybar);

                txtMaxSpread.Text = currentSession.groupSize.ToString();
            } else {
                txtMeanRadius.Text = "";
                txtWindage.Text = "";
                txtElevation.Text = "";
                txtMaxSpread.Text = "";
                imgElevation.CreateGraphics().Clear(this.BackColor);
                imgWindage.CreateGraphics().Clear(this.BackColor);
            }
        }

        //draw 2 small arrows next to windage and elevation
        private void drawWindageAndElevationArrows(decimal xbar, decimal ybar) {
            Bitmap bmp = new Bitmap(imgWindage.Width, imgWindage.Height);
            Graphics g = Graphics.FromImage(bmp);
            g.Clear(this.BackColor);
            g.SmoothingMode = SmoothingMode.AntiAlias;
            Pen arr = new Pen(Color.Black);
            arr.CustomEndCap = new AdjustableArrowCap(3, 3, true);
            if (xbar < 0) {
                g.DrawLine(arr, imgWindage.Width - 2, (imgWindage.Height / 2) - 2, 2, (imgWindage.Height / 2) - 2);
            } else {
                g.DrawLine(arr, 2, (imgWindage.Height / 2) - 2, imgWindage.Width - 2, (imgWindage.Height / 2) - 2);
            }


            imgWindage.Image = bmp;

            bmp = new Bitmap(imgElevation.Width, imgElevation.Height);
            g = Graphics.FromImage(bmp);
            g.Clear(this.BackColor);
            g.SmoothingMode = SmoothingMode.AntiAlias;
            if (ybar < 0) {
                g.DrawLine(arr, (imgElevation.Height / 2) - 2, 2, (imgElevation.Height / 2) - 2, imgElevation.Width - 2);
            } else {
                g.DrawLine(arr, (imgElevation.Height / 2) - 2, imgElevation.Width - 2, (imgElevation.Height / 2) - 2, 2);
            }

            imgElevation.Image = bmp;
        }

        private int getZoom() {
            if (trkZoom.InvokeRequired) {
                int r = 1;
                trkZoom.Invoke(new MethodInvoker(delegate {
                    r = trkZoom.Value;
                }));
                return r;
            } else {
                return trkZoom.Value;
            }
        }

        //draw direction arrow
        private void drawArrow(Shot shot) {
            Bitmap bmp = new Bitmap(imgArrow.Width, imgArrow.Height);
            Graphics g = Graphics.FromImage(bmp);
            g.SmoothingMode = SmoothingMode.AntiAlias;
            g.Clear(Color.White);

            int margin = 3;

            if (shot.miss == true) {
                Brush br = new SolidBrush(Color.Black);
                Font f = new Font("Arial", 20);
                StringFormat format = new StringFormat();
                format.LineAlignment = StringAlignment.Center;
                format.Alignment = StringAlignment.Center;
                g.DrawString("M", f, br, imgArrow.Width / 2, imgArrow.Height / 2, format);
            } else {
                if (shot.decimalScore < 10.9m) {
                    RectangleF range = new RectangleF(margin, margin, imgArrow.Width - margin * 3, imgArrow.Height - margin * 3);
                    double xp = 18.0, yp = 18.0;
                    double θ = (double)shot.angle * (Math.PI / 180);

                    double[] t = new double[4];
                    t[0] = (range.Left - xp) / Math.Cos(θ);
                    t[1] = (range.Right - xp) / Math.Cos(θ);
                    t[2] = (range.Top - yp) / Math.Sin(θ);
                    t[3] = (range.Bottom - yp) / Math.Sin(θ);
                    Array.Sort(t);
                    // pick middle two points
                    var X1 = xp + t[1] * Math.Cos(θ);
                    var Y1 = yp + t[1] * Math.Sin(θ);
                    var X2 = xp + t[2] * Math.Cos(θ);
                    var Y2 = yp + t[2] * Math.Sin(θ);

                    Pen arr = new Pen(Color.Black, 2);
                    arr.CustomEndCap = new AdjustableArrowCap(6, 6, true);
                    g.DrawLine(arr, (float)X1, (float)Y2, (float)X2, (float)Y1);
                } else { //if 10.9 draw a dot
                    Brush br = new SolidBrush(Color.Black);
                    g.FillEllipse(br, new Rectangle(new Point((imgArrow.Width / 2) - 4, (imgArrow.Height / 2) - 4), new Size(6, 6)));
                }
            }

            imgArrow.Image = bmp;

            //save drawn image (arrow) to imagelist for listview column
            try {
                imgListDirections.Images.Add((shot.index + 1).ToString(), imgArrow.Image);
            } catch (Exception ex) {
                Console.WriteLine("Error adding image to list " + ex.Message);
            }

            Thread.Sleep(100);
        }


        private decimal getScaledDimension(decimal input) {
            decimal ret = 100 * input / Settings.Default.targetDistance;
            ret = decimal.Round(ret, 9, MidpointRounding.AwayFromZero);
            return ret;
        }


        /**
        *  incoming message contains all the data without the accolades
        */
        private Shot parseJson(string json) {
            //parse json shot data


            string[] t2 = json.Split(',');

            if (t2[0].Contains("shot")) {
                Shot ret = new Shot(calibrationX, calibrationY, calibrationAngle);
                try {
                    foreach (string t3 in t2) {
                        string[] t4 = t3.Split(':');
                        if (t4[0].Trim() == "\"shot\"") {
                            ret.count = int.Parse(t4[1], CultureInfo.InvariantCulture);
                        } else if (t4[0].Trim() == "\"x\"") {
                            ret.setX(getScaledDimension(decimal.Parse(t4[1], CultureInfo.InvariantCulture)));
                        } else if (t4[0].Trim() == "\"y\"") {
                            ret.setY(getScaledDimension(decimal.Parse(t4[1], CultureInfo.InvariantCulture)));
                        } else if (t4[0].Trim() == "\"r\"") {
                            ret.radius = getScaledDimension(decimal.Parse(t4[1], CultureInfo.InvariantCulture));
                        } else if (t4[0].Trim() == "\"a\"") {
                            ret.angle = decimal.Parse(t4[1], CultureInfo.InvariantCulture);
                        } else if (t4[0].Trim() == "\"miss\"") {
                            int mix = int.Parse(t4[1].Trim(), CultureInfo.InvariantCulture);
                            if (mix == 1) {
                                //miss reported by the target
                                ret.miss = true;
                            }

                        }
                    }
                } catch (FormatException ex) {
                    Console.WriteLine("Could not parse: " + json + " Error: " + ex.Message);
                    Shot err = new Shot(calibrationX, calibrationY, calibrationAngle);
                    err.count = -1;
                    return err;
                }

                Console.WriteLine("Shot X:" + ret.getX() + " Y:" + ret.getY() + " R:" + ret.radius + " A:" + ret.angle + " Miss:" + ret.miss);



                ret.computeScore(currentSession.getTarget());

                ret.timestamp = DateTime.Now;

                return ret;

            } else if (t2[0].Contains("timer")) {
                // bad shot. do anything?
                displayMessage("Bad shot reported by the target: " + json, false);
                return null;
            } else {
                return null;
            }


        }


        private void imgArrow_LoadCompleted(object sender, AsyncCompletedEventArgs e) {
            if (e.Error != null) {
                // You got the Error image, e.Error tells you why
                Console.WriteLine("Image load error! " + e.Error);
            }
        }

        private void btnConfig_Click(object sender, EventArgs e) {
            frmSettings settingsFrom = new frmSettings(this);
            if (settingsFrom.ShowDialog(this) == DialogResult.OK) {
                Properties.Settings.Default.name = settingsFrom.txtName.Text;
                Properties.Settings.Default.baudRate = int.Parse(settingsFrom.txtBaud.Text, CultureInfo.InvariantCulture);
                Properties.Settings.Default.displayDebugConsole = settingsFrom.chkDisplayConsole.Checked;
                Properties.Settings.Default.portName = settingsFrom.cmbPorts.GetItemText(settingsFrom.cmbPorts.SelectedItem);

                Event ev = (Event)settingsFrom.cmbWeapons.SelectedItem;
                Properties.Settings.Default.defaultTarget = ev.Name;
                Properties.Settings.Default.targetColor = Color.FromName(settingsFrom.cmbColor.GetItemText(settingsFrom.cmbColor.SelectedItem));
                Properties.Settings.Default.drawMeanGroup = settingsFrom.chkDrawMeanG.Checked;
                Properties.Settings.Default.OnlySeries = settingsFrom.chkSeries.Checked;
                Properties.Settings.Default.voiceCommands = settingsFrom.chkVoice.Checked;
                Properties.Settings.Default.pdfPath = settingsFrom.txtPDFlocation.Text;
                Properties.Settings.Default.targetDistance = decimal.Parse(settingsFrom.txtDistance.Text, CultureInfo.InvariantCulture);
                Properties.Settings.Default.scoreVoice = settingsFrom.chkScoreVoice.Checked;
                Properties.Settings.Default.fileLogging = settingsFrom.chkLog.Checked;
                Properties.Settings.Default.ignoreMiss = settingsFrom.chkMiss.Checked;
                Properties.Settings.Default.showScoring = settingsFrom.chkShowScoring.Checked;

                if (Properties.Settings.Default.targetDistance != 100) {
                    btnConfig.BackColor = Properties.Settings.Default.targetColor;
                } else {
                    btnConfig.BackColor = SystemColors.Control;
                }
                toolTipButtons.SetToolTip(btnConfig, "Settings - Target distance percent: " + Properties.Settings.Default.targetDistance);

                if (currentSession != null) {
                    displayShotStatistics(getShotList());
                    displayDebugConsole(Properties.Settings.Default.displayDebugConsole);
                }

                Properties.Settings.Default.score10BackgroundColor = Color.FromName(settingsFrom.cmb10Back.GetItemText(settingsFrom.cmb10Back.SelectedItem));
                Properties.Settings.Default.score10PenColor = Color.FromName(settingsFrom.cmb10Pen.GetItemText(settingsFrom.cmb10Pen.SelectedItem));
                Properties.Settings.Default.score9BackgroundColor = Color.FromName(settingsFrom.cmb9Back.GetItemText(settingsFrom.cmb9Back.SelectedItem));
                Properties.Settings.Default.score9PenColor = Color.FromName(settingsFrom.cmb9Pen.GetItemText(settingsFrom.cmb9Pen.SelectedItem));
                Properties.Settings.Default.scoreDefaultBackgroundColor = Color.FromName(settingsFrom.cmbDefBack.GetItemText(settingsFrom.cmbDefBack.SelectedItem));
                Properties.Settings.Default.scoreDefaultPenColor = Color.FromName(settingsFrom.cmbDefPen.GetItemText(settingsFrom.cmbDefPen.SelectedItem));
                Properties.Settings.Default.scoreOldBackgroundColor = Color.FromName(settingsFrom.cmbOldBack.GetItemText(settingsFrom.cmbOldBack.SelectedItem));
                Properties.Settings.Default.scoreOldPenColor = Color.FromName(settingsFrom.cmbOldPen.GetItemText(settingsFrom.cmbOldPen.SelectedItem));

                Properties.Settings.Default.CommProtocol = settingsFrom.cmbCommProtocol.SelectedItem.ToString();
                Properties.Settings.Default.TcpIP = settingsFrom.txtIP.Text;
                Properties.Settings.Default.TcpPort = int.Parse(settingsFrom.txtPort.Text, CultureInfo.InvariantCulture);

                if (settingsFrom.rbArduino.Checked) {
                    Properties.Settings.Default.Board = Arduino;
                    btnArduino.Image = imgListBoards.Images[0];
                } else {
                    Properties.Settings.Default.Board = ESP32;
                    btnArduino.Image = imgListBoards.Images[1];
                }

                Properties.Settings.Default.Save();
                saveSettings(); //save settings to DB as well

                List<Event> activeEvents = new List<Event>();
                foreach (object oev in settingsFrom.lstbActiveEvents.Items) {
                    activeEvents.Add((Event)oev);
                }

                //save only to DB. the new active events list with become live after app restart.
                storage.updateActiveEvents(activeEvents);
                eventManager.setActiveEventsList(storage.loadActiveEventsIDs());

            }

            settingsFrom.Dispose();
        }

        private void displayDebugConsole(bool display) {
            if (display) {
                txtOutput.Visible = true;
                splitContainer.Panel2Collapsed = false;
            } else {
                txtOutput.Visible = false;
                splitContainer.Panel2Collapsed = true;
            }
            targetRefresh();
            Application.DoEvents();
        }



        private void frmMainWindow_FormClosing(object sender, FormClosingEventArgs e) {
            if (currentStatus == Status.CONNECTED) {
                if (clearShots()) {
                    e.Cancel = true;
                    return;
                }

                disconnect();

            }

            log("@@@@@ normal shutdown @@@@@");
        }

        private void frmMainWindow_Shown(object sender, EventArgs e) {
            displayDebugConsole(Properties.Settings.Default.displayDebugConsole);

            if (splitContainer != null && splitContainer.Panel1 != null) {
                splitContainer.SplitterDistance = splitContainer.Panel1.Height;
            }

            trkZoom.Focus();
        }

        private void initNewSession() {
            if (currentSession != null && currentSession.Shots.Count > 0 && currentStatus != Status.LOADED) {
                if (currentSession.id == 0) {
                    storage.storeSession(currentSession, true);
                    displayMessage("Session saved", true);
                } else {
                    storage.updateSession(currentSession);
                    displayMessage("Session " + currentSession.id + " updated", true);
                }

            }

            string sessionName = tcSessionType.SelectedTab.Text.Trim();
            if (recoverySessionName != "") {
                sessionName = recoverySessionName.Trim();
                foreach (TabPage tab in tcSessionType.TabPages) {
                    if (sessionName.Contains(tab.Text.Trim())) {
                        tcSessionType.SelectedTab = tab;
                        break;
                    }
                }
            }

            this.log("       Starting new session with name: " + sessionName);

            Event ev = eventManager.findEventByName(sessionName);
            if (ev == null) {
                this.displayMessage("Could not find event with name " + sessionName, false);
                this.log("Could not find event with name " + sessionName);
                MessageBox.Show("Could not find event with name " + sessionName, "Configuration error", MessageBoxButtons.OK, MessageBoxIcon.Error);

                return;
            }

            currentSession = Session.createNewSession(ev, Settings.Default.name);
            currentSession.start();
            this.log("### New Session '" + currentSession.ToString() + "' started ###");
            if (commModule != null) { // there is a default session starting before the connection to the target is done
                String sessionStart = "{\"ATHLETE\":\"" + currentSession.user + "\", \"EVENT\":\"" + currentSession.ToString() + "\", \"TARGET_NAME\":\"" + currentSession.targetType + "\"}";
                commModule.sendData(sessionStart);
                log("Sending: " + sessionStart);
            }

            setTrkZoom(currentSession.getTarget());

            clearShots();
            targetRefresh();
            imgTarget.BackColor = Settings.Default.targetColor;
            drawSessionName();

            if (ev.RapidFire) {
                btnStart.Visible = true;
                btnStart.Enabled = true;
            } else {
                btnStart.Visible = false;
                btnStart.Enabled = false;
            }
        }

        private void setTrkZoom(targets.aTarget target) {
            trkZoom.Minimum = target.getTrkZoomMinimum();
            trkZoom.Maximum = target.getTrkZoomMaximum();
            trkZoom.Value = target.getTrkZoomValue();
        }

        private void trkZoom_ValueChanged(object sender, EventArgs e) {
            targetRefresh();
        }

        private void frmMainWindow_Resize(object sender, EventArgs e) {
            targetRefresh();
            tcSessionType.Height = this.Height - 100;
        }

        private void targetRefresh() {

            int height = splitContainer.Panel1.Height;
            int width = splitContainer.Panel1.Width;

            if (height < width) {
                imgTarget.Height = height;
                imgTarget.Width = height;

                btnStart.Width = height - 4;
                btnStart.Top = splitContainer.Panel1.Height - btnStart.Height - 2;

            } else {
                imgTarget.Height = width;
                imgTarget.Width = width;

                btnStart.Width = width - 4;
                btnStart.Top = splitContainer.Panel1.Height - btnStart.Height - 2;
            }

            drawTarget();
        }

        private void drawTarget() {
            if (currentSession != null) {
                bool notConnected = currentStatus == Status.NOT_CONNECTED;
                imgTarget.Image = currentSession.getTarget().paintTarget(imgTarget.Height, getZoom(), notConnected, currentSession, getShotList());
            } else {
                Console.WriteLine("Current session is null");
            }
        }

        private bool clearShots() {
            if (currentSession.Shots.Count > 0) {
                DialogResult result = MessageBox.Show("Current session is unsaved. Do you want to save it?"
                    + Environment.NewLine + Environment.NewLine
                    + "'Yes' and 'No' will close the session."
                    + Environment.NewLine + "'Cancel' will keep the current session alive.", "Save session", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);

                if (result == DialogResult.Yes) {

                    if (currentSession.id == 0) {
                        storage.storeSession(currentSession, true);
                        displayMessage("Session saved", true);
                    } else {
                        storage.updateSession(currentSession);
                        displayMessage("Session " + currentSession.id + " updated", true);
                    }

                } else if (result == DialogResult.Cancel) {
                    return true;
                }
            }

            currentSession.clear();
            targetRefresh();
            shotsList.Items.Clear();
            shotsList.Refresh();
            txtLastShot.Text = "";
            imgArrow.Image = new Bitmap(imgArrow.Width, imgArrow.Height);
            imgArrow.Refresh();
            imgListDirections.Images.Clear();
            Application.DoEvents();
            txtTotal.Text = "";
            txtTime.Text = "";
            txtWindage.Text = "";
            txtElevation.Text = "";
            txtMaxSpread.Text = "";
            txtMeanRadius.Text = "";
            imgElevation.CreateGraphics().Clear(this.BackColor);
            imgWindage.CreateGraphics().Clear(this.BackColor);
            gridTargets.Rows.Clear();
            clearBreakdownChart();
            digitalClock.Value = "";
            digitalClock.ColorLight = Color.White;

            return false;
        }

        private void timer_Tick(object sender, EventArgs e) {
            Color c = Color.White;

            txtTime.Text = DateTime.Now.ToString("yyyy-MM-dd  HH:mm:ss");
            string time = currentSession.getTime(out c);

            if (time.StartsWith("@")) {
                digitalClock.segments[1].ColonShow = false;
                digitalClock.segments[3].ColonShow = false;
                digitalClock.segments[1].ColonOn = false;
                digitalClock.segments[3].ColonOn = false;
            } else {
                digitalClock.segments[1].ColonShow = true;
                digitalClock.segments[3].ColonShow = true;
                digitalClock.segments[1].ColonOn = true;
                digitalClock.segments[3].ColonOn = true;
            }
            digitalClock.Value = time;
            digitalClock.ColorLight = c;

        }







        private void btnCalibration_Click(object sender, EventArgs e) {
            frmCalibration frmCal = frmCalibration.getInstance(this);
            frmCal.Show();
        }

        public void calibrateX(decimal increment) {
            calibrationX += increment;

            int sessionScore = 0;
            decimal sessionDecimalScore = 0;
            int sessionInnerTens = 0;

            foreach (Shot shot in getShots()) {
                shot.calibrationX = calibrationX;
                shot.computeScore(this.currentSession.getTarget());

                sessionScore += shot.score;
                sessionDecimalScore += shot.decimalScore;
                if (shot.innerTen) {
                    sessionInnerTens++;
                }
            }

            this.currentSession.score = sessionScore;
            this.currentSession.decimalScore = sessionDecimalScore;
            this.currentSession.innerX = sessionInnerTens;

            shotsList.Items.Clear();
            shotsList.Refresh();
            imgListDirections.Images.Clear();

            gridTargets.Rows.Clear();
            clearBreakdownChart();
            foreach (Shot shot in getShots()) { //refresh all shots, not just currest series
                displayShotData(shot);
            }
            targetRefresh();

            if (calibrationX == 0 && calibrationY == 0 && calibrationAngle == 0) {
                btnCalibration.BackColor = this.BackColor;
            } else {
                btnCalibration.BackColor = Settings.Default.targetColor;
            }

            toolTipButtons.SetToolTip(btnCalibration, "Calibration - X: " + calibrationX + " Y: " + calibrationY + " Angle: " + calibrationAngle);
        }

        public void calibrateY(decimal increment) {
            calibrationY += increment;

            int sessionScore = 0;
            decimal sessionDecimalScore = 0;
            int sessionInnerTens = 0;

            foreach (Shot shot in getShots()) {
                shot.calibrationY = calibrationY;
                shot.computeScore(this.currentSession.getTarget());

                sessionScore += shot.score;
                sessionDecimalScore += shot.decimalScore;
                if (shot.innerTen) {
                    sessionInnerTens++;
                }
            }

            this.currentSession.score = sessionScore;
            this.currentSession.decimalScore = sessionDecimalScore;
            this.currentSession.innerX = sessionInnerTens;

            shotsList.Items.Clear();
            shotsList.Refresh();
            imgListDirections.Images.Clear();

            gridTargets.Rows.Clear();
            clearBreakdownChart();
            foreach (Shot shot in getShots()) { //refresh all shots, not just currest series
                displayShotData(shot);
            }
            targetRefresh();

            if (calibrationX == 0 && calibrationY == 0 && calibrationAngle == 0) {
                btnCalibration.BackColor = this.BackColor;
            } else {
                btnCalibration.BackColor = Settings.Default.targetColor;
            }
            toolTipButtons.SetToolTip(btnCalibration, "Calibration - X: " + calibrationX + " Y: " + calibrationY + " Angle: " + calibrationAngle);
        }

        public void calibrateAngle(decimal angle) {
            calibrationAngle += angle;
            foreach (Shot shot in getShots()) {
                shot.calibrationAngle = calibrationAngle;
                shot.computeScore(this.currentSession.getTarget());
            }
            shotsList.Items.Clear();
            shotsList.Refresh();
            gridTargets.Rows.Clear();
            clearBreakdownChart();
            foreach (Shot shot in getShots()) { //refresh all shots, not just currest series
                displayShotData(shot);
            }
            targetRefresh();

            if (calibrationX == 0 && calibrationY == 0 && calibrationAngle == 0) {
                btnCalibration.BackColor = this.BackColor;
            } else {
                btnCalibration.BackColor = Settings.Default.targetColor;
            }
            toolTipButtons.SetToolTip(btnCalibration, "Calibration - X: " + calibrationX + " Y: " + calibrationY + " Angle: " + calibrationAngle);
        }

        public void resetCalibration() {
            calibrationX = 0;
            calibrationY = 0;
            calibrationAngle = 0;

            foreach (Shot shot in getShotList()) {
                shot.calibrationX = calibrationX;
                shot.calibrationY = calibrationY;
                shot.calibrationAngle = calibrationAngle;
                shot.computeScore(this.currentSession.getTarget());
            }

            shotsList.Items.Clear();
            shotsList.Refresh();
            imgListDirections.Images.Clear();

            gridTargets.Rows.Clear();
            clearBreakdownChart();
            foreach (Shot shot in getShots()) { //refresh all shots, not just currest series
                displayShotData(shot);
            }

            targetRefresh();

            btnCalibration.BackColor = this.BackColor;
            toolTipButtons.SetToolTip(btnCalibration, "Calibration - X: " + calibrationX + " Y: " + calibrationY + " Angle: " + calibrationAngle);
        }

        public void saveCalibration() {
            Settings.Default.calibrationX = calibrationX;
            Settings.Default.calibrationY = calibrationY;
            Settings.Default.calibrationAngle = calibrationAngle;
            Settings.Default.Save();
            saveSettings(); //save settings to DB as well
        }

        private void tabControl1_DrawItem(object sender, DrawItemEventArgs e) {
            Color backC = tcSessionType.TabPages[e.Index].BackColor;
            Color foreC = tcSessionType.TabPages[e.Index].ForeColor;
            if (tcSessionType.Enabled == false) {
                int grayScale = (int)((backC.R * 0.3) + (backC.G * 0.59) + (backC.B * 0.11));
                backC = Color.FromArgb(backC.A, grayScale, grayScale, grayScale);
            }

            e.Graphics.FillRectangle(new SolidBrush(backC), e.Bounds);
            Rectangle paddedBounds = e.Bounds;

            bool sel = false;
            if (tcSessionType.SelectedIndex == e.Index) {
                sel = true;
            }

            paddedBounds.Inflate(-3, -3);

            StringFormat format1h = new StringFormat(StringFormatFlags.DirectionVertical | StringFormatFlags.DirectionRightToLeft);
            Font f;
            if (sel) {
                f = new Font(e.Font, FontStyle.Bold | FontStyle.Italic);
                paddedBounds.X = 0;
            } else {
                f = new Font(e.Font, FontStyle.Bold);
            }
            e.Graphics.DrawString(tcSessionType.TabPages[e.Index].Text, f, new SolidBrush(foreC), paddedBounds, format1h);
        }

        private void tcSessionType_SelectedIndexChanged(object sender, EventArgs e) {
            initNewSession();
        }

        private void drawSessionName() {

            Color color = Color.Black;
            foreach (TabPage tab in tcSessionType.TabPages) {
                if (currentSession.eventType.Name.Contains(tab.Text.Trim())) {
                    color = tab.BackColor;
                    break;
                }
            }

            Bitmap bmpTarget = new Bitmap(imgSessionName.Width, imgSessionName.Height);
            Graphics g = Graphics.FromImage(bmpTarget);
            g.SmoothingMode = SmoothingMode.AntiAlias;
            if (currentStatus == Status.CONNECTED || currentStatus == Status.LOADED) {
                g.Clear(color);
            } else {
                g.Clear(Color.Black);
            }
            Font f = new Font("Tahoma", 10, FontStyle.Bold);
            Brush b = new SolidBrush(Color.Black);
            StringFormat format = new StringFormat();
            format.LineAlignment = StringAlignment.Center;
            format.Alignment = StringAlignment.Center;

            string name = Settings.Default.name + " - " + currentSession.eventType;
            if (currentSession.numberOfShots > 0) {
                name += " " + currentSession.numberOfShots;
            }
            g.DrawString(name, f, b, imgSessionName.ClientRectangle, format);

            imgSessionName.Image = bmpTarget;
        }

        private void writeToGrid(Shot shot) {
            int rowShot;
            int cellShot;
            Event ev = currentSession.eventType;
            if (currentSession.sessionType != Event.EventType.Final) {
                rowShot = shot.index / 10;
                cellShot = shot.index % 10;
            } else {
                //final - 2 rows of 5 shots and than rows of 2 shots
                if (shot.index < ev.Final_NumberOfShotsBeforeSingleShotSeries) {
                    //series rows
                    rowShot = shot.index / ev.Final_NumberOfShotPerSeries;
                    cellShot = shot.index % ev.Final_NumberOfShotPerSeries;
                } else {
                    //individual shot(s) series
                    rowShot = (shot.index - (ev.Final_NumberOfShotPerSeries + 1)) / ev.Final_NumberOfShotsInSingleShotSeries;
                    cellShot = shot.index % ev.Final_NumberOfShotsInSingleShotSeries;
                }
            }

            if (rowShot < 0) {
                displayMessage("Error computing grid row " + rowShot, true);
                return;
            }

            if (cellShot < 0 || cellShot > 9) {
                displayMessage("Error computing grid cell " + cellShot, true);
                return;
            }

            DataGridViewRow row;

            if (gridTargets.Rows.Count < rowShot + 1) { //no row, add it
                int index = gridTargets.Rows.Add();
                row = gridTargets.Rows[index];
                row.HeaderCell.Value = "T" + (rowShot + 1);
            } else {
                row = gridTargets.Rows[rowShot];
            }

            DataGridViewCell cell = row.Cells[cellShot];
            if (Properties.Settings.Default.showScoring) {
                if (currentSession.decimalScoring) {
                    cell.Value = shot.decimalScore.ToString(CultureInfo.InvariantCulture);
                } else {
                    cell.Value = shot.score;
                }
            } else {
                if (currentSession.decimalScoring) {
                    cell.Value = "XX.X";
                } else {
                    cell.Value = "XX";
                }
            }

            DataGridViewCell totalCell = row.Cells[10];
            if (Properties.Settings.Default.showScoring) {
                decimal total = 0;
                for (int i = 0; i < row.Cells.Count - 1; i++) {
                    if (row.Cells[i].Value != null) {
                        total += Decimal.Parse(row.Cells[i].Value.ToString(), CultureInfo.InvariantCulture);
                    }
                }
                totalCell.Value = total;
            } else {
                totalCell.Value = "XX";
            }

            gridTargets.ClearSelection();
            gridSeriesSelected = false;
        }

        private void fillBreakdownChart(List<Shot> shotList) {
            int[] breakdown = new int[12];
            foreach (Shot s in shotList) {
                if (s.score == 10) {
                    if (s.innerTen) {
                        breakdown[0]++;
                    } else {
                        breakdown[1]++;
                    }
                } else {
                    breakdown[11 - s.score]++;
                }
            }

            for (int i = 0; i < chartBreakdown.Series[0].Points.Count; i++) {
                DataPoint p = chartBreakdown.Series[0].Points[i];
                p.SetValueY(breakdown[i]);
            }

            chartBreakdown.ResetAutoValues();
            chartBreakdown.Update();
        }

        private void clearBreakdownChart() {
            for (int i = 0; i < chartBreakdown.Series[0].Points.Count; i++) {
                DataPoint p = chartBreakdown.Series[0].Points[i];
                p.SetValueY(0);
            }

            chartBreakdown.ResetAutoValues();
            chartBreakdown.Update();
        }

        private void imgSessionName_Click(object sender, EventArgs e) {
            gridTargets.ClearSelection();
            gridSeriesSelected = false;
            displayShotStatistics(getShotList());
            targetRefresh();
        }

        private void gridTargets_Click(object sender, EventArgs e) {
            if (gridTargets.SelectedRows.Count > 0) {
                gridSeriesSelected = true;
                int rowIndex = gridTargets.CurrentCell.RowIndex;
                currentSession.CurrentSeries = currentSession.AllSeries[rowIndex];
                displayShotStatistics(getShotList());
                targetRefresh();
            }
        }

        private List<Shot> getShotList() {
            if (Settings.Default.OnlySeries || gridSeriesSelected) {
                return currentSession.CurrentSeries;
            } else {
                return getShots();
            }
        }

        private List<Shot> getShots() {
            if (currentStatus == Status.LOADED) {
                return currentSession.LoadedShots;
            } else {
                return currentSession.Shots;
            }
        }

        private void btnJournal_Click(object sender, EventArgs e) {
            if (currentStatus == Status.CONNECTED) {
                if (!clearShots()) {
                    disconnect();
                    showJournalForm();
                }
            } else {
                showJournalForm();
            }
            btnConnect.Enabled = false;
        }

        private void disconnect() {
            try {
                commModule.close();
                this.reconnectTimer.Enabled = false;
                this.rapidFireTimer.Stop();
                commModule = null;
            } catch (IOException) {
                MessageBox.Show("Error closing the serial port. Please try again.", "Error disconnecting", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }
            btnConnect.Text = "Connect";
            displayMessage("Disconnected", false);
            log("*********\nDisconnected.");
            currentStatus = Status.NOT_CONNECTED;
            connectionStatus.BackColor = SystemColors.Control;

            statusText.Text = "Disconnected";
            timer.Enabled = false;
            btnConnect.ImageKey = "connect";

            shotsList.Enabled = false;
            btnCalibration.Enabled = false;
            btnArduino.Enabled = false;
            btnTargetSettings.Enabled = false;
            btnUpload.Enabled = true;
            btnResume.Enabled = false;
            trkZoom.Enabled = false;
            tcSessionType.Enabled = false;
            tcSessionType.Refresh();

            //initNewSession();
            targetRefresh();

            frmArduino ard = frmArduino.getInstance(this);
            ard.Hide();

            SetThreadExecutionState(ES_CONTINUOUS); //reenable screensaver

            lastEcho = null;
        }

        private void showJournalForm() {
            frmJournal form = (frmJournal)Application.OpenForms["frmJournal"];
            if (form != null) {
                form.BringToFront();
            } else {
                form = new frmJournal(this);
                form.Show();
            }
        }

        public void loadSessionFromJournal(Session session) {
            if (this.currentSession.id == session.id) {
                displayMessage("Session " + session.id + " already loaded", true);
                return;
            }
            this.currentSession = session;
            this.currentStatus = Status.LOADED;
            this.currentSession.AllSeries.Clear();
            this.currentSession.LoadedShots.Clear();

            trkZoom.Enabled = true;
            shotsList.Enabled = true;
            setTrkZoom(currentSession.getTarget());
            drawSessionName();

            shotsList.Items.Clear();
            shotsList.Refresh();
            imgListDirections.Images.Clear();
            gridTargets.Rows.Clear();
            clearBreakdownChart();


            frmJournal journalForm = (frmJournal)Application.OpenForms["frmJournal"];

            foreach (Shot s in currentSession.Shots) {
                currentSession.addLoadedShot(s);
                displayShotData(s);
                Application.DoEvents();
                targetRefresh();
                if (journalForm != null) {
                    if (!journalForm.isSessionLoading()) {
                        break;
                    }
                }
            }

            displayMessage("Session " + session.id + " loaded", false);
        }

        public void unloadJournalSession() {
            initDefaultSession();
            this.currentStatus = Status.NOT_CONNECTED;
            shotsList.Items.Clear();
            shotsList.Refresh();
            imgListDirections.Images.Clear();
            gridTargets.Rows.Clear();
            clearBreakdownChart();
            targetRefresh();
            trkZoom.Enabled = false;
            shotsList.Enabled = false;
            setTrkZoom(currentSession.getTarget());
            drawSessionName();
        }

        private void imgLogo_Click(object sender, EventArgs e) {
            MessageBox.Show("Copyright (c) 2020-2024 Azmodan -> youtube.com/ArmeVechi");
        }

        private void btnArduino_Click(object sender, EventArgs e) {
            frmArduino frmArd = frmArduino.getInstance(this);
            frmArd.Show();
        }

        private void mouseWheel(object sender, MouseEventArgs e) {
            ((HandledMouseEventArgs)e).Handled = true;//disable default mouse wheel
            if (e.Delta > 0) {
                if (trkZoom.Value < trkZoom.Maximum) {
                    trkZoom.Value++;
                }
            } else {
                if (trkZoom.Value > trkZoom.Minimum) {
                    trkZoom.Value--;
                }
            }
        }

        private void imgTarget_Click(object sender, EventArgs e) {
            trkZoom.Focus();
        }

        private void btnUpload_Click(object sender, EventArgs e) {
            frmUpload upload = new frmUpload(this);
            upload.ShowDialog();
        }

        private void loadEventsOnTabs() {

            this.tcSessionType.Controls.Clear();
            int index = 0;
            foreach (Event ev in eventManager.getActiveEventsList()) {

                TabPage tab = new System.Windows.Forms.TabPage();

                tab.BackColor = ev.TabColor;
                //tab.Location = new System.Drawing.Point(52, 4);
                tab.Name = ev.ID.ToString();
                //tab.Padding = new System.Windows.Forms.Padding(5);
                //tab.Size = new System.Drawing.Size(250, 522);
                tab.TabIndex = index++;
                tab.Text = ev.Name;

                this.tcSessionType.Controls.Add(tab);
            }
        }

        private void splitContainer_SplitterMoved(object sender, SplitterEventArgs e) {
            targetRefresh();
        }

        private void btnTargetSettings_Click(object sender, EventArgs e) {
            frmTargetSettings settingsFrom = new frmTargetSettings(this);
            if (settingsFrom.ShowDialog(this) == DialogResult.OK) {
            }
        }

        private void initSettingsDB() {

            List<string> exclusionList = new List<string> { "SettingsKey", "Item" };
            Type sett = Settings.Default.GetType();
            PropertyInfo[] members = sett.GetProperties();
            Console.WriteLine("--------------");
            foreach (PropertyInfo m in members) {
                if (m.CanWrite && !exclusionList.Contains(m.Name)) {
                    //Console.WriteLine("Name: " + m.Name + " - Type: " + m.PropertyType + " - Value: " + m.GetValue(Settings.Default));
                    object obj = storage.getSetting(m.Name);
                    if (obj == null) {
                        //setting not in the DB. create it
                        storage.storeSetting(m.Name, m.PropertyType, m.GetValue(Settings.Default));
                    }
                }

            }
            Console.WriteLine("--------------");

        }

        private void loadSettingsFromDB() {
            List<string> exclusionList = new List<string> { "SettingsKey", "Item" };
            Type sett = Settings.Default.GetType();
            PropertyInfo[] members = sett.GetProperties();

            log("Loading settings from DB:");
            foreach (PropertyInfo m in members) {
                if (m.CanWrite && !exclusionList.Contains(m.Name)) {
                    log("Name: " + m.Name + " - Type: " + m.PropertyType + " - Value: " + m.GetValue(Settings.Default));
                    object obj = storage.getSetting(m.Name);
                    m.SetValue(Settings.Default, obj);
                }
            }
        }

        public void saveSettings() {
            List<string> exclusionList = new List<string> { "SettingsKey", "Item" };
            Type sett = Settings.Default.GetType();
            PropertyInfo[] members = sett.GetProperties();
            foreach (PropertyInfo m in members) {
                if (m.CanWrite && !exclusionList.Contains(m.Name)) {
                    object obj = storage.getSetting(m.Name);

                    //update the setting value
                    storage.updateSetting(m.Name, m.GetValue(Settings.Default));

                }

            }
        }

        private void toolTipTimer_Tick(object sender, EventArgs e) {
            DateTime now = DateTime.Now;
            double diff = (now - tooltipDisplayTime).TotalSeconds;

            if (diff > 3) {
                toolTip.RemoveAll();
            }
        }

        private void Disconnected(object sender, comms.CommEventArgs e) {

            connectionStatus.BackColor = Color.Red;
            this.reconnectTimer.Enabled = true;
        }

        private void reconnectTimer_Tick(object sender, EventArgs e) {
            comms.OpenParams para = new comms.TcpOpenParams();
            ((comms.TcpOpenParams)para).IP = Settings.Default.TcpIP;
            ((comms.TcpOpenParams)para).port = Settings.Default.TcpPort;

            try {
                Console.WriteLine("Attempting reconnect...");
                log("Attempting reconnect...");
                commModule.open(para);
                Console.WriteLine("Reconnect succesfull.");
                log("Reconnect succesfull.");
                this.reconnectTimer.Enabled = false;
                connectionStatus.BackColor = Color.Green;
                String ssid = getWifiSSID();
                if (!ssid.StartsWith(FET_SSID_PREFIX)) {
                    MessageBox.Show("Current Wi-Fi network(" + ssid + ") is not a freETarget SSID. It should start with 'FET-'", "Wrong WiFi", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            } catch (Exception) {
                Console.WriteLine("Reconnect failed.");
                log("Reconnect failed.");
            }
        }

        private String getWifiSSID() {
            string s1 = "N/A";
            try {
                System.Diagnostics.Process p = new System.Diagnostics.Process();
                p.StartInfo.FileName = "netsh.exe";
                p.StartInfo.Arguments = "wlan show interfaces";
                p.StartInfo.UseShellExecute = false;
                p.StartInfo.RedirectStandardOutput = true;
                p.Start();

                string s = p.StandardOutput.ReadToEnd();
                s1 = s.Substring(s.IndexOf("SSID"));
                s1 = s1.Substring(s1.IndexOf(":"));
                s1 = s1.Substring(2, s1.IndexOf("\n")).Trim();

                string s2 = s.Substring(s.IndexOf("Signal"));
                s2 = s2.Substring(s2.IndexOf(":"));
                s2 = s2.Substring(2, s2.IndexOf("\n")).Trim();

                Console.WriteLine("WIFI connected to " + s1 + "  " + s2);
                log("WIFI connected to " + s1 + "  " + s2);
                p.WaitForExit();
            } catch (Exception) {
                //do nothing
            }
            return s1;
        }

        private void btnStart_Click(object sender, EventArgs e) {
            //disble button
            btnStart.Enabled = false;
            this.currentSession.resetVRO();

            //load command
            rapidFireTimer.Interval = this.currentSession.eventType.RF_LoadTime * 1000;
            this.RFduelCounter = -1;
            rapidFireTimer.Start();
            Console.WriteLine("LOAD");
            currentSession.RFseriesActive = true;
        }

        private void rapidFireTimer_Tick(object sender, EventArgs e) {

            if (RFduelCounter == -1) {
                //first tick - load time over
                this.RFduelCounter = 0;

                //send first commands to target
                if (this.currentSession.eventType.RF_TimePerShot > 0) {
                    //duel
                    Console.WriteLine("LOAD");
                    StringBuilder sb = new StringBuilder("{");
                    sb.Append("\"RAPID_COUNT\": 1 , ");
                    sb.Append("\"RAPID_WAIT\":" + this.currentSession.eventType.RF_TimeBetweenShots + ", ");
                    sb.Append("\"RAPID_TIME\":" + this.currentSession.eventType.RF_TimePerShot + ", ");
                    sb.Append("\"RAPID_ENABLE\": 1");
                    sb.Append(" }");
                    Console.WriteLine(sb.ToString());
                    commModule.sendData(sb.ToString());
                    log("Sending: " + sb.ToString());
                    this.RFduelCounter++;
                    rapidFireTimer.Interval = (this.currentSession.eventType.RF_TimeBetweenShots + this.currentSession.eventType.RF_TimePerShot) * 1000;
                } else if (this.currentSession.eventType.RF_TimePerSerie > 0) {
                    //normal rapid fire
                    Console.WriteLine("ATTENTION");
                    StringBuilder sb = new StringBuilder("{");
                    sb.Append("\"RAPID_COUNT\":" + this.currentSession.eventType.RF_NumberOfShots + ", ");
                    sb.Append("\"RAPID_WAIT\":" + this.currentSession.eventType.RF_TimeBetweenShots + ", ");
                    sb.Append("\"RAPID_TIME\":" + this.currentSession.eventType.RF_TimePerSerie + ", ");
                    sb.Append("\"RAPID_ENABLE\": 1");
                    sb.Append(" }");
                    Console.WriteLine(sb.ToString());
                    commModule.sendData(sb.ToString());
                    log("Sending: " + sb.ToString());
                    rapidFireTimer.Interval = (this.currentSession.eventType.RF_TimeBetweenShots + this.currentSession.eventType.RF_TimePerSerie) * 1000;
                } else {
                    Console.WriteLine("rapid fire misconfiguration");
                    log("rapid fire misconfiguration");
                    btnStart.Enabled = true;
                    rapidFireTimer.Stop();
                    currentSession.RFseriesActive = false;
                }
            } else if (RFduelCounter >= 0 && RFduelCounter < 1000) {
                //second tick onward
                if (this.currentSession.eventType.RF_TimePerShot > 0) {
                    //duel
                    if (RFduelCounter < this.currentSession.eventType.RF_NumberOfShots) {
                        StringBuilder sb = new StringBuilder("{");
                        sb.Append("\"RAPID_COUNT\": 1 , ");
                        sb.Append("\"RAPID_WAIT\":" + this.currentSession.eventType.RF_TimeBetweenShots + ", ");
                        sb.Append("\"RAPID_TIME\":" + this.currentSession.eventType.RF_TimePerShot + ", ");
                        sb.Append("\"RAPID_ENABLE\": 1");
                        sb.Append(" }");
                        Console.WriteLine(sb.ToString());
                        commModule.sendData(sb.ToString());
                        log("Sending: " + sb.ToString());
                        this.RFduelCounter++;
                    } else {
                        rapidFireTimer.Interval = RFcooldown * 1000;
                        this.RFduelCounter = 1000;
                        Console.WriteLine("END");
                    }

                } else if (this.currentSession.eventType.RF_TimePerSerie > 0) {
                    //normal rapid fire
                    rapidFireTimer.Interval = RFcooldown * 1000;
                    this.RFduelCounter = 1000;
                    Console.WriteLine("END");
                } else {
                    Console.WriteLine("rapid fire misconfiguration");
                    log("rapid fire misconfiguration");
                    btnStart.Enabled = true;
                    rapidFireTimer.Stop();
                    currentSession.RFseriesActive = false;
                }
            } else {
                //last tick
                btnStart.Enabled = true;
                rapidFireTimer.Stop();
                StringBuilder sb = new StringBuilder("{");
                sb.Append("\"RAPID_ENABLE\": 0 }");
                Console.WriteLine(sb.ToString());
                commModule.sendData(sb.ToString());
                log("Sending: " + sb.ToString());
                Console.WriteLine("UNLOAD");
                currentSession.RFseriesActive = false;
            }


        }

        private void btnResume_Click(object sender, EventArgs e) {
            frmResumeSession resumeForm = new frmResumeSession(this);
            if (resumeForm.ShowDialog(this) == DialogResult.OK) {

                shotsList.Items.Clear();
                shotsList.Refresh();
                imgListDirections.Images.Clear();
                gridTargets.Rows.Clear();
                clearBreakdownChart();

                ListBoxSessionItem sessionToBeResumed = resumeForm.selectedSession;
                currentSession = storage.findSession(sessionToBeResumed.id);
                currentSession.repopulateSeries();


                foreach (Shot s in currentSession.Shots) {
                    displayShotData(s);
                    Application.DoEvents();
                }

                targetRefresh();

                if (currentSession.minutes > 0) {
                    currentSession.endTime = currentSession.startTime.AddMinutes(currentSession.minutes);
                } else {
                    currentSession.endTime = DateTime.MinValue;
                }

                displayMessage("Session " + currentSession.id + " resumed", false);
                log("Session " + currentSession.id + " resumed");
            }
            resumeForm.Dispose();
        }

        public Session getCurrentSession() {
            return this.currentSession;
        }

        private string recovery() {
            DialogResult result = MessageBox.Show("freETarget has detected that the program did not shutdown corectly."
                    + Environment.NewLine + Environment.NewLine
                    + "Do you want to try to recover the shots of the last session from the log?"
                    + Environment.NewLine + Environment.NewLine
                    + "Recovery will happen after the PC client connects to the target."
                   , "Abnormal shutdown detected", MessageBoxButtons.YesNo, MessageBoxIcon.Question);

            if (result == DialogResult.Yes) {
                //parsing log from the end to search for unsaved shots
                FileStream stream = new FileStream(this.logFile, FileMode.Open, FileAccess.Read);
                var reader = new ReverseTextReader(stream, Encoding.Default);
                List<String> tempLines = new List<string>();
                while (!reader.EndOfStream) {
                    String line = reader.ReadLine().Trim();
                    //Console.WriteLine(line);
                    tempLines.Add(line);
                    if (line.Contains("### New Session")) {
                        //we reached the start of the last session
                        break;
                    }
                }
                reader = null;
                stream.Close();

                //select only the actual shots
                List<String> sessionLines = new List<string>();
                String sessionTitle = "";
                for (int i = tempLines.Count - 1; i >= 0; i--) {
                    if (tempLines[i].Contains("------------------------------------------------------------------------")) { 
                        //stop at the last start
                        break; 
                    }

                    //isolate the session line
                    if (tempLines[i].Contains("### New Session")) {
                        sessionTitle = tempLines[i];
                    }

                    sessionLines.Add(tempLines[i]);
                }

                int firstQuote = sessionTitle.IndexOf("'");
                int secondQuote = sessionTitle.LastIndexOf("'");
                if (firstQuote > 0) {
                    recoverySessionTime = sessionTitle.Substring(0, sessionTitle.IndexOf("|")-1);
                    sessionTitle = sessionTitle.Substring(firstQuote + 1, secondQuote - firstQuote - 1);
                }

                //isolate just the shot jsons
                for (int i =0; i < sessionLines.Count; i++) {
                    if (sessionLines[i].IndexOf("|") == -1){
                        continue;
                    }
                    sessionLines[i] = sessionLines[i].Substring(sessionLines[i].IndexOf("|") + 2);
                }

                log("Recovering session '" + sessionTitle + "' that started at <" + recoverySessionTime + "> ... after connect");

                recoveryLines = sessionLines;
                return sessionTitle;
            }
            return "";
        }

        private bool checkLogForCleanDisconnect() {
            IEnumerable<String> lines = File.ReadLines(this.logFile);
            int count = lines.Count();
            if (count > 0) {
                String lastLine = lines.Last();
                if (lastLine.Contains("@@@@@ normal shutdown @@@@@")) {
                    return true;
                } else {
                    return false;
                }
            } else {
                return true;
            }
        }
    }

    /**
     * Custom StringBuilder with an event triggered when text is appended, for use in binding it to a textbox
     */
    public class StringBuilderWrapper {
        private StringBuilder _builder = new StringBuilder();
        public EventHandler TextChanged;
        public void add(string text) {
            _builder.Append(text + Environment.NewLine);
            if (TextChanged != null)
                TextChanged(this, null);
        }
        public string Text {
            get { return _builder.ToString(); }
        }
    }

}
