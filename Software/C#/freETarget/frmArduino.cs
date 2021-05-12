using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Net.Http.Headers;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace freETarget {
    public partial class frmArduino : Form {


        private static frmArduino instance;
        frmMainWindow mainWindow;

        private delegate void SafeCallDelegate2(string text);

        private frmArduino(frmMainWindow mainWin) {
            InitializeComponent();
            this.mainWindow = mainWin;

            //custom stringbuilder event. bound to the textbox
            mainWindow.output.TextChanged += new EventHandler(frmArduino_Activated);
        }

        public static frmArduino getInstance(frmMainWindow mainWin) {
            if (instance != null) {
                return instance;
            } else {
                instance = new frmArduino(mainWin);
                return instance;
            }
        }

        private void frmArduino_Load(object sender, EventArgs e) {

        }

        private void btnClose_Click(object sender, EventArgs e) {
            this.Close();
        }

        private void btnEcho_Click(object sender, EventArgs e) {
            mainWindow.serialPort.Write("{\"ECHO\":0}");
        }

        private void frmArduino_FormClosing(object sender, FormClosingEventArgs e) {
            e.Cancel = true;
            this.Hide();
        }

        private void frmArduino_Activated(object sender, EventArgs e) {
            displayMessage(mainWindow.output.Text);
        }


        public void displayMessage(string text) {
            if (txtOutput.InvokeRequired) {
                var d = new SafeCallDelegate2(displayMessage);
                txtOutput.Invoke(d, new object[] { text});
                return;
            } else {
                txtOutput.Text = text;
                if (ckbAutoscroll.Checked) { 
                    txtOutput.SelectionStart = txtOutput.Text.Length;
                    txtOutput.ScrollToCaret();
                }
            }
        }

        private void btnDip_Click(object sender, EventArgs e) {
            mainWindow.serialPort.Write("{\"DIP\":"+txtDip.Text+"}");
        }

        private void btnPaper_Click(object sender, EventArgs e) {
            mainWindow.serialPort.Write("{\"PAPER\":" + txtPaper.Text + "}");
        }

        private void btnSensor_Click(object sender, EventArgs e) {
            mainWindow.serialPort.Write("{\"SENSOR\":" + txtSensor.Text + "}");
        }

        private void btnTest_Click(object sender, EventArgs e) {
            mainWindow.serialPort.Write("{\"TEST\":" + txtTest.Text + "}");
        }

        private void btnOffset_Click(object sender, EventArgs e) {
            mainWindow.serialPort.Write("{\"CALIBREx10\":" + txtOffset.Text + "}");
        }

        private void btnCalibration_Click(object sender, EventArgs e) {
            if (btnCalibration.Text == "CAL") {
                mainWindow.serialPort.Write("{\"CAL\":0}");
                btnCalibration.Text = "STOP CAL";
            } else {
                //STOP CAL
                mainWindow.serialPort.Write("!");
                btnCalibration.Text = "CAL";
            }


        }

        private void btnVersion_Click(object sender, EventArgs e) {
            mainWindow.serialPort.Write("{\"VERSION\":7}");
        }

        private void btnLed_Click(object sender, EventArgs e) {
            mainWindow.serialPort.Write("{\"LED_BRIGHT\":" + txtLed.Text + "}");
        }

        private void btnInit_Click(object sender, EventArgs e) {
            mainWindow.serialPort.Write("{\"INIT\":0}");
        }

        private void btnNameID_Click(object sender, EventArgs e) {
            mainWindow.serialPort.Write("{\"NAME_ID\":" + txtNameID.Text + "}");
        }

        private void btnTrace_Click(object sender, EventArgs e) {
            mainWindow.serialPort.Write("{\"TRACE\":" + txtTrace.Text + "}");
        }

        private void btnPower_Click(object sender, EventArgs e) {
            mainWindow.serialPort.Write("{\"POWER_SAVE\":" + txtPower.Text + "}");
        }

        private void btnSendMiss_Click(object sender, EventArgs e) {
            mainWindow.serialPort.Write("{\"SEND_MISS\":" + txtSendMiss.Text + "}");
        }

        private void btnTargetRing_Click(object sender, EventArgs e) {
            mainWindow.serialPort.Write("{\"TRGT_1_RINGx10\":" + txtTargetRing.Text + "}");
        }
    }
}
