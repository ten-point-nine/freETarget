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
            this.SuspendLayout();
            // 
            // btnOK
            // 
            this.btnOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnOK.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnOK.Location = new System.Drawing.Point(248, 267);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(75, 23);
            this.btnOK.TabIndex = 0;
            this.btnOK.Text = "OK";
            this.btnOK.UseVisualStyleBackColor = true;
            // 
            // btnCancel
            // 
            this.btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(167, 267);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 1;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 13);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(35, 13);
            this.label1.TabIndex = 2;
            this.label1.Text = "Name";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(12, 38);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(26, 13);
            this.label2.TabIndex = 4;
            this.label2.Text = "Port";
            // 
            // txtBaud
            // 
            this.txtBaud.Location = new System.Drawing.Point(100, 60);
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
            this.cmbPorts.Location = new System.Drawing.Point(100, 34);
            this.cmbPorts.Name = "cmbPorts";
            this.cmbPorts.Size = new System.Drawing.Size(190, 21);
            this.cmbPorts.TabIndex = 11;
            // 
            // chkDisplayConsole
            // 
            this.chkDisplayConsole.AutoSize = true;
            this.chkDisplayConsole.Location = new System.Drawing.Point(12, 211);
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
            this.cmbWeapons.Location = new System.Drawing.Point(100, 85);
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
            this.cmbColor.Location = new System.Drawing.Point(100, 111);
            this.cmbColor.Name = "cmbColor";
            this.cmbColor.Size = new System.Drawing.Size(190, 21);
            this.cmbColor.TabIndex = 16;
            this.cmbColor.DrawItem += new System.Windows.Forms.DrawItemEventHandler(this.cmbColor_DrawItem);
            // 
            // chkDrawMeanG
            // 
            this.chkDrawMeanG.AutoSize = true;
            this.chkDrawMeanG.Location = new System.Drawing.Point(12, 165);
            this.chkDrawMeanG.Name = "chkDrawMeanG";
            this.chkDrawMeanG.Size = new System.Drawing.Size(113, 17);
            this.chkDrawMeanG.TabIndex = 17;
            this.chkDrawMeanG.Text = "Draw Mean Group";
            this.chkDrawMeanG.UseVisualStyleBackColor = true;
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(12, 140);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(115, 13);
            this.label6.TabIndex = 18;
            this.label6.Text = "Match number of shots";
            // 
            // rdb60
            // 
            this.rdb60.AutoSize = true;
            this.rdb60.Location = new System.Drawing.Point(225, 140);
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
            this.rdb40.Location = new System.Drawing.Point(144, 139);
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
            this.chkSeries.Location = new System.Drawing.Point(12, 188);
            this.chkSeries.Name = "chkSeries";
            this.chkSeries.Size = new System.Drawing.Size(178, 17);
            this.chkSeries.TabIndex = 21;
            this.chkSeries.Text = "Display only current series target";
            this.chkSeries.UseVisualStyleBackColor = true;
            // 
            // chkVoice
            // 
            this.chkVoice.AutoSize = true;
            this.chkVoice.Location = new System.Drawing.Point(12, 234);
            this.chkVoice.Name = "chkVoice";
            this.chkVoice.Size = new System.Drawing.Size(133, 17);
            this.chkVoice.TabIndex = 22;
            this.chkVoice.Text = "Final Voice Commands";
            this.chkVoice.UseVisualStyleBackColor = true;
            // 
            // frmSettings
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(335, 302);
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
    }
}