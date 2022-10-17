using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Diagnostics;

namespace freETarget {
    public partial class frmUpload : Form {
        string filePath = string.Empty;
        frmMainWindow mainWindow;

        public frmUpload(frmMainWindow mainWin) {
            InitializeComponent();
            this.mainWindow = mainWin;
            string com = Properties.Settings.Default.portName;
            string baud = Properties.Settings.Default.baudRate.ToString();
            lblPort.Text = "Port: " + com + " @ " + baud;
        }

        private void btnClose_Click(object sender, EventArgs e) {
            this.Close();
        }

        private void btnSelectFile_Click(object sender, EventArgs e) {
            

            using (OpenFileDialog openFileDialog = new OpenFileDialog()) {
                openFileDialog.InitialDirectory = ".";
                openFileDialog.Filter = "hex files (*.hex)|*.hex|All files (*.*)|*.*";
                openFileDialog.FilterIndex = 1;
                openFileDialog.RestoreDirectory = true;

                if (openFileDialog.ShowDialog() == DialogResult.OK) {
                    filePath = openFileDialog.FileName;
                    lblHexFile.Text = filePath;
                    btnUpload.Enabled = true;
                } else {
                    btnUpload.Enabled = false;
                }
            }

            
        }

        private void btnUpload_Click(object sender, EventArgs e) {
            string avrPath = @".\avrdude\";
            string com = Properties.Settings.Default.portName;
            string baud = Properties.Settings.Default.baudRate.ToString();

            using (System.Diagnostics.Process pProcess = new System.Diagnostics.Process()) {
                pProcess.StartInfo.FileName = avrPath + "avrdude.exe";
                pProcess.StartInfo.Arguments = "-C" + avrPath + "avrdude.conf" + " "
                                                + "-v -patmega2560 -cwiring -P"+com+" -b"+baud+" -D" + " "
                                                + "-Uflash:w:" + filePath + ":i";
                //pProcess.StartInfo.Arguments = "-?";
                pProcess.StartInfo.UseShellExecute = false;
                pProcess.StartInfo.RedirectStandardOutput = true;
                pProcess.StartInfo.RedirectStandardError = true;
                pProcess.StartInfo.WindowStyle = System.Diagnostics.ProcessWindowStyle.Hidden;
                pProcess.StartInfo.CreateNoWindow = true; //not diplay a windows

                pProcess.OutputDataReceived += new DataReceivedEventHandler(sortOutputHandler);
                pProcess.ErrorDataReceived += new DataReceivedEventHandler(sortOutputHandler);

                pProcess.Start();

                pProcess.BeginOutputReadLine();
                pProcess.BeginErrorReadLine();

                while (!pProcess.HasExited) {
                    Application.DoEvents(); // This keeps your form responsive by processing events
                }
            }

            Console.WriteLine("AVRDUDE finished");
            mainWindow.displayMessage("Upload firmware finished.", false);
            mainWindow.log("Firmware upload of file " + filePath + " completed.");

        }

        private void sortOutputHandler(object sendingProcess, DataReceivedEventArgs e) {
            Console.WriteLine(e.Data);
            this.BeginInvoke(new MethodInvoker(() => {
                txtUploadConsole.AppendText(e.Data + Environment.NewLine ?? string.Empty);
                mainWindow.log(e.Data);
            }));


        }
    }
}
