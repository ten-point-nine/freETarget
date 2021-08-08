using PdfSharp.Drawing.BarCodes;
using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Management;
using System.Globalization;

namespace freETarget
{
    public partial class frmSettings : Form {

        frmMainWindow mainWindow;

        private List<Control> eventControls = new List<Control>();

        private List<string> targetNames = new List<string>() { "TARGET",                                                     //  0
                        "1",      "2",        "3",     "4",      "5",       "6",       "7",     "8",     "9",      "10",    //  1
                        "DOC",    "DOPEY",  "HAPPY",   "GRUMPY", "BASHFUL", "SNEEZEY", "SLEEPY",                            // 11
                        "RUDOLF", "DONNER", "BLITXEM", "DASHER", "PRANCER", "VIXEN",   "COMET", "CUPID", "DUNDER",          // 18  
                        "ODIN",   "WODEN",   "THOR",   "BALDAR"                                                            // 26
                        };

        public frmSettings(frmMainWindow mainWin)
        {
            InitializeComponent();
            this.mainWindow = mainWin;

            eventControls.Add(txtEventName);
            eventControls.Add(cmbEventTypes);
            eventControls.Add(chkDecimalScoring);
            eventControls.Add(txtNoOfShots);
            eventControls.Add(cmbTargets);
            eventControls.Add(txtCaliber);
            eventControls.Add(txtMinutes);
            eventControls.Add(cmbTabColor);
            eventControls.Add(txtShotPerSeries);
            eventControls.Add(txtSeriesDuration);
            eventControls.Add(txtShotsInSeries);
            eventControls.Add(txtSingleShotDuration);
            eventControls.Add(txtShotsSingles);
            eventControls.Add(btnSaveEvent);
            eventControls.Add(btnCancelEventSave);
        }

        private void frmSettings_Load(object sender, EventArgs e)
        {


            try {
                using (var devices = new ManagementObjectSearcher("SELECT * FROM WIN32_SerialPort")) {
                    string[] portnames = SerialPort.GetPortNames();

                    //detect names of devices connected to COM ports
                    var ports = devices.Get().Cast<ManagementBaseObject>().ToList();
                    var device_list = (from n in portnames
                                       join p in ports on n equals p["DeviceID"].ToString()
                                       select p["Caption"]).ToList();

                    //detect arduino on COM
                    bool arduinoFound = false;
                    for (int i = 0; i < portnames.Length; i++) {
                        cmbPorts.Items.Add(portnames[i]);

                        if (i < device_list.Count) {
                            string portDevice = device_list[i].ToString();

                            if (portDevice.Contains("Arduino")) {
                                cmbPorts.SelectedItem = portnames[i];
                                mainWindow.log("Arduino device: '" + portDevice + "' found on port: " + portnames[i]);
                                arduinoFound = true;
                            }
                        }
                    }

                    //no arduino found, select first COM
                    if (arduinoFound == false && portnames.Length > 0) {
                        cmbPorts.SelectedItem = portnames[0];
                    }
                }
            }catch(Exception ex) {
                Console.WriteLine(ex.Message);
            }

            ArrayList ColorList = new ArrayList();
            Type colorType = typeof(System.Drawing.Color);
            PropertyInfo[] propInfoList = colorType.GetProperties(BindingFlags.Static | BindingFlags.DeclaredOnly | BindingFlags.Public);
            foreach (PropertyInfo c in propInfoList)
            {
                this.cmbColor.Items.Add(c.Name);

                this.cmb10Pen.Items.Add(c.Name);
                this.cmb10Back.Items.Add(c.Name);
                this.cmb9Pen.Items.Add(c.Name);
                this.cmb9Back.Items.Add(c.Name);
                this.cmbDefPen.Items.Add(c.Name);
                this.cmbDefBack.Items.Add(c.Name);
                this.cmbOldPen.Items.Add(c.Name);
                this.cmbOldBack.Items.Add(c.Name);

                this.cmbTabColor.Items.Add(c.Name);
            }

            //load eventTypes enum
            foreach(Event.EventType et in Enum.GetValues(typeof(Event.EventType))) {
                cmbEventTypes.Items.Add(et);
            }

            //load targets 
            //List<targets.aTarget> objects = new List<targets.aTarget>();
            foreach (Type type in Assembly.GetAssembly(typeof(targets.aTarget)).GetTypes()
                .Where(myType => myType.IsClass && !myType.IsAbstract && myType.IsSubclassOf(typeof(targets.aTarget)))) {
                //objects.Add((targets.aTarget)Activator.CreateInstance(type));
                Console.WriteLine(type);
                cmbTargets.Items.Add(type.ToString());
            }

            loadEvents();

            loadTargetNames();

            loadSettings();



            System.Reflection.Assembly assembly = System.Reflection.Assembly.GetExecutingAssembly();
            lblVersion.Text = "freETarget Project  -  v"+ assembly.GetName().Version.Major + "." + assembly.GetName().Version.Minor + "." + assembly.GetName().Version.Build + "   (c) 2020-2021";
        }

