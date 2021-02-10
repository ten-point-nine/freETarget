namespace freETarget {
    partial class frmCalibration {
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmCalibration));
            this.btnLeft = new System.Windows.Forms.Button();
            this.btnRight = new System.Windows.Forms.Button();
            this.btnDown = new System.Windows.Forms.Button();
            this.btnUp = new System.Windows.Forms.Button();
            this.btnReset = new System.Windows.Forms.Button();
            this.btnClose = new System.Windows.Forms.Button();
            this.toolTip = new System.Windows.Forms.ToolTip(this.components);
            this.txtIncrement = new System.Windows.Forms.TextBox();
            this.txtAngle = new System.Windows.Forms.TextBox();
            this.btnClockwise = new System.Windows.Forms.Button();
            this.btnAntiClockwise = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // btnLeft
            // 
            this.btnLeft.Image = ((System.Drawing.Image)(resources.GetObject("btnLeft.Image")));
            this.btnLeft.Location = new System.Drawing.Point(12, 46);
            this.btnLeft.Name = "btnLeft";
            this.btnLeft.Size = new System.Drawing.Size(30, 30);
            this.btnLeft.TabIndex = 0;
            this.btnLeft.UseVisualStyleBackColor = true;
            this.btnLeft.Click += new System.EventHandler(this.btnLeft_Click);
            // 
            // btnRight
            // 
            this.btnRight.Image = ((System.Drawing.Image)(resources.GetObject("btnRight.Image")));
            this.btnRight.Location = new System.Drawing.Point(83, 46);
            this.btnRight.Name = "btnRight";
            this.btnRight.Size = new System.Drawing.Size(30, 30);
            this.btnRight.TabIndex = 1;
            this.btnRight.UseVisualStyleBackColor = true;
            this.btnRight.Click += new System.EventHandler(this.btnRight_Click);
            // 
            // btnDown
            // 
            this.btnDown.Image = ((System.Drawing.Image)(resources.GetObject("btnDown.Image")));
            this.btnDown.Location = new System.Drawing.Point(48, 82);
            this.btnDown.Name = "btnDown";
            this.btnDown.Size = new System.Drawing.Size(30, 30);
            this.btnDown.TabIndex = 2;
            this.btnDown.UseVisualStyleBackColor = true;
            this.btnDown.Click += new System.EventHandler(this.btnDown_Click);
            // 
            // btnUp
            // 
            this.btnUp.Image = ((System.Drawing.Image)(resources.GetObject("btnUp.Image")));
            this.btnUp.Location = new System.Drawing.Point(47, 12);
            this.btnUp.Name = "btnUp";
            this.btnUp.Size = new System.Drawing.Size(30, 30);
            this.btnUp.TabIndex = 3;
            this.btnUp.UseVisualStyleBackColor = true;
            this.btnUp.Click += new System.EventHandler(this.btnUp_Click);
            // 
            // btnReset
            // 
            this.btnReset.Location = new System.Drawing.Point(126, 12);
            this.btnReset.Name = "btnReset";
            this.btnReset.Size = new System.Drawing.Size(75, 23);
            this.btnReset.TabIndex = 4;
            this.btnReset.Text = "Reset";
            this.toolTip.SetToolTip(this.btnReset, "Reset calibration to zero");
            this.btnReset.UseVisualStyleBackColor = true;
            this.btnReset.Click += new System.EventHandler(this.btnReset_Click);
            // 
            // btnClose
            // 
            this.btnClose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.btnClose.Location = new System.Drawing.Point(104, 88);
            this.btnClose.Name = "btnClose";
            this.btnClose.Size = new System.Drawing.Size(97, 23);
            this.btnClose.TabIndex = 5;
            this.btnClose.Text = "Save && Close";
            this.btnClose.UseVisualStyleBackColor = true;
            this.btnClose.Click += new System.EventHandler(this.btnClose_Click);
            // 
            // toolTip
            // 
            this.toolTip.AutomaticDelay = 1000;
            // 
            // txtIncrement
            // 
            this.txtIncrement.Location = new System.Drawing.Point(50, 51);
            this.txtIncrement.Name = "txtIncrement";
            this.txtIncrement.Size = new System.Drawing.Size(26, 20);
            this.txtIncrement.TabIndex = 6;
            this.txtIncrement.Text = "0.5";
            this.txtIncrement.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.toolTip.SetToolTip(this.txtIncrement, "Calibration increment; between 0.01 and 10");
            // 
            // txtAngle
            // 
            this.txtAngle.Location = new System.Drawing.Point(50, 136);
            this.txtAngle.Name = "txtAngle";
            this.txtAngle.Size = new System.Drawing.Size(26, 20);
            this.txtAngle.TabIndex = 9;
            this.txtAngle.Text = "1";
            this.txtAngle.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.toolTip.SetToolTip(this.txtAngle, "Calibration increment; between 0.01 and 10");
            // 
            // btnClockwise
            // 
            this.btnClockwise.Image = ((System.Drawing.Image)(resources.GetObject("btnClockwise.Image")));
            this.btnClockwise.Location = new System.Drawing.Point(12, 130);
            this.btnClockwise.Name = "btnClockwise";
            this.btnClockwise.Size = new System.Drawing.Size(30, 30);
            this.btnClockwise.TabIndex = 7;
            this.btnClockwise.UseVisualStyleBackColor = true;
            this.btnClockwise.Click += new System.EventHandler(this.btnClockwise_Click);
            // 
            // btnAntiClockwise
            // 
            this.btnAntiClockwise.Image = ((System.Drawing.Image)(resources.GetObject("btnAntiClockwise.Image")));
            this.btnAntiClockwise.Location = new System.Drawing.Point(83, 130);
            this.btnAntiClockwise.Name = "btnAntiClockwise";
            this.btnAntiClockwise.Size = new System.Drawing.Size(30, 30);
            this.btnAntiClockwise.TabIndex = 8;
            this.btnAntiClockwise.UseVisualStyleBackColor = true;
            this.btnAntiClockwise.Click += new System.EventHandler(this.btnAntiClockwise_Click);
            // 
            // frmCalibration
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(213, 123);
            this.Controls.Add(this.txtAngle);
            this.Controls.Add(this.btnAntiClockwise);
            this.Controls.Add(this.btnClockwise);
            this.Controls.Add(this.txtIncrement);
            this.Controls.Add(this.btnClose);
            this.Controls.Add(this.btnReset);
            this.Controls.Add(this.btnUp);
            this.Controls.Add(this.btnDown);
            this.Controls.Add(this.btnRight);
            this.Controls.Add(this.btnLeft);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "frmCalibration";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Calibration";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.frmCalibration_FormClosing);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btnLeft;
        private System.Windows.Forms.Button btnRight;
        private System.Windows.Forms.Button btnDown;
        private System.Windows.Forms.Button btnUp;
        private System.Windows.Forms.Button btnReset;
        private System.Windows.Forms.Button btnClose;
        private System.Windows.Forms.ToolTip toolTip;
        private System.Windows.Forms.TextBox txtIncrement;
        private System.Windows.Forms.Button btnClockwise;
        private System.Windows.Forms.Button btnAntiClockwise;
        private System.Windows.Forms.TextBox txtAngle;
    }
}