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


        private Command[] firmwareCommands = new Command[] {
            new Command("CALIBREx10","45"),
            new Command("DIP","0"),
            new Command("LED_BRIGHT","50"),
            new Command("MFS","0"),
            new Command("NAME_ID","1"),
            new Command("PAPER_TIME","0"),
            new Command("POWER_SAVE","30"),
            new Command("SEND_MISS","0"),
            new Command("SENSOR","230.00"),
            new Command("STEP_COUNT","0"),
            new Command("STEP_TIME","0"),
            new Command("TEST","0"),
            new Command("TRACE","0"),
            new Command("Z_OFFSET","0"),
            new Command("PAPER_ECO","0"),
            new Command("TARGET_TYPE","0"),
            new Command("NORTH_X","0"),
            new Command("NORTH_Y","0"),
            new Command("EAST_X","0"),
            new Command("EAST_Y","0"),
            new Command("SOUTH_X","0"),
            new Command("SOUTH_Y","0"),
            new Command("WEST_X","0"),
            new Command("WEST_Y","0"),
            new Command("ANGLE","45")
        };


        private static frmArduino instance;
        frmMainWindow mainWindow;

        private delegate void SafeCallDelegate2(string text);

        private frmArduino(frmMainWindow mainWin) {
            InitializeComponent();
            this.mainWindow = mainWin;

            cmbCommands.Items.AddRange(firmwareCommands);
            cmbCommands.SelectedIndex = 0;

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

        private void frmArduino_FormClosing(object sender, FormClosingEventArgs e) {
            e.Cancel = true;
            this.Hide();
        }

        public void displayMessage(string text) {
            if (txtOutput.InvokeRequired) {
                var d = new SafeCallDelegate2(displayMessage);
                txtOutput.Invoke(d, new object[] { text });
                return;
            } else {
                txtOutput.Text = text;
                if (ckbAutoscroll.Checked) {
                    txtOutput.SelectionStart = txtOutput.Text.Length;
                    txtOutput.ScrollToCaret();
                }
            }
        }

        private void frmArduino_Activated(object sender, EventArgs e) {
            displayMessage(mainWindow.output.Text);
        }


        private void btnEcho_Click(object sender, EventArgs e) {
            mainWindow.commModule.sendData("{\"ECHO\":0}");
        }

        private void btnInit_Click(object sender, EventArgs e) {
            mainWindow.commModule.sendData("{\"INIT\":0}");
        }

        private void btnCalibration_Click(object sender, EventArgs e) {
            if (btnCalibration.Text == "CAL") {
                mainWindow.commModule.sendData("{\"CAL\":0}");
                btnCalibration.Text = "STOP CAL";
            } else {
                //STOP CAL
                mainWindow.commModule.sendData("!");
                btnCalibration.Text = "CAL";
            }


        }

        private void btnVersion_Click(object sender, EventArgs e) {
            mainWindow.commModule.sendData("{\"VERSION\":7}");
        }




        private void cmbCommands_SelectedIndexChanged(object sender, EventArgs e) {
            Command c = (Command)cmbCommands.SelectedItem;
            txtParameter.Text = c.defaultValue;
        }

        private void btnSend_Click(object sender, EventArgs e) {
            mainWindow.commModule.sendData("{\"" + cmbCommands.SelectedItem.ToString() +  "\":" + txtParameter.Text + "}");
        }

    }

    class Command {
        public String command;
        public String defaultValue;

        public Command(String com, String value) {
            this.command = com;
            this.defaultValue = value;
        }

        public override string ToString() {
            return command;
        }
    }
}