        private void loadEvents() {
            lstbAllEvents.Items.Clear();
            lstbActiveEvents.Items.Clear();
            lstbEvents.Items.Clear();

            List<Event> evList = mainWindow.eventManager.getEventsList();
            List<Event> activeEvList = mainWindow.eventManager.getActiveEventsList();
            foreach(Event ev in evList) {
                
                lstbAllEvents.Items.Add(ev);
                lstbEvents.Items.Add(ev);
            }

            foreach(Event ev in activeEvList) {
                cmbWeapons.Items.Add(ev);

                lstbAllEvents.Items.Remove(ev);
                lstbActiveEvents.Items.Add(ev);
            }
        }

        private void loadSettings()
        {
            txtName.Text = Properties.Settings.Default.name;
            txtBaud.Text = Properties.Settings.Default.baudRate.ToString();
            chkDisplayConsole.Checked = Properties.Settings.Default.displayDebugConsole;
            cmbPorts.SelectedItem = Properties.Settings.Default.portName;
            foreach(Event ev in mainWindow.eventManager.getActiveEventsList()) {
                if(ev.Name == Properties.Settings.Default.defaultTarget) {
                    cmbWeapons.SelectedItem = ev;
                    break;
                }
            }
            
            cmbColor.SelectedItem = Properties.Settings.Default.targetColor.Name;
            chkDrawMeanG.Checked = Properties.Settings.Default.drawMeanGroup;
            chkSeries.Checked = Properties.Settings.Default.OnlySeries;
            chkVoice.Checked = Properties.Settings.Default.voiceCommands;
            txtPDFlocation.Text = Properties.Settings.Default.pdfPath;
            txtDistance.Text = Properties.Settings.Default.targetDistance.ToString();
            chkScoreVoice.Checked = Properties.Settings.Default.scoreVoice;
            chkLog.Checked = Properties.Settings.Default.fileLogging;
            chkMiss.Checked = Properties.Settings.Default.ignoreMiss;
            trkLEDbright.Value = Properties.Settings.Default.LEDbright;

            cmb10Pen.SelectedItem = Properties.Settings.Default.score10PenColor.Name;
            cmb10Back.SelectedItem = Properties.Settings.Default.score10BackgroundColor.Name;
            cmb9Pen.SelectedItem = Properties.Settings.Default.score9PenColor.Name;
            cmb9Back.SelectedItem = Properties.Settings.Default.score9BackgroundColor.Name;
            cmbDefPen.SelectedItem = Properties.Settings.Default.scoreDefaultPenColor.Name;
            cmbDefBack.SelectedItem = Properties.Settings.Default.scoreDefaultBackgroundColor.Name;
            cmbOldPen.SelectedItem = Properties.Settings.Default.scoreOldPenColor.Name;
            cmbOldBack.SelectedItem = Properties.Settings.Default.scoreOldBackgroundColor.Name;

            txtSensorDiameter.Text = Properties.Settings.Default.SensorDiameter.ToString();
            txtZOffset.Text = Properties.Settings.Default.ZOffset.ToString();

            txtNorthX.Text = Properties.Settings.Default.SensorNorthX.ToString();
            txtNorthY.Text = Properties.Settings.Default.SensorNorthY.ToString();
            txtWestX.Text = Properties.Settings.Default.SensorWestX.ToString();
            txtWestY.Text = Properties.Settings.Default.SensorWestY.ToString();
            txtSouthX.Text = Properties.Settings.Default.SensorSouthX.ToString();
            txtSouthY.Text = Properties.Settings.Default.SensorSouthY.ToString();
            txtEastX.Text = Properties.Settings.Default.SensorEastX.ToString();
            txtEastY.Text = Properties.Settings.Default.SensorEastY.ToString();

            txtCalibre.Text = Properties.Settings.Default.Calibre.ToString();

            txtSteps.Text = Properties.Settings.Default.StepCount.ToString();
            txtStepTime.Text = Properties.Settings.Default.StepTime.ToString();
            txtPaperTime.Text = Properties.Settings.Default.PaperTime.ToString();

            if (Properties.Settings.Default.PaperTime > 0) {
                rbStepper.Checked = false;
                rbDC.Checked = true;
            } else {
                rbStepper.Checked = true;
                rbDC.Checked = false;
            }

            cmbName.SelectedIndex = Properties.Settings.Default.targetName;

        }

