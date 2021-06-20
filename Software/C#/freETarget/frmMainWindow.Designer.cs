using System.Windows.Forms.DataVisualization.Charting;

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
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle1 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle3 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle2 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea1 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Legend legend1 = new System.Windows.Forms.DataVisualization.Charting.Legend();
            System.Windows.Forms.DataVisualization.Charting.Series series1 = new System.Windows.Forms.DataVisualization.Charting.Series();
            this.serialPort = new System.IO.Ports.SerialPort(this.components);
            this.btnConnect = new System.Windows.Forms.Button();
            this.imgListIcons = new System.Windows.Forms.ImageList(this.components);
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this.statusText = new System.Windows.Forms.ToolStripStatusLabel();
            this.toolStripStatusLabel1 = new System.Windows.Forms.ToolStripStatusLabel();
            this.statusVersion = new System.Windows.Forms.ToolStripStatusLabel();
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
            this.btnArduino = new System.Windows.Forms.Button();
            this.btnUpload = new System.Windows.Forms.Button();
            this.imgSessionName = new System.Windows.Forms.PictureBox();
            this.gridTargets = new System.Windows.Forms.DataGridView();
            this.column1 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Column2 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Column3 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Column4 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Column5 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Column6 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Column7 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Column8 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Column9 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Column10 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.ColumnTotal = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.chartBreakdown = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.tcSessionType = new System.Windows.Forms.TabControl();
            this.tabPistolPractice = new System.Windows.Forms.TabPage();
            this.tabPistolMatch = new System.Windows.Forms.TabPage();
            this.tabPistolFinal = new System.Windows.Forms.TabPage();
            this.tabRiflePractice = new System.Windows.Forms.TabPage();
            this.tabRifleMatch = new System.Windows.Forms.TabPage();
            this.tabRifleFinal = new System.Windows.Forms.TabPage();
            this.panel1 = new System.Windows.Forms.Panel();
            this.btnJournal = new System.Windows.Forms.Button();
            this.imgLogo = new System.Windows.Forms.PictureBox();
            this.digitalClock = new freETarget.SevenSegmentArray();
            this.statusStrip1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.imgTarget)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.imgArrow)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trkZoom)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.imgWindage)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.imgElevation)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.imgSessionName)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.gridTargets)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.chartBreakdown)).BeginInit();
            this.tcSessionType.SuspendLayout();
            this.panel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.imgLogo)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.digitalClock)).BeginInit();
            this.SuspendLayout();
            // 
            // serialPort
            // 
            this.serialPort.DataReceived += new System.IO.Ports.SerialDataReceivedEventHandler(this.serialPort_DataReceived);
            // 
            // btnConnect
            // 
            this.btnConnect.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.btnConnect.ImageIndex = 0;
            this.btnConnect.ImageList = this.imgListIcons;
            this.btnConnect.Location = new System.Drawing.Point(7, 6);
            this.btnConnect.Name = "btnConnect";
            this.btnConnect.Size = new System.Drawing.Size(100, 25);
            this.btnConnect.TabIndex = 0;
            this.btnConnect.Text = "Connect";
            this.btnConnect.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.btnConnect.UseVisualStyleBackColor = true;
            this.btnConnect.Click += new System.EventHandler(this.btnConnect_Click);
            // 
            // imgListIcons
            // 
            this.imgListIcons.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imgListIcons.ImageStream")));
            this.imgListIcons.TransparentColor = System.Drawing.Color.Transparent;
            this.imgListIcons.Images.SetKeyName(0, "connect");
            this.imgListIcons.Images.SetKeyName(1, "disconnect");
            this.imgListIcons.Images.SetKeyName(2, "book");
            // 
            // statusStrip1
            // 
            this.statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.statusText,
            this.toolStripStatusLabel1,
            this.statusVersion});
            this.statusStrip1.Location = new System.Drawing.Point(0, 676);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Size = new System.Drawing.Size(1185, 24);
            this.statusStrip1.SizingGrip = false;
            this.statusStrip1.TabIndex = 1;
            this.statusStrip1.Text = "statusStrip";
            // 
            // statusText
            // 
            this.statusText.Name = "statusText";
            this.statusText.Size = new System.Drawing.Size(39, 19);
            this.statusText.Text = "Ready";
            // 
            // toolStripStatusLabel1
            // 
            this.toolStripStatusLabel1.Name = "toolStripStatusLabel1";
            this.toolStripStatusLabel1.Size = new System.Drawing.Size(1090, 19);
            this.toolStripStatusLabel1.Spring = true;
            // 
            // statusVersion
            // 
            this.statusVersion.BorderSides = ((System.Windows.Forms.ToolStripStatusLabelBorderSides)((((System.Windows.Forms.ToolStripStatusLabelBorderSides.Left | System.Windows.Forms.ToolStripStatusLabelBorderSides.Top) 
            | System.Windows.Forms.ToolStripStatusLabelBorderSides.Right) 
            | System.Windows.Forms.ToolStripStatusLabelBorderSides.Bottom)));
            this.statusVersion.BorderStyle = System.Windows.Forms.Border3DStyle.Etched;
            this.statusVersion.Name = "statusVersion";
            this.statusVersion.Size = new System.Drawing.Size(41, 19);
            this.statusVersion.Text = "v1.0.0";
            // 
            // txtOutput
            // 
            this.txtOutput.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.txtOutput.BackColor = System.Drawing.SystemColors.Window;
            this.txtOutput.Location = new System.Drawing.Point(433, 37);
            this.txtOutput.Multiline = true;
            this.txtOutput.Name = "txtOutput";
            this.txtOutput.ReadOnly = true;
            this.txtOutput.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.txtOutput.Size = new System.Drawing.Size(328, 634);
            this.txtOutput.TabIndex = 3;
            this.txtOutput.WordWrap = false;
            // 
            // imgTarget
            // 
            this.imgTarget.BackColor = System.Drawing.Color.Transparent;
            this.imgTarget.InitialImage = null;
            this.imgTarget.Location = new System.Drawing.Point(216, 37);
            this.imgTarget.Name = "imgTarget";
            this.imgTarget.Size = new System.Drawing.Size(501, 500);
            this.imgTarget.SizeMode = System.Windows.Forms.PictureBoxSizeMode.CenterImage;
            this.imgTarget.TabIndex = 4;
            this.imgTarget.TabStop = false;
            this.imgTarget.WaitOnLoad = true;
            this.imgTarget.Click += new System.EventHandler(this.imgTarget_Click);
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
            this.shotsList.Enabled = false;
            this.shotsList.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.shotsList.FullRowSelect = true;
            this.shotsList.GridLines = true;
            this.shotsList.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.shotsList.HideSelection = false;
            this.shotsList.Location = new System.Drawing.Point(7, 37);
            this.shotsList.MultiSelect = false;
            this.shotsList.Name = "shotsList";
            this.shotsList.Size = new System.Drawing.Size(200, 483);
            this.shotsList.SmallImageList = this.imgListDirections;
            this.shotsList.StateImageList = this.imgListDirections;
            this.shotsList.TabIndex = 5;
            this.shotsList.UseCompatibleStateImageBehavior = false;
            this.shotsList.View = System.Windows.Forms.View.Details;
            // 
            // col4
            // 
            this.col4.Text = "Dir";
            this.col4.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.col4.Width = 40;
            // 
            // col1
            // 
            this.col1.Text = "Nr";
            this.col1.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.col1.Width = 35;
            // 
            // col2
            // 
            this.col2.Text = "Score";
            this.col2.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.col2.Width = 48;
            // 
            // col3
            // 
            this.col3.Text = "Deci";
            this.col3.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            this.col3.Width = 55;
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
            this.txtTotal.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.txtTotal.Location = new System.Drawing.Point(37, 567);
            this.txtTotal.Name = "txtTotal";
            this.txtTotal.ReadOnly = true;
            this.txtTotal.Size = new System.Drawing.Size(170, 26);
            this.txtTotal.TabIndex = 6;
            this.txtTotal.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            // 
            // label1
            // 
            this.label1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(2, 574);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(31, 13);
            this.label1.TabIndex = 7;
            this.label1.Text = "Total";
            // 
            // txtLastShot
            // 
            this.txtLastShot.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.txtLastShot.Font = new System.Drawing.Font("Microsoft Sans Serif", 20.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.txtLastShot.Location = new System.Drawing.Point(61, 526);
            this.txtLastShot.Name = "txtLastShot";
            this.txtLastShot.ReadOnly = true;
            this.txtLastShot.Size = new System.Drawing.Size(102, 38);
            this.txtLastShot.TabIndex = 8;
            this.txtLastShot.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            // 
            // label2
            // 
            this.label2.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(4, 539);
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
            this.imgArrow.Location = new System.Drawing.Point(169, 526);
            this.imgArrow.Name = "imgArrow";
            this.imgArrow.Size = new System.Drawing.Size(38, 38);
            this.imgArrow.TabIndex = 10;
            this.imgArrow.TabStop = false;
            this.imgArrow.LoadCompleted += new System.ComponentModel.AsyncCompletedEventHandler(this.imgArrow_LoadCompleted);
            // 
            // btnConfig
            // 
            this.btnConfig.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnConfig.Image = ((System.Drawing.Image)(resources.GetObject("btnConfig.Image")));
            this.btnConfig.Location = new System.Drawing.Point(1151, 4);
            this.btnConfig.Name = "btnConfig";
            this.btnConfig.Size = new System.Drawing.Size(27, 27);
            this.btnConfig.TabIndex = 11;
            this.toolTip.SetToolTip(this.btnConfig, "Settings");
            this.btnConfig.UseVisualStyleBackColor = true;
            this.btnConfig.Click += new System.EventHandler(this.btnConfig_Click);
            // 
            // trkZoom
            // 
            this.trkZoom.Enabled = false;
            this.trkZoom.LargeChange = 1;
            this.trkZoom.Location = new System.Drawing.Point(257, 8);
            this.trkZoom.Maximum = 5;
            this.trkZoom.Name = "trkZoom";
            this.trkZoom.Size = new System.Drawing.Size(104, 45);
            this.trkZoom.TabIndex = 15;
            this.trkZoom.ValueChanged += new System.EventHandler(this.trkZoom_ValueChanged);
            this.trkZoom.MouseWheel += new System.Windows.Forms.MouseEventHandler(this.mouseWheel);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(217, 11);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(34, 13);
            this.label3.TabIndex = 16;
            this.label3.Text = "Zoom";
            // 
            // btnClear
            // 
            this.btnClear.Location = new System.Drawing.Point(652, 8);
            this.btnClear.Name = "btnClear";
            this.btnClear.Size = new System.Drawing.Size(75, 23);
            this.btnClear.TabIndex = 17;
            this.btnClear.Text = "Clear";
            this.btnClear.UseVisualStyleBackColor = true;
            this.btnClear.Visible = false;
            this.btnClear.Click += new System.EventHandler(this.btnClear_Click);
            // 
            // txtTime
            // 
            this.txtTime.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.txtTime.BackColor = System.Drawing.SystemColors.Control;
            this.txtTime.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.txtTime.ForeColor = System.Drawing.SystemColors.WindowText;
            this.txtTime.Location = new System.Drawing.Point(33, 647);
            this.txtTime.Name = "txtTime";
            this.txtTime.ReadOnly = true;
            this.txtTime.Size = new System.Drawing.Size(174, 26);
            this.txtTime.TabIndex = 18;
            this.txtTime.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            // 
            // label4
            // 
            this.label4.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(3, 654);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(30, 13);
            this.label4.TabIndex = 19;
            this.label4.Text = "Time";
            // 
            // timer
            // 
            this.timer.Interval = 500;
            this.timer.Tick += new System.EventHandler(this.timer_Tick);
            // 
            // txtMeanRadius
            // 
            this.txtMeanRadius.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.txtMeanRadius.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.txtMeanRadius.Location = new System.Drawing.Point(171, 622);
            this.txtMeanRadius.Name = "txtMeanRadius";
            this.txtMeanRadius.ReadOnly = true;
            this.txtMeanRadius.Size = new System.Drawing.Size(36, 22);
            this.txtMeanRadius.TabIndex = 20;
            this.txtMeanRadius.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            // 
            // label5
            // 
            this.label5.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(101, 627);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(70, 13);
            this.label5.TabIndex = 21;
            this.label5.Text = "Mean Radius";
            // 
            // txtMaxSpread
            // 
            this.txtMaxSpread.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.txtMaxSpread.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.txtMaxSpread.Location = new System.Drawing.Point(171, 596);
            this.txtMaxSpread.Name = "txtMaxSpread";
            this.txtMaxSpread.ReadOnly = true;
            this.txtMaxSpread.Size = new System.Drawing.Size(36, 22);
            this.txtMaxSpread.TabIndex = 22;
            this.txtMaxSpread.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            // 
            // label6
            // 
            this.label6.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(107, 601);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(64, 13);
            this.label6.TabIndex = 23;
            this.label6.Text = "Max Spread";
            // 
            // label7
            // 
            this.label7.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(3, 601);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(50, 13);
            this.label7.TabIndex = 24;
            this.label7.Text = "Windage";
            // 
            // label8
            // 
            this.label8.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(3, 627);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(51, 13);
            this.label8.TabIndex = 25;
            this.label8.Text = "Elevation";
            // 
            // txtWindage
            // 
            this.txtWindage.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.txtWindage.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.txtWindage.Location = new System.Drawing.Point(52, 596);
            this.txtWindage.Name = "txtWindage";
            this.txtWindage.ReadOnly = true;
            this.txtWindage.Size = new System.Drawing.Size(29, 22);
            this.txtWindage.TabIndex = 26;
            this.txtWindage.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            // 
            // txtElevation
            // 
            this.txtElevation.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.txtElevation.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.txtElevation.Location = new System.Drawing.Point(52, 622);
            this.txtElevation.Name = "txtElevation";
            this.txtElevation.ReadOnly = true;
            this.txtElevation.Size = new System.Drawing.Size(29, 22);
            this.txtElevation.TabIndex = 27;
            this.txtElevation.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            // 
            // imgWindage
            // 
            this.imgWindage.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.imgWindage.Location = new System.Drawing.Point(81, 597);
            this.imgWindage.Name = "imgWindage";
            this.imgWindage.Size = new System.Drawing.Size(20, 20);
            this.imgWindage.TabIndex = 28;
            this.imgWindage.TabStop = false;
            // 
            // imgElevation
            // 
            this.imgElevation.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.imgElevation.Location = new System.Drawing.Point(81, 623);
            this.imgElevation.Name = "imgElevation";
            this.imgElevation.Size = new System.Drawing.Size(20, 20);
            this.imgElevation.TabIndex = 29;
            this.imgElevation.TabStop = false;
            // 
            // btnCalibration
            // 
            this.btnCalibration.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCalibration.Enabled = false;
            this.btnCalibration.Image = ((System.Drawing.Image)(resources.GetObject("btnCalibration.Image")));
            this.btnCalibration.Location = new System.Drawing.Point(1052, 4);
            this.btnCalibration.Name = "btnCalibration";
            this.btnCalibration.Size = new System.Drawing.Size(27, 27);
            this.btnCalibration.TabIndex = 30;
            this.toolTip.SetToolTip(this.btnCalibration, "Calibration");
            this.btnCalibration.UseVisualStyleBackColor = true;
            this.btnCalibration.Click += new System.EventHandler(this.btnCalibration_Click);
            // 
            // btnArduino
            // 
            this.btnArduino.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnArduino.Enabled = false;
            this.btnArduino.Image = ((System.Drawing.Image)(resources.GetObject("btnArduino.Image")));
            this.btnArduino.Location = new System.Drawing.Point(1118, 4);
            this.btnArduino.Name = "btnArduino";
            this.btnArduino.Size = new System.Drawing.Size(27, 27);
            this.btnArduino.TabIndex = 38;
            this.toolTip.SetToolTip(this.btnArduino, "Arduino");
            this.btnArduino.UseVisualStyleBackColor = true;
            this.btnArduino.Click += new System.EventHandler(this.btnArduino_Click);
            // 
            // btnUpload
            // 
            this.btnUpload.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnUpload.Image = ((System.Drawing.Image)(resources.GetObject("btnUpload.Image")));
            this.btnUpload.Location = new System.Drawing.Point(1085, 4);
            this.btnUpload.Name = "btnUpload";
            this.btnUpload.Size = new System.Drawing.Size(27, 27);
            this.btnUpload.TabIndex = 39;
            this.toolTip.SetToolTip(this.btnUpload, "Firmware upload");
            this.btnUpload.UseVisualStyleBackColor = true;
            this.btnUpload.Click += new System.EventHandler(this.btnUpload_Click);
            // 
            // imgSessionName
            // 
            this.imgSessionName.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.imgSessionName.BackColor = System.Drawing.Color.Black;
            this.imgSessionName.Location = new System.Drawing.Point(762, 37);
            this.imgSessionName.Name = "imgSessionName";
            this.imgSessionName.Size = new System.Drawing.Size(395, 19);
            this.imgSessionName.TabIndex = 1;
            this.imgSessionName.TabStop = false;
            this.imgSessionName.Click += new System.EventHandler(this.imgSessionName_Click);
            // 
            // gridTargets
            // 
            this.gridTargets.AllowUserToAddRows = false;
            this.gridTargets.AllowUserToDeleteRows = false;
            this.gridTargets.AllowUserToResizeColumns = false;
            this.gridTargets.AllowUserToResizeRows = false;
            this.gridTargets.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Right)));
            dataGridViewCellStyle1.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleCenter;
            dataGridViewCellStyle1.BackColor = System.Drawing.SystemColors.Control;
            dataGridViewCellStyle1.Font = new System.Drawing.Font("Microsoft Sans Serif", 7F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            dataGridViewCellStyle1.ForeColor = System.Drawing.SystemColors.WindowText;
            dataGridViewCellStyle1.SelectionBackColor = System.Drawing.SystemColors.Control;
            dataGridViewCellStyle1.SelectionForeColor = System.Drawing.SystemColors.WindowText;
            dataGridViewCellStyle1.WrapMode = System.Windows.Forms.DataGridViewTriState.True;
            this.gridTargets.ColumnHeadersDefaultCellStyle = dataGridViewCellStyle1;
            this.gridTargets.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.DisableResizing;
            this.gridTargets.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.column1,
            this.Column2,
            this.Column3,
            this.Column4,
            this.Column5,
            this.Column6,
            this.Column7,
            this.Column8,
            this.Column9,
            this.Column10,
            this.ColumnTotal});
            this.gridTargets.Location = new System.Drawing.Point(762, 132);
            this.gridTargets.MultiSelect = false;
            this.gridTargets.Name = "gridTargets";
            this.gridTargets.ReadOnly = true;
            dataGridViewCellStyle3.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle3.BackColor = System.Drawing.SystemColors.Control;
            dataGridViewCellStyle3.Font = new System.Drawing.Font("Microsoft Sans Serif", 7.5F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            dataGridViewCellStyle3.ForeColor = System.Drawing.SystemColors.WindowText;
            dataGridViewCellStyle3.SelectionBackColor = System.Drawing.SystemColors.Highlight;
            dataGridViewCellStyle3.SelectionForeColor = System.Drawing.SystemColors.HighlightText;
            this.gridTargets.RowHeadersDefaultCellStyle = dataGridViewCellStyle3;
            this.gridTargets.RowHeadersWidth = 52;
            this.gridTargets.RowHeadersWidthSizeMode = System.Windows.Forms.DataGridViewRowHeadersWidthSizeMode.DisableResizing;
            this.gridTargets.RowTemplate.DefaultCellStyle.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleCenter;
            this.gridTargets.RowTemplate.ReadOnly = true;
            this.gridTargets.RowTemplate.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.gridTargets.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.FullRowSelect;
            this.gridTargets.Size = new System.Drawing.Size(395, 385);
            this.gridTargets.TabIndex = 0;
            this.gridTargets.Click += new System.EventHandler(this.gridTargets_Click);
            // 
            // column1
            // 
            this.column1.HeaderText = "1";
            this.column1.Name = "column1";
            this.column1.ReadOnly = true;
            this.column1.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.column1.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.column1.Width = 30;
            // 
            // Column2
            // 
            this.Column2.HeaderText = "2";
            this.Column2.Name = "Column2";
            this.Column2.ReadOnly = true;
            this.Column2.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.Column2.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.Column2.Width = 30;
            // 
            // Column3
            // 
            this.Column3.HeaderText = "3";
            this.Column3.Name = "Column3";
            this.Column3.ReadOnly = true;
            this.Column3.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.Column3.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.Column3.Width = 30;
            // 
            // Column4
            // 
            this.Column4.HeaderText = "4";
            this.Column4.Name = "Column4";
            this.Column4.ReadOnly = true;
            this.Column4.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.Column4.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.Column4.Width = 30;
            // 
            // Column5
            // 
            this.Column5.HeaderText = "5";
            this.Column5.Name = "Column5";
            this.Column5.ReadOnly = true;
            this.Column5.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.Column5.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.Column5.Width = 30;
            // 
            // Column6
            // 
            this.Column6.HeaderText = "6";
            this.Column6.Name = "Column6";
            this.Column6.ReadOnly = true;
            this.Column6.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.Column6.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.Column6.Width = 30;
            // 
            // Column7
            // 
            this.Column7.HeaderText = "7";
            this.Column7.Name = "Column7";
            this.Column7.ReadOnly = true;
            this.Column7.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.Column7.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.Column7.Width = 30;
            // 
            // Column8
            // 
            this.Column8.HeaderText = "8";
            this.Column8.Name = "Column8";
            this.Column8.ReadOnly = true;
            this.Column8.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.Column8.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.Column8.Width = 30;
            // 
            // Column9
            // 
            this.Column9.HeaderText = "9";
            this.Column9.Name = "Column9";
            this.Column9.ReadOnly = true;
            this.Column9.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.Column9.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.Column9.Width = 30;
            // 
            // Column10
            // 
            this.Column10.HeaderText = "10";
            this.Column10.Name = "Column10";
            this.Column10.ReadOnly = true;
            this.Column10.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.Column10.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.Column10.Width = 30;
            // 
            // ColumnTotal
            // 
            dataGridViewCellStyle2.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.ColumnTotal.DefaultCellStyle = dataGridViewCellStyle2;
            this.ColumnTotal.HeaderText = "Total";
            this.ColumnTotal.Name = "ColumnTotal";
            this.ColumnTotal.ReadOnly = true;
            this.ColumnTotal.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.ColumnTotal.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.ColumnTotal.Width = 41;
            // 
            // chartBreakdown
            // 
            this.chartBreakdown.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.chartBreakdown.BackColor = System.Drawing.SystemColors.ControlDark;
            this.chartBreakdown.BorderlineColor = System.Drawing.Color.Black;
            chartArea1.AxisX.Interval = 1D;
            chartArea1.AxisX.LineWidth = 0;
            chartArea1.AxisX.MajorGrid.LineWidth = 0;
            chartArea1.AxisX.MajorTickMark.Enabled = false;
            chartArea1.AxisY.Enabled = System.Windows.Forms.DataVisualization.Charting.AxisEnabled.False;
            chartArea1.AxisY.MajorGrid.LineWidth = 0;
            chartArea1.BackColor = System.Drawing.SystemColors.ControlDark;
            chartArea1.Name = "ChartArea1";
            chartArea1.Position.Auto = false;
            chartArea1.Position.Height = 100F;
            chartArea1.Position.Width = 100F;
            this.chartBreakdown.ChartAreas.Add(chartArea1);
            legend1.Name = "Legend1";
            this.chartBreakdown.Legends.Add(legend1);
            this.chartBreakdown.Location = new System.Drawing.Point(-1, -2);
            this.chartBreakdown.Name = "chartBreakdown";
            this.chartBreakdown.Palette = System.Windows.Forms.DataVisualization.Charting.ChartColorPalette.None;
            this.chartBreakdown.PaletteCustomColors = new System.Drawing.Color[] {
        System.Drawing.Color.Blue};
            series1.ChartArea = "ChartArea1";
            series1.CustomProperties = "LabelStyle=Bottom";
            series1.IsValueShownAsLabel = true;
            series1.IsVisibleInLegend = false;
            series1.LabelForeColor = System.Drawing.Color.White;
            series1.Legend = "Legend1";
            series1.Name = "Series1";
            series1.SmartLabelStyle.AllowOutsidePlotArea = System.Windows.Forms.DataVisualization.Charting.LabelOutsidePlotAreaStyle.No;
            this.chartBreakdown.Series.Add(series1);
            this.chartBreakdown.Size = new System.Drawing.Size(395, 148);
            this.chartBreakdown.TabIndex = 4;
            this.chartBreakdown.Text = "chart1";
            // 
            // tcSessionType
            // 
            this.tcSessionType.Alignment = System.Windows.Forms.TabAlignment.Left;
            this.tcSessionType.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.tcSessionType.Controls.Add(this.tabPistolPractice);
            this.tcSessionType.Controls.Add(this.tabPistolMatch);
            this.tcSessionType.Controls.Add(this.tabPistolFinal);
            this.tcSessionType.Controls.Add(this.tabRiflePractice);
            this.tcSessionType.Controls.Add(this.tabRifleMatch);
            this.tcSessionType.Controls.Add(this.tabRifleFinal);
            this.tcSessionType.DrawMode = System.Windows.Forms.TabDrawMode.OwnerDrawFixed;
            this.tcSessionType.Enabled = false;
            this.tcSessionType.Font = new System.Drawing.Font("Microsoft Sans Serif", 11.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.tcSessionType.Location = new System.Drawing.Point(1163, 37);
            this.tcSessionType.Multiline = true;
            this.tcSessionType.Name = "tcSessionType";
            this.tcSessionType.SelectedIndex = 0;
            this.tcSessionType.Size = new System.Drawing.Size(200, 530);
            this.tcSessionType.TabIndex = 32;
            this.tcSessionType.DrawItem += new System.Windows.Forms.DrawItemEventHandler(this.tabControl1_DrawItem);
            this.tcSessionType.SelectedIndexChanged += new System.EventHandler(this.tcSessionType_SelectedIndexChanged);
            // 
            // tabPistolPractice
            // 
            this.tabPistolPractice.BackColor = System.Drawing.Color.Gold;
            this.tabPistolPractice.Location = new System.Drawing.Point(52, 4);
            this.tabPistolPractice.Name = "tabPistolPractice";
            this.tabPistolPractice.Padding = new System.Windows.Forms.Padding(3);
            this.tabPistolPractice.Size = new System.Drawing.Size(144, 522);
            this.tabPistolPractice.TabIndex = 0;
            this.tabPistolPractice.Text = "Pistol Practice";
            // 
            // tabPistolMatch
            // 
            this.tabPistolMatch.BackColor = System.Drawing.Color.Orange;
            this.tabPistolMatch.Location = new System.Drawing.Point(52, 4);
            this.tabPistolMatch.Name = "tabPistolMatch";
            this.tabPistolMatch.Padding = new System.Windows.Forms.Padding(3);
            this.tabPistolMatch.Size = new System.Drawing.Size(144, 522);
            this.tabPistolMatch.TabIndex = 1;
            this.tabPistolMatch.Text = "Pistol Match ";
            // 
            // tabPistolFinal
            // 
            this.tabPistolFinal.BackColor = System.Drawing.Color.Red;
            this.tabPistolFinal.Location = new System.Drawing.Point(52, 4);
            this.tabPistolFinal.Name = "tabPistolFinal";
            this.tabPistolFinal.Size = new System.Drawing.Size(144, 522);
            this.tabPistolFinal.TabIndex = 2;
            this.tabPistolFinal.Text = "Pistol Final";
            // 
            // tabRiflePractice
            // 
            this.tabRiflePractice.BackColor = System.Drawing.Color.LimeGreen;
            this.tabRiflePractice.Location = new System.Drawing.Point(52, 4);
            this.tabRiflePractice.Name = "tabRiflePractice";
            this.tabRiflePractice.Size = new System.Drawing.Size(144, 522);
            this.tabRiflePractice.TabIndex = 3;
            this.tabRiflePractice.Text = "Rifle Practice";
            // 
            // tabRifleMatch
            // 
            this.tabRifleMatch.BackColor = System.Drawing.Color.Turquoise;
            this.tabRifleMatch.Location = new System.Drawing.Point(52, 4);
            this.tabRifleMatch.Name = "tabRifleMatch";
            this.tabRifleMatch.Size = new System.Drawing.Size(144, 522);
            this.tabRifleMatch.TabIndex = 4;
            this.tabRifleMatch.Text = "Rifle Match ";
            // 
            // tabRifleFinal
            // 
            this.tabRifleFinal.BackColor = System.Drawing.Color.DodgerBlue;
            this.tabRifleFinal.Location = new System.Drawing.Point(52, 4);
            this.tabRifleFinal.Name = "tabRifleFinal";
            this.tabRifleFinal.Size = new System.Drawing.Size(144, 522);
            this.tabRifleFinal.TabIndex = 5;
            this.tabRifleFinal.Text = "Rifle Final ";
            // 
            // panel1
            // 
            this.panel1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.panel1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.panel1.Controls.Add(this.chartBreakdown);
            this.panel1.Location = new System.Drawing.Point(762, 519);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(395, 148);
            this.panel1.TabIndex = 33;
            // 
            // btnJournal
            // 
            this.btnJournal.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.btnJournal.ImageIndex = 2;
            this.btnJournal.ImageList = this.imgListIcons;
            this.btnJournal.Location = new System.Drawing.Point(107, 6);
            this.btnJournal.Name = "btnJournal";
            this.btnJournal.Size = new System.Drawing.Size(100, 25);
            this.btnJournal.TabIndex = 36;
            this.btnJournal.Text = "Journal";
            this.btnJournal.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.btnJournal.UseVisualStyleBackColor = true;
            this.btnJournal.Click += new System.EventHandler(this.btnJournal_Click);
            // 
            // imgLogo
            // 
            this.imgLogo.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.imgLogo.BackColor = System.Drawing.Color.Transparent;
            this.imgLogo.Image = ((System.Drawing.Image)(resources.GetObject("imgLogo.Image")));
            this.imgLogo.Location = new System.Drawing.Point(917, 6);
            this.imgLogo.Name = "imgLogo";
            this.imgLogo.Size = new System.Drawing.Size(129, 24);
            this.imgLogo.TabIndex = 37;
            this.imgLogo.TabStop = false;
            this.imgLogo.Click += new System.EventHandler(this.imgLogo_Click);
            // 
            // digitalClock
            // 
            this.digitalClock.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.digitalClock.ArrayCount = 6;
            this.digitalClock.ColorBackground = System.Drawing.Color.Black;
            this.digitalClock.ColorDark = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(40)))), ((int)(((byte)(40)))));
            this.digitalClock.ColorLight = System.Drawing.Color.White;
            this.digitalClock.DecimalShow = false;
            this.digitalClock.ElementPadding = new System.Windows.Forms.Padding(4);
            this.digitalClock.ElementWidth = 10;
            this.digitalClock.ItalicFactor = 0F;
            this.digitalClock.Location = new System.Drawing.Point(762, 56);
            this.digitalClock.Name = "digitalClock";
            this.digitalClock.Size = new System.Drawing.Size(395, 76);
            this.digitalClock.TabIndex = 35;
            this.digitalClock.TabStop = false;
            this.digitalClock.Value = null;
            // 
            // frmMainWindow
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1185, 700);
            this.Controls.Add(this.btnUpload);
            this.Controls.Add(this.btnArduino);
            this.Controls.Add(this.imgLogo);
            this.Controls.Add(this.btnJournal);
            this.Controls.Add(this.txtTime);
            this.Controls.Add(this.digitalClock);
            this.Controls.Add(this.panel1);
            this.Controls.Add(this.imgSessionName);
            this.Controls.Add(this.gridTargets);
            this.Controls.Add(this.tcSessionType);
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
            this.Controls.Add(this.btnClear);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.btnConfig);
            this.Controls.Add(this.imgArrow);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.txtLastShot);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.txtTotal);
            this.Controls.Add(this.shotsList);
            this.Controls.Add(this.imgTarget);
            this.Controls.Add(this.statusStrip1);
            this.Controls.Add(this.btnConnect);
            this.Controls.Add(this.trkZoom);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.txtOutput);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MinimumSize = new System.Drawing.Size(1000, 738);
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
            ((System.ComponentModel.ISupportInitialize)(this.imgSessionName)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.gridTargets)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.chartBreakdown)).EndInit();
            this.tcSessionType.ResumeLayout(false);
            this.panel1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.imgLogo)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.digitalClock)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        public System.IO.Ports.SerialPort serialPort;
        public System.Windows.Forms.Button btnConnect;
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
        private System.Windows.Forms.DataGridView gridTargets;
        private System.Windows.Forms.TabControl tcSessionType;
        private System.Windows.Forms.TabPage tabPistolPractice;
        private System.Windows.Forms.TabPage tabPistolMatch;
        private System.Windows.Forms.TabPage tabPistolFinal;
        private System.Windows.Forms.TabPage tabRiflePractice;
        private System.Windows.Forms.TabPage tabRifleMatch;
        private System.Windows.Forms.TabPage tabRifleFinal;
        private System.Windows.Forms.PictureBox imgSessionName;
        private System.Windows.Forms.DataVisualization.Charting.Chart chartBreakdown;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.ImageList imgListIcons;
        private freETarget.SevenSegmentArray digitalClock;
        private System.Windows.Forms.Button btnJournal;
        private System.Windows.Forms.PictureBox imgLogo;
        private System.Windows.Forms.DataGridViewTextBoxColumn column1;
        private System.Windows.Forms.DataGridViewTextBoxColumn Column2;
        private System.Windows.Forms.DataGridViewTextBoxColumn Column3;
        private System.Windows.Forms.DataGridViewTextBoxColumn Column4;
        private System.Windows.Forms.DataGridViewTextBoxColumn Column5;
        private System.Windows.Forms.DataGridViewTextBoxColumn Column6;
        private System.Windows.Forms.DataGridViewTextBoxColumn Column7;
        private System.Windows.Forms.DataGridViewTextBoxColumn Column8;
        private System.Windows.Forms.DataGridViewTextBoxColumn Column9;
        private System.Windows.Forms.DataGridViewTextBoxColumn Column10;
        private System.Windows.Forms.DataGridViewTextBoxColumn ColumnTotal;
        private System.Windows.Forms.Button btnArduino;
        private System.Windows.Forms.ToolStripStatusLabel statusVersion;
        private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel1;
        private System.Windows.Forms.Button btnUpload;
    }
}

