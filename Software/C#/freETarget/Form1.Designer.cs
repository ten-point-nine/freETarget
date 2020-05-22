namespace freETarget
{
    partial class frmMainWindow
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmMainWindow));
            this.serialPort = new System.IO.Ports.SerialPort(this.components);
            this.btnConnect = new System.Windows.Forms.Button();
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this.statusText = new System.Windows.Forms.ToolStripStatusLabel();
            this.txtOutput = new System.Windows.Forms.TextBox();
            this.imgTarget = new System.Windows.Forms.PictureBox();
            this.shotsList = new System.Windows.Forms.ListView();
            this.col4 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.col1 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.col2 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.col3 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.imgListDirections = new System.Windows.Forms.ImageList(this.components);
            this.txtTotal = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.txtLastShot = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.imgArrow = new System.Windows.Forms.PictureBox();
            this.btnConfig = new System.Windows.Forms.Button();
            this.cmbWeapon = new System.Windows.Forms.ComboBox();
            this.trkZoom = new System.Windows.Forms.TrackBar();
            this.label3 = new System.Windows.Forms.Label();
            this.btnClear = new System.Windows.Forms.Button();
            this.txtTime = new System.Windows.Forms.TextBox();
            this.label4 = new System.Windows.Forms.Label();
            this.timer = new System.Windows.Forms.Timer(this.components);
            this.txtMeanRadius = new System.Windows.Forms.TextBox();
            this.label5 = new System.Windows.Forms.Label();
            this.txtMaxSpread = new System.Windows.Forms.TextBox();
            this.label6 = new System.Windows.Forms.Label();
            this.label7 = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.txtWindage = new System.Windows.Forms.TextBox();
            this.txtElevation = new System.Windows.Forms.TextBox();
            this.imgWindage = new System.Windows.Forms.PictureBox();
            this.imgElevation = new System.Windows.Forms.PictureBox();
            this.btnCalibration = new System.Windows.Forms.Button();
            this.toolTip = new System.Windows.Forms.ToolTip(this.components);
            this.statusStrip1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.imgTarget)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.imgArrow)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trkZoom)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.imgWindage)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.imgElevation)).BeginInit();
            this.SuspendLayout();
            // 
            // serialPort
            // 
            this.serialPort.DataReceived += new System.IO.Ports.SerialDataReceivedEventHandler(this.serialPort_DataReceived);
            // 
            // btnConnect
            // 
            this.btnConnect.Location = new System.Drawing.Point(7, 8);
            this.btnConnect.Name = "btnConnect";
            this.btnConnect.Size = new System.Drawing.Size(75, 23);
            this.btnConnect.TabIndex = 0;
            this.btnConnect.Text = "Connect";
            this.btnConnect.UseVisualStyleBackColor = true;
            this.btnConnect.Click += new System.EventHandler(this.btnConnect_Click);
            // 
            // statusStrip1
            // 
            this.statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.statusText});
            this.statusStrip1.Location = new System.Drawing.Point(0, 544);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Size = new System.Drawing.Size(724, 22);
            this.statusStrip1.SizingGrip = false;
            this.statusStrip1.TabIndex = 1;
            this.statusStrip1.Text = "statusStrip";
            // 
            // statusText
            // 
            this.statusText.Name = "statusText";
            this.statusText.Size = new System.Drawing.Size(39, 17);
            this.statusText.Text = "Ready";
            // 
            // txtOutput
            // 
            this.txtOutput.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtOutput.BackColor = System.Drawing.SystemColors.Window;
            this.txtOutput.Location = new System.Drawing.Point(402, 37);
            this.txtOutput.Multiline = true;
            this.txtOutput.Name = "txtOutput";
            this.txtOutput.ReadOnly = true;
            this.txtOutput.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.txtOutput.Size = new System.Drawing.Size(315, 500);
            this.txtOutput.TabIndex = 3;
            this.txtOutput.WordWrap = false;
            // 
            // imgTarget
            // 
            this.imgTarget.BackColor = System.Drawing.Color.Moccasin;
            this.imgTarget.InitialImage = null;
            this.imgTarget.Location = new System.Drawing.Point(217, 37);
            this.imgTarget.Name = "imgTarget";
            this.imgTarget.Size = new System.Drawing.Size(500, 500);
            this.imgTarget.SizeMode = System.Windows.Forms.PictureBoxSizeMode.CenterImage;
            this.imgTarget.TabIndex = 4;
            this.imgTarget.TabStop = false;
            this.imgTarget.WaitOnLoad = true;
            this.imgTarget.Paint += new System.Windows.Forms.PaintEventHandler(this.imgTarget_Paint);
            // 
            // shotsList
            // 
            this.shotsList.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left)));
            this.shotsList.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.col4,
            this.col1,
            this.col2,
            this.col3});
            this.shotsList.FullRowSelect = true;
            this.shotsList.GridLines = true;
            this.shotsList.HideSelection = false;
            this.shotsList.Location = new System.Drawing.Point(7, 37);
            this.shotsList.MultiSelect = false;
            this.shotsList.Name = "shotsList";
            this.shotsList.Size = new System.Drawing.Size(204, 354);
            this.shotsList.SmallImageList = this.imgListDirections;
            this.shotsList.StateImageList = this.imgListDirections;
            this.shotsList.TabIndex = 5;
            this.shotsList.UseCompatibleStateImageBehavior = false;
            this.shotsList.View = System.Windows.Forms.View.Details;
            // 
            // col4
            // 
            this.col4.Text = "Direction";
            this.col4.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.col4.Width = 56;
            // 
            // col1
            // 
            this.col1.Text = "Shot";
            this.col1.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.col1.Width = 36;
            // 
            // col2
            // 
            this.col2.Text = "Score";
            this.col2.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.col2.Width = 40;
            // 
            // col3
            // 
            this.col3.Text = "Decimal";
            this.col3.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.col3.Width = 51;
            // 
            // imgListDirections
            // 
            this.imgListDirections.ColorDepth = System.Windows.Forms.ColorDepth.Depth8Bit;
            this.imgListDirections.ImageSize = new System.Drawing.Size(16, 16);
            this.imgListDirections.TransparentColor = System.Drawing.Color.Transparent;
            // 
            // txtTotal
            // 
            this.txtTotal.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.txtTotal.Location = new System.Drawing.Point(41, 435);
            this.txtTotal.Name = "txtTotal";
            this.txtTotal.ReadOnly = true;
            this.txtTotal.Size = new System.Drawing.Size(170, 20);
            this.txtTotal.TabIndex = 6;
            this.txtTotal.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            // 
            // label1
            // 
            this.label1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(4, 438);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(31, 13);
            this.label1.TabIndex = 7;
            this.label1.Text = "Total";
            // 
            // txtLastShot
            // 
            this.txtLastShot.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.txtLastShot.Font = new System.Drawing.Font("Microsoft Sans Serif", 15.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.txtLastShot.Location = new System.Drawing.Point(75, 397);
            this.txtLastShot.Name = "txtLastShot";
            this.txtLastShot.ReadOnly = true;
            this.txtLastShot.Size = new System.Drawing.Size(100, 31);
            this.txtLastShot.TabIndex = 8;
            this.txtLastShot.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            // 
            // label2
            // 
            this.label2.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(4, 406);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(52, 13);
            this.label2.TabIndex = 9;
            this.label2.Text = "Last Shot";
            // 
            // imgArrow
            // 
            this.imgArrow.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.imgArrow.BackColor = System.Drawing.Color.White;
            this.imgArrow.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.imgArrow.Image = ((System.Drawing.Image)(resources.GetObject("imgArrow.Image")));
            this.imgArrow.Location = new System.Drawing.Point(181, 397);
            this.imgArrow.Name = "imgArrow";
            this.imgArrow.Size = new System.Drawing.Size(30, 30);
            this.imgArrow.TabIndex = 10;
            this.imgArrow.TabStop = false;
            this.imgArrow.LoadCompleted += new System.ComponentModel.AsyncCompletedEventHandler(this.imgArrow_LoadCompleted);
            // 
            // btnConfig
            // 
            this.btnConfig.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnConfig.Image = ((System.Drawing.Image)(resources.GetObject("btnConfig.Image")));
            this.btnConfig.Location = new System.Drawing.Point(690, 4);
            this.btnConfig.Name = "btnConfig";
            this.btnConfig.Size = new System.Drawing.Size(27, 27);
            this.btnConfig.TabIndex = 11;
            this.toolTip.SetToolTip(this.btnConfig, "Settings");
            this.btnConfig.UseVisualStyleBackColor = true;
            this.btnConfig.Click += new System.EventHandler(this.btnConfig_Click);
            // 
            // cmbWeapon
            // 
            this.cmbWeapon.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbWeapon.Location = new System.Drawing.Point(217, 8);
            this.cmbWeapon.Name = "cmbWeapon";
            this.cmbWeapon.Size = new System.Drawing.Size(121, 21);
            this.cmbWeapon.TabIndex = 12;
            this.cmbWeapon.SelectedIndexChanged += new System.EventHandler(this.cmbWeapon_SelectedIndexChanged);
            // 
            // trkZoom
            // 
            this.trkZoom.LargeChange = 1;
            this.trkZoom.Location = new System.Drawing.Point(467, 6);
            this.trkZoom.Maximum = 5;
            this.trkZoom.Name = "trkZoom";
            this.trkZoom.Size = new System.Drawing.Size(104, 45);
            this.trkZoom.TabIndex = 15;
            this.trkZoom.ValueChanged += new System.EventHandler(this.trkZoom_ValueChanged);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(427, 9);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(34, 13);
            this.label3.TabIndex = 16;
            this.label3.Text = "Zoom";
            // 
            // btnClear
            // 
            this.btnClear.Location = new System.Drawing.Point(89, 8);
            this.btnClear.Name = "btnClear";
            this.btnClear.Size = new System.Drawing.Size(75, 23);
            this.btnClear.TabIndex = 17;
            this.btnClear.Text = "Clear";
            this.btnClear.UseVisualStyleBackColor = true;
            this.btnClear.Click += new System.EventHandler(this.btnClear_Click);
            // 
            // txtTime
            // 
            this.txtTime.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.txtTime.Location = new System.Drawing.Point(41, 515);
            this.txtTime.Name = "txtTime";
            this.txtTime.ReadOnly = true;
            this.txtTime.Size = new System.Drawing.Size(170, 20);
            this.txtTime.TabIndex = 18;
            this.txtTime.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            // 
            // label4
            // 
            this.label4.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(4, 518);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(30, 13);
            this.label4.TabIndex = 19;
            this.label4.Text = "Time";
            // 
            // timer
            // 
            this.timer.Tick += new System.EventHandler(this.timer_Tick);
            // 
            // txtMeanRadius
            // 
            this.txtMeanRadius.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.txtMeanRadius.Location = new System.Drawing.Point(175, 488);
            this.txtMeanRadius.Name = "txtMeanRadius";
            this.txtMeanRadius.ReadOnly = true;
            this.txtMeanRadius.Size = new System.Drawing.Size(36, 20);
            this.txtMeanRadius.TabIndex = 20;
            this.txtMeanRadius.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            // 
            // label5
            // 
            this.label5.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(105, 491);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(70, 13);
            this.label5.TabIndex = 21;
            this.label5.Text = "Mean Radius";
            // 
            // txtMaxSpread
            // 
            this.txtMaxSpread.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.txtMaxSpread.Location = new System.Drawing.Point(175, 462);
            this.txtMaxSpread.Name = "txtMaxSpread";
            this.txtMaxSpread.ReadOnly = true;
            this.txtMaxSpread.Size = new System.Drawing.Size(36, 20);
            this.txtMaxSpread.TabIndex = 22;
            this.txtMaxSpread.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            // 
            // label6
            // 
            this.label6.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(111, 465);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(64, 13);
            this.label6.TabIndex = 23;
            this.label6.Text = "Max Spread";
            // 
            // label7
            // 
            this.label7.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(4, 465);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(50, 13);
            this.label7.TabIndex = 24;
            this.label7.Text = "Windage";
            // 
            // label8
            // 
            this.label8.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(4, 491);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(51, 13);
            this.label8.TabIndex = 25;
            this.label8.Text = "Elevation";
            // 
            // txtWindage
            // 
            this.txtWindage.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.txtWindage.Location = new System.Drawing.Point(53, 462);
            this.txtWindage.Name = "txtWindage";
            this.txtWindage.ReadOnly = true;
            this.txtWindage.Size = new System.Drawing.Size(29, 20);
            this.txtWindage.TabIndex = 26;
            this.txtWindage.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            // 
            // txtElevation
            // 
            this.txtElevation.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.txtElevation.Location = new System.Drawing.Point(53, 488);
            this.txtElevation.Name = "txtElevation";
            this.txtElevation.ReadOnly = true;
            this.txtElevation.Size = new System.Drawing.Size(29, 20);
            this.txtElevation.TabIndex = 27;
            this.txtElevation.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            // 
            // imgWindage
            // 
            this.imgWindage.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.imgWindage.Location = new System.Drawing.Point(82, 462);
            this.imgWindage.Name = "imgWindage";
            this.imgWindage.Size = new System.Drawing.Size(20, 20);
            this.imgWindage.TabIndex = 28;
            this.imgWindage.TabStop = false;
            // 
            // imgElevation
            // 
            this.imgElevation.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.imgElevation.Location = new System.Drawing.Point(82, 488);
            this.imgElevation.Name = "imgElevation";
            this.imgElevation.Size = new System.Drawing.Size(20, 20);
            this.imgElevation.TabIndex = 29;
            this.imgElevation.TabStop = false;
            // 
            // btnCalibration
            // 
            this.btnCalibration.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCalibration.Image = ((System.Drawing.Image)(resources.GetObject("btnCalibration.Image")));
            this.btnCalibration.Location = new System.Drawing.Point(657, 4);
            this.btnCalibration.Name = "btnCalibration";
            this.btnCalibration.Size = new System.Drawing.Size(27, 27);
            this.btnCalibration.TabIndex = 30;
            this.toolTip.SetToolTip(this.btnCalibration, "Calibration");
            this.btnCalibration.UseVisualStyleBackColor = true;
            this.btnCalibration.Click += new System.EventHandler(this.btnCalibration_Click);
            // 
            // frmMainWindow
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(724, 566);
            this.Controls.Add(this.btnCalibration);
            this.Controls.Add(this.imgElevation);
            this.Controls.Add(this.imgWindage);
            this.Controls.Add(this.txtElevation);
            this.Controls.Add(this.txtWindage);
            this.Controls.Add(this.label8);
            this.Controls.Add(this.label7);
            this.Controls.Add(this.label6);
            this.Controls.Add(this.txtMaxSpread);
            this.Controls.Add(this.label5);
            this.Controls.Add(this.txtMeanRadius);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.txtTime);
            this.Controls.Add(this.btnClear);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.cmbWeapon);
            this.Controls.Add(this.btnConfig);
            this.Controls.Add(this.imgArrow);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.txtLastShot);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.txtTotal);
            this.Controls.Add(this.shotsList);
            this.Controls.Add(this.imgTarget);
            this.Controls.Add(this.txtOutput);
            this.Controls.Add(this.statusStrip1);
            this.Controls.Add(this.btnConnect);
            this.Controls.Add(this.trkZoom);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MinimumSize = new System.Drawing.Size(740, 604);
            this.Name = "frmMainWindow";
            this.Text = "freETarget";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.frmMainWindow_FormClosing);
            this.Load += new System.EventHandler(this.frmMainWindow_Load);
            this.Shown += new System.EventHandler(this.frmMainWindow_Shown);
            this.Resize += new System.EventHandler(this.frmMainWindow_Resize);
            this.statusStrip1.ResumeLayout(false);
            this.statusStrip1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.imgTarget)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.imgArrow)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trkZoom)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.imgWindage)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.imgElevation)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.IO.Ports.SerialPort serialPort;
        private System.Windows.Forms.Button btnConnect;
        private System.Windows.Forms.StatusStrip statusStrip1;
        private System.Windows.Forms.ToolStripStatusLabel statusText;
        public System.Windows.Forms.TextBox txtOutput;
        private System.Windows.Forms.PictureBox imgTarget;
        private System.Windows.Forms.ListView shotsList;
        private System.Windows.Forms.ColumnHeader col1;
        private System.Windows.Forms.ColumnHeader col2;
        private System.Windows.Forms.ColumnHeader col3;
        private System.Windows.Forms.ColumnHeader col4;
        private System.Windows.Forms.TextBox txtTotal;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox txtLastShot;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.PictureBox imgArrow;
        private System.Windows.Forms.ImageList imgListDirections;
        private System.Windows.Forms.Button btnConfig;
        private System.Windows.Forms.ComboBox cmbWeapon;
        private System.Windows.Forms.TrackBar trkZoom;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Button btnClear;
        private System.Windows.Forms.TextBox txtTime;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Timer timer;
        private System.Windows.Forms.TextBox txtMeanRadius;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.TextBox txtMaxSpread;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.TextBox txtWindage;
        private System.Windows.Forms.TextBox txtElevation;
        private System.Windows.Forms.PictureBox imgWindage;
        private System.Windows.Forms.PictureBox imgElevation;
        private System.Windows.Forms.ToolTip toolTip;
        private System.Windows.Forms.Button btnCalibration;
    }
}