        private void loadTargetNames() {
            foreach(string s in targetNames) {
                cmbName.Items.Add(s);
            }
        }

        private void cmbColor_DrawItem(object sender, DrawItemEventArgs e)
        {
            e.DrawBackground();

            Graphics g = e.Graphics;
            Rectangle rect = e.Bounds;
            if (e.Index >= 0)
            {
                string n = ((ComboBox)sender).Items[e.Index].ToString();
                Font f = new Font("Microsoft Sans Serif", 8.25f, FontStyle.Regular);
                Color c = Color.FromName(n);
                Brush b = new SolidBrush(c);
                g.DrawString(n, f, Brushes.Black, rect.X, rect.Top);
                g.FillRectangle(b, rect.X + 115, rect.Y + 5, rect.Width - 5, rect.Height - 7);
            }
        }

        private void btnBrowse_Click(object sender, EventArgs e) {
            folderBrowserDialog.SelectedPath = txtPDFlocation.Text;
            SendKeys.Send("{TAB}{TAB}{RIGHT}");
            if (folderBrowserDialog.ShowDialog() == DialogResult.OK) {
                txtPDFlocation.Text = folderBrowserDialog.SelectedPath;
            }
        }

        private void btnOK_Click(object sender, EventArgs e) {
            if(btnSaveEvent.Enabled == true) {
                //event modify active
                MessageBox.Show("An Event is still under modification. " + Environment.NewLine + "Please Save or Cancel the modification.", "Event modified", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                tabControl1.SelectedTab = tabEvents;
                return;
            }


            if (validateData()) {
                Event defaultEvent = (Event)cmbWeapons.SelectedItem;
                bool found = false;
                if (defaultEvent != null) { 
                    foreach (Event ev in lstbActiveEvents.Items) {
                        if (defaultEvent.Name == ev.Name) {
                            found = true;
                            break;
                        }
                    }
                }
                if (found == false) {
                    MessageBox.Show("The 'Default Event' combobox in the 'Target' tab does not point to an active event." + Environment.NewLine + "Please select an active event.", "Invalida default event", MessageBoxButtons.OK, MessageBoxIcon.Error);
                } else {
                    DialogResult = DialogResult.OK;
                    this.Close();
                }
            }
        }

        private void linkLabel1_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e) {
            System.Diagnostics.Process.Start(linkLabel.Text);
        }

        private void linkLabel1_LinkClicked_1(object sender, LinkLabelLinkClickedEventArgs e) {
            System.Diagnostics.Process.Start(linkLabel1.Text);
        }

