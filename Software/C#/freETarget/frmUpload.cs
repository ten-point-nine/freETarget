using freETarget.comms;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Xml.Linq;

namespace freETarget {
    public partial class frmUpload : Form {

        enum directUploadStatus { NOT_CONNECTED, CONNECTED, UPLOAD_REQUESTED, UPLOAD_CONFIRMED, UPLOAD_STARTED, UPLOAD_FINISHED, UPLOAD_DONE, UPLOAD_FAILED };

        private delegate void SafeCallDelegate(string text);

        string filePath = string.Empty;
        frmMainWindow mainWindow;
        string board  = string.Empty;
        private string incomingJSON = "";               // Cumulative serial message
        directUploadStatus status = directUploadStatus.NOT_CONNECTED;
        int blockSize = 0;
        int fileSize = 0;
        int nrOfBlocks = 0;
        int currentBlock = 0;
        byte[] data = null;
        DateTime startTimestamp = DateTime.Now;

        //FileStream debugFile;

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
                txtUploadConsole.Top = txtUploaderLocation.Top + 26;
            } else {
                txtUploaderLocation.Text = "";
                lblEsptoolExplain.Visible = true;
                txtUploadConsole.Top = txtUploaderLocation.Top + 39;
            }

            if(board == "Arduino") {
                tabControl.TabPages.Remove(tabDirect);
            }

