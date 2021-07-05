namespace freETarget {
    partial class frmArduino {
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmArduino));
            this.btnClose = new System.Windows.Forms.Button();
            this.txtOutput = new System.Windows.Forms.TextBox();
            this.btnEcho = new System.Windows.Forms.Button();
            this.ckbAutoscroll = new System.Windows.Forms.CheckBox();
            this.btnVersion = new System.Windows.Forms.Button();
            this.btnCalibration = new System.Windows.Forms.Button();
            this.btnInit = new System.Windows.Forms.Button();
            this.cmbCommands = new System.Windows.Forms.ComboBox();
            this.txtParameter = new System.Windows.Forms.TextBox();
            this.btnSend = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.SuspendLayout();
            // 
            // btnClose
            // 
            this.btnClose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnClose.Location = new System.Drawing.Point(422, 9);
            this.btnClose.Name = "btnClose";
            this.btnClose.Size = new System.Drawing.Size(75, 23);
            this.btnClose.TabIndex = 0;
            this.btnClose.Text = "Close";
            this.btnClose.UseVisualStyleBackColor = true;
            this.btnClose.Click += new System.EventHandler(this.btnClose_Click);
            // 
            // txtOutput
            // 
            this.txtOutput.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtOutput.BackColor = System.Drawing.SystemColors.Window;
            this.txtOutput.Location = new System.Drawing.Point(9, 125);
            this.txtOutput.Multiline = true;
            this.txtOutput.Name = "txtOutput";
            this.txtOutput.ReadOnly = true;
            this.txtOutput.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.txtOutput.Size = new System.Drawing.Size(488, 276);
            this.txtOutput.TabIndex = 2;
            // 
            // btnEcho
            // 
            this.btnEcho.Location = new System.Drawing.Point(6, 22);
            this.btnEcho.Name = "btnEcho";
            this.btnEcho.Size = new System.Drawing.Size(82, 23);
            this.btnEcho.TabIndex = 1;
            this.btnEcho.Text = "ECHO";
            this.btnEcho.UseVisualStyleBackColor = true;
            this.btnEcho.Click += new System.EventHandler(this.btnEcho_Click);
            // 
            // ckbAutoscroll
            // 
            this.ckbAutoscroll.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.ckbAutoscroll.AutoSize = true;
            this.ckbAutoscroll.Checked = true;
            this.ckbAutoscroll.CheckState = System.Windows.Forms.CheckState.Checked;
            this.ckbAutoscroll.Location = new System.Drawing.Point(420, 102);
            this.ckbAutoscroll.Name = "ckbAutoscroll";
            this.ckbAutoscroll.Size = new System.Drawing.Size(72, 17);
            this.ckbAutoscroll.TabIndex = 11;
            this.ckbAutoscroll.Text = "Autoscroll";
            this.ckbAutoscroll.UseVisualStyleBackColor = true;
            // 
            // btnVersion
            // 
            this.btnVersion.Location = new System.Drawing.Point(6, 51);
            this.btnVersion.Name = "btnVersion";
            this.btnVersion.Size = new System.Drawing.Size(82, 23);
            this.btnVersion.TabIndex = 14;
            this.btnVersion.Text = "VERSION";
            this.btnVersion.UseVisualStyleBackColor = true;
            this.btnVersion.Click += new System.EventHandler(this.btnVersion_Click);
            // 
            // btnCalibration
            // 
            this.btnCalibration.Location = new System.Drawing.Point(94, 22);
            this.btnCalibration.Name = "btnCalibration";
            this.btnCalibration.Size = new System.Drawing.Size(82, 23);
            this.btnCalibration.TabIndex = 15;
            this.btnCalibration.Text = "CAL";
            this.btnCalibration.UseVisualStyleBackColor = true;
            this.btnCalibration.Click += new System.EventHandler(this.btnCalibration_Click);
            // 
            // btnInit
            // 
            this.btnInit.Location = new System.Drawing.Point(94, 51);
            this.btnInit.Name = "btnInit";
            this.btnInit.Size = new System.Drawing.Size(82, 23);
            this.btnInit.TabIndex = 20;
            this.btnInit.Text = "INIT";
            this.btnInit.UseVisualStyleBackColor = true;
            this.btnInit.Click += new System.EventHandler(this.btnInit_Click);
            // 
            // cmbCommands
            // 
            this.cmbCommands.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbCommands.FormattingEnabled = true;
            this.cmbCommands.Location = new System.Drawing.Point(66, 19);
            this.cmbCommands.Name = "cmbCommands";
            this.cmbCommands.Size = new System.Drawing.Size(121, 21);
            this.cmbCommands.TabIndex = 31;
            this.cmbCommands.SelectedIndexChanged += new System.EventHandler(this.cmbCommands_SelectedIndexChanged);
            // 
            // txtParameter
            // 
            this.txtParameter.Location = new System.Drawing.Point(67, 48);
            this.txtParameter.Name = "txtParameter";
            this.txtParameter.Size = new System.Drawing.Size(120, 20);
            this.txtParameter.TabIndex = 32;
            // 
            // btnSend
            // 
            this.btnSend.Location = new System.Drawing.Point(67, 75);
            this.btnSend.Name = "btnSend";
            this.btnSend.Size = new System.Drawing.Size(120, 23);
            this.btnSend.TabIndex = 33;
            this.btnSend.Text = "Send";
            this.btnSend.UseVisualStyleBackColor = true;
            this.btnSend.Click += new System.EventHandler(this.btnSend_Click);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(6, 22);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(54, 13);
            this.label1.TabIndex = 34;
            this.label1.Text = "Command";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(6, 51);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(55, 13);
            this.label2.TabIndex = 35;
            this.label2.Text = "Parameter";
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.cmbCommands);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Controls.Add(this.txtParameter);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Controls.Add(this.btnSend);
            this.groupBox1.Location = new System.Drawing.Point(206, 12);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(202, 107);
            this.groupBox1.TabIndex = 36;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Parametrized commands";
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.btnEcho);
            this.groupBox2.Controls.Add(this.btnVersion);
            this.groupBox2.Controls.Add(this.btnCalibration);
            this.groupBox2.Controls.Add(this.btnInit);
            this.groupBox2.Location = new System.Drawing.Point(12, 12);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(188, 107);
            this.groupBox2.TabIndex = 37;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Commands";
            // 
            // frmArduino
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(504, 412);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.ckbAutoscroll);
            this.Controls.Add(this.btnClose);
            this.Controls.Add(this.txtOutput);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MinimumSize = new System.Drawing.Size(520, 450);
            this.Name = "frmArduino";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Arduino";
            this.Activated += new System.EventHandler(this.frmArduino_Activated);
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.frmArduino_FormClosing);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.Button btnClose;
        private System.Windows.Forms.TextBox txtOutput;
        private System.Windows.Forms.Button btnEcho;
        private System.Windows.Forms.CheckBox ckbAutoscroll;
        private System.Windows.Forms.Button btnVersion;
        private System.Windows.Forms.Button btnCalibration;
        private System.Windows.Forms.Button btnInit;
        private System.Windows.Forms.ComboBox cmbCommands;
        private System.Windows.Forms.TextBox txtParameter;
        private System.Windows.Forms.Button btnSend;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.GroupBox groupBox2;
    }
}