using freETarget.Properties;
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

        private string incomingJSON = "";               // Cumulative serial message


        public decimal calibrationX = 0;
        public decimal calibrationY = 0;
        public decimal calibrationAngle = 0;

        //accumulator of ALL incoming text from the arduino. it will all be displayed in the arduino window
        public StringBuilderWrapper output = new StringBuilderWrapper();

        private Session currentSession;

        public StorageController storage;
        public EventManager eventManager;


        private String logFile = "";

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


        public frmMainWindow() {
            InitializeComponent();
            initLog();

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

            toolTip.SetToolTip(btnCalibration, "Calibration - X: " + calibrationX + " Y: " + calibrationY + " Angle: " + calibrationAngle);

            toolTip.SetToolTip(btnConfig, "Setting - Target distance percent: " + Properties.Settings.Default.targetDistance);

            initBreakdownChart();

            digitalClock.segments[1].ColonShow = true;
            digitalClock.segments[3].ColonShow = true;
            digitalClock.segments[1].ColonOn = true;
            digitalClock.segments[3].ColonOn = true;
            digitalClock.ResizeSegments();

            loadEventsOnTabs();

        }

        private void initLog() {
            //init log location
            string logDirectory = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments) + @"\freETarget\log\";
            if (!Directory.Exists(logDirectory)) {
                try {
                    Directory.CreateDirectory(logDirectory);
                }catch(Exception ex) {
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
                } catch(Exception ex) {
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
        }

        public void log(string s) {
            log(s, this.logFile);
        }

        public void log(string s, string logFile) {
            if (s == null || s == "" || logFile == null) {
                return;
            }

            if (Properties.Settings.Default.fileLogging) { //log enabled from settings

                try {
                    //Opens a new file stream which allows asynchronous reading and writing
                    using (StreamWriter sw = new StreamWriter(new FileStream(logFile, FileMode.Append, FileAccess.Write, FileShare.ReadWrite))) {

                        sw.WriteLine(DateTime.Now.ToString("yyyy/MM/dd HH:mm:ss:fff") + " | " + s.Trim());

                    }
                } catch (Exception ex) {
                    //oh well...
                    Console.WriteLine("Error logging ("+logFile+"): " + ex.Message);
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
                MessageBox.Show("Default event is not available: " + Settings.Default.defaultTarget.Trim(),"Configuration error",MessageBoxButtons.OK,MessageBoxIcon.Error);
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
            string indata = e.Text.Trim();

            //first incoming text from target after port open is "freETarget VX.x" - use this to confirm connection
            if (indata.Contains("freETarget") && currentStatus == Status.CONECTING) {
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
                    incomingJSON = incomingJSON.Substring(indexClosedBracket+1);

                    //Console.WriteLine("Complete json message: " + message);

                    //parse json message. might be a shot data or a test message
                    Shot shot = parseJson(message);

                    if (shot!=null && shot.count >= 0) {
                        if (shot.miss == true) {
                            if (Settings.Default.ignoreMiss == true) {
                                //do nothing. ignore shot
                            } else {
                                currentSession.addShot(shot);

                                displayMessage(message, false);
                                displayShotData(shot);
                                VirtualRO vro = new VirtualRO(currentSession.eventType);
                                vro.speakShot(shot);
                            }
                        } else {
                            currentSession.addShot(shot);

                            displayMessage(message, false);
                            displayShotData(shot);
                            VirtualRO vro = new VirtualRO(currentSession.eventType);
                            vro.speakShot(shot);

                            var d = new SafeCallDelegate3(targetRefresh); //draw shot
                            this.Invoke(d);
                        }

                        if (incomingJSON.IndexOf("}") != -1) {

                            comms.CommEventArgs e2 = new comms.CommEventArgs("");
                            DataReceived(sender, e2); //call the event again to parse the remains. maybe there is another full message in there
                        }
                        
                    } else {
                        lastEcho = Echo.parseJson(message);

                        
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

                    if (Properties.Settings.Default.TcpPort <= 0 ) {
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

                //use parameters for the selected comm
                comms.OpenParams para;
                if (Settings.Default.CommProtocol == "USB") {
                    para = new comms.UsbOpenParams();
                    ((comms.UsbOpenParams)para).portName = Properties.Settings.Default.portName;
                    ((comms.UsbOpenParams)para).baudRate = Properties.Settings.Default.baudRate;
                    ((comms.UsbOpenParams)para).dataBits = 8;
                    ((comms.UsbOpenParams)para).dtrEnable = true;
                } else {
                    para = new comms.TcpOpenParams();
                    ((comms.TcpOpenParams)para).IP = Settings.Default.TcpIP;
                    ((comms.TcpOpenParams)para).port = Settings.Default.TcpPort;
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
            Thread.Sleep(100);


            btnConnect.Text = "Disconnect";
            currentStatus = Status.CONNECTED;
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
            trkZoom.Enabled = true;
            tcSessionType.Enabled = true;
            tcSessionType.Refresh();

            SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED); //disable screensaver while connected

            initNewSession();
            targetRefresh();
            
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
                txtTotal.Text = getShots().Count + ": " + currentSession.score.ToString() + " (" + currentSession.decimalScore.ToString(CultureInfo.InvariantCulture) + ") -" + currentSession.innerX.ToString() + "x";

                //write to last shot textbox
                string lastShot = shot.decimalScore.ToString(CultureInfo.InvariantCulture);
                txtLastShot.Text = lastShot + inner;

                drawArrow(shot);

                //write score to listview
                ListViewItem item = new ListViewItem(new string[] { "" }, (shot.index+1).ToString());
                item.UseItemStyleForSubItems = false;
                ListViewItem.ListViewSubItem countItem = item.SubItems.Add((shot.index+1).ToString());
                countItem.Font = new Font("MS Sans Serif", 7f, FontStyle.Italic|FontStyle.Bold);
                ListViewItem.ListViewSubItem scoreItem = item.SubItems.Add(shot.score.ToString());
                scoreItem.Font = new Font("MS Sans Serif", 9.75f,  FontStyle.Bold);
                ListViewItem.ListViewSubItem decimalItem = item.SubItems.Add(shot.decimalScore.ToString(CultureInfo.InvariantCulture) + inner);
                decimalItem.Font = new Font("MS Sans Serif", 9.75f, FontStyle.Bold);

                shotsList.Items.Add(item);
                shotsList.EnsureVisible(shotsList.Items.Count - 1);

                computeShotStatistics(getShotList());
                writeToGrid(shot);
                fillBreakdownChart(getShots());

            }
        }

        private void computeShotStatistics(List<Shot> shotList) {
            if (shotList.Count > 1) {
                decimal localRbar;
                decimal localXbar;
                decimal localYbar;
                calculateMeanRadius(out localRbar, out localXbar, out localYbar, shotList);
                currentSession.rbar = localRbar;
                currentSession.xbar = localXbar;
                currentSession.ybar = localYbar;

                txtMeanRadius.Text = Math.Round(localRbar, 1).ToString();
                txtWindage.Text = Math.Round(Math.Abs(localXbar), 1).ToString();
                txtElevation.Text = Math.Round(Math.Abs(localYbar), 1).ToString();
                drawWindageAndElevationArrows(localXbar, localYbar);

                currentSession.groupSize = Math.Round(calculateMaxSpread(shotList), 1);
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
                g.DrawString("M", f, br, imgArrow.Width /2 , imgArrow.Height /2 , format);
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
                imgListDirections.Images.Add((shot.index+1).ToString(), imgArrow.Image);
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
                Shot ret = new Shot(calibrationX,calibrationY, calibrationAngle);
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
                            if(mix == 1) {
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
                Properties.Settings.Default.baudRate = int.Parse(settingsFrom.txtBaud.Text);
                Properties.Settings.Default.displayDebugConsole = settingsFrom.chkDisplayConsole.Checked;
                Properties.Settings.Default.portName = settingsFrom.cmbPorts.GetItemText(settingsFrom.cmbPorts.SelectedItem);

                Event ev = (Event)settingsFrom.cmbWeapons.SelectedItem;
                Properties.Settings.Default.defaultTarget = ev.Name;
                Properties.Settings.Default.targetColor = Color.FromName(settingsFrom.cmbColor.GetItemText(settingsFrom.cmbColor.SelectedItem));
                Properties.Settings.Default.drawMeanGroup = settingsFrom.chkDrawMeanG.Checked;
                Properties.Settings.Default.OnlySeries = settingsFrom.chkSeries.Checked;
                Properties.Settings.Default.voiceCommands = settingsFrom.chkVoice.Checked;
                Properties.Settings.Default.pdfPath = settingsFrom.txtPDFlocation.Text;
                Properties.Settings.Default.targetDistance = int.Parse(settingsFrom.txtDistance.Text);
                Properties.Settings.Default.scoreVoice = settingsFrom.chkScoreVoice.Checked;
                Properties.Settings.Default.fileLogging = settingsFrom.chkLog.Checked;
                Properties.Settings.Default.ignoreMiss = settingsFrom.chkMiss.Checked;

                if (Properties.Settings.Default.targetDistance != 100) {
                    btnConfig.BackColor = Properties.Settings.Default.targetColor;
                } else {
                    btnConfig.BackColor = SystemColors.Control;
                }
                toolTip.SetToolTip(btnConfig, "Settings - Target distance percent: " + Properties.Settings.Default.targetDistance);

                if (currentSession != null) {
                    computeShotStatistics(getShotList());
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
                Properties.Settings.Default.TcpPort = int.Parse(settingsFrom.txtPort.Text);

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
        }

        private void frmMainWindow_Shown(object sender, EventArgs e) {
            displayDebugConsole(Properties.Settings.Default.displayDebugConsole);



            trkZoom.Focus();
        }

        private void initNewSession() {
            if (currentSession!=null && currentSession.Shots.Count > 0 && currentStatus != Status.LOADED) {
                storage.storeSession(currentSession, true);
                displayMessage("Session saved", true);
                
            }

            Event ev = eventManager.findEventByName(tcSessionType.SelectedTab.Text.Trim());
            currentSession = Session.createNewSession(ev, Settings.Default.name);
            currentSession.start();
            this.log("### New Session '" + currentSession.ToString() + "' started ###");

            setTrkZoom(currentSession.getTarget());

            clearShots();
            targetRefresh();
            imgTarget.BackColor = Settings.Default.targetColor;
            drawSessionName();
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
            } else {
                imgTarget.Height = width;
                imgTarget.Width = width;

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
                DialogResult result = MessageBox.Show("Current session is unsaved. Do you want to save it?", "Save session", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);

                if (result == DialogResult.Yes) {
                    storage.storeSession(currentSession, true);
                    displayMessage("Session saved", true);

                } else if(result == DialogResult.Cancel) {
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
            DateTime now = DateTime.Now;
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

        //computes the average radius of all the shots
        private void calculateMeanRadius( out decimal rbar, out decimal xbar, out decimal ybar, List<Shot> shots) {

            decimal xsum = 0;
            decimal ysum = 0;
            int shotCount = 0;

            foreach(Shot shot in shots) {
                if (shot.miss == true) {
                    continue;
                }
                xsum += shot.getX();
                ysum += shot.getY();
                shotCount++;
            }


            xbar = xsum / shotCount;
            ybar = ysum / shotCount;

            decimal[] r = new decimal[shotCount];
            int k = 0;
            for(int i=0; i< shots.Count; i++) {
                if (shots[i].miss == true) {
                    continue;
                }
                r[k] = (decimal)Math.Sqrt((double)(((shots[i].getX() - xbar) * (shots[i].getX() - xbar)) + ((shots[i].getY() - ybar) * (shots[i].getY() - ybar))));
                k++;
            }

            decimal rsum = 0;
            foreach(decimal ri in r) {
                rsum += ri;
            }

            rbar = rsum / shotCount;

        }

        //computes group size (maxium distance between 2 shots), but measuring from the center of the shot, not outside
        private decimal calculateMaxSpread(List<Shot> shots) { 
            List<double> spreads = new List<double>();
            for(int i = 0; i < shots.Count; i++) {
                for(int j = 0; j < shots.Count; j++) {
                    double powX = Math.Pow((double)shots[i].getX() - (double)shots[j].getX(), 2);
                    double powY = Math.Pow((double)shots[i].getY() - (double)shots[j].getY(), 2);
                    double sqrt;
                    if (powX > powY) {
                        sqrt = Math.Sqrt(powX - powY);
                    } else {
                        sqrt = Math.Sqrt(powY - powX);
                    }
                    spreads.Add(sqrt);
                }
            }

            return (decimal)spreads.Max();
        }


        private void btnCalibration_Click(object sender, EventArgs e) {
            frmCalibration frmCal = frmCalibration.getInstance(this);
            frmCal.Show();
        }

        public void calibrateX(decimal increment) {
            calibrationX += increment;
            foreach (Shot shot in getShotList()) {
                shot.calibrationX = calibrationX;
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

            if(calibrationX == 0 && calibrationY == 0 && calibrationAngle ==0) {
                btnCalibration.BackColor = this.BackColor;
            } else {
                btnCalibration.BackColor = Settings.Default.targetColor;
            }

            toolTip.SetToolTip(btnCalibration, "Calibration - X: " + calibrationX + " Y: " + calibrationY + " Angle: " + calibrationAngle);
        }

        public void calibrateY(decimal increment) {
            calibrationY += increment;
            foreach (Shot shot in getShotList()) {
                shot.calibrationY = calibrationY;
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
            toolTip.SetToolTip(btnCalibration, "Calibration - X: " + calibrationX + " Y: " + calibrationY + " Angle: " + calibrationAngle);
        }

        public void calibrateAngle(decimal angle) {
            calibrationAngle += angle;
            foreach (Shot shot in getShotList()) {
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
            toolTip.SetToolTip(btnCalibration, "Calibration - X: " + calibrationX + " Y: " + calibrationY + " Angle: " + calibrationAngle);
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
            gridTargets.Rows.Clear();
            clearBreakdownChart();
            foreach (Shot shot in getShots()) { //refresh all shots, not just currest series
                displayShotData(shot);
            }

            targetRefresh();

            btnCalibration.BackColor = this.BackColor;
            toolTip.SetToolTip(btnCalibration, "Calibration - X: " + calibrationX + " Y: " + calibrationY + " Angle: " + calibrationAngle);
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
            if (tcSessionType.Enabled==false) {
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
                f = new Font(e.Font,FontStyle.Bold| FontStyle.Italic);
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
                if (shot.index < ev.Final_NumberOfShotPerSeries) {
                    //first row
                    rowShot = 0;
                    cellShot = shot.index % ev.Final_NumberOfShotPerSeries;
                } else if (shot.index >= ev.Final_NumberOfShotPerSeries && shot.index < ev.Final_NumberOfShotsBeforeSingleShotSeries) {
                    //second row
                    rowShot = 1;
                    cellShot = shot.index % ev.Final_NumberOfShotPerSeries;
                } else {
                    //row of 2
                    rowShot = (shot.index - (ev.Final_NumberOfShotPerSeries + 1)) / ev.Final_NumberOfShotsInSingleShotSeries;
                    cellShot = shot.index % ev.Final_NumberOfShotsInSingleShotSeries;
                }
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
            if (currentSession.decimalScoring) {
                cell.Value = shot.decimalScore.ToString(CultureInfo.InvariantCulture);
            } else {
                cell.Value = shot.score;
            }

            DataGridViewCell totalCell = row.Cells[10];
            decimal total = 0;
            for (int i = 0; i < row.Cells.Count - 1; i++) {
                if (row.Cells[i].Value != null) {
                    total += Decimal.Parse(row.Cells[i].Value.ToString(), CultureInfo.InvariantCulture);
                }
            }
            totalCell.Value = total;

            gridTargets.ClearSelection();
            gridSeriesSelected = false;
        }

        private void fillBreakdownChart(List<Shot> shotList) {
            int[] breakdown = new int[12];
            foreach(Shot s in shotList) {
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

            for(int i = 0; i < chartBreakdown.Series[0].Points.Count; i++) {
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
            computeShotStatistics(getShotList());
            targetRefresh();
        }

        private void gridTargets_Click(object sender, EventArgs e) {
            if (gridTargets.SelectedRows.Count > 0) {
                gridSeriesSelected = true;
                int rowIndex = gridTargets.CurrentCell.RowIndex;
                currentSession.CurrentSeries = currentSession.AllSeries[rowIndex];
                computeShotStatistics(getShotList());
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
            if(currentStatus == Status.LOADED) {
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
            }catch(IOException) {
                MessageBox.Show("Error closing the serial port. Please try again.","Error disconnecting",MessageBoxButtons.OK,MessageBoxIcon.Error);
                return;
            }
            btnConnect.Text = "Connect";
            displayMessage("Disconnected",false);
            log("*********\nDisconnected.");
            currentStatus = Status.NOT_CONNECTED;


            statusText.Text = "Disconnected";
            timer.Enabled = false;
            btnConnect.ImageKey = "connect";

            shotsList.Enabled = false;
            btnCalibration.Enabled = false;
            btnArduino.Enabled = false;
            btnTargetSettings.Enabled = false;
            btnUpload.Enabled = true;
            trkZoom.Enabled = false;
            tcSessionType.Enabled = false;
            tcSessionType.Refresh();

            initNewSession();
            targetRefresh();

            frmArduino ard = frmArduino.getInstance(this);
            ard.Hide();

            SetThreadExecutionState(ES_CONTINUOUS); //reenable screensaver
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
            if(this.currentSession.id == session.id) {
                displayMessage("Session "+ session.id + " already loaded", true);
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
            MessageBox.Show("Copyright (c) 2020-2021 Azmodan -> youtube.com/ArmeVechi");
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
                    Console.WriteLine("Name: " + m.Name + " - Type: " + m.PropertyType + " - Value: " + m.GetValue(Settings.Default));
                    object obj = storage.getSetting(m.Name);
                    if(obj == null) {
                        //setting not in the DB. create it
                        storage.storeSetting(m.Name, m.PropertyType, m.GetValue(Settings.Default));
                    }
                }

            }
            Console.WriteLine("--------------");

        }

        private void loadSettingsFromDB() {
            List<string> exclusionList = new List<string> { "SettingsKey" ,"Item"};
            Type sett = Settings.Default.GetType();
            PropertyInfo[] members = sett.GetProperties();

            foreach(PropertyInfo m in members) {
                if (m.CanWrite && !exclusionList.Contains(m.Name)) {
                    //Console.WriteLine("Name: " + m.Name + " - Type: " + m.PropertyType + " - Value: " + m.GetValue(Settings.Default));
                    object obj = storage.getSetting(m.Name);
                    m.SetValue(Settings.Default, obj);
                }              
            }
        }

        private void saveSettings() {
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
    }

    /**
     * Custom StringBuilder with an event triggered when text is appended, for use in binding it to a textbox
     */
    public class StringBuilderWrapper {
        private StringBuilder _builder = new StringBuilder();
        public EventHandler TextChanged;
        public void add(string text) {
            _builder.Append(text);
            if (TextChanged != null)
                TextChanged(this, null);
        }
        public string Text {
            get { return _builder.ToString(); }
        }
    }

}