        private bool validateData() {

            if(txtName.Text == null || txtName.Text == "") {
                MessageBox.Show("Shooter name empty. Please fill in your name.", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }

            //baud rate
            if (!validNumber(txtBaud.Text)) {
                MessageBox.Show("Baud rate is not a number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }

            if (!positiveNumber(txtBaud.Text)) {
                MessageBox.Show("Baud rate must be a positive number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }


            //target distance
            if (!validNumber(txtDistance.Text)) {
                MessageBox.Show("Distance is not a number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            } else {
                if (int.Parse(txtDistance.Text) < 30) {
                    MessageBox.Show("Minimum safe target distance is 3 meters, so a value of at least 30 must be entered", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                    return false;
                }
            }

            //pdf location
            if (!Directory.Exists(txtPDFlocation.Text)) {
                MessageBox.Show("PDF save location must exist", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }

            //sensor diameter
            if (!validDecimal(txtSensorDiameter.Text)) {
                MessageBox.Show("Sensor diameter is not a number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            } else {
                if (decimal.Parse(txtSensorDiameter.Text) < 230) {
                    MessageBox.Show("Sensor diameter is smaller than 230, the default value.", "Small diameter", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            }

            //zoffset
            if (!validNumber(txtZOffset.Text)) {
                MessageBox.Show("Z offset is not a number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }

            //northX
            if (!validNumber(txtNorthX.Text)) {
                MessageBox.Show("Offset for North X sensor is not a number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }

            //northY
            if (!validNumber(txtNorthY.Text)) {
                MessageBox.Show("Offset for North Y sensor is not a number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }

            //westX
            if (!validNumber(txtWestX.Text)) {
                MessageBox.Show("Offset for West X sensor is not a number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }

            //westY
            if (!validNumber(txtWestY.Text)) {
                MessageBox.Show("Offset for West Y sensor is not a number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }

            //southX
            if (!validNumber(txtSouthX.Text)) {
                MessageBox.Show("Offset for South X sensor is not a number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }

            //southY
            if (!validNumber(txtSouthY.Text)) {
                MessageBox.Show("Offset for South Y sensor is not a number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }

            //eastX
            if (!validNumber(txtEastX.Text)) {
                MessageBox.Show("Offset for East X sensor is not a number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }

            //easyY
            if (!validNumber(txtEastY.Text)) {
                MessageBox.Show("Offset for East Y sensor is not a number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }

            //calibre
            if (!validNumber(txtCalibre.Text)) {
                MessageBox.Show("Calibre is not a number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            } else {
                if (int.Parse(txtCalibre.Text) < 45) {
                    MessageBox.Show("Calibre is cannot be smaller than 45 (4.5 mm x 10)", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                    return false;
                }
            }

            //stepper
            if (!validNumber(txtSteps.Text)) {
                MessageBox.Show("Stepper steps is not a number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }

            if (!positiveNumber(txtSteps.Text)) {
                MessageBox.Show("Stepper steps is not a number must be a positive number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }

            if (!validNumber(txtStepTime.Text)) {
                MessageBox.Show("Stepper duration is not a number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }

            if (!positiveNumber(txtStepTime.Text)) {
                MessageBox.Show("Stepper duration must be a positive number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }

            //dc
            if (!validNumber(txtPaperTime.Text)) {
                MessageBox.Show("Direct Current time is not a number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }

            if (!positiveNumber(txtPaperTime.Text)) {
                MessageBox.Show("Direct Current time must be a positive number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }

            return true;
        }
        private bool validNumber(String text) {
            try {
                int.Parse(text, CultureInfo.InvariantCulture);
                return true;

            } catch (Exception) {
                return false;
            }
        }

        private bool validDecimal(String text) {
            try {
                
                decimal r = decimal.Parse(text,CultureInfo.InvariantCulture);
                string newText = r.ToString(CultureInfo.InvariantCulture);
                //after parsing and converting to string again, the text should be identical. otherwise, the parsing was not correct
                //this is to avoid the decimal separator being different but parse would not throw an error but return a wrong number
                if (newText == text) { 
                    return true;
                } else {
                    return false;
                }
                

            } catch (Exception) {
                return false;
            }
        }

        private bool positiveNumber(String text) {
            try {
                int x = int.Parse(text);
                if (x >= 0) {
                    return true;
                } else {
                    return false;
                }

            } catch (Exception) {
                return false;
            }
        }

        private void rbDC_CheckedChanged(object sender, EventArgs e) {
            radios();
        }

        private void rbStepper_CheckedChanged(object sender, EventArgs e) {
            radios();
        }

        private void radios() {
            if (rbDC.Checked) {
                txtPaperTime.Enabled = true;

                txtSteps.Enabled = false;
                txtStepTime.Enabled = false;
                txtSteps.Text = "0";
                txtStepTime.Text = "0";
            } else { //stepper
                txtPaperTime.Enabled = false;
                txtPaperTime.Text = "0";

                txtSteps.Enabled = true;
                txtStepTime.Enabled = true;
            }
        }

        private void btnLeftToRight_Click(object sender, EventArgs e) {
            Event ev = (Event)lstbAllEvents.SelectedItem;
            lstbActiveEvents.Items.Add(ev);
            cmbWeapons.Items.Add(ev);
            lstbAllEvents.Items.Remove(ev);
            btnLeftToRight.Enabled = false;
        }

        private void btnRightToLeft_Click(object sender, EventArgs e) {
            Event ev = (Event)lstbActiveEvents.SelectedItem;
            lstbAllEvents.Items.Add(ev);
            lstbActiveEvents.Items.Remove(ev);
            cmbWeapons.Items.Remove(ev);
            btnRightToLeft.Enabled = false;

            btnUp.Enabled = false;
            btnDown.Enabled = false;
        }

        private void btnUp_Click(object sender, EventArgs e) {
            Event ev = (Event)lstbActiveEvents.SelectedItem;
            int oldindex = lstbActiveEvents.SelectedIndex;

            lstbActiveEvents.Items.Remove(ev);
            lstbActiveEvents.Items.Insert(oldindex - 1, ev);

            btnUp.Enabled = false;
            btnDown.Enabled = false;
            btnRightToLeft.Enabled = false;
            btnLeftToRight.Enabled = false;
        }

        private void btnDown_Click(object sender, EventArgs e) {
            Event ev = (Event)lstbActiveEvents.SelectedItem;
            int oldindex = lstbActiveEvents.SelectedIndex;

            lstbActiveEvents.Items.Remove(ev);
            lstbActiveEvents.Items.Insert(oldindex + 1, ev);

            btnUp.Enabled = false;
            btnDown.Enabled = false;
            btnRightToLeft.Enabled = false;
            btnLeftToRight.Enabled = false;
        }

        private void lstbAllEvents_SelectedIndexChanged(object sender, EventArgs e) {
            if (lstbAllEvents.SelectedItem != null) {
                btnLeftToRight.Enabled = true;
                btnRightToLeft.Enabled = false;
                lstbActiveEvents.SelectedItem = null;

                btnUp.Enabled = false;
                btnDown.Enabled = false;
            }
        }

        private void lstbActiveEvents_SelectedIndexChanged(object sender, EventArgs e) {
            if (lstbActiveEvents.SelectedItem != null) {
                btnLeftToRight.Enabled = false;
                btnRightToLeft.Enabled = true;
                lstbAllEvents.SelectedItem = null;

                int index = lstbActiveEvents.SelectedIndex;
                if (index > 0) {
                    btnUp.Enabled = true;
                } else {
                    btnUp.Enabled = false;
                }

                if (index < lstbActiveEvents.Items.Count - 1) {
                    btnDown.Enabled = true;
                } else {
                    btnDown.Enabled = false;
                }
            }
        }

        private void lstbEvents_SelectedIndexChanged(object sender, EventArgs e) {
            if (lstbEvents.SelectedItem != null) {
                btnModifyEvent.Enabled = true;

                Event ev = (Event)lstbEvents.SelectedItem;
                txtEventName.Text = ev.Name;
                cmbEventTypes.SelectedItem = ev.Type;
                chkDecimalScoring.Checked = ev.DecimalScoring;
                txtNoOfShots.Text = ev.NumberOfShots.ToString(CultureInfo.InvariantCulture);
                cmbTargets.SelectedItem = ev.Target.getName();
                txtCaliber.Text = ev.ProjectileCaliber.ToString(CultureInfo.InvariantCulture);
                txtMinutes.Text = ev.Minutes.ToString(CultureInfo.InvariantCulture);
                cmbTabColor.SelectedItem = ev.TabColor.Name;
                txtShotPerSeries.Text = ev.Final_NumberOfShotPerSeries.ToString(CultureInfo.InvariantCulture);
                txtSeriesDuration.Text = ev.Final_SeriesSeconds.ToString(CultureInfo.InvariantCulture);
                txtShotsInSeries.Text = ev.Final_NumberOfShotsBeforeSingleShotSeries.ToString(CultureInfo.InvariantCulture);
                txtSingleShotDuration.Text = ev.Final_SingleShotSeconds.ToString(CultureInfo.InvariantCulture);
                txtShotsSingles.Text = ev.Final_NumberOfShotsInSingleShotSeries.ToString(CultureInfo.InvariantCulture);
            } else {
                btnModifyEvent.Enabled = false;
            }
        }

        private void btnAddEvent_Click(object sender, EventArgs e) {
            lstbEvents.SelectedItem = null;

            //clean controls
            txtEventName.Clear();
            cmbEventTypes.SelectedItem = null;
            chkDecimalScoring.Checked = false;
            txtNoOfShots.Clear();
            cmbTargets.SelectedItem = null;
            txtCaliber.Clear();
            txtMinutes.Clear();
            cmbTabColor.SelectedItem = null;
            txtShotPerSeries.Clear();
            txtSeriesDuration.Clear();
            txtShotsInSeries.Clear();
            txtSingleShotDuration.Clear();
            txtShotsSingles.Clear();

            foreach (Control c in eventControls) {
                c.Enabled = true;
            }
            btnAddEvent.Enabled = false;
            btnModifyEvent.Enabled = false;
            lstbEvents.Enabled = false;
        }

        private void btnModifyEvent_Click(object sender, EventArgs e) {
            foreach(Control  c in eventControls) {
                c.Enabled = true;
            }
            btnAddEvent.Enabled = false;
            btnModifyEvent.Enabled = false;
            lstbEvents.Enabled = false;

            cmbEventTypes_SelectedIndexChanged(null, null);
        }


        private string validateEventFields() {
            //preset field with empty value
            if (cmbEventTypes.SelectedItem != null) {
                Event.EventType type = (Event.EventType)cmbEventTypes.SelectedItem;

                switch (type) {
                    case Event.EventType.Practice:
                        txtNoOfShots.Text = "-1";
                        txtMinutes.Text = "-1";
                        txtShotPerSeries.Text = "-1";
                        txtSeriesDuration.Text = "-1";
                        txtShotsInSeries.Text = "-1";
                        txtSingleShotDuration.Text = "-1";
                        txtShotsSingles.Text = "-1";
                        break;
                    case Event.EventType.Match:
                        txtShotPerSeries.Text = "-1";
                        txtSeriesDuration.Text = "-1";
                        txtShotsInSeries.Text = "-1";
                        txtSingleShotDuration.Text = "-1";
                        txtShotsSingles.Text = "-1";
                        break;
                    case Event.EventType.Final:

                        break;
                }
            }


            string err = "";

            if (txtEventName.Text.Length == 0) {
                err += " * 'Name' is required." + Environment.NewLine;
            }
            if (cmbEventTypes.SelectedItem == null) {
                err += " * Please select an event 'Type'." + Environment.NewLine;
            }

            if(txtNoOfShots.Text.Length == 0) {
                err += " * 'Number of Shots' is required." + Environment.NewLine;
            } else if(!validNumber(txtNoOfShots.Text)) {
                err += " * 'Number of Shots' field is not a number." + Environment.NewLine;
            }

            if(txtCaliber.Text.Length == 0) {
                err += " * 'Caliber' is required." + Environment.NewLine;
            } else if (!validDecimal(txtCaliber.Text)) {
                err += " * 'Caliber' field is not a decimal." + Environment.NewLine;
            }

            if (cmbTargets.SelectedItem == null) {
                err += " * Please select a 'Target'." + Environment.NewLine;
            }

            if (txtMinutes.Text.Length == 0) {
                err += " * 'Duration' is required." + Environment.NewLine;
            } else if(!validNumber(txtMinutes.Text)) {
                err += " * 'Duration' field is not a number." + Environment.NewLine;
            }

            if (cmbTabColor.SelectedItem == null) {
                err += " * Please select a 'Color'." + Environment.NewLine;
            }

            if (txtShotPerSeries.Text.Length == 0) {
                err += " * 'Shots per Serie' is required." + Environment.NewLine;
            } else if (!validNumber(txtShotPerSeries.Text)) {
                err += " * 'Shots per Serie' field is not a number." + Environment.NewLine;
            }

            if(txtSeriesDuration.Text.Length == 0) {
                err += " * 'Series Duration' is required." + Environment.NewLine;
            } else if (!validNumber(txtSeriesDuration.Text)) {
                err += " * 'Series Duration' field is not a number." + Environment.NewLine;
            }

            if(txtShotsInSeries.Text.Length == 0) {
                err += " * 'Shots in All Series' is required." + Environment.NewLine;
            } else if (!validNumber(txtShotsInSeries.Text)) {
                err += " * 'Shots in All Series' field is not a number." + Environment.NewLine;
            }

            if(txtSingleShotDuration.Text.Length == 0) {
                err += " * 'Single Shot Duration' is required." + Environment.NewLine;
            } else if (!validNumber(txtSingleShotDuration.Text)) {
                err += " * 'Single Shot Duration' field is not a number." + Environment.NewLine;
            }

            if (txtShotsSingles.Text.Length == 0) {
                err += " * 'Shots in Single Series' is required." + Environment.NewLine;
            } else if (!validNumber(txtShotsSingles.Text)) {
                err += " * 'Shots in Single Series' field is not a number." + Environment.NewLine;
            }

            return err;
        }

        private void btnSaveEvent_Click(object sender, EventArgs e) {

            string err = validateEventFields();
            if (err.Length > 0) {
                MessageBox.Show("There are some validation errors in the Event. Please fix and save again:" + Environment.NewLine + err, "Event Validation", MessageBoxButtons.OK, MessageBoxIcon.Stop);
                return;
            }

            string ev_Name= txtEventName.Text;
            Event.EventType ev_Type = (Event.EventType)cmbEventTypes.SelectedItem;
            bool ev_DecimalScoring = chkDecimalScoring.Checked;
            int ev_NumberOfShots = Int32.Parse(txtNoOfShots.Text, CultureInfo.InvariantCulture);
            decimal ev_ProjectileCaliber = Decimal.Parse(txtCaliber.Text, CultureInfo.InvariantCulture);

            Type target_type = Type.GetType((string)cmbTargets.SelectedItem);
            targets.aTarget ev_Target = (targets.aTarget)Activator.CreateInstance(target_type, new object[] { ev_ProjectileCaliber });
            
            int ev_Minutes = Int32.Parse(txtMinutes.Text, CultureInfo.InvariantCulture);
            Color ev_TabColor = System.Drawing.Color.FromName((string)cmbTabColor.SelectedItem);
            int ev_Final_NumberOfShotPerSeries = Int32.Parse(txtShotPerSeries.Text,CultureInfo.InvariantCulture);
            int ev_Final_SeriesSeconds = Int32.Parse(txtSeriesDuration.Text, CultureInfo.InvariantCulture);
            int ev_Final_NumberOfShotsBeforeSingleShotSeries = Int32.Parse(txtShotsInSeries.Text, CultureInfo.InvariantCulture);
            int ev_Final_SingleShotSeconds = Int32.Parse(txtSingleShotDuration.Text, CultureInfo.InvariantCulture);
            int ev_Final_NumberOfShotsInSingleShotSeries = Int32.Parse(txtShotsSingles.Text, CultureInfo.InvariantCulture);



            if (lstbEvents.SelectedItem != null) {
                //modify
                Event ev = (Event)lstbEvents.SelectedItem;
                ev.Name = ev_Name;
                ev.Type = ev_Type;
                ev.DecimalScoring = ev_DecimalScoring;
                ev.NumberOfShots = ev_NumberOfShots;
                ev.ProjectileCaliber = ev_ProjectileCaliber;
                ev.Target = ev_Target;
                ev.Minutes = ev_Minutes;
                ev.TabColor = ev_TabColor;
                ev.Final_NumberOfShotPerSeries = ev_Final_NumberOfShotPerSeries;
                ev.Final_SeriesSeconds = ev_Final_SeriesSeconds;
                ev.Final_NumberOfShotsBeforeSingleShotSeries = ev_Final_NumberOfShotsBeforeSingleShotSeries;
                ev.Final_SingleShotSeconds = ev_Final_SingleShotSeconds;
                ev.Final_NumberOfShotsInSingleShotSeries = ev_Final_NumberOfShotsInSingleShotSeries;

                mainWindow.storage.updateEvent(ev);
            } else {
                //add new
                Event ev = new Event(-1, ev_Name, ev_DecimalScoring, ev_Type, ev_NumberOfShots, ev_Target, ev_Minutes,
                    ev_ProjectileCaliber, ev_Final_NumberOfShotPerSeries, ev_Final_SeriesSeconds,
                    ev_Final_NumberOfShotsBeforeSingleShotSeries, ev_Final_SingleShotSeconds,
                    ev_Final_NumberOfShotsInSingleShotSeries, ev_TabColor);

                mainWindow.storage.storeEvent(ev);
                mainWindow.eventManager.setEventsList(mainWindow.storage.loadEvents());
                loadEvents();
            }

            foreach (Control c in eventControls) {
                c.Enabled = false;
            }
            btnAddEvent.Enabled = true;
            btnModifyEvent.Enabled = true;
            lstbEvents.Enabled = true;
        }

        private void btnCancelEventSave_Click(object sender, EventArgs e) {
            foreach (Control c in eventControls) {
                c.Enabled = false;
            }
            btnAddEvent.Enabled = true;
            btnModifyEvent.Enabled = true;
            lstbEvents.Enabled = true;
        }

        private void cmbEventTypes_SelectedIndexChanged(object sender, EventArgs e) {
            if (btnSaveEvent.Enabled == false) {
                return;
            }
            if (cmbEventTypes.SelectedItem != null) {
                Event.EventType type = (Event.EventType)cmbEventTypes.SelectedItem;
                switch (type) {
                    case Event.EventType.Practice:
                        txtNoOfShots.Enabled = false;
                        txtMinutes.Enabled = false;
                        txtShotPerSeries.Enabled = false;
                        txtSeriesDuration.Enabled = false;
                        txtShotsInSeries.Enabled = false;
                        txtSingleShotDuration.Enabled = false;
                        txtShotsSingles.Enabled = false;
                        break;
                    case Event.EventType.Match:
                        txtNoOfShots.Enabled = true;
                        txtMinutes.Enabled = true;
                        txtShotPerSeries.Enabled = false;
                        txtSeriesDuration.Enabled = false;
                        txtShotsInSeries.Enabled = false;
                        txtSingleShotDuration.Enabled = false;
                        txtShotsSingles.Enabled = false;
                        break;
                    case Event.EventType.Final:
                        txtNoOfShots.Enabled = true;
                        txtMinutes.Enabled = true;
                        txtShotPerSeries.Enabled = true;
                        txtSeriesDuration.Enabled = true;
                        txtShotsInSeries.Enabled = true;
                        txtSingleShotDuration.Enabled = true;
                        txtShotsSingles.Enabled = true;
                        break;
                }
            }
        }
    }
}
