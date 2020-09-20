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
            this.SuspendLayout();
            // 
            // btnClose
            // 
            this.btnClose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnClose.Location = new System.Drawing.Point(549, 9);
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
            this.txtOutput.Location = new System.Drawing.Point(9, 103);
            this.txtOutput.Multiline = true;
            this.txtOutput.Name = "txtOutput";
            this.txtOutput.ReadOnly = true;
            this.txtOutput.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.txtOutput.Size = new System.Drawing.Size(615, 148);
            this.txtOutput.TabIndex = 2;
            // 
            // btnEcho
            // 
            this.btnEcho.Location = new System.Drawing.Point(107, 10);
            this.btnEcho.Name = "btnEcho";
            this.btnEcho.Size = new System.Drawing.Size(74, 24);
            this.btnEcho.TabIndex = 1;
            this.btnEcho.Text = "ECHO";
            this.btnEcho.UseVisualStyleBackColor = true;
            this.btnEcho.Click += new System.EventHandler(this.btnEcho_Click);
            // 
            // btnDip
            // 
            this.btnDip.Location = new System.Drawing.Point(107, 40);
            this.btnDip.Name = "btnDip";
            this.btnDip.Size = new System.Drawing.Size(75, 23);
            this.btnDip.TabIndex = 3;
            this.btnDip.Text = "DIP";
            this.btnDip.UseVisualStyleBackColor = true;
            this.btnDip.Click += new System.EventHandler(this.btnDip_Click);
            // 
            // btnPaper
            // 
            this.btnPaper.Location = new System.Drawing.Point(107, 69);
            this.btnPaper.Name = "btnPaper";
            this.btnPaper.Size = new System.Drawing.Size(75, 23);
            this.btnPaper.TabIndex = 4;
            this.btnPaper.Text = "PAPER";
            this.btnPaper.UseVisualStyleBackColor = true;
            this.btnPaper.Click += new System.EventHandler(this.btnPaper_Click);
            // 
            // btnSensor
            // 
            this.btnSensor.Location = new System.Drawing.Point(360, 10);
            this.btnSensor.Name = "btnSensor";
            this.btnSensor.Size = new System.Drawing.Size(75, 23);
            this.btnSensor.TabIndex = 5;
            this.btnSensor.Text = "SENSOR";
            this.btnSensor.UseVisualStyleBackColor = true;
            this.btnSensor.Click += new System.EventHandler(this.btnSensor_Click);
            // 
            // btnTest
            // 
            this.btnTest.Location = new System.Drawing.Point(360, 39);
            this.btnTest.Name = "btnTest";
            this.btnTest.Size = new System.Drawing.Size(75, 23);
            this.btnTest.TabIndex = 6;
            this.btnTest.Text = "TEST";
            this.btnTest.UseVisualStyleBackColor = true;
            this.btnTest.Click += new System.EventHandler(this.btnTest_Click);
            // 
            // txtDip
            // 
            this.txtDip.Location = new System.Drawing.Point(11, 42);
            this.txtDip.Name = "txtDip";
            this.txtDip.Size = new System.Drawing.Size(90, 20);
            this.txtDip.TabIndex = 7;
            this.txtDip.Text = "0";
            // 
            // txtPaper
            // 
            this.txtPaper.Location = new System.Drawing.Point(11, 71);
            this.txtPaper.Name = "txtPaper";
            this.txtPaper.Size = new System.Drawing.Size(90, 20);
            this.txtPaper.TabIndex = 8;
            this.txtPaper.Text = "65535";
            // 
            // txtSensor
            // 
            this.txtSensor.Location = new System.Drawing.Point(264, 12);
            this.txtSensor.Name = "txtSensor";
            this.txtSensor.Size = new System.Drawing.Size(90, 20);
            this.txtSensor.TabIndex = 9;
            this.txtSensor.Text = "230.00";
            // 
            // txtTest
            // 
            this.txtTest.Location = new System.Drawing.Point(264, 41);
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
            this.ckbAutoscroll.Location = new System.Drawing.Point(547, 80);
            this.ckbAutoscroll.Name = "ckbAutoscroll";
            this.ckbAutoscroll.Size = new System.Drawing.Size(72, 17);
            this.ckbAutoscroll.TabIndex = 11;
            this.ckbAutoscroll.Text = "Autoscroll";
            this.ckbAutoscroll.UseVisualStyleBackColor = true;
            // 
            // frmArduino
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(631, 262);
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
            this.MinimumSize = new System.Drawing.Size(550, 300);
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
    }
}