using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Threading;
using System.Globalization;

namespace freETarget {
    public partial class frmTargetSettings : Form {

        frmMainWindow mainWindow;

        bool closeRequested = false;

        private List<string> targetNames = new List<string>() { "TARGET",                                                     //  0
                        "1",      "2",        "3",     "4",      "5",       "6",       "7",     "8",     "9",      "10",    //  1
                        "DOC",    "DOPEY",  "HAPPY",   "GRUMPY", "BASHFUL", "SNEEZEY", "SLEEPY",                            // 11
                        "RUDOLF", "DONNER", "BLITXEM", "DASHER", "PRANCER", "VIXEN",   "COMET", "CUPID", "DUNDER",          // 18  
                        "ODIN",   "WODEN",   "THOR",   "BALDAR"                                                            // 26
                        };

        public frmTargetSettings(frmMainWindow mainWin) {
            InitializeComponent();
            this.mainWindow = mainWin;

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

        private void trkLEDbright_ValueChanged(object sender, EventArgs e) {
            lblLED.Text = "LED Brigtness (" + trkLEDbright.Value + ")";
        }

 
        private void frmTargetSettings_Load(object sender, EventArgs e) {

            pnlWait.Visible = true;
            pnlWait.Top = 125;

            loadTargetNames();

        }

        private void loadTargetNames() {
            foreach (string s in targetNames) {
                cmbName.Items.Add(s);
            }
        }

        private void frmTargetSettings_Shown(object sender, EventArgs e) {
            mainWindow.lastEcho = null;
            mainWindow.commModule.sendData("{\"ECHO\":0}");
            mainWindow.log("Sending: " + "{\"ECHO\":0}");

            while (mainWindow.lastEcho == null) {
                Application.DoEvents();
                Thread.Sleep(100);
                if(closeRequested == true) {
                    return;
                }
            }

            pnlWait.Visible = false;
            pnlWait.Top = 513;
            tabControl.Enabled = true;
            btnApply.Enabled = true;

            loadEcho(mainWindow.lastEcho);
        }

        private void btnClose_Click(object sender, EventArgs e) {
            closeRequested = true;
        }

        private void loadEcho (Echo echo) {
            string n = echo.NAME.Substring(1, echo.NAME.Length - 2);
            cmbName.SelectedItem = n;
            txtPaperTime.Text = echo.PAPER_TIME.ToString(CultureInfo.InvariantCulture);
            txtStepTime.Text = echo.STEP_TIME.ToString(CultureInfo.InvariantCulture);
            txtSteps.Text = echo.STEP_COUNT.ToString(CultureInfo.InvariantCulture);
            if(echo.PAPER_TIME > 0) {
                rbStepper.Checked = false;
                rbDC.Checked = true;
            } else {
                rbStepper.Checked = true;
                rbDC.Checked = false;
            }

            trkLEDbright.Value = echo.LED_BRIGHT;
            lblLED.Text = "LED Brigtness (" + trkLEDbright.Value + ")";


            txtSensorDiameter.Text = echo.SENSOR.ToString(CultureInfo.InvariantCulture);
            txtZOffset.Text = echo.Z_OFFSET.ToString(CultureInfo.InvariantCulture);

            txtNorthX.Text = echo.NORTH_X.ToString(CultureInfo.InvariantCulture);
            txtNorthY.Text = echo.NORTH_Y.ToString(CultureInfo.InvariantCulture);
            txtWestX.Text = echo.WEST_X.ToString(CultureInfo.InvariantCulture);
            txtWestY.Text = echo.WEST_Y.ToString(CultureInfo.InvariantCulture);
            txtSouthX.Text = echo.SOUTH_X.ToString(CultureInfo.InvariantCulture);
            txtSouthY.Text = echo.SOUTH_Y.ToString(CultureInfo.InvariantCulture);
            txtEastX.Text = echo.EAST_X.ToString(CultureInfo.InvariantCulture);
            txtEastY.Text = echo.EAST_Y.ToString(CultureInfo.InvariantCulture);
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

                decimal r = decimal.Parse(text, CultureInfo.InvariantCulture);
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

        private bool validateData() {

            //sensor diameter
            if (!validDecimal(txtSensorDiameter.Text)) {
                MessageBox.Show("Sensor diameter is not a decimal", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
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

        private void btnApply_Click(object sender, EventArgs e) {
            if (validateData()) {
                StringBuilder sb = new StringBuilder("{");

                //send sensor and hardware parameters to target

                sb.Append("\"SENSOR\":" + txtSensorDiameter.Text + ", ");
                sb.Append("\"Z_OFFSET\":" + txtZOffset.Text + ", ");


                if (int.Parse(txtPaperTime.Text) > 0) {
                    sb.Append("\"PAPER_TIME\":" + txtPaperTime.Text + ", ");
                    sb.Append("\"STEP_TIME\":0, ");
                    sb.Append("\"STEP_COUNT\":0, ");
                } else {

                    sb.Append("\"PAPER_TIME\":0, ");
                    sb.Append("\"STEP_TIME\":" + txtStepTime.Text + ", ");
                    sb.Append("\"STEP_COUNT\":" + txtSteps.Text + ", ");
                }
                sb.Append("\"NORTH_X\":" + txtNorthX.Text + ", ");
                sb.Append("\"NORTH_Y\":" + txtNorthY.Text + ", ");
                sb.Append("\"EAST_X\":" + txtEastX.Text + ", ");
                sb.Append("\"EAST_Y\":" + txtEastY.Text + ", ");
                sb.Append("\"SOUTH_X\":" + txtSouthX.Text + ", ");
                sb.Append("\"SOUTH_Y\":" + txtSouthY.Text + ", ");
                sb.Append("\"WEST_X\":" + txtWestX.Text + ", ");
                sb.Append("\"WEST_Y\":" + txtWestY.Text + ", ");
                sb.Append("\"LED_BRIGHT\":" + trkLEDbright.Value + ", ");
                sb.Append("\"NAME_ID\":" + cmbName.SelectedIndex + ", ");

                sb.Append("\"ECHO\":9 }");

                mainWindow.commModule.sendData(sb.ToString());
                mainWindow.log("Sending: " + sb.ToString());

            }
        }


    }
}
