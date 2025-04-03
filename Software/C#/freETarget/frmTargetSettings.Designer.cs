
namespace freETarget {
    partial class frmTargetSettings {
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmTargetSettings));
            this.tabControl = new System.Windows.Forms.TabControl();
            this.tabPageHardware = new System.Windows.Forms.TabPage();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.label3 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.txtYoffset = new System.Windows.Forms.TextBox();
            this.txtXoffset = new System.Windows.Forms.TextBox();
            this.cmbName = new System.Windows.Forms.ComboBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.lblStepTime = new System.Windows.Forms.Label();
            this.txtStepTime = new System.Windows.Forms.TextBox();
            this.lblDCTime = new System.Windows.Forms.Label();
            this.lblSteps = new System.Windows.Forms.Label();
            this.txtPaperTime = new System.Windows.Forms.TextBox();
            this.rbDC = new System.Windows.Forms.RadioButton();
            this.rbStepper = new System.Windows.Forms.RadioButton();
            this.txtSteps = new System.Windows.Forms.TextBox();
            this.lblLED = new System.Windows.Forms.Label();
            this.label22 = new System.Windows.Forms.Label();
            this.trkLEDbright = new System.Windows.Forms.TrackBar();
            this.tabPageSensors = new System.Windows.Forms.TabPage();
            this.txtZOffset = new System.Windows.Forms.TextBox();
            this.label37 = new System.Windows.Forms.Label();
            this.txtSensorDiameter = new System.Windows.Forms.TextBox();
            this.label18 = new System.Windows.Forms.Label();
            this.txtSouthX = new System.Windows.Forms.TextBox();
            this.txtSouthY = new System.Windows.Forms.TextBox();
            this.txtWestX = new System.Windows.Forms.TextBox();
            this.txtWestY = new System.Windows.Forms.TextBox();
            this.txtEastX = new System.Windows.Forms.TextBox();
            this.txtEastY = new System.Windows.Forms.TextBox();
            this.txtNorthY = new System.Windows.Forms.TextBox();
            this.txtNorthX = new System.Windows.Forms.TextBox();
            this.picTarget = new System.Windows.Forms.PictureBox();
            this.lblWarning = new System.Windows.Forms.Label();
            this.btnApply = new System.Windows.Forms.Button();
            this.helpProvider = new System.Windows.Forms.HelpProvider();
            this.btnClose = new System.Windows.Forms.Button();
            this.pnlWait = new System.Windows.Forms.Panel();
            this.label1 = new System.Windows.Forms.Label();
            this.tabControl.SuspendLayout();
            this.tabPageHardware.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.trkLEDbright)).BeginInit();
            this.tabPageSensors.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.picTarget)).BeginInit();
            this.pnlWait.SuspendLayout();
            this.SuspendLayout();
            // 
            // tabControl
            // 
            this.tabControl.Controls.Add(this.tabPageHardware);
            this.tabControl.Controls.Add(this.tabPageSensors);
            this.tabControl.Enabled = false;
            this.tabControl.Location = new System.Drawing.Point(12, 12);
            this.tabControl.Name = "tabControl";
            this.tabControl.SelectedIndex = 0;
            this.tabControl.Size = new System.Drawing.Size(338, 403);
            this.tabControl.TabIndex = 35;
            // 
            // tabPageHardware
            // 
            this.tabPageHardware.Controls.Add(this.groupBox1);
            this.tabPageHardware.Controls.Add(this.cmbName);
            this.tabPageHardware.Controls.Add(this.groupBox2);
            this.tabPageHardware.Controls.Add(this.lblLED);
            this.tabPageHardware.Controls.Add(this.label22);
            this.tabPageHardware.Controls.Add(this.trkLEDbright);
            this.tabPageHardware.Location = new System.Drawing.Point(4, 22);
            this.tabPageHardware.Name = "tabPageHardware";
            this.tabPageHardware.Size = new System.Drawing.Size(330, 377);
            this.tabPageHardware.TabIndex = 3;
            this.tabPageHardware.Text = "Hardware";
            this.tabPageHardware.UseVisualStyleBackColor = true;
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.label3);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Controls.Add(this.txtYoffset);
            this.groupBox1.Controls.Add(this.txtXoffset);
            this.groupBox1.Location = new System.Drawing.Point(13, 144);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(283, 79);
            this.groupBox1.TabIndex = 24;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Calibration offset";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(6, 49);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(61, 13);
            this.label3.TabIndex = 3;
            this.label3.Text = "Y_OFFSET";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(6, 23);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(61, 13);
            this.label2.TabIndex = 2;
            this.label2.Text = "X_OFFSET";
            // 
            // txtYoffset
            // 
            this.txtYoffset.Location = new System.Drawing.Point(93, 46);
            this.txtYoffset.Name = "txtYoffset";
            this.txtYoffset.Size = new System.Drawing.Size(184, 20);
            this.txtYoffset.TabIndex = 1;
            // 
            // txtXoffset
            // 
            this.txtXoffset.Location = new System.Drawing.Point(93, 20);
            this.txtXoffset.Name = "txtXoffset";
            this.txtXoffset.Size = new System.Drawing.Size(184, 20);
            this.txtXoffset.TabIndex = 0;
            // 
            // cmbName
            // 
            this.cmbName.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbName.FormattingEnabled = true;
            this.helpProvider.SetHelpString(this.cmbName, "Name of the target. It will also designate the WiFi name");
            this.cmbName.Location = new System.Drawing.Point(106, 13);
            this.cmbName.Name = "cmbName";
            this.helpProvider.SetShowHelp(this.cmbName, true);
            this.cmbName.Size = new System.Drawing.Size(190, 21);
            this.cmbName.TabIndex = 22;
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.lblStepTime);
            this.groupBox2.Controls.Add(this.txtStepTime);
            this.groupBox2.Controls.Add(this.lblDCTime);
            this.groupBox2.Controls.Add(this.lblSteps);
            this.groupBox2.Controls.Add(this.txtPaperTime);
            this.groupBox2.Controls.Add(this.rbDC);
            this.groupBox2.Controls.Add(this.rbStepper);
            this.groupBox2.Controls.Add(this.txtSteps);
            this.groupBox2.Location = new System.Drawing.Point(13, 40);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(283, 98);
            this.groupBox2.TabIndex = 23;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Motor";
            // 
            // lblStepTime
            // 
            this.lblStepTime.AutoSize = true;
            this.lblStepTime.Location = new System.Drawing.Point(109, 73);
            this.lblStepTime.Name = "lblStepTime";
            this.lblStepTime.Size = new System.Drawing.Size(94, 13);
            this.lblStepTime.TabIndex = 21;
            this.lblStepTime.Text = "Step Duration (ms)";
            // 
            // txtStepTime
            // 
            this.helpProvider.SetHelpString(this.txtStepTime, "Number of steps the stepper motor advances the paper.   A larger time means more " +
        "paper is used for each shot");
            this.txtStepTime.Location = new System.Drawing.Point(209, 70);
            this.txtStepTime.Name = "txtStepTime";
            this.helpProvider.SetShowHelp(this.txtStepTime, true);
            this.txtStepTime.Size = new System.Drawing.Size(68, 20);
            this.txtStepTime.TabIndex = 20;
            this.txtStepTime.Text = "0";
            // 
            // lblDCTime
            // 
            this.lblDCTime.AutoSize = true;
            this.lblDCTime.Location = new System.Drawing.Point(109, 21);
            this.lblDCTime.Name = "lblDCTime";
            this.lblDCTime.Size = new System.Drawing.Size(52, 13);
            this.lblDCTime.TabIndex = 19;
            this.lblDCTime.Text = "Time (ms)";
            // 
            // lblSteps
            // 
            this.lblSteps.AutoSize = true;
            this.lblSteps.Location = new System.Drawing.Point(109, 47);
            this.lblSteps.Name = "lblSteps";
            this.lblSteps.Size = new System.Drawing.Size(34, 13);
            this.lblSteps.TabIndex = 18;
            this.lblSteps.Text = "Steps";
            // 
            // txtPaperTime
            // 
            this.helpProvider.SetHelpString(this.txtPaperTime, "Number of milliseconds to run the DC motor that scrolls the paper.   A larger tim" +
        "e means more paper is used for each shot");
            this.txtPaperTime.Location = new System.Drawing.Point(209, 18);
            this.txtPaperTime.Name = "txtPaperTime";
            this.helpProvider.SetShowHelp(this.txtPaperTime, true);
            this.txtPaperTime.Size = new System.Drawing.Size(68, 20);
            this.txtPaperTime.TabIndex = 17;
            this.txtPaperTime.Text = "0";
            // 
            // rbDC
            // 
            this.rbDC.AutoSize = true;
            this.rbDC.Location = new System.Drawing.Point(6, 19);
            this.rbDC.Name = "rbDC";
            this.rbDC.Size = new System.Drawing.Size(90, 17);
            this.rbDC.TabIndex = 1;
            this.rbDC.TabStop = true;
            this.rbDC.Text = "Direct Current";
            this.rbDC.UseVisualStyleBackColor = true;
            this.rbDC.CheckedChanged += new System.EventHandler(this.rbDC_CheckedChanged);
            // 
            // rbStepper
            // 
            this.rbStepper.AutoSize = true;
            this.rbStepper.Location = new System.Drawing.Point(6, 45);
            this.rbStepper.Name = "rbStepper";
            this.rbStepper.Size = new System.Drawing.Size(62, 17);
            this.rbStepper.TabIndex = 0;
            this.rbStepper.TabStop = true;
            this.rbStepper.Text = "Stepper";
            this.rbStepper.UseVisualStyleBackColor = true;
            this.rbStepper.CheckedChanged += new System.EventHandler(this.rbStepper_CheckedChanged);
            // 
            // txtSteps
            // 
            this.helpProvider.SetHelpString(this.txtSteps, "Duration in milliseconds of the stepper motor pulse.");
            this.txtSteps.Location = new System.Drawing.Point(209, 44);
            this.txtSteps.Name = "txtSteps";
            this.helpProvider.SetShowHelp(this.txtSteps, true);
            this.txtSteps.Size = new System.Drawing.Size(68, 20);
            this.txtSteps.TabIndex = 16;
            this.txtSteps.Text = "0";
            // 
            // lblLED
            // 
            this.lblLED.AutoSize = true;
            this.lblLED.Location = new System.Drawing.Point(10, 249);
            this.lblLED.Name = "lblLED";
            this.lblLED.Size = new System.Drawing.Size(74, 13);
            this.lblLED.TabIndex = 20;
            this.lblLED.Text = "LED Brigtness";
            // 
            // label22
            // 
            this.label22.AutoSize = true;
            this.label22.Location = new System.Drawing.Point(10, 17);
            this.label22.Name = "label22";
            this.label22.Size = new System.Drawing.Size(69, 13);
            this.label22.TabIndex = 21;
            this.label22.Text = "Target Name";
            // 
            // trkLEDbright
            // 
            this.trkLEDbright.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(250)))), ((int)(((byte)(250)))), ((int)(((byte)(250)))));
            this.helpProvider.SetHelpString(this.trkLEDbright, "External LED brigtness control (percent)");
            this.trkLEDbright.LargeChange = 10;
            this.trkLEDbright.Location = new System.Drawing.Point(106, 243);
            this.trkLEDbright.Maximum = 99;
            this.trkLEDbright.Name = "trkLEDbright";
            this.helpProvider.SetShowHelp(this.trkLEDbright, true);
            this.trkLEDbright.Size = new System.Drawing.Size(190, 45);
            this.trkLEDbright.SmallChange = 5;
            this.trkLEDbright.TabIndex = 19;
            this.trkLEDbright.TickFrequency = 5;
            this.trkLEDbright.ValueChanged += new System.EventHandler(this.trkLEDbright_ValueChanged);
            // 
            // tabPageSensors
            // 
            this.tabPageSensors.Controls.Add(this.txtZOffset);
            this.tabPageSensors.Controls.Add(this.label37);
            this.tabPageSensors.Controls.Add(this.txtSensorDiameter);
            this.tabPageSensors.Controls.Add(this.label18);
            this.tabPageSensors.Controls.Add(this.txtSouthX);
            this.tabPageSensors.Controls.Add(this.txtSouthY);
            this.tabPageSensors.Controls.Add(this.txtWestX);
            this.tabPageSensors.Controls.Add(this.txtWestY);
            this.tabPageSensors.Controls.Add(this.txtEastX);
            this.tabPageSensors.Controls.Add(this.txtEastY);
            this.tabPageSensors.Controls.Add(this.txtNorthY);
            this.tabPageSensors.Controls.Add(this.txtNorthX);
            this.tabPageSensors.Controls.Add(this.picTarget);
            this.tabPageSensors.Location = new System.Drawing.Point(4, 22);
            this.tabPageSensors.Name = "tabPageSensors";
            this.tabPageSensors.Size = new System.Drawing.Size(330, 377);
            this.tabPageSensors.TabIndex = 2;
            this.tabPageSensors.Text = "Sensors";
            this.tabPageSensors.UseVisualStyleBackColor = true;
            // 
            // txtZOffset
            // 
            this.helpProvider.SetHelpString(this.txtZOffset, "Distance in millimeters from paper to sensor plane");
            this.txtZOffset.Location = new System.Drawing.Point(137, 35);
            this.txtZOffset.Name = "txtZOffset";
            this.helpProvider.SetShowHelp(this.txtZOffset, true);
            this.txtZOffset.Size = new System.Drawing.Size(160, 20);
            this.txtZOffset.TabIndex = 38;
            this.txtZOffset.Text = "0";
            // 
            // label37
            // 
            this.label37.AutoSize = true;
            this.label37.Location = new System.Drawing.Point(13, 38);
            this.label37.Name = "label37";
            this.label37.Size = new System.Drawing.Size(106, 13);
            this.label37.TabIndex = 37;
            this.label37.Text = "Sensor Z Offset (mm)";
            // 
            // txtSensorDiameter
            // 
            this.helpProvider.SetHelpString(this.txtSensorDiameter, "Distance between 2 oposing sensors. This parameter is dependant of target type (1" +
        "0m, 25m, 50m, etc).");
            this.txtSensorDiameter.Location = new System.Drawing.Point(137, 9);
            this.txtSensorDiameter.Name = "txtSensorDiameter";
            this.helpProvider.SetShowHelp(this.txtSensorDiameter, true);
            this.txtSensorDiameter.Size = new System.Drawing.Size(160, 20);
            this.txtSensorDiameter.TabIndex = 35;
            this.txtSensorDiameter.Text = "230";
            // 
            // label18
            // 
            this.label18.AutoSize = true;
            this.label18.Location = new System.Drawing.Point(13, 12);
            this.label18.Name = "label18";
            this.label18.Size = new System.Drawing.Size(108, 13);
            this.label18.TabIndex = 36;
            this.label18.Text = "Sensor diameter (mm)";
            // 
            // txtSouthX
            // 
            this.helpProvider.SetHelpString(this.txtSouthX, "Deviation from the ideal position on the X axis of the South sensor");
            this.txtSouthX.Location = new System.Drawing.Point(297, 351);
            this.txtSouthX.Name = "txtSouthX";
            this.helpProvider.SetShowHelp(this.txtSouthX, true);
            this.txtSouthX.Size = new System.Drawing.Size(29, 20);
            this.txtSouthX.TabIndex = 20;
            this.txtSouthX.Text = "0";
            // 
            // txtSouthY
            // 
            this.helpProvider.SetHelpString(this.txtSouthY, "Deviation from the ideal position on the Y axis of the South sensor");
            this.txtSouthY.Location = new System.Drawing.Point(225, 278);
            this.txtSouthY.Name = "txtSouthY";
            this.helpProvider.SetShowHelp(this.txtSouthY, true);
            this.txtSouthY.Size = new System.Drawing.Size(29, 20);
            this.txtSouthY.TabIndex = 19;
            this.txtSouthY.Text = "0";
            // 
            // txtWestX
            // 
            this.helpProvider.SetHelpString(this.txtWestX, "Deviation from the ideal position on the X axis of the West sensor");
            this.txtWestX.Location = new System.Drawing.Point(82, 351);
            this.txtWestX.Name = "txtWestX";
            this.helpProvider.SetShowHelp(this.txtWestX, true);
            this.txtWestX.Size = new System.Drawing.Size(29, 20);
            this.txtWestX.TabIndex = 18;
            this.txtWestX.Text = "0";
            // 
            // txtWestY
            // 
            this.helpProvider.SetHelpString(this.txtWestY, "Deviation from the ideal position on the Y axis of the West sensor");
            this.txtWestY.Location = new System.Drawing.Point(14, 278);
            this.txtWestY.Name = "txtWestY";
            this.helpProvider.SetShowHelp(this.txtWestY, true);
            this.txtWestY.Size = new System.Drawing.Size(29, 20);
            this.txtWestY.TabIndex = 17;
            this.txtWestY.Text = "0";
            // 
            // txtEastX
            // 
            this.helpProvider.SetHelpString(this.txtEastX, "Deviation from the ideal position on the X axis of the East sensor");
            this.txtEastX.Location = new System.Drawing.Point(297, 133);
            this.txtEastX.Name = "txtEastX";
            this.helpProvider.SetShowHelp(this.txtEastX, true);
            this.txtEastX.Size = new System.Drawing.Size(29, 20);
            this.txtEastX.TabIndex = 16;
            this.txtEastX.Text = "0";
            // 
            // txtEastY
            // 
            this.helpProvider.SetHelpString(this.txtEastY, "Deviation from the ideal position on the Y axis of the East sensor");
            this.txtEastY.Location = new System.Drawing.Point(225, 67);
            this.txtEastY.Name = "txtEastY";
            this.helpProvider.SetShowHelp(this.txtEastY, true);
            this.txtEastY.Size = new System.Drawing.Size(29, 20);
            this.txtEastY.TabIndex = 15;
            this.txtEastY.Text = "0";
            // 
            // txtNorthY
            // 
            this.helpProvider.SetHelpString(this.txtNorthY, "Deviation from the ideal position on the Y axis of the North sensor");
            this.txtNorthY.Location = new System.Drawing.Point(14, 67);
            this.txtNorthY.Name = "txtNorthY";
            this.helpProvider.SetShowHelp(this.txtNorthY, true);
            this.txtNorthY.Size = new System.Drawing.Size(29, 20);
            this.txtNorthY.TabIndex = 14;
            this.txtNorthY.Text = "0";
            // 
            // txtNorthX
            // 
            this.helpProvider.SetHelpString(this.txtNorthX, "Deviation from the ideal position on the X axis of the North sensor");
            this.txtNorthX.Location = new System.Drawing.Point(82, 133);
            this.txtNorthX.Name = "txtNorthX";
            this.helpProvider.SetShowHelp(this.txtNorthX, true);
            this.txtNorthX.Size = new System.Drawing.Size(29, 20);
            this.txtNorthX.TabIndex = 13;
            this.txtNorthX.Text = "0";
            // 
            // picTarget
            // 
            this.picTarget.Image = ((System.Drawing.Image)(resources.GetObject("picTarget.Image")));
            this.picTarget.InitialImage = null;
            this.picTarget.Location = new System.Drawing.Point(14, 88);
            this.picTarget.Name = "picTarget";
            this.picTarget.Size = new System.Drawing.Size(283, 283);
            this.picTarget.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
            this.picTarget.TabIndex = 12;
            this.picTarget.TabStop = false;
            // 
            // lblWarning
            // 
            this.lblWarning.AutoSize = true;
            this.lblWarning.Location = new System.Drawing.Point(13, 420);
            this.lblWarning.MaximumSize = new System.Drawing.Size(330, 0);
            this.lblWarning.Name = "lblWarning";
            this.lblWarning.Size = new System.Drawing.Size(304, 26);
            this.lblWarning.TabIndex = 36;
            this.lblWarning.Text = "All modifications on this page are sent directly to the target and stored in its " +
    "internal storage";
            // 
            // btnApply
            // 
            this.btnApply.Enabled = false;
            this.btnApply.Location = new System.Drawing.Point(131, 459);
            this.btnApply.Name = "btnApply";
            this.btnApply.Size = new System.Drawing.Size(136, 24);
            this.btnApply.TabIndex = 37;
            this.btnApply.Text = "Apply settings to target";
            this.btnApply.UseVisualStyleBackColor = true;
            this.btnApply.Click += new System.EventHandler(this.btnApply_Click);
            // 
            // btnClose
            // 
            this.btnClose.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnClose.Location = new System.Drawing.Point(273, 460);
            this.btnClose.Name = "btnClose";
            this.btnClose.Size = new System.Drawing.Size(75, 23);
            this.btnClose.TabIndex = 38;
            this.btnClose.Text = "Close";
            this.btnClose.UseVisualStyleBackColor = true;
            this.btnClose.Click += new System.EventHandler(this.btnClose_Click);
            // 
            // pnlWait
            // 
            this.pnlWait.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.pnlWait.Controls.Add(this.label1);
            this.pnlWait.Location = new System.Drawing.Point(3, 513);
            this.pnlWait.Name = "pnlWait";
            this.pnlWait.Size = new System.Drawing.Size(356, 41);
            this.pnlWait.TabIndex = 39;
            this.pnlWait.Visible = false;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(72, 12);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(206, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Please wait while retriving target settings...";
            // 
            // frmTargetSettings
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(360, 490);
            this.Controls.Add(this.pnlWait);
            this.Controls.Add(this.btnClose);
            this.Controls.Add(this.btnApply);
            this.Controls.Add(this.lblWarning);
            this.Controls.Add(this.tabControl);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.HelpButton = true;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "frmTargetSettings";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Target Settings";
            this.Load += new System.EventHandler(this.frmTargetSettings_Load);
            this.Shown += new System.EventHandler(this.frmTargetSettings_Shown);
            this.tabControl.ResumeLayout(false);
            this.tabPageHardware.ResumeLayout(false);
            this.tabPageHardware.PerformLayout();
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.trkLEDbright)).EndInit();
            this.tabPageSensors.ResumeLayout(false);
            this.tabPageSensors.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.picTarget)).EndInit();
            this.pnlWait.ResumeLayout(false);
            this.pnlWait.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TabControl tabControl;
        private System.Windows.Forms.TabPage tabPageHardware;
        public System.Windows.Forms.ComboBox cmbName;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Label lblStepTime;
        public System.Windows.Forms.TextBox txtStepTime;
        private System.Windows.Forms.Label lblDCTime;
        private System.Windows.Forms.Label lblSteps;
        public System.Windows.Forms.TextBox txtPaperTime;
        private System.Windows.Forms.RadioButton rbDC;
        private System.Windows.Forms.RadioButton rbStepper;
        public System.Windows.Forms.TextBox txtSteps;
        private System.Windows.Forms.Label lblLED;
        private System.Windows.Forms.Label label22;
        public System.Windows.Forms.TrackBar trkLEDbright;
        private System.Windows.Forms.TabPage tabPageSensors;
        public System.Windows.Forms.TextBox txtZOffset;
        private System.Windows.Forms.Label label37;
        public System.Windows.Forms.TextBox txtSensorDiameter;
        private System.Windows.Forms.Label label18;
        public System.Windows.Forms.TextBox txtSouthX;
        public System.Windows.Forms.TextBox txtSouthY;
        public System.Windows.Forms.TextBox txtWestX;
        public System.Windows.Forms.TextBox txtWestY;
        public System.Windows.Forms.TextBox txtEastX;
        public System.Windows.Forms.TextBox txtEastY;
        public System.Windows.Forms.TextBox txtNorthY;
        public System.Windows.Forms.TextBox txtNorthX;
        private System.Windows.Forms.PictureBox picTarget;
        private System.Windows.Forms.Label lblWarning;
        private System.Windows.Forms.Button btnApply;
        private System.Windows.Forms.HelpProvider helpProvider;
        private System.Windows.Forms.Button btnClose;
        private System.Windows.Forms.Panel pnlWait;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox txtYoffset;
        private System.Windows.Forms.TextBox txtXoffset;
    }
}