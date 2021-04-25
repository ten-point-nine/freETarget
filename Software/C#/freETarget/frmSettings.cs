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

namespace freETarget
{
    public partial class frmSettings : Form {

        frmMainWindow mainWindow;


        public frmSettings(frmMainWindow mainWin)
        {
            InitializeComponent();
            this.mainWindow = mainWin;
        }

        private void frmSettings_Load(object sender, EventArgs e)
        {


            using (var devices = new ManagementObjectSearcher("SELECT * FROM WIN32_SerialPort")) {
                string[] portnames = SerialPort.GetPortNames();

                //detect names of devices connected to COM ports
                var ports = devices.Get().Cast<ManagementBaseObject>().ToList();
                var device_list = (from n in portnames
                                   join p in ports on n equals p["DeviceID"].ToString()
                                   select p["Caption"]).ToList();

                //detect arduino on COM
                bool arduinoFound = false;
                for(int i = 0; i < portnames.Length; i++) {
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
            }

            cmbWeapons.Items.Add("Air Pistol Practice");
            cmbWeapons.Items.Add("Air Rifle Practice");

            loadSettings();

            System.Reflection.Assembly assembly = System.Reflection.Assembly.GetExecutingAssembly();
            lblVersion.Text = "freETarget Project  -  v"+ assembly.GetName().Version.Major + "." + assembly.GetName().Version.Minor + "." + assembly.GetName().Version.Build + "   (c) 2020-2021";
        }

        private void loadSettings()
        {
            txtName.Text = Properties.Settings.Default.name;
            txtBaud.Text = Properties.Settings.Default.baudRate.ToString();
            chkDisplayConsole.Checked = Properties.Settings.Default.displayDebugConsole;
            cmbPorts.SelectedItem = Properties.Settings.Default.portName;
            cmbWeapons.SelectedItem = Properties.Settings.Default.defaultTarget;
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

            if (Properties.Settings.Default.MatchShots == 60) {
                rdb60.Checked = true;
                rdb40.Checked = false;
            } else if (Properties.Settings.Default.MatchShots == 40) {
                rdb60.Checked = false;
                rdb40.Checked = true;
            } else {
                rdb60.Checked = false;
                rdb40.Checked = false;
            }

            cmb10Pen.SelectedItem = Properties.Settings.Default.score10PenColor.Name;
            cmb10Back.SelectedItem = Properties.Settings.Default.score10BackgroundColor.Name;
            cmb9Pen.SelectedItem = Properties.Settings.Default.score9PenColor.Name;
            cmb9Back.SelectedItem = Properties.Settings.Default.score9BackgroundColor.Name;
            cmbDefPen.SelectedItem = Properties.Settings.Default.scoreDefaultPenColor.Name;
            cmbDefBack.SelectedItem = Properties.Settings.Default.scoreDefaultBackgroundColor.Name;
            cmbOldPen.SelectedItem = Properties.Settings.Default.scoreOldPenColor.Name;
            cmbOldBack.SelectedItem = Properties.Settings.Default.scoreOldBackgroundColor.Name;

            txtSensorDiameter.Text = Properties.Settings.Default.SensorDiameter.ToString();

            txtNorthX.Text = Properties.Settings.Default.SensorNorthX.ToString();
            txtNorthY.Text = Properties.Settings.Default.SensorNorthY.ToString();
            txtWestX.Text = Properties.Settings.Default.SensorWestX.ToString();
            txtWestY.Text = Properties.Settings.Default.SensorWestY.ToString();
            txtSouthX.Text = Properties.Settings.Default.SensorSouthX.ToString();
            txtSouthY.Text = Properties.Settings.Default.SensorSouthY.ToString();
            txtEastX.Text = Properties.Settings.Default.SensorEastX.ToString();
            txtEastY.Text = Properties.Settings.Default.SensorEastY.ToString();

            txtCalibre.Text = Properties.Settings.Default.Calibre.ToString();
            txtPaper.Text = Properties.Settings.Default.Paper.ToString();

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
                g.FillRectangle(b, rect.X + 130, rect.Y + 5, rect.Width - 5, rect.Height - 7);
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

            if (validateData()) {
                DialogResult = DialogResult.OK;
                this.Close();
            }
        }

        private void linkLabel1_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e) {
            System.Diagnostics.Process.Start(linkLabel.Text);
        }

        private void linkLabel1_LinkClicked_1(object sender, LinkLabelLinkClickedEventArgs e) {
            System.Diagnostics.Process.Start(linkLabel1.Text);
        }

        private bool validateData() {
            //baud rate
            if (!validNumber(txtBaud.Text)) {
                MessageBox.Show("Baud rate is not a number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
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

            //paper
            if (!validNumber(txtPaper.Text)) {
                MessageBox.Show("Paper scroll time is not a number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }


            return true;
        }
        private bool validNumber(String text) {
            try {
                int.Parse(text);
                return true;

            } catch (Exception) {
                return false;
            }
        }

        private bool validDecimal(String text) {
            try {
                decimal.Parse(text);
                return true;

            } catch (Exception) {
                return false;
            }
        }
    }
}
