namespace freETarget
{
    partial class frmSettings
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmSettings));
            this.btnOK = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.txtBaud = new System.Windows.Forms.TextBox();
            this.txtName = new System.Windows.Forms.TextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.cmbPorts = new System.Windows.Forms.ComboBox();
            this.chkDisplayConsole = new System.Windows.Forms.CheckBox();
            this.cmbWeapons = new System.Windows.Forms.ComboBox();
            this.label4 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.cmbColor = new System.Windows.Forms.ComboBox();
            this.chkDrawMeanG = new System.Windows.Forms.CheckBox();
            this.label6 = new System.Windows.Forms.Label();
            this.rdb60 = new System.Windows.Forms.RadioButton();
            this.rdb40 = new System.Windows.Forms.RadioButton();
            this.chkSeries = new System.Windows.Forms.CheckBox();
            this.chkVoice = new System.Windows.Forms.CheckBox();
            this.label7 = new System.Windows.Forms.Label();
            this.txtPDFlocation = new System.Windows.Forms.TextBox();
            this.btnBrowse = new System.Windows.Forms.Button();
            this.folderBrowserDialog = new System.Windows.Forms.FolderBrowserDialog();
            this.label8 = new System.Windows.Forms.Label();
            this.txtDistance = new System.Windows.Forms.TextBox();
            this.label9 = new System.Windows.Forms.Label();
            this.label10 = new System.Windows.Forms.Label();
            this.linkLabel = new System.Windows.Forms.LinkLabel();
            this.SuspendLayout();
            // 
            // btnOK
            // 
            this.btnOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnOK.Location = new System.Drawing.Point(242, 416);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(75, 23);
            this.btnOK.TabIndex = 0;
            this.btnOK.Text = "OK";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
            // 
            // btnCancel
            // 
            this.btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(161, 416);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 1;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 12);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(35, 13);
            this.label1.TabIndex = 2;
            this.label1.Text = "Name";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(12, 37);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(26, 13);
            this.label2.TabIndex = 4;
            this.label2.Text = "Port";
            // 
            // txtBaud
            // 
            this.txtBaud.Location = new System.Drawing.Point(100, 59);
            this.txtBaud.Name = "txtBaud";
            this.txtBaud.Size = new System.Drawing.Size(190, 20);
            this.txtBaud.TabIndex = 6;
            // 
            // txtName
            // 
            this.txtName.Location = new System.Drawing.Point(100, 8);
            this.txtName.Name = "txtName";
            this.txtName.Size = new System.Drawing.Size(190, 20);
            this.txtName.TabIndex = 7;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(12, 63);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(58, 13);
            this.label3.TabIndex = 9;
            this.label3.Text = "Baud Rate";
            // 
            // cmbPorts
            // 
            this.cmbPorts.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbPorts.FormattingEnabled = true;
            this.cmbPorts.Location = new System.Drawing.Point(100, 33);
            this.cmbPorts.Name = "cmbPorts";
            this.cmbPorts.Size = new System.Drawing.Size(190, 21);
            this.cmbPorts.TabIndex = 11;
            // 
            // chkDisplayConsole
            // 
            this.chkDisplayConsole.AutoSize = true;
            this.chkDisplayConsole.Location = new System.Drawing.Point(15, 260);
            this.chkDisplayConsole.Name = "chkDisplayConsole";
            this.chkDisplayConsole.Size = new System.Drawing.Size(136, 17);
            this.chkDisplayConsole.TabIndex = 12;
            this.chkDisplayConsole.Text = "Display Debug Console";
            this.chkDisplayConsole.UseVisualStyleBackColor = true;
            // 
            // cmbWeapons
            // 
            this.cmbWeapons.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbWeapons.FormattingEnabled = true;
            this.cmbWeapons.Location = new System.Drawing.Point(100, 84);
            this.cmbWeapons.Name = "cmbWeapons";
            this.cmbWeapons.Size = new System.Drawing.Size(190, 21);
            this.cmbWeapons.TabIndex = 13;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(12, 88);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(75, 13);
            this.label4.TabIndex = 14;
            this.label4.Text = "Default Target";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(12, 114);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(65, 13);
            this.label5.TabIndex = 15;
            this.label5.Text = "Target Color";
            // 
            // cmbColor
            // 
            this.cmbColor.DrawMode = System.Windows.Forms.DrawMode.OwnerDrawFixed;
            this.cmbColor.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbColor.FormattingEnabled = true;
            this.cmbColor.Location = new System.Drawing.Point(100, 110);
            this.cmbColor.Name = "cmbColor";
            this.cmbColor.Size = new System.Drawing.Size(190, 21);
            this.cmbColor.TabIndex = 16;
            this.cmbColor.DrawItem += new System.Windows.Forms.DrawItemEventHandler(this.cmbColor_DrawItem);
            // 
            // chkDrawMeanG
            // 
            this.chkDrawMeanG.AutoSize = true;
            this.chkDrawMeanG.Location = new System.Drawing.Point(15, 214);
            this.chkDrawMeanG.Name = "chkDrawMeanG";
            this.chkDrawMeanG.Size = new System.Drawing.Size(113, 17);
            this.chkDrawMeanG.TabIndex = 17;
            this.chkDrawMeanG.Text = "Draw Mean Group";
            this.chkDrawMeanG.UseVisualStyleBackColor = true;
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(12, 189);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(115, 13);
            this.label6.TabIndex = 18;
            this.label6.Text = "Match number of shots";
            // 
            // rdb60
            // 
            this.rdb60.AutoSize = true;
            this.rdb60.Location = new System.Drawing.Point(228, 189);
            this.rdb60.Name = "rdb60";
            this.rdb60.Size = new System.Drawing.Size(65, 17);
            this.rdb60.TabIndex = 19;
            this.rdb60.TabStop = true;
            this.rdb60.Text = "60 shots";
            this.rdb60.UseVisualStyleBackColor = true;
            // 
            // rdb40
            // 
            this.rdb40.AutoSize = true;
            this.rdb40.Location = new System.Drawing.Point(147, 188);
            this.rdb40.Name = "rdb40";
            this.rdb40.Size = new System.Drawing.Size(65, 17);
            this.rdb40.TabIndex = 20;
            this.rdb40.TabStop = true;
            this.rdb40.Text = "40 shots";
            this.rdb40.UseVisualStyleBackColor = true;
            // 
            // chkSeries
            // 
            this.chkSeries.AutoSize = true;
            this.chkSeries.Location = new System.Drawing.Point(15, 237);
            this.chkSeries.Name = "chkSeries";
            this.chkSeries.Size = new System.Drawing.Size(178, 17);
            this.chkSeries.TabIndex = 21;
            this.chkSeries.Text = "Display only current series target";
            this.chkSeries.UseVisualStyleBackColor = true;
            // 
            // chkVoice
            // 
            this.chkVoice.AutoSize = true;
            this.chkVoice.Location = new System.Drawing.Point(15, 283);
            this.chkVoice.Name = "chkVoice";
            this.chkVoice.Size = new System.Drawing.Size(145, 17);
            this.chkVoice.TabIndex = 22;
            this.chkVoice.Text = "Voice commands in finals";
            this.chkVoice.UseVisualStyleBackColor = true;
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(12, 140);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(68, 13);
            this.label7.TabIndex = 23;
            this.label7.Text = "PDF location";
            // 
            // txtPDFlocation
            // 
            this.txtPDFlocation.Location = new System.Drawing.Point(100, 136);
            this.txtPDFlocation.Name = "txtPDFlocation";
            this.txtPDFlocation.Size = new System.Drawing.Size(190, 20);
            this.txtPDFlocation.TabIndex = 24;
            // 
            // btnBrowse
            // 
            this.btnBrowse.Location = new System.Drawing.Point(291, 135);
            this.btnBrowse.Name = "btnBrowse";
            this.btnBrowse.Size = new System.Drawing.Size(25, 20);
            this.btnBrowse.TabIndex = 25;
            this.btnBrowse.Text = "...";
            this.btnBrowse.UseVisualStyleBackColor = true;
            this.btnBrowse.Click += new System.EventHandler(this.btnBrowse_Click);
            // 
            // folderBrowserDialog
            // 
            this.folderBrowserDialog.RootFolder = System.Environment.SpecialFolder.MyComputer;
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(12, 165);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(131, 13);
            this.label8.TabIndex = 26;
            this.label8.Text = "Distance to target (meters)";
            // 
            // txtDistance
            // 
            this.txtDistance.Location = new System.Drawing.Point(147, 162);
            this.txtDistance.Name = "txtDistance";
            this.txtDistance.Size = new System.Drawing.Size(143, 20);
            this.txtDistance.TabIndex = 27;
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.label9.Location = new System.Drawing.Point(12, 331);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(215, 13);
            this.label9.TabIndex = 28;
            this.label9.Text = "freETarget Project  -  v1.0   (c) 2020";
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.Location = new System.Drawing.Point(12, 354);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(193, 13);
            this.label10.TabIndex = 29;
            this.label10.Text = "Open Source Electronic Scoring Target";
            // 
            // linkLabel
            // 
            this.linkLabel.AutoSize = true;
            this.linkLabel.Location = new System.Drawing.Point(12, 377);
            this.linkLabel.Name = "linkLabel";
            this.linkLabel.Size = new System.Drawing.Size(219, 13);
            this.linkLabel.TabIndex = 30;
            this.linkLabel.TabStop = true;
            this.linkLabel.Text = "https://github.com/ten-point-nine/freETarget";
            this.linkLabel.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabel1_LinkClicked);
            // 
            // frmSettings
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(329, 451);
            this.Controls.Add(this.linkLabel);
            this.Controls.Add(this.label10);
            this.Controls.Add(this.label9);
            this.Controls.Add(this.txtDistance);
            this.Controls.Add(this.label8);
            this.Controls.Add(this.btnBrowse);
            this.Controls.Add(this.txtPDFlocation);
            this.Controls.Add(this.label7);
            this.Controls.Add(this.chkVoice);
            this.Controls.Add(this.chkSeries);
            this.Controls.Add(this.rdb40);
            this.Controls.Add(this.rdb60);
            this.Controls.Add(this.label6);
            this.Controls.Add(this.chkDrawMeanG);
            this.Controls.Add(this.cmbColor);
            this.Controls.Add(this.label5);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.cmbWeapons);
            this.Controls.Add(this.chkDisplayConsole);
            this.Controls.Add(this.cmbPorts);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.txtName);
            this.Controls.Add(this.txtBaud);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOK);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "frmSettings";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Settings";
            this.Load += new System.EventHandler(this.frmSettings_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btnOK;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        public System.Windows.Forms.TextBox txtBaud;
        public System.Windows.Forms.TextBox txtName;
        private System.Windows.Forms.Label label3;
        public System.Windows.Forms.ComboBox cmbPorts;
        public System.Windows.Forms.CheckBox chkDisplayConsole;
        public System.Windows.Forms.ComboBox cmbWeapons;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label5;
        public System.Windows.Forms.ComboBox cmbColor;
        public System.Windows.Forms.CheckBox chkDrawMeanG;
        private System.Windows.Forms.Label label6;
        public System.Windows.Forms.RadioButton rdb60;
        public System.Windows.Forms.RadioButton rdb40;
        public System.Windows.Forms.CheckBox chkSeries;
        public System.Windows.Forms.CheckBox chkVoice;
        private System.Windows.Forms.Label label7;
        public System.Windows.Forms.TextBox txtPDFlocation;
        private System.Windows.Forms.Button btnBrowse;
        private System.Windows.Forms.FolderBrowserDialog folderBrowserDialog;
        private System.Windows.Forms.Label label8;
        public System.Windows.Forms.TextBox txtDistance;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.Label label10;
        private System.Windows.Forms.LinkLabel linkLabel;
    }
}