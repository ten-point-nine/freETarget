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
            this.btnDip = new System.Windows.Forms.Button();
            this.btnPaper = new System.Windows.Forms.Button();
            this.btnSensor = new System.Windows.Forms.Button();
            this.btnTest = new System.Windows.Forms.Button();
            this.txtDip = new System.Windows.Forms.TextBox();
            this.txtPaper = new System.Windows.Forms.TextBox();
            this.txtSensor = new System.Windows.Forms.TextBox();
            this.txtTest = new System.Windows.Forms.TextBox();
            this.ckbAutoscroll = new System.Windows.Forms.CheckBox();
            this.txtOffset = new System.Windows.Forms.TextBox();
            this.btnOffset = new System.Windows.Forms.Button();
            this.btnVersion = new System.Windows.Forms.Button();
            this.btnCalibration = new System.Windows.Forms.Button();
            this.txtLed = new System.Windows.Forms.TextBox();
            this.btnLed = new System.Windows.Forms.Button();
            this.txtNameID = new System.Windows.Forms.TextBox();
            this.btnNameID = new System.Windows.Forms.Button();
            this.btnInit = new System.Windows.Forms.Button();
            this.txtTrace = new System.Windows.Forms.TextBox();
            this.btnTrace = new System.Windows.Forms.Button();
            this.txtPower = new System.Windows.Forms.TextBox();
            this.btnPower = new System.Windows.Forms.Button();
            this.txtSendMiss = new System.Windows.Forms.TextBox();
            this.btnSendMiss = new System.Windows.Forms.Button();
            this.txtTargetRing = new System.Windows.Forms.TextBox();
            this.btnTargetRing = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // btnClose
            // 
            this.btnClose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnClose.Location = new System.Drawing.Point(782, 9);
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
            this.txtOutput.Size = new System.Drawing.Size(848, 276);
            this.txtOutput.TabIndex = 2;
            // 
            // btnEcho
            // 
            this.btnEcho.Location = new System.Drawing.Point(12, 9);
            this.btnEcho.Name = "btnEcho";
            this.btnEcho.Size = new System.Drawing.Size(82, 23);
            this.btnEcho.TabIndex = 1;
            this.btnEcho.Text = "ECHO";
            this.btnEcho.UseVisualStyleBackColor = true;
            this.btnEcho.Click += new System.EventHandler(this.btnEcho_Click);
            // 
            // btnDip
            // 
            this.btnDip.Location = new System.Drawing.Point(423, 9);
            this.btnDip.Name = "btnDip";
            this.btnDip.Size = new System.Drawing.Size(109, 23);
            this.btnDip.TabIndex = 3;
            this.btnDip.Text = "DIP";
            this.btnDip.UseVisualStyleBackColor = true;
            this.btnDip.Click += new System.EventHandler(this.btnDip_Click);
            // 
            // btnPaper
            // 
            this.btnPaper.Location = new System.Drawing.Point(423, 38);
            this.btnPaper.Name = "btnPaper";
            this.btnPaper.Size = new System.Drawing.Size(109, 23);
            this.btnPaper.TabIndex = 4;
            this.btnPaper.Text = "PAPER";
            this.btnPaper.UseVisualStyleBackColor = true;
            this.btnPaper.Click += new System.EventHandler(this.btnPaper_Click);
            // 
            // btnSensor
            // 
            this.btnSensor.Location = new System.Drawing.Point(204, 9);
            this.btnSensor.Name = "btnSensor";
            this.btnSensor.Size = new System.Drawing.Size(109, 23);
            this.btnSensor.TabIndex = 5;
            this.btnSensor.Text = "SENSOR";
            this.btnSensor.UseVisualStyleBackColor = true;
            this.btnSensor.Click += new System.EventHandler(this.btnSensor_Click);
            // 
            // btnTest
            // 
            this.btnTest.Location = new System.Drawing.Point(204, 38);
            this.btnTest.Name = "btnTest";
            this.btnTest.Size = new System.Drawing.Size(109, 23);
            this.btnTest.TabIndex = 6;
            this.btnTest.Text = "TEST";
            this.btnTest.UseVisualStyleBackColor = true;
            this.btnTest.Click += new System.EventHandler(this.btnTest_Click);
            // 
            // txtDip
            // 
            this.txtDip.Location = new System.Drawing.Point(327, 11);
            this.txtDip.Name = "txtDip";
            this.txtDip.Size = new System.Drawing.Size(90, 20);
            this.txtDip.TabIndex = 7;
            this.txtDip.Text = "0";
            // 
            // txtPaper
            // 
            this.txtPaper.Location = new System.Drawing.Point(327, 40);
            this.txtPaper.Name = "txtPaper";
            this.txtPaper.Size = new System.Drawing.Size(90, 20);
            this.txtPaper.TabIndex = 8;
            this.txtPaper.Text = "0";
            // 
            // txtSensor
            // 
            this.txtSensor.Location = new System.Drawing.Point(108, 11);
            this.txtSensor.Name = "txtSensor";
            this.txtSensor.Size = new System.Drawing.Size(90, 20);
            this.txtSensor.TabIndex = 9;
            this.txtSensor.Text = "230.00";
            // 
            // txtTest
            // 
            this.txtTest.Location = new System.Drawing.Point(108, 40);
            this.txtTest.Name = "txtTest";
            this.txtTest.Size = new System.Drawing.Size(90, 20);
            this.txtTest.TabIndex = 10;
            this.txtTest.Text = "0";
            // 
            // ckbAutoscroll
            // 
            this.ckbAutoscroll.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.ckbAutoscroll.AutoSize = true;
            this.ckbAutoscroll.Checked = true;
            this.ckbAutoscroll.CheckState = System.Windows.Forms.CheckState.Checked;
            this.ckbAutoscroll.Location = new System.Drawing.Point(780, 102);
            this.ckbAutoscroll.Name = "ckbAutoscroll";
            this.ckbAutoscroll.Size = new System.Drawing.Size(72, 17);
            this.ckbAutoscroll.TabIndex = 11;
            this.ckbAutoscroll.Text = "Autoscroll";
            this.ckbAutoscroll.UseVisualStyleBackColor = true;
            // 
            // txtOffset
            // 
            this.txtOffset.Location = new System.Drawing.Point(108, 69);
            this.txtOffset.Name = "txtOffset";
            this.txtOffset.Size = new System.Drawing.Size(90, 20);
            this.txtOffset.TabIndex = 13;
            this.txtOffset.Text = "45";
            // 
            // btnOffset
            // 
            this.btnOffset.Location = new System.Drawing.Point(204, 67);
            this.btnOffset.Name = "btnOffset";
            this.btnOffset.Size = new System.Drawing.Size(109, 23);
            this.btnOffset.TabIndex = 12;
            this.btnOffset.Text = "CALIBREx10";
            this.btnOffset.UseVisualStyleBackColor = true;
            this.btnOffset.Click += new System.EventHandler(this.btnOffset_Click);
            // 
            // btnVersion
            // 
            this.btnVersion.Location = new System.Drawing.Point(12, 38);
            this.btnVersion.Name = "btnVersion";
            this.btnVersion.Size = new System.Drawing.Size(82, 23);
            this.btnVersion.TabIndex = 14;
            this.btnVersion.Text = "VERSION";
            this.btnVersion.UseVisualStyleBackColor = true;
            this.btnVersion.Click += new System.EventHandler(this.btnVersion_Click);
            // 
            // btnCalibration
            // 
            this.btnCalibration.Location = new System.Drawing.Point(12, 67);
            this.btnCalibration.Name = "btnCalibration";
            this.btnCalibration.Size = new System.Drawing.Size(82, 23);
            this.btnCalibration.TabIndex = 15;
            this.btnCalibration.Text = "CAL";
            this.btnCalibration.UseVisualStyleBackColor = true;
            this.btnCalibration.Click += new System.EventHandler(this.btnCalibration_Click);
            // 
            // txtLed
            // 
            this.txtLed.Location = new System.Drawing.Point(327, 69);
            this.txtLed.Name = "txtLed";
            this.txtLed.Size = new System.Drawing.Size(90, 20);
            this.txtLed.TabIndex = 17;
            this.txtLed.Text = "50";
            // 
            // btnLed
            // 
            this.btnLed.Location = new System.Drawing.Point(423, 67);
            this.btnLed.Name = "btnLed";
            this.btnLed.Size = new System.Drawing.Size(109, 23);
            this.btnLed.TabIndex = 16;
            this.btnLed.Text = "LED_BRIGHT";
            this.btnLed.UseVisualStyleBackColor = true;
            this.btnLed.Click += new System.EventHandler(this.btnLed_Click);
            // 
            // txtNameID
            // 
            this.txtNameID.Location = new System.Drawing.Point(546, 11);
            this.txtNameID.Name = "txtNameID";
            this.txtNameID.Size = new System.Drawing.Size(90, 20);
            this.txtNameID.TabIndex = 19;
            this.txtNameID.Text = "1";
            // 
            // btnNameID
            // 
            this.btnNameID.Location = new System.Drawing.Point(642, 9);
            this.btnNameID.Name = "btnNameID";
            this.btnNameID.Size = new System.Drawing.Size(109, 23);
            this.btnNameID.TabIndex = 18;
            this.btnNameID.Text = "NAME_ID";
            this.btnNameID.UseVisualStyleBackColor = true;
            this.btnNameID.Click += new System.EventHandler(this.btnNameID_Click);
            // 
            // btnInit
            // 
            this.btnInit.Location = new System.Drawing.Point(12, 96);
            this.btnInit.Name = "btnInit";
            this.btnInit.Size = new System.Drawing.Size(82, 23);
            this.btnInit.TabIndex = 20;
            this.btnInit.Text = "INIT";
            this.btnInit.UseVisualStyleBackColor = true;
            this.btnInit.Click += new System.EventHandler(this.btnInit_Click);
            // 
            // txtTrace
            // 
            this.txtTrace.Location = new System.Drawing.Point(546, 40);
            this.txtTrace.Name = "txtTrace";
            this.txtTrace.Size = new System.Drawing.Size(90, 20);
            this.txtTrace.TabIndex = 24;
            this.txtTrace.Text = "0";
            // 
            // btnTrace
            // 
            this.btnTrace.Location = new System.Drawing.Point(642, 38);
            this.btnTrace.Name = "btnTrace";
            this.btnTrace.Size = new System.Drawing.Size(109, 23);
            this.btnTrace.TabIndex = 23;
            this.btnTrace.Text = "TRACE";
            this.btnTrace.UseVisualStyleBackColor = true;
            this.btnTrace.Click += new System.EventHandler(this.btnTrace_Click);
            // 
            // txtPower
            // 
            this.txtPower.Location = new System.Drawing.Point(108, 98);
            this.txtPower.Name = "txtPower";
            this.txtPower.Size = new System.Drawing.Size(90, 20);
            this.txtPower.TabIndex = 26;
            this.txtPower.Text = "30";
            // 
            // btnPower
            // 
            this.btnPower.Location = new System.Drawing.Point(204, 96);
            this.btnPower.Name = "btnPower";
            this.btnPower.Size = new System.Drawing.Size(109, 23);
            this.btnPower.TabIndex = 25;
            this.btnPower.Text = "POWER_SAVE";
            this.btnPower.UseVisualStyleBackColor = true;
            this.btnPower.Click += new System.EventHandler(this.btnPower_Click);
            // 
            // txtSendMiss
            // 
            this.txtSendMiss.Location = new System.Drawing.Point(546, 69);
            this.txtSendMiss.Name = "txtSendMiss";
            this.txtSendMiss.Size = new System.Drawing.Size(90, 20);
            this.txtSendMiss.TabIndex = 28;
            this.txtSendMiss.Text = "0";
            // 
            // btnSendMiss
            // 
            this.btnSendMiss.Location = new System.Drawing.Point(642, 67);
            this.btnSendMiss.Name = "btnSendMiss";
            this.btnSendMiss.Size = new System.Drawing.Size(109, 23);
            this.btnSendMiss.TabIndex = 27;
            this.btnSendMiss.Text = "SEND_MISS";
            this.btnSendMiss.UseVisualStyleBackColor = true;
            this.btnSendMiss.Click += new System.EventHandler(this.btnSendMiss_Click);
            // 
            // txtTargetRing
            // 
            this.txtTargetRing.Location = new System.Drawing.Point(327, 98);
            this.txtTargetRing.Name = "txtTargetRing";
            this.txtTargetRing.Size = new System.Drawing.Size(90, 20);
            this.txtTargetRing.TabIndex = 30;
            this.txtTargetRing.Text = "1555";
            // 
            // btnTargetRing
            // 
            this.btnTargetRing.Location = new System.Drawing.Point(423, 96);
            this.btnTargetRing.Name = "btnTargetRing";
            this.btnTargetRing.Size = new System.Drawing.Size(109, 23);
            this.btnTargetRing.TabIndex = 29;
            this.btnTargetRing.Text = "TRGT_1_RINGx10";
            this.btnTargetRing.UseVisualStyleBackColor = true;
            this.btnTargetRing.Click += new System.EventHandler(this.btnTargetRing_Click);
            // 
            // frmArduino
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(864, 412);
            this.Controls.Add(this.txtTargetRing);
            this.Controls.Add(this.btnTargetRing);
            this.Controls.Add(this.txtSendMiss);
            this.Controls.Add(this.btnSendMiss);
            this.Controls.Add(this.txtPower);
            this.Controls.Add(this.btnPower);
            this.Controls.Add(this.txtTrace);
            this.Controls.Add(this.btnTrace);
            this.Controls.Add(this.btnInit);
            this.Controls.Add(this.txtNameID);
            this.Controls.Add(this.btnNameID);
            this.Controls.Add(this.txtLed);
            this.Controls.Add(this.btnLed);
            this.Controls.Add(this.btnCalibration);
            this.Controls.Add(this.btnVersion);
            this.Controls.Add(this.txtOffset);
            this.Controls.Add(this.btnOffset);
            this.Controls.Add(this.ckbAutoscroll);
            this.Controls.Add(this.txtTest);
            this.Controls.Add(this.txtSensor);
            this.Controls.Add(this.txtPaper);
            this.Controls.Add(this.txtDip);
            this.Controls.Add(this.btnTest);
            this.Controls.Add(this.btnSensor);
            this.Controls.Add(this.btnPaper);
            this.Controls.Add(this.btnDip);
            this.Controls.Add(this.btnClose);
            this.Controls.Add(this.btnEcho);
            this.Controls.Add(this.txtOutput);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MinimumSize = new System.Drawing.Size(880, 450);
            this.Name = "frmArduino";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Arduino";
            this.Activated += new System.EventHandler(this.frmArduino_Activated);
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.frmArduino_FormClosing);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.Button btnClose;
        private System.Windows.Forms.TextBox txtOutput;
        private System.Windows.Forms.Button btnEcho;
        private System.Windows.Forms.Button btnDip;
        private System.Windows.Forms.Button btnPaper;
        private System.Windows.Forms.Button btnSensor;
        private System.Windows.Forms.Button btnTest;
        private System.Windows.Forms.TextBox txtDip;
        private System.Windows.Forms.TextBox txtPaper;
        private System.Windows.Forms.TextBox txtSensor;
        private System.Windows.Forms.TextBox txtTest;
        private System.Windows.Forms.CheckBox ckbAutoscroll;
        private System.Windows.Forms.TextBox txtOffset;
        private System.Windows.Forms.Button btnOffset;
        private System.Windows.Forms.Button btnVersion;
        private System.Windows.Forms.Button btnCalibration;
        private System.Windows.Forms.TextBox txtLed;
        private System.Windows.Forms.Button btnLed;
        private System.Windows.Forms.TextBox txtNameID;
        private System.Windows.Forms.Button btnNameID;
        private System.Windows.Forms.Button btnInit;
        private System.Windows.Forms.TextBox txtTrace;
        private System.Windows.Forms.Button btnTrace;
        private System.Windows.Forms.TextBox txtPower;
        private System.Windows.Forms.Button btnPower;
        private System.Windows.Forms.TextBox txtSendMiss;
        private System.Windows.Forms.Button btnSendMiss;
        private System.Windows.Forms.TextBox txtTargetRing;
        private System.Windows.Forms.Button btnTargetRing;
    }
}