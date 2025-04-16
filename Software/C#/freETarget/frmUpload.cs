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
using System.IO;

namespace freETarget {
    public partial class frmUpload : Form {
        string filePath = string.Empty;
        frmMainWindow mainWindow;
        string board  = string.Empty;

        public frmUpload(frmMainWindow mainWin) {
            InitializeComponent();
            this.mainWindow = mainWin;
            string com = Properties.Settings.Default.portName;
            string baud = Properties.Settings.Default.baudRate.ToString();
            board = Properties.Settings.Default.Board;
            lblPort.Text = "Board: " + board + "     Port: " + com + " @ " + baud;
            if (Properties.Settings.Default.Board == frmMainWindow.Arduino) {
                txtUploaderLocation.Text = @".\avrdude";
                lblEsptoolExplain.Visible = false;
                txtUploadConsole.Top = 67;
            } else {
                txtUploaderLocation.Text = "";
                lblEsptoolExplain.Visible = true;
                txtUploadConsole.Top = 79;
            }


        }

        private void btnClose_Click(object sender, EventArgs e) {
            this.Close();
        }

        private void btnSelectFile_Click(object sender, EventArgs e) {
            

            using (OpenFileDialog openFileDialog = new OpenFileDialog()) {
                openFileDialog.InitialDirectory = ".";
                if (board == frmMainWindow.Arduino) {
                    openFileDialog.Filter = "hex files (*.hex)|*.hex|All files (*.*)|*.*";
                } else {
                    openFileDialog.Filter = "bin files (*.bin)|*.bin|All files (*.*)|*.*";
                }
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
            string path = txtUploaderLocation.Text;
            if (path != "") { //if path is empty, rely on exe being on the system PATH environment variable
                if (!path.EndsWith("\\")) {
                    path+="\\";
                }
            }

            string com = Properties.Settings.Default.portName;
            string baud = Properties.Settings.Default.baudRate.ToString();

            using (System.Diagnostics.Process pProcess = new System.Diagnostics.Process()) {
                if (Properties.Settings.Default.Board == frmMainWindow.Arduino) {
                    pProcess.StartInfo.FileName = "\"" + path + "avrdude.exe" + "\"";
                    pProcess.StartInfo.Arguments = "-C" + path + "avrdude.conf" + " "
                                                    + "-v -patmega2560 -cwiring -P" + com + " -b" + baud + " -D" + " "
                                                    + "-Uflash:w:" + filePath + ":i";
                } else {
                    //check for 6.1+ aditional bin files
                    string partitionBin = "./binary/partition-table.bin";
                    string otaDataBin = "./binary/ota_data_initial.bin";
                    string bootLoaderBin = "./binary/bootloader.bin";

                    string additionalFilesPath = "";
                    if(File.Exists(partitionBin) && File.Exists(otaDataBin)) {
                        additionalFilesPath = " 0x0 " + bootLoaderBin + " 0x8000 " + partitionBin + " 0xd000 " + otaDataBin;
                        mainWindow.log("found additional binary files");
                    }
                    pProcess.StartInfo.FileName = "\"" + path  + "esptool" + "\"";
                    pProcess.StartInfo.Arguments = "-p "+com+" -b " + baud+ " " +
                        "--before default_reset " +
                        "--after hard_reset " +
                        "--chip esp32s3 write_flash --flash_mode dio --flash_freq 80m --flash_size 8MB " + additionalFilesPath + " 0x10000 " + filePath;
                }
                //pProcess.StartInfo.Arguments = "-?";
                pProcess.StartInfo.UseShellExecute = false;
                pProcess.StartInfo.RedirectStandardOutput = true;
                pProcess.StartInfo.RedirectStandardError = true;
                pProcess.StartInfo.WindowStyle = System.Diagnostics.ProcessWindowStyle.Hidden;
                pProcess.StartInfo.CreateNoWindow = true; //not diplay a windows

                mainWindow.log("Launching: " + pProcess.StartInfo.FileName + " " + pProcess.StartInfo.Arguments);

                pProcess.OutputDataReceived += new DataReceivedEventHandler(sortOutputHandler);
                pProcess.ErrorDataReceived += new DataReceivedEventHandler(sortOutputHandler);

                try {
                    btnClose.Enabled = false;
                    btnBrowse.Enabled = false;
                    btnUpload.Enabled= false;
                    btnSelectFile.Enabled = false;
                    Application.DoEvents();
                    pProcess.Start();
                } catch (Exception ex) {
                    String s = ex.Message;
                    if (Properties.Settings.Default.Board == frmMainWindow.Arduino) {
                        s = "Could not start the external uploader tool - " + path + "avrdude.exe" + Environment.NewLine
                            + "Make sure it is in the avrdude subdirectory." + Environment.NewLine + ex.Message;
                    } else {
                        s = "Could not start the external uploader tool - " + path + "esptool." + Environment.NewLine +
                            "Make sure Python is installed in the system and " + Environment.NewLine +
                            "that the esptool was installed with the 'pip install esptool' command." + Environment.NewLine + ex.Message;
                    }
                    MessageBox.Show(s, "Upload error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }

                pProcess.BeginOutputReadLine();
                pProcess.BeginErrorReadLine();

                while (!pProcess.HasExited) {
                    Application.DoEvents(); // This keeps your form responsive by processing events
                   
                }
                btnClose.Enabled = true;
                btnBrowse.Enabled = true;
                btnUpload.Enabled = true;
                btnSelectFile.Enabled= true;
            }

            Console.WriteLine("Upload finished");
            mainWindow.displayMessage("Upload firmware to " + board + " finished.", false);
            mainWindow.log("Firmware upload to " + board + " of file " + filePath + " completed.");

        }

        private void sortOutputHandler(object sendingProcess, DataReceivedEventArgs e) {
            Console.WriteLine(e.Data);
            this.BeginInvoke(new MethodInvoker(() => {
                txtUploadConsole.AppendText(e.Data + Environment.NewLine ?? string.Empty);
                mainWindow.log(e.Data);
            }));


        }

        private void btnBrowse_Click(object sender, EventArgs e) {
            folderBrowserDialog.SelectedPath = txtUploaderLocation.Text;
            SendKeys.Send("{TAB}{TAB}{RIGHT}");
            if (folderBrowserDialog.ShowDialog() == DialogResult.OK) {
                txtUploaderLocation.Text = folderBrowserDialog.SelectedPath;
            }
        }

        private void frmUpload_Load(object sender, EventArgs e) {
            txtUploaderLocation.Text = Properties.Settings.Default.uploaderLoc;
        }

        private void frmUpload_FormClosing(object sender, FormClosingEventArgs e) {
            Properties.Settings.Default.uploaderLoc = txtUploaderLocation.Text;
            Properties.Settings.Default.Save();
            mainWindow.saveSettings();
        }
    }
}
