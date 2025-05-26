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
            new Command("PAPER_ECO","0"),
            new Command("PAPER_TIME","0"),
            new Command("POWER_SAVE","30"),
            new Command("SEND_MISS","0"),
            new Command("SENSOR","230.00"),
            new Command("STEP_COUNT","0"),
            new Command("STEP_TIME","0"),
            new Command("TABATA_ENABLE","0"),
            new Command("TABATA_CYCLES","0"),
            new Command("TABATA_ON","0"),
            new Command("TABATA_REST","0"),
            new Command("TARGET_TYPE","0"),
            new Command("RAPID_ENABLE","0"),
            new Command("RAPID_CYCLES","0"),
            new Command("RAPID_ON","0"),
            new Command("RAPID_REST","0"),
            new Command("RAPID_TYPE","0"),
            new Command("TARGET_TYPE","0"),
            new Command("TEST","0"),
            new Command("TRACE","0"),
            new Command("Z_OFFSET","0"),
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
            this.Text = "Terminal " + Properties.Settings.Default.Board;
            if (Properties.Settings.Default.Board == frmMainWindow.Arduino) {
                this.Icon = Properties.Resources.arduino;
            } else {
                this.Icon = Properties.Resources.esp32;
            }
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
            mainWindow.log("Sending: " + "{\"ECHO\":0}");
        }

        private void btnInit_Click(object sender, EventArgs e) {
            string input = "INIT KEY";
            if (ShowInputDialog(ref input) == DialogResult.OK) {
                mainWindow.commModule.sendData("{\"INIT\":" + input + "}");
                mainWindow.log("Sending INIT to the target: " + "{\"INIT\":" + input + "}");
            }
        }



        private static DialogResult ShowInputDialog(ref string input) {
            System.Drawing.Size size = new System.Drawing.Size(200, 70);
            Form inputBox = new Form();

            inputBox.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            inputBox.StartPosition = FormStartPosition.CenterScreen;
            inputBox.FormBorderStyle = FormBorderStyle.FixedDialog;
            inputBox.MinimizeBox = false;
            inputBox.MaximizeBox = false;
            inputBox.ClientSize = size;
            inputBox.Text = "INIT key";

            System.Windows.Forms.TextBox textBox = new TextBox();
            textBox.Size = new System.Drawing.Size(size.Width - 10, 23);
            textBox.Location = new System.Drawing.Point(5, 5);
            textBox.Text = input;
            inputBox.Controls.Add(textBox);

            Button okButton = new Button();
            okButton.DialogResult = System.Windows.Forms.DialogResult.OK;
            okButton.Name = "okButton";
            okButton.Size = new System.Drawing.Size(75, 23);
            okButton.Text = "&OK";
            okButton.Location = new System.Drawing.Point(size.Width - 80 - 80, 39);
            inputBox.Controls.Add(okButton);

            Button cancelButton = new Button();
            cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            cancelButton.Name = "cancelButton";
            cancelButton.Size = new System.Drawing.Size(75, 23);
            cancelButton.Text = "&Cancel";
            cancelButton.Location = new System.Drawing.Point(size.Width - 80, 39);
            inputBox.Controls.Add(cancelButton);

            inputBox.AcceptButton = okButton;
            inputBox.CancelButton = cancelButton;

            DialogResult result = inputBox.ShowDialog();
            input = textBox.Text;
            return result;
        }

        private void btnCalibration_Click(object sender, EventArgs e) {
            if (btnCalibration.Text == "CAL") {
                mainWindow.commModule.sendData("{\"CAL\":0}");
                mainWindow.log("Sending: " + "{\"CAL\":0}");
                btnCalibration.Text = "STOP CAL";
            } else {
                //STOP CAL
                mainWindow.commModule.sendData("!");
                mainWindow.log("Sending: " + "!");
                btnCalibration.Text = "CAL";
            }


        }

        private void btnVersion_Click(object sender, EventArgs e) {
            mainWindow.commModule.sendData("{\"VERSION\":7}");
            mainWindow.log("Sending: " + "{\"VERSION\":7}");
        }




        private void cmbCommands_SelectedIndexChanged(object sender, EventArgs e) {
            Command c = (Command)cmbCommands.SelectedItem;
            txtParameter.Text = c.defaultValue;
        }

        private void btnSend_Click(object sender, EventArgs e) {
            string param = txtParameter.Text;
            if (param != null && param != "") {
                mainWindow.commModule.sendData("{\"" + cmbCommands.SelectedItem.ToString() + "\":" + param + "}");
                mainWindow.log("Sending: " + "{\"" + cmbCommands.SelectedItem.ToString() + "\":" + param + "}");
            } else {
                MessageBox.Show("Cannot send empty value " + param, "Empty parameter", MessageBoxButtons.OK,MessageBoxIcon.Stop);
            }
        }

        private void btnSend2_Click(object sender, EventArgs e) {
            mainWindow.commModule.sendData(txtGenericCommand.Text);
            mainWindow.log("Sending: " + txtGenericCommand.Text);
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
