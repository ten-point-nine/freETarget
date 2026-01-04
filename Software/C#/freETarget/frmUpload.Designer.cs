
namespace freETarget {
    partial class frmUpload {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing) {
            if (disposing && (components != null)) {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent() {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmUpload));
            this.txtUploadConsole = new System.Windows.Forms.TextBox();
            this.lblHexFile = new System.Windows.Forms.Label();
            this.btnSelectFile = new System.Windows.Forms.Button();
            this.btnUpload = new System.Windows.Forms.Button();
            this.btnClose = new System.Windows.Forms.Button();
            this.lblPort = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.txtUploaderLocation = new System.Windows.Forms.TextBox();
            this.btnBrowse = new System.Windows.Forms.Button();
            this.folderBrowserDialog = new System.Windows.Forms.FolderBrowserDialog();
            this.lblEsptoolExplain = new System.Windows.Forms.Label();
            this.tabControl = new System.Windows.Forms.TabControl();
            this.tabExternal = new System.Windows.Forms.TabPage();
            this.tabDirect = new System.Windows.Forms.TabPage();
            this.txtTimerDelay = new System.Windows.Forms.TextBox();
            this.lblTimerDelay = new System.Windows.Forms.Label();
            this.txtDelay = new System.Windows.Forms.TextBox();
            this.lblDelay = new System.Windows.Forms.Label();
            this.btnDirectUpload = new System.Windows.Forms.Button();
            this.txtUploadConsole2 = new System.Windows.Forms.TextBox();
            this.serialPort = new System.IO.Ports.SerialPort(this.components);
            this.timer = new System.Windows.Forms.Timer(this.components);
            this.tabControl.SuspendLayout();
            this.tabExternal.SuspendLayout();
            this.tabDirect.SuspendLayout();
            this.SuspendLayout();
            // 
            // txtUploadConsole
            // 
            this.txtUploadConsole.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtUploadConsole.BackColor = System.Drawing.SystemColors.Window;
            this.txtUploadConsole.Location = new System.Drawing.Point(12, 49);
            this.txtUploadConsole.Multiline = true;
            this.txtUploadConsole.Name = "txtUploadConsole";
            this.txtUploadConsole.ReadOnly = true;
            this.txtUploadConsole.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.txtUploadConsole.Size = new System.Drawing.Size(920, 283);
            this.txtUploadConsole.TabIndex = 0;
            // 
            // lblHexFile
            // 
            this.lblHexFile.AutoSize = true;
            this.lblHexFile.Location = new System.Drawing.Point(12, 17);
            this.lblHexFile.Name = "lblHexFile";
            this.lblHexFile.Size = new System.Drawing.Size(80, 13);
            this.lblHexFile.TabIndex = 1;
            this.lblHexFile.Text = "No file selected";
            // 
            // btnSelectFile
            // 
            this.btnSelectFile.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnSelectFile.Location = new System.Drawing.Point(864, 12);
            this.btnSelectFile.Name = "btnSelectFile";
            this.btnSelectFile.Size = new System.Drawing.Size(75, 23);
            this.btnSelectFile.TabIndex = 2;
            this.btnSelectFile.Text = "Select file...";
            this.btnSelectFile.UseVisualStyleBackColor = true;
            this.btnSelectFile.Click += new System.EventHandler(this.btnSelectFile_Click);
            // 
            // btnUpload
            // 
            this.btnUpload.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnUpload.Enabled = false;
            this.btnUpload.Location = new System.Drawing.Point(793, 8);
            this.btnUpload.Name = "btnUpload";
            this.btnUpload.Size = new System.Drawing.Size(139, 23);
            this.btnUpload.TabIndex = 3;
            this.btnUpload.Text = "Upload firmware";
            this.btnUpload.UseVisualStyleBackColor = true;
            this.btnUpload.Click += new System.EventHandler(this.btnUpload_Click);
            // 
            // btnClose
            // 
            this.btnClose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnClose.Location = new System.Drawing.Point(864, 41);
            this.btnClose.Name = "btnClose";
            this.btnClose.Size = new System.Drawing.Size(75, 23);
            this.btnClose.TabIndex = 4;
            this.btnClose.Text = "Close";
            this.btnClose.UseVisualStyleBackColor = true;
            this.btnClose.Click += new System.EventHandler(this.btnClose_Click);
            // 
            // lblPort
            // 
            this.lblPort.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.lblPort.Location = new System.Drawing.Point(525, 15);
            this.lblPort.Name = "lblPort";
            this.lblPort.Size = new System.Drawing.Size(333, 16);
            this.lblPort.TabIndex = 5;
            this.lblPort.Text = "Port";
            this.lblPort.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(9, 13);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(90, 13);
            this.label1.TabIndex = 6;
            this.label1.Text = "Uploader location";
            // 
            // txtUploaderLocation
            // 
            this.txtUploaderLocation.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtUploaderLocation.Location = new System.Drawing.Point(112, 10);
            this.txtUploaderLocation.Name = "txtUploaderLocation";
            this.txtUploaderLocation.Size = new System.Drawing.Size(645, 20);
            this.txtUploaderLocation.TabIndex = 7;
            // 
            // btnBrowse
            // 
            this.btnBrowse.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnBrowse.Location = new System.Drawing.Point(763, 8);
            this.btnBrowse.Name = "btnBrowse";
            this.btnBrowse.Size = new System.Drawing.Size(24, 23);
            this.btnBrowse.TabIndex = 8;
            this.btnBrowse.Text = "...";
            this.btnBrowse.UseVisualStyleBackColor = true;
            this.btnBrowse.Click += new System.EventHandler(this.btnBrowse_Click);
            // 
            // folderBrowserDialog
            // 
            this.folderBrowserDialog.RootFolder = System.Environment.SpecialFolder.MyComputer;
            // 
            // lblEsptoolExplain
            // 
            this.lblEsptoolExplain.AutoSize = true;
            this.lblEsptoolExplain.Location = new System.Drawing.Point(109, 33);
            this.lblEsptoolExplain.Name = "lblEsptoolExplain";
            this.lblEsptoolExplain.Size = new System.Drawing.Size(285, 13);
            this.lblEsptoolExplain.TabIndex = 9;
            this.lblEsptoolExplain.Text = "If \'esptool\' is on the system PATH, leave the location empty";
            // 
            // tabControl
            // 
            this.tabControl.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tabControl.Controls.Add(this.tabExternal);
            this.tabControl.Controls.Add(this.tabDirect);
            this.tabControl.Location = new System.Drawing.Point(3, 63);
            this.tabControl.Name = "tabControl";
            this.tabControl.SelectedIndex = 0;
            this.tabControl.Size = new System.Drawing.Size(946, 364);
            this.tabControl.TabIndex = 10;
            // 
            // tabExternal
            // 
            this.tabExternal.Controls.Add(this.btnBrowse);
            this.tabExternal.Controls.Add(this.txtUploaderLocation);
            this.tabExternal.Controls.Add(this.label1);
            this.tabExternal.Controls.Add(this.btnUpload);
            this.tabExternal.Controls.Add(this.txtUploadConsole);
            this.tabExternal.Controls.Add(this.lblEsptoolExplain);
            this.tabExternal.Location = new System.Drawing.Point(4, 22);
            this.tabExternal.Name = "tabExternal";
            this.tabExternal.Padding = new System.Windows.Forms.Padding(3);
            this.tabExternal.Size = new System.Drawing.Size(938, 338);
            this.tabExternal.TabIndex = 0;
            this.tabExternal.Text = "External Tool";
            this.tabExternal.UseVisualStyleBackColor = true;
            // 
            // tabDirect
            // 
            this.tabDirect.Controls.Add(this.txtTimerDelay);
            this.tabDirect.Controls.Add(this.lblTimerDelay);
            this.tabDirect.Controls.Add(this.txtDelay);
            this.tabDirect.Controls.Add(this.lblDelay);
            this.tabDirect.Controls.Add(this.btnDirectUpload);
            this.tabDirect.Controls.Add(this.txtUploadConsole2);
            this.tabDirect.Location = new System.Drawing.Point(4, 22);
            this.tabDirect.Name = "tabDirect";
            this.tabDirect.Padding = new System.Windows.Forms.Padding(3);
            this.tabDirect.Size = new System.Drawing.Size(938, 338);
            this.tabDirect.TabIndex = 1;
            this.tabDirect.Text = "Direct Upload";
            this.tabDirect.UseVisualStyleBackColor = true;
            // 
            // txtTimerDelay
            // 
            this.txtTimerDelay.Location = new System.Drawing.Point(198, 8);
            this.txtTimerDelay.Name = "txtTimerDelay";
            this.txtTimerDelay.Size = new System.Drawing.Size(44, 20);
            this.txtTimerDelay.TabIndex = 6;
            this.txtTimerDelay.Text = "200";
            this.txtTimerDelay.TextChanged += new System.EventHandler(this.txtTimerDelay_TextChanged);
            // 
            // lblTimerDelay
            // 
            this.lblTimerDelay.AutoSize = true;
            this.lblTimerDelay.Location = new System.Drawing.Point(12, 11);
            this.lblTimerDelay.Name = "lblTimerDelay";
            this.lblTimerDelay.Size = new System.Drawing.Size(180, 13);
            this.lblTimerDelay.TabIndex = 5;
            this.lblTimerDelay.Text = "Delay between blocks in miliseconds";
            // 
            // txtDelay
            // 
            this.txtDelay.Location = new System.Drawing.Point(669, 8);
            this.txtDelay.Name = "txtDelay";
            this.txtDelay.Size = new System.Drawing.Size(50, 20);
            this.txtDelay.TabIndex = 4;
            this.txtDelay.Text = "16";
            this.txtDelay.Visible = false;
            this.txtDelay.TextChanged += new System.EventHandler(this.txtDelay_TextChanged);
            // 
            // lblDelay
            // 
            this.lblDelay.AutoSize = true;
            this.lblDelay.Location = new System.Drawing.Point(540, 11);
            this.lblDelay.Name = "lblDelay";
            this.lblDelay.Size = new System.Drawing.Size(123, 13);
            this.lblDelay.TabIndex = 3;
            this.lblDelay.Text = "Upload delay in seconds";
            this.lblDelay.Visible = false;
            // 
            // btnDirectUpload
            // 
            this.btnDirectUpload.Enabled = false;
            this.btnDirectUpload.Location = new System.Drawing.Point(815, 6);
            this.btnDirectUpload.Name = "btnDirectUpload";
            this.btnDirectUpload.Size = new System.Drawing.Size(117, 23);
            this.btnDirectUpload.TabIndex = 2;
            this.btnDirectUpload.Text = "Upload Firmware";
            this.btnDirectUpload.UseVisualStyleBackColor = true;
            this.btnDirectUpload.Click += new System.EventHandler(this.btnDirectUpload_Click);
            // 
            // txtUploadConsole2
            // 
            this.txtUploadConsole2.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtUploadConsole2.BackColor = System.Drawing.SystemColors.Window;
            this.txtUploadConsole2.Location = new System.Drawing.Point(15, 35);
            this.txtUploadConsole2.Multiline = true;
            this.txtUploadConsole2.Name = "txtUploadConsole2";
            this.txtUploadConsole2.ReadOnly = true;
            this.txtUploadConsole2.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.txtUploadConsole2.Size = new System.Drawing.Size(920, 297);
            this.txtUploadConsole2.TabIndex = 1;
            // 
            // serialPort
            // 
            this.serialPort.WriteTimeout = 100;
            this.serialPort.DataReceived += new System.IO.Ports.SerialDataReceivedEventHandler(this.serialPort_DataReceived);
            // 
            // timer
            // 
            this.timer.Interval = 25;
            this.timer.Tick += new System.EventHandler(this.timer_Tick);
            // 
            // frmUpload
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(956, 435);
            this.Controls.Add(this.lblHexFile);
            this.Controls.Add(this.tabControl);
            this.Controls.Add(this.btnSelectFile);
            this.Controls.Add(this.btnClose);
            this.Controls.Add(this.lblPort);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MinimizeBox = false;
            this.MinimumSize = new System.Drawing.Size(700, 350);
            this.Name = "frmUpload";
            this.ShowInTaskbar = false;
            this.Text = "Upload firmware";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.frmUpload_FormClosing);
            this.Load += new System.EventHandler(this.frmUpload_Load);
            this.tabControl.ResumeLayout(false);
            this.tabExternal.ResumeLayout(false);
            this.tabExternal.PerformLayout();
            this.tabDirect.ResumeLayout(false);
            this.tabDirect.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TextBox txtUploadConsole;
        private System.Windows.Forms.Label lblHexFile;
        private System.Windows.Forms.Button btnSelectFile;
        private System.Windows.Forms.Button btnUpload;
        private System.Windows.Forms.Button btnClose;
        private System.Windows.Forms.Label lblPort;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox txtUploaderLocation;
        private System.Windows.Forms.Button btnBrowse;
        private System.Windows.Forms.FolderBrowserDialog folderBrowserDialog;
        private System.Windows.Forms.Label lblEsptoolExplain;
        private System.Windows.Forms.TabControl tabControl;
        private System.Windows.Forms.TabPage tabExternal;
        private System.Windows.Forms.TabPage tabDirect;
        private System.Windows.Forms.Button btnDirectUpload;
        private System.Windows.Forms.TextBox txtUploadConsole2;
        private System.IO.Ports.SerialPort serialPort;
        private System.Windows.Forms.Timer timer;
        private System.Windows.Forms.Label lblDelay;
        private System.Windows.Forms.TextBox txtDelay;
        private System.Windows.Forms.TextBox txtTimerDelay;
        private System.Windows.Forms.Label lblTimerDelay;
    }
}