           // debugFile = File.Create("C:\\Users\\Radu\\Desktop\\" + "debugFile.bin");
        }

        private void btnClose_Click(object sender, EventArgs e) {
            if (status != directUploadStatus.NOT_CONNECTED 
                && status != directUploadStatus.UPLOAD_DONE 
                && status != directUploadStatus.UPLOAD_FAILED) {
                MessageBox.Show("Upload in progress. Cannot close the window now.","Wait",MessageBoxButtons.OK,MessageBoxIcon.Hand);
                return;
            }
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
                    btnDirectUpload.Enabled = true;
                } else {
                    btnUpload.Enabled = false;
                    btnDirectUpload.Enabled = false;
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

        private void btnDirectUpload_Click(object sender, EventArgs e) {
            FileInfo fileInfo = new FileInfo(filePath);

            try {
                if (fileInfo.Exists) {
                    if (status.Equals(directUploadStatus.NOT_CONNECTED)) {
                        btnDirectUpload.Enabled = false;
                        btnSelectFile.Enabled = false;
                        timer.Interval = int.Parse(txtTimerDelay.Text);
                        timer.Enabled = true;

                        serialPort.PortName = Properties.Settings.Default.portName;
                        serialPort.BaudRate = Properties.Settings.Default.baudRate;
                        serialPort.DataBits = 8;
                        serialPort.DtrEnable = true;
                        serialPort.RtsEnable = true;


                        serialPort.Open();
                        status = directUploadStatus.CONNECTED;
                        updateConsole("Connected to target. Starting upload in " + (int.Parse(txtDelay.Text)) + " seconds. Please wait...");
                        int i = 0;
                        while (i < int.Parse(txtDelay.Text)*10) { //this will result in slighlty more wait time
                            Thread.Sleep(100); 
                            Application.DoEvents();
                            i++;
                        }
                        //Thread.Sleep(int.Parse(txtDelay.Text) * 1000);
                        data = File.ReadAllBytes(filePath);
                        fileSize = data.Length;

                        mainWindow.log("Direct uploading file " + filePath + " to board " + board + " on port: "+serialPort.PortName + "@" + serialPort.BaudRate);

                        serialPort.Write("{\"DOWNLOAD\":" + fileSize + "}");
                        Console.WriteLine("{\"DOWNLOAD\":" + fileSize + "}");
                        updateConsole("Firmware file size is " + fileSize + " bytes");
                        status = directUploadStatus.UPLOAD_REQUESTED;
                        startTimestamp = DateTime.Now;
                    } else {
                        updateConsole("Invalid status " + status);
                    }
                } else {
                    updateConsole("Selected file does not exists");
                }
            } catch (Exception ex) {
                updateConsole("Error uploading " + ex.Message);
                Console.WriteLine( ex.Message );
                btnDirectUpload.Enabled = true;
                btnSelectFile.Enabled = true;
                timer.Enabled = false;
                status = directUploadStatus.UPLOAD_FAILED;
            }

        }

        private void serialPort_DataReceived(object sender, System.IO.Ports.SerialDataReceivedEventArgs e) {
            SerialPort sp = (SerialPort)sender;
            string indata = sp.ReadExisting();
            Console.WriteLine(indata);

            incomingJSON += indata;                         // Accumulate the input
            int indexOpenBracket = incomingJSON.IndexOf('{');
            if (indexOpenBracket > -1) {
                int indexClosedBracket = incomingJSON.IndexOf('}', indexOpenBracket);

                if (indexClosedBracket > -1) {

                    String message = incomingJSON.Substring(indexOpenBracket + 1, indexClosedBracket - indexOpenBracket - 1);
                    incomingJSON = incomingJSON.Substring(indexClosedBracket + 1);

                    string[] t1 = message.Split(':');
                    if (t1[0].Contains("DOWNLOAD_BLOCK")) {
                        try {
                            blockSize = int.Parse(t1[1], CultureInfo.InvariantCulture);
                        } catch (Exception) {
                            inkoveString("Could not parse block size: " + t1[1]);
                            status = directUploadStatus.UPLOAD_FAILED;
                            btnDirectUpload.BeginInvoke((Action)delegate () {
                                btnDirectUpload.Enabled = true;
                            });
                            btnSelectFile.BeginInvoke((Action)delegate () {
                                btnSelectFile.Enabled = true;
                            });
                            mainWindow.displayMessage("Upload firmware to " + board + " failed because of parsing error of " + message, false);
                            timer.Enabled = false;
                            return;
                        }
                        inkoveString("Uploading blocks of " + blockSize + " bytes...");
                        nrOfBlocks = (fileSize / blockSize) + 1;
                        Console.WriteLine("Number of blocks = " + nrOfBlocks);

                        status = directUploadStatus.UPLOAD_CONFIRMED;

                    } else if (t1[0].Contains("RESPONSE")) {
                        string responseMessage = t1[1];
                        if(responseMessage.Contains("PASS")) {
                            Console.WriteLine("RESPONSE = " + responseMessage);
                            DateTime end = DateTime.Now;
                            TimeSpan dif = end - startTimestamp;
                            inkoveString("Target reports firmware upload was succesful ["+responseMessage+"] in " + dif.TotalSeconds + " seconds.");
                            status = directUploadStatus.UPLOAD_DONE;
                            btnDirectUpload.BeginInvoke((Action)delegate () {
                                btnDirectUpload.Enabled = true;
                            });
                            btnSelectFile.BeginInvoke((Action)delegate () {
                                btnSelectFile.Enabled = true;
                            });
                            timer.Enabled = false;
                            mainWindow.displayMessage("Upload firmware to " + board + " finished.", false);
                        } else if (responseMessage.Contains("FAIL")) {
                            Console.WriteLine("RESPONSE = "+responseMessage);
                            inkoveString("Upload failed with message: " + responseMessage);
                            status = directUploadStatus.UPLOAD_FAILED;
                            btnDirectUpload.BeginInvoke((Action)delegate () {
                                btnDirectUpload.Enabled = true;
                            });
                            btnSelectFile.BeginInvoke((Action)delegate () {
                                btnSelectFile.Enabled = true;
                            });
                            timer.Enabled = false;
                            mainWindow.displayMessage("Upload firmware to " + board + " failed.", false);
                        } else {
                            Console.WriteLine("RESPONSE = " + responseMessage);
                            inkoveString("Info message from target: " + responseMessage + ". Continuing...");
                            mainWindow.log("Info message from target: " + responseMessage);
                        }
                    } else {
                        inkoveString("Received unknown command " + message + ". Ignoring...");
                    }

                    if (incomingJSON.IndexOf("}") != -1) {

                        serialPort_DataReceived(sender, e); //call the event again to parse the remains. maybe there is another full message in there
                    }
                }
            }
        }

        private void inkoveString(string text) {
            var d = new SafeCallDelegate(updateConsole); //confirm connect
            this.Invoke(d, new object[] { text.Trim() });
        }

        private void updateConsole(string text) {
            updateConsole(text, true);
        }
        private void updateConsole(string text, Boolean mainlog) {
            txtUploadConsole2.AppendText(text + Environment.NewLine);
            if (mainlog) {
                mainWindow.log(text);
            }
        }

        private void timer_Tick(object sender, EventArgs e) {
            Application.DoEvents();
            try {
                if (status == directUploadStatus.UPLOAD_CONFIRMED) {
                    if (data == null) {
                        updateConsole("no data in buffer");
                        return;
                    }
                    updateConsole("Uploading block " + (currentBlock + 1) + " of " + nrOfBlocks, false);
                    serialPort.Write(data, 0, blockSize);
                    //debugFile.Write(data,0,blockSize);
                    //Console.WriteLine("filesize: " + fileSize + "block: " + currentBlock +  " offset: " + 0 + " count: " + blockSize );
                    status = directUploadStatus.UPLOAD_STARTED;
                    currentBlock = 1;

                } else if (status == directUploadStatus.UPLOAD_STARTED) {
                    updateConsole("Uploading block " + (currentBlock + 1) + " of " + nrOfBlocks, false);
                    if (currentBlock < nrOfBlocks - 1) {
                        serialPort.Write(data, currentBlock * blockSize, blockSize);
                        //debugFile.Write(data, currentBlock * blockSize, blockSize);
                        //Console.WriteLine("filesize: " + fileSize + "block: " + currentBlock + " offset: " + currentBlock * blockSize + " count: " + blockSize);

                        currentBlock++;
                    } else {
                        serialPort.Write(data, currentBlock * blockSize, fileSize - (currentBlock * blockSize));
                        //debugFile.Write(data, currentBlock * blockSize, fileSize - (currentBlock * blockSize));
                        //Console.WriteLine("filesize: " + fileSize + "block: " + currentBlock + " offset: " + currentBlock * blockSize + " count: " + (fileSize - (currentBlock * blockSize)));

                        status = directUploadStatus.UPLOAD_FINISHED;
                        updateConsole("Uploading finished.");
                    }
                } else {
                    //Console.WriteLine("status = " + status);
                }
            } catch (TimeoutException ex){
                status = directUploadStatus.UPLOAD_FAILED;
                updateConsole("Timeout while writing to port " + ex.Message);
                timer.Enabled = false;
                btnDirectUpload.BeginInvoke((Action)delegate () {
                    btnDirectUpload.Enabled = true;
                });
                btnSelectFile.BeginInvoke((Action)delegate () {
                    btnSelectFile.Enabled = true;
                });
                mainWindow.log("Upload firmware to " + board + " timedout." + Environment.NewLine + ex.Message);
            } catch (Exception ex) {
                status = directUploadStatus.UPLOAD_FAILED;
                updateConsole("Upload failed with exception " + ex.Message);
                timer.Enabled = false;
                btnDirectUpload.BeginInvoke((Action)delegate () {
                    btnDirectUpload.Enabled = true;
                });
                btnSelectFile.BeginInvoke((Action)delegate () {
                    btnSelectFile.Enabled = true;
                });
                mainWindow.log("Upload firmware to " + board + " failed." + Environment.NewLine + ex.Message);
            }
        }

        private void txtDelay_TextChanged(object sender, EventArgs e) {
            if (!positiveNumber(txtDelay.Text)) {
                txtDelay.Text = "16";
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

        private void txtTimerDelay_TextChanged(object sender, EventArgs e) {
            if (!positiveNumber(txtDelay.Text)) {
                txtDelay.Text = "200";
            }
        }
    }
}
