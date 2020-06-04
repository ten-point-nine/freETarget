namespace freETarget {
    partial class frmJournal {
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmJournal));
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea1 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Legend legend1 = new System.Windows.Forms.DataVisualization.Charting.Legend();
            System.Windows.Forms.DataVisualization.Charting.Series series1 = new System.Windows.Forms.DataVisualization.Charting.Series();
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea2 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Legend legend2 = new System.Windows.Forms.DataVisualization.Charting.Legend();
            System.Windows.Forms.DataVisualization.Charting.Series series2 = new System.Windows.Forms.DataVisualization.Charting.Series();
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea3 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Legend legend3 = new System.Windows.Forms.DataVisualization.Charting.Legend();
            System.Windows.Forms.DataVisualization.Charting.Series series3 = new System.Windows.Forms.DataVisualization.Charting.Series();
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea4 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Legend legend4 = new System.Windows.Forms.DataVisualization.Charting.Legend();
            System.Windows.Forms.DataVisualization.Charting.Series series4 = new System.Windows.Forms.DataVisualization.Charting.Series();
            this.cmbUsers = new System.Windows.Forms.ComboBox();
            this.tabEvents = new System.Windows.Forms.TabControl();
            this.tabPistolPractice = new System.Windows.Forms.TabPage();
            this.tabPistolMatch = new System.Windows.Forms.TabPage();
            this.tabPistolFinal = new System.Windows.Forms.TabPage();
            this.tabRiflePractice = new System.Windows.Forms.TabPage();
            this.tabRifleMatch = new System.Windows.Forms.TabPage();
            this.tabRifleFinal = new System.Windows.Forms.TabPage();
            this.btnClose = new System.Windows.Forms.Button();
            this.tabDetails = new System.Windows.Forms.TabControl();
            this.tabSessionList = new System.Windows.Forms.TabPage();
            this.btnDelete = new System.Windows.Forms.Button();
            this.pGridSession = new System.Windows.Forms.PropertyGrid();
            this.btnDiary = new System.Windows.Forms.Button();
            this.btnPrint = new System.Windows.Forms.Button();
            this.btnLoadSession = new System.Windows.Forms.Button();
            this.lstbSessions = new System.Windows.Forms.ListBox();
            this.tabStats = new System.Windows.Forms.TabPage();
            this.tableLayoutPanelStatistics = new System.Windows.Forms.TableLayoutPanel();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.chartScore = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.chartMeanRadius = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.chartWindage = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.groupBox4 = new System.Windows.Forms.GroupBox();
            this.chartElevation = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.imgLogo = new System.Windows.Forms.PictureBox();
            this.tabEvents.SuspendLayout();
            this.tabDetails.SuspendLayout();
            this.tabSessionList.SuspendLayout();
            this.tabStats.SuspendLayout();
            this.tableLayoutPanelStatistics.SuspendLayout();
            this.groupBox1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.chartScore)).BeginInit();
            this.groupBox2.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.chartMeanRadius)).BeginInit();
            this.groupBox3.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.chartWindage)).BeginInit();
            this.groupBox4.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.chartElevation)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.imgLogo)).BeginInit();
            this.SuspendLayout();
            // 
            // cmbUsers
            // 
            this.cmbUsers.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cmbUsers.FormattingEnabled = true;
            this.cmbUsers.Location = new System.Drawing.Point(12, 12);
            this.cmbUsers.Name = "cmbUsers";
            this.cmbUsers.Size = new System.Drawing.Size(121, 21);
            this.cmbUsers.TabIndex = 0;
            this.cmbUsers.SelectedIndexChanged += new System.EventHandler(this.cmbUsers_SelectedIndexChanged);
            // 
            // tabEvents
            // 
            this.tabEvents.Alignment = System.Windows.Forms.TabAlignment.Left;
            this.tabEvents.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.tabEvents.Controls.Add(this.tabPistolPractice);
            this.tabEvents.Controls.Add(this.tabPistolMatch);
            this.tabEvents.Controls.Add(this.tabPistolFinal);
            this.tabEvents.Controls.Add(this.tabRiflePractice);
            this.tabEvents.Controls.Add(this.tabRifleMatch);
            this.tabEvents.Controls.Add(this.tabRifleFinal);
            this.tabEvents.DrawMode = System.Windows.Forms.TabDrawMode.OwnerDrawFixed;
            this.tabEvents.Location = new System.Drawing.Point(514, 64);
            this.tabEvents.Multiline = true;
            this.tabEvents.Name = "tabEvents";
            this.tabEvents.SelectedIndex = 0;
            this.tabEvents.Size = new System.Drawing.Size(56, 446);
            this.tabEvents.TabIndex = 1;
            this.tabEvents.DrawItem += new System.Windows.Forms.DrawItemEventHandler(this.tabSessions_DrawItem);
            this.tabEvents.SelectedIndexChanged += new System.EventHandler(this.tabEvents_SelectedIndexChanged);
            // 
            // tabPistolPractice
            // 
            this.tabPistolPractice.BackColor = System.Drawing.Color.Gold;
            this.tabPistolPractice.Location = new System.Drawing.Point(23, 4);
            this.tabPistolPractice.Name = "tabPistolPractice";
            this.tabPistolPractice.Padding = new System.Windows.Forms.Padding(3);
            this.tabPistolPractice.Size = new System.Drawing.Size(29, 438);
            this.tabPistolPractice.TabIndex = 0;
            this.tabPistolPractice.Text = "Pistol Practice";
            // 
            // tabPistolMatch
            // 
            this.tabPistolMatch.BackColor = System.Drawing.Color.Orange;
            this.tabPistolMatch.Location = new System.Drawing.Point(23, 4);
            this.tabPistolMatch.Name = "tabPistolMatch";
            this.tabPistolMatch.Padding = new System.Windows.Forms.Padding(3);
            this.tabPistolMatch.Size = new System.Drawing.Size(29, 438);
            this.tabPistolMatch.TabIndex = 1;
            this.tabPistolMatch.Text = "Pistol Match ";
            // 
            // tabPistolFinal
            // 
            this.tabPistolFinal.BackColor = System.Drawing.Color.Red;
            this.tabPistolFinal.Location = new System.Drawing.Point(23, 4);
            this.tabPistolFinal.Name = "tabPistolFinal";
            this.tabPistolFinal.Size = new System.Drawing.Size(29, 438);
            this.tabPistolFinal.TabIndex = 2;
            this.tabPistolFinal.Text = "Pistol Final";
            // 
            // tabRiflePractice
            // 
            this.tabRiflePractice.BackColor = System.Drawing.Color.LimeGreen;
            this.tabRiflePractice.Location = new System.Drawing.Point(23, 4);
            this.tabRiflePractice.Name = "tabRiflePractice";
            this.tabRiflePractice.Size = new System.Drawing.Size(29, 438);
            this.tabRiflePractice.TabIndex = 3;
            this.tabRiflePractice.Text = "Rifle Practice";
            // 
            // tabRifleMatch
            // 
            this.tabRifleMatch.BackColor = System.Drawing.Color.Turquoise;
            this.tabRifleMatch.Location = new System.Drawing.Point(23, 4);
            this.tabRifleMatch.Name = "tabRifleMatch";
            this.tabRifleMatch.Size = new System.Drawing.Size(29, 438);
            this.tabRifleMatch.TabIndex = 4;
            this.tabRifleMatch.Text = "Rifle Match ";
            // 
            // tabRifleFinal
            // 
            this.tabRifleFinal.BackColor = System.Drawing.Color.DodgerBlue;
            this.tabRifleFinal.Location = new System.Drawing.Point(23, 4);
            this.tabRifleFinal.Name = "tabRifleFinal";
            this.tabRifleFinal.Size = new System.Drawing.Size(29, 438);
            this.tabRifleFinal.TabIndex = 5;
            this.tabRifleFinal.Text = "Rifle Final";
            // 
            // btnClose
            // 
            this.btnClose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnClose.Location = new System.Drawing.Point(448, 10);
            this.btnClose.Name = "btnClose";
            this.btnClose.Size = new System.Drawing.Size(75, 23);
            this.btnClose.TabIndex = 2;
            this.btnClose.Text = "Close";
            this.btnClose.UseVisualStyleBackColor = true;
            this.btnClose.Click += new System.EventHandler(this.btnClose_Click);
            // 
            // tabDetails
            // 
            this.tabDetails.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tabDetails.Controls.Add(this.tabSessionList);
            this.tabDetails.Controls.Add(this.tabStats);
            this.tabDetails.Location = new System.Drawing.Point(12, 43);
            this.tabDetails.Name = "tabDetails";
            this.tabDetails.SelectedIndex = 0;
            this.tabDetails.Size = new System.Drawing.Size(496, 504);
            this.tabDetails.TabIndex = 3;
            // 
            // tabSessionList
            // 
            this.tabSessionList.Controls.Add(this.btnDelete);
            this.tabSessionList.Controls.Add(this.pGridSession);
            this.tabSessionList.Controls.Add(this.btnDiary);
            this.tabSessionList.Controls.Add(this.btnPrint);
            this.tabSessionList.Controls.Add(this.btnLoadSession);
            this.tabSessionList.Controls.Add(this.lstbSessions);
            this.tabSessionList.Location = new System.Drawing.Point(4, 22);
            this.tabSessionList.Name = "tabSessionList";
            this.tabSessionList.Padding = new System.Windows.Forms.Padding(3);
            this.tabSessionList.Size = new System.Drawing.Size(488, 478);
            this.tabSessionList.TabIndex = 0;
            this.tabSessionList.Text = "Sessions";
            this.tabSessionList.UseVisualStyleBackColor = true;
            // 
            // btnDelete
            // 
            this.btnDelete.Enabled = false;
            this.btnDelete.Image = ((System.Drawing.Image)(resources.GetObject("btnDelete.Image")));
            this.btnDelete.ImageAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.btnDelete.Location = new System.Drawing.Point(160, 6);
            this.btnDelete.Name = "btnDelete";
            this.btnDelete.Size = new System.Drawing.Size(75, 23);
            this.btnDelete.TabIndex = 5;
            this.btnDelete.Text = "Delete";
            this.btnDelete.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.btnDelete.UseVisualStyleBackColor = true;
            this.btnDelete.Click += new System.EventHandler(this.btnDelete_Click);
            // 
            // pGridSession
            // 
            this.pGridSession.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.pGridSession.DisabledItemForeColor = System.Drawing.SystemColors.ControlText;
            this.pGridSession.Location = new System.Drawing.Point(160, 36);
            this.pGridSession.Name = "pGridSession";
            this.pGridSession.PropertySort = System.Windows.Forms.PropertySort.Categorized;
            this.pGridSession.Size = new System.Drawing.Size(318, 436);
            this.pGridSession.TabIndex = 4;
            this.pGridSession.ToolbarVisible = false;
            // 
            // btnDiary
            // 
            this.btnDiary.Enabled = false;
            this.btnDiary.Image = ((System.Drawing.Image)(resources.GetObject("btnDiary.Image")));
            this.btnDiary.Location = new System.Drawing.Point(403, 6);
            this.btnDiary.Name = "btnDiary";
            this.btnDiary.Size = new System.Drawing.Size(75, 23);
            this.btnDiary.TabIndex = 3;
            this.btnDiary.Text = "Diary";
            this.btnDiary.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.btnDiary.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.btnDiary.UseVisualStyleBackColor = true;
            this.btnDiary.Click += new System.EventHandler(this.btnDiary_Click);
            // 
            // btnPrint
            // 
            this.btnPrint.Enabled = false;
            this.btnPrint.Image = ((System.Drawing.Image)(resources.GetObject("btnPrint.Image")));
            this.btnPrint.Location = new System.Drawing.Point(322, 6);
            this.btnPrint.Name = "btnPrint";
            this.btnPrint.Size = new System.Drawing.Size(75, 23);
            this.btnPrint.TabIndex = 2;
            this.btnPrint.Text = "Print";
            this.btnPrint.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.btnPrint.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.btnPrint.UseVisualStyleBackColor = true;
            this.btnPrint.Click += new System.EventHandler(this.btnPrint_Click);
            // 
            // btnLoadSession
            // 
            this.btnLoadSession.Enabled = false;
            this.btnLoadSession.Image = ((System.Drawing.Image)(resources.GetObject("btnLoadSession.Image")));
            this.btnLoadSession.ImageAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.btnLoadSession.Location = new System.Drawing.Point(241, 6);
            this.btnLoadSession.Name = "btnLoadSession";
            this.btnLoadSession.Size = new System.Drawing.Size(75, 23);
            this.btnLoadSession.TabIndex = 1;
            this.btnLoadSession.Text = "Load";
            this.btnLoadSession.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.btnLoadSession.UseVisualStyleBackColor = true;
            this.btnLoadSession.Click += new System.EventHandler(this.btnLoadSession_Click);
            // 
            // lstbSessions
            // 
            this.lstbSessions.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left)));
            this.lstbSessions.FormattingEnabled = true;
            this.lstbSessions.Location = new System.Drawing.Point(6, 6);
            this.lstbSessions.Name = "lstbSessions";
            this.lstbSessions.Size = new System.Drawing.Size(147, 459);
            this.lstbSessions.TabIndex = 0;
            this.lstbSessions.SelectedIndexChanged += new System.EventHandler(this.lstbSessions_SelectedIndexChanged);
            // 
            // tabStats
            // 
            this.tabStats.Controls.Add(this.tableLayoutPanelStatistics);
            this.tabStats.Location = new System.Drawing.Point(4, 22);
            this.tabStats.Name = "tabStats";
            this.tabStats.Padding = new System.Windows.Forms.Padding(3);
            this.tabStats.Size = new System.Drawing.Size(488, 478);
            this.tabStats.TabIndex = 1;
            this.tabStats.Text = "Statistics";
            this.tabStats.UseVisualStyleBackColor = true;
            // 
            // tableLayoutPanelStatistics
            // 
            this.tableLayoutPanelStatistics.AutoScroll = true;
            this.tableLayoutPanelStatistics.AutoSize = true;
            this.tableLayoutPanelStatistics.ColumnCount = 1;
            this.tableLayoutPanelStatistics.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanelStatistics.Controls.Add(this.groupBox1, 0, 0);
            this.tableLayoutPanelStatistics.Controls.Add(this.groupBox2, 0, 1);
            this.tableLayoutPanelStatistics.Controls.Add(this.groupBox3, 0, 2);
            this.tableLayoutPanelStatistics.Controls.Add(this.groupBox4, 0, 3);
            this.tableLayoutPanelStatistics.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanelStatistics.Location = new System.Drawing.Point(3, 3);
            this.tableLayoutPanelStatistics.Name = "tableLayoutPanelStatistics";
            this.tableLayoutPanelStatistics.Padding = new System.Windows.Forms.Padding(0, 0, 20, 0);
            this.tableLayoutPanelStatistics.RowCount = 4;
            this.tableLayoutPanelStatistics.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 200F));
            this.tableLayoutPanelStatistics.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 200F));
            this.tableLayoutPanelStatistics.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 200F));
            this.tableLayoutPanelStatistics.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 200F));
            this.tableLayoutPanelStatistics.Size = new System.Drawing.Size(482, 472);
            this.tableLayoutPanelStatistics.TabIndex = 0;
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.chartScore);
            this.groupBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox1.Location = new System.Drawing.Point(3, 3);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(439, 194);
            this.groupBox1.TabIndex = 4;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Scores";
            // 
            // chartScore
            // 
            chartArea1.AxisX.Enabled = System.Windows.Forms.DataVisualization.Charting.AxisEnabled.False;
            chartArea1.AxisX.MajorGrid.Enabled = false;
            chartArea1.AxisX.MajorTickMark.Enabled = false;
            chartArea1.AxisY.Enabled = System.Windows.Forms.DataVisualization.Charting.AxisEnabled.False;
            chartArea1.Name = "ChartArea1";
            chartArea1.Position.Auto = false;
            chartArea1.Position.Height = 85F;
            chartArea1.Position.Width = 100F;
            this.chartScore.ChartAreas.Add(chartArea1);
            this.chartScore.Dock = System.Windows.Forms.DockStyle.Fill;
            legend1.Alignment = System.Drawing.StringAlignment.Center;
            legend1.DockedToChartArea = "ChartArea1";
            legend1.Docking = System.Windows.Forms.DataVisualization.Charting.Docking.Bottom;
            legend1.IsDockedInsideChartArea = false;
            legend1.Name = "Legend1";
            legend1.TextWrapThreshold = 100;
            this.chartScore.Legends.Add(legend1);
            this.chartScore.Location = new System.Drawing.Point(3, 16);
            this.chartScore.Name = "chartScore";
            series1.ChartArea = "ChartArea1";
            series1.CustomProperties = "LabelStyle=Bottom";
            series1.Font = new System.Drawing.Font("Microsoft Sans Serif", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            series1.IsValueShownAsLabel = true;
            series1.Legend = "Legend1";
            series1.LegendToolTip = "Average ToolTip";
            series1.Name = "Average";
            series1.SmartLabelStyle.AllowOutsidePlotArea = System.Windows.Forms.DataVisualization.Charting.LabelOutsidePlotAreaStyle.Yes;
            series1.SmartLabelStyle.CalloutLineColor = System.Drawing.Color.White;
            series1.SmartLabelStyle.CalloutLineWidth = 0;
            series1.SmartLabelStyle.Enabled = false;
            this.chartScore.Series.Add(series1);
            this.chartScore.Size = new System.Drawing.Size(433, 175);
            this.chartScore.TabIndex = 0;
            this.chartScore.Text = "chart1";
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.chartMeanRadius);
            this.groupBox2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox2.Location = new System.Drawing.Point(3, 203);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(439, 194);
            this.groupBox2.TabIndex = 5;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Mean Radius";
            // 
            // chartMeanRadius
            // 
            chartArea2.AxisX.Enabled = System.Windows.Forms.DataVisualization.Charting.AxisEnabled.False;
            chartArea2.AxisX.MajorGrid.Enabled = false;
            chartArea2.AxisX.MajorTickMark.Enabled = false;
            chartArea2.AxisY.Enabled = System.Windows.Forms.DataVisualization.Charting.AxisEnabled.False;
            chartArea2.Name = "ChartArea1";
            chartArea2.Position.Auto = false;
            chartArea2.Position.Height = 85F;
            chartArea2.Position.Width = 100F;
            this.chartMeanRadius.ChartAreas.Add(chartArea2);
            this.chartMeanRadius.Dock = System.Windows.Forms.DockStyle.Fill;
            legend2.Alignment = System.Drawing.StringAlignment.Center;
            legend2.DockedToChartArea = "ChartArea1";
            legend2.Docking = System.Windows.Forms.DataVisualization.Charting.Docking.Bottom;
            legend2.IsDockedInsideChartArea = false;
            legend2.Name = "Legend1";
            legend2.TextWrapThreshold = 100;
            this.chartMeanRadius.Legends.Add(legend2);
            this.chartMeanRadius.Location = new System.Drawing.Point(3, 16);
            this.chartMeanRadius.Name = "chartMeanRadius";
            series2.ChartArea = "ChartArea1";
            series2.CustomProperties = "LabelStyle=Bottom";
            series2.Font = new System.Drawing.Font("Microsoft Sans Serif", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            series2.IsValueShownAsLabel = true;
            series2.Legend = "Legend1";
            series2.Name = "Series1";
            series2.SmartLabelStyle.AllowOutsidePlotArea = System.Windows.Forms.DataVisualization.Charting.LabelOutsidePlotAreaStyle.Yes;
            series2.SmartLabelStyle.CalloutLineColor = System.Drawing.Color.White;
            series2.SmartLabelStyle.CalloutLineWidth = 0;
            series2.SmartLabelStyle.Enabled = false;
            this.chartMeanRadius.Series.Add(series2);
            this.chartMeanRadius.Size = new System.Drawing.Size(433, 175);
            this.chartMeanRadius.TabIndex = 1;
            this.chartMeanRadius.Text = "chart2";
            // 
            // groupBox3
            // 
            this.groupBox3.Controls.Add(this.chartWindage);
            this.groupBox3.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox3.Location = new System.Drawing.Point(3, 403);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(439, 194);
            this.groupBox3.TabIndex = 6;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "Windage";
            // 
            // chartWindage
            // 
            chartArea3.AxisX.Enabled = System.Windows.Forms.DataVisualization.Charting.AxisEnabled.False;
            chartArea3.AxisX.MajorGrid.Enabled = false;
            chartArea3.AxisX.MajorTickMark.Enabled = false;
            chartArea3.AxisY.Enabled = System.Windows.Forms.DataVisualization.Charting.AxisEnabled.False;
            chartArea3.Name = "ChartArea1";
            chartArea3.Position.Auto = false;
            chartArea3.Position.Height = 85F;
            chartArea3.Position.Width = 100F;
            this.chartWindage.ChartAreas.Add(chartArea3);
            this.chartWindage.Dock = System.Windows.Forms.DockStyle.Fill;
            legend3.Alignment = System.Drawing.StringAlignment.Center;
            legend3.DockedToChartArea = "ChartArea1";
            legend3.Docking = System.Windows.Forms.DataVisualization.Charting.Docking.Bottom;
            legend3.IsDockedInsideChartArea = false;
            legend3.Name = "Legend1";
            legend3.TextWrapThreshold = 100;
            this.chartWindage.Legends.Add(legend3);
            this.chartWindage.Location = new System.Drawing.Point(3, 16);
            this.chartWindage.Name = "chartWindage";
            series3.ChartArea = "ChartArea1";
            series3.CustomProperties = "LabelStyle=Bottom";
            series3.Font = new System.Drawing.Font("Microsoft Sans Serif", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            series3.IsValueShownAsLabel = true;
            series3.Legend = "Legend1";
            series3.Name = "Series1";
            series3.SmartLabelStyle.AllowOutsidePlotArea = System.Windows.Forms.DataVisualization.Charting.LabelOutsidePlotAreaStyle.Yes;
            series3.SmartLabelStyle.CalloutLineColor = System.Drawing.Color.White;
            series3.SmartLabelStyle.CalloutLineWidth = 0;
            series3.SmartLabelStyle.Enabled = false;
            this.chartWindage.Series.Add(series3);
            this.chartWindage.Size = new System.Drawing.Size(433, 175);
            this.chartWindage.TabIndex = 2;
            this.chartWindage.Text = "chart3";
            // 
            // groupBox4
            // 
            this.groupBox4.Controls.Add(this.chartElevation);
            this.groupBox4.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox4.Location = new System.Drawing.Point(3, 603);
            this.groupBox4.Name = "groupBox4";
            this.groupBox4.Size = new System.Drawing.Size(439, 194);
            this.groupBox4.TabIndex = 7;
            this.groupBox4.TabStop = false;
            this.groupBox4.Text = "Elevation";
            // 
            // chartElevation
            // 
            chartArea4.AxisX.Enabled = System.Windows.Forms.DataVisualization.Charting.AxisEnabled.False;
            chartArea4.AxisX.MajorGrid.Enabled = false;
            chartArea4.AxisX.MajorTickMark.Enabled = false;
            chartArea4.AxisY.Enabled = System.Windows.Forms.DataVisualization.Charting.AxisEnabled.False;
            chartArea4.Name = "ChartArea1";
            chartArea4.Position.Auto = false;
            chartArea4.Position.Height = 85F;
            chartArea4.Position.Width = 100F;
            this.chartElevation.ChartAreas.Add(chartArea4);
            this.chartElevation.Dock = System.Windows.Forms.DockStyle.Fill;
            legend4.Alignment = System.Drawing.StringAlignment.Center;
            legend4.DockedToChartArea = "ChartArea1";
            legend4.Docking = System.Windows.Forms.DataVisualization.Charting.Docking.Bottom;
            legend4.IsDockedInsideChartArea = false;
            legend4.Name = "Legend1";
            legend4.TextWrapThreshold = 100;
            this.chartElevation.Legends.Add(legend4);
            this.chartElevation.Location = new System.Drawing.Point(3, 16);
            this.chartElevation.Name = "chartElevation";
            series4.ChartArea = "ChartArea1";
            series4.CustomProperties = "LabelStyle=Bottom";
            series4.Font = new System.Drawing.Font("Microsoft Sans Serif", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            series4.IsValueShownAsLabel = true;
            series4.Legend = "Legend1";
            series4.Name = "Series1";
            series4.SmartLabelStyle.AllowOutsidePlotArea = System.Windows.Forms.DataVisualization.Charting.LabelOutsidePlotAreaStyle.Yes;
            series4.SmartLabelStyle.CalloutLineColor = System.Drawing.Color.White;
            series4.SmartLabelStyle.CalloutLineWidth = 0;
            series4.SmartLabelStyle.Enabled = false;
            this.chartElevation.Series.Add(series4);
            this.chartElevation.Size = new System.Drawing.Size(433, 175);
            this.chartElevation.TabIndex = 3;
            this.chartElevation.Text = "chart4";
            // 
            // imgLogo
            // 
            this.imgLogo.Image = ((System.Drawing.Image)(resources.GetObject("imgLogo.Image")));
            this.imgLogo.Location = new System.Drawing.Point(220, 23);
            this.imgLogo.Name = "imgLogo";
            this.imgLogo.Size = new System.Drawing.Size(128, 27);
            this.imgLogo.TabIndex = 4;
            this.imgLogo.TabStop = false;
            this.imgLogo.Visible = false;
            // 
            // frmJournal
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(535, 559);
            this.Controls.Add(this.imgLogo);
            this.Controls.Add(this.tabDetails);
            this.Controls.Add(this.btnClose);
            this.Controls.Add(this.tabEvents);
            this.Controls.Add(this.cmbUsers);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MinimumSize = new System.Drawing.Size(551, 564);
            this.Name = "frmJournal";
            this.Text = "Journal";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.frmJournal_FormClosing);
            this.Load += new System.EventHandler(this.frmJournal_Load);
            this.tabEvents.ResumeLayout(false);
            this.tabDetails.ResumeLayout(false);
            this.tabSessionList.ResumeLayout(false);
            this.tabStats.ResumeLayout(false);
            this.tabStats.PerformLayout();
            this.tableLayoutPanelStatistics.ResumeLayout(false);
            this.groupBox1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.chartScore)).EndInit();
            this.groupBox2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.chartMeanRadius)).EndInit();
            this.groupBox3.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.chartWindage)).EndInit();
            this.groupBox4.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.chartElevation)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.imgLogo)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.ComboBox cmbUsers;
        private System.Windows.Forms.TabControl tabEvents;
        private System.Windows.Forms.TabPage tabPistolPractice;
        private System.Windows.Forms.TabPage tabPistolMatch;
        private System.Windows.Forms.TabPage tabPistolFinal;
        private System.Windows.Forms.TabPage tabRiflePractice;
        private System.Windows.Forms.TabPage tabRifleMatch;
        private System.Windows.Forms.TabPage tabRifleFinal;
        private System.Windows.Forms.Button btnClose;
        private System.Windows.Forms.TabControl tabDetails;
        private System.Windows.Forms.TabPage tabSessionList;
        private System.Windows.Forms.TabPage tabStats;
        private System.Windows.Forms.ListBox lstbSessions;
        private System.Windows.Forms.Button btnDiary;
        private System.Windows.Forms.Button btnPrint;
        private System.Windows.Forms.Button btnLoadSession;
        private System.Windows.Forms.PropertyGrid pGridSession;
        private System.Windows.Forms.Button btnDelete;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanelStatistics;
        private System.Windows.Forms.DataVisualization.Charting.Chart chartScore;
        private System.Windows.Forms.DataVisualization.Charting.Chart chartMeanRadius;
        private System.Windows.Forms.DataVisualization.Charting.Chart chartWindage;
        private System.Windows.Forms.DataVisualization.Charting.Chart chartElevation;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.GroupBox groupBox3;
        private System.Windows.Forms.GroupBox groupBox4;
        private System.Windows.Forms.PictureBox imgLogo;
    }
}