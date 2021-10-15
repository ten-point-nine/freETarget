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
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea13 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Legend legend13 = new System.Windows.Forms.DataVisualization.Charting.Legend();
            System.Windows.Forms.DataVisualization.Charting.Series series13 = new System.Windows.Forms.DataVisualization.Charting.Series();
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea14 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Legend legend14 = new System.Windows.Forms.DataVisualization.Charting.Legend();
            System.Windows.Forms.DataVisualization.Charting.Series series14 = new System.Windows.Forms.DataVisualization.Charting.Series();
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea15 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Legend legend15 = new System.Windows.Forms.DataVisualization.Charting.Legend();
            System.Windows.Forms.DataVisualization.Charting.Series series15 = new System.Windows.Forms.DataVisualization.Charting.Series();
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea16 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Legend legend16 = new System.Windows.Forms.DataVisualization.Charting.Legend();
            System.Windows.Forms.DataVisualization.Charting.Series series16 = new System.Windows.Forms.DataVisualization.Charting.Series();
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
            this.btnImport = new System.Windows.Forms.Button();
            this.btnExport = new System.Windows.Forms.Button();
            this.btnGraph = new System.Windows.Forms.Button();
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
            this.tabEvents.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tabEvents.Controls.Add(this.tabPistolPractice);
            this.tabEvents.Controls.Add(this.tabPistolMatch);
            this.tabEvents.Controls.Add(this.tabPistolFinal);
            this.tabEvents.Controls.Add(this.tabRiflePractice);
            this.tabEvents.Controls.Add(this.tabRifleMatch);
            this.tabEvents.Controls.Add(this.tabRifleFinal);
            this.tabEvents.DrawMode = System.Windows.Forms.TabDrawMode.OwnerDrawFixed;
            this.tabEvents.Location = new System.Drawing.Point(545, 64);
            this.tabEvents.Multiline = true;
            this.tabEvents.Name = "tabEvents";
            this.tabEvents.SelectedIndex = 0;
            this.tabEvents.Size = new System.Drawing.Size(56, 449);
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
            this.tabPistolPractice.Size = new System.Drawing.Size(29, 441);
            this.tabPistolPractice.TabIndex = 0;
            this.tabPistolPractice.Text = "Pistol Practice";
            // 
            // tabPistolMatch
            // 
            this.tabPistolMatch.BackColor = System.Drawing.Color.Orange;
            this.tabPistolMatch.Location = new System.Drawing.Point(42, 4);
            this.tabPistolMatch.Name = "tabPistolMatch";
            this.tabPistolMatch.Padding = new System.Windows.Forms.Padding(3);
            this.tabPistolMatch.Size = new System.Drawing.Size(10, 376);
            this.tabPistolMatch.TabIndex = 1;
            this.tabPistolMatch.Text = "Pistol Match ";
            // 
            // tabPistolFinal
            // 
            this.tabPistolFinal.BackColor = System.Drawing.Color.Red;
            this.tabPistolFinal.Location = new System.Drawing.Point(42, 4);
            this.tabPistolFinal.Name = "tabPistolFinal";
            this.tabPistolFinal.Size = new System.Drawing.Size(10, 376);
            this.tabPistolFinal.TabIndex = 2;
            this.tabPistolFinal.Text = "Pistol Final";
            // 
            // tabRiflePractice
            // 
            this.tabRiflePractice.BackColor = System.Drawing.Color.LimeGreen;
            this.tabRiflePractice.Location = new System.Drawing.Point(42, 4);
            this.tabRiflePractice.Name = "tabRiflePractice";
            this.tabRiflePractice.Size = new System.Drawing.Size(10, 376);
            this.tabRiflePractice.TabIndex = 3;
            this.tabRiflePractice.Text = "Rifle Practice";
            // 
            // tabRifleMatch
            // 
            this.tabRifleMatch.BackColor = System.Drawing.Color.Turquoise;
            this.tabRifleMatch.Location = new System.Drawing.Point(42, 4);
            this.tabRifleMatch.Name = "tabRifleMatch";
            this.tabRifleMatch.Size = new System.Drawing.Size(10, 376);
            this.tabRifleMatch.TabIndex = 4;
            this.tabRifleMatch.Text = "Rifle Match ";
            // 
            // tabRifleFinal
            // 
            this.tabRifleFinal.BackColor = System.Drawing.Color.DodgerBlue;
            this.tabRifleFinal.Location = new System.Drawing.Point(42, 4);
            this.tabRifleFinal.Name = "tabRifleFinal";
            this.tabRifleFinal.Size = new System.Drawing.Size(10, 376);
            this.tabRifleFinal.TabIndex = 5;
            this.tabRifleFinal.Text = "Rifle Final";
            // 
            // btnClose
            // 
            this.btnClose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnClose.Location = new System.Drawing.Point(479, 10);
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
            this.tabDetails.Size = new System.Drawing.Size(527, 536);
            this.tabDetails.TabIndex = 3;
            // 
            // tabSessionList
            // 
            this.tabSessionList.Controls.Add(this.btnImport);
            this.tabSessionList.Controls.Add(this.btnExport);
            this.tabSessionList.Controls.Add(this.btnGraph);
            this.tabSessionList.Controls.Add(this.btnDelete);
            this.tabSessionList.Controls.Add(this.pGridSession);
            this.tabSessionList.Controls.Add(this.btnDiary);
            this.tabSessionList.Controls.Add(this.btnPrint);
            this.tabSessionList.Controls.Add(this.btnLoadSession);
            this.tabSessionList.Controls.Add(this.lstbSessions);
            this.tabSessionList.Location = new System.Drawing.Point(4, 22);
            this.tabSessionList.Name = "tabSessionList";
            this.tabSessionList.Padding = new System.Windows.Forms.Padding(3);
            this.tabSessionList.Size = new System.Drawing.Size(519, 510);
            this.tabSessionList.TabIndex = 0;
            this.tabSessionList.Text = "Sessions";
            this.tabSessionList.UseVisualStyleBackColor = true;
            // 
            // btnImport
            // 
            this.btnImport.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.btnImport.Location = new System.Drawing.Point(83, 478);
            this.btnImport.Name = "btnImport";
            this.btnImport.Size = new System.Drawing.Size(70, 24);
            this.btnImport.TabIndex = 8;
            this.btnImport.Text = "Import";
            this.btnImport.UseVisualStyleBackColor = true;
            this.btnImport.Click += new System.EventHandler(this.btnImport_Click);
            // 
            // btnExport
            // 
            this.btnExport.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.btnExport.Enabled = false;
            this.btnExport.Location = new System.Drawing.Point(6, 478);
            this.btnExport.Name = "btnExport";
            this.btnExport.Size = new System.Drawing.Size(70, 24);
            this.btnExport.TabIndex = 7;
            this.btnExport.Text = "Export";
            this.btnExport.UseVisualStyleBackColor = true;
            this.btnExport.Click += new System.EventHandler(this.btnExport_Click);
            // 
            // btnGraph
            // 
            this.btnGraph.Enabled = false;
            this.btnGraph.Image = ((System.Drawing.Image)(resources.GetObject("btnGraph.Image")));
            this.btnGraph.Location = new System.Drawing.Point(444, 6);
            this.btnGraph.Name = "btnGraph";
            this.btnGraph.Size = new System.Drawing.Size(65, 23);
            this.btnGraph.TabIndex = 6;
            this.btnGraph.Text = "Chart";
            this.btnGraph.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.btnGraph.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.btnGraph.UseVisualStyleBackColor = true;
            this.btnGraph.Click += new System.EventHandler(this.btnGraph_Click);
            // 
            // btnDelete
            // 
            this.btnDelete.Enabled = false;
            this.btnDelete.Image = ((System.Drawing.Image)(resources.GetObject("btnDelete.Image")));
            this.btnDelete.ImageAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.btnDelete.Location = new System.Drawing.Point(160, 6);
            this.btnDelete.Name = "btnDelete";
            this.btnDelete.Size = new System.Drawing.Size(65, 23);
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
            this.pGridSession.Size = new System.Drawing.Size(349, 468);
            this.pGridSession.TabIndex = 4;
            this.pGridSession.ToolbarVisible = false;
            // 
            // btnDiary
            // 
            this.btnDiary.Enabled = false;
            this.btnDiary.Image = ((System.Drawing.Image)(resources.GetObject("btnDiary.Image")));
            this.btnDiary.Location = new System.Drawing.Point(373, 6);
            this.btnDiary.Name = "btnDiary";
            this.btnDiary.Size = new System.Drawing.Size(65, 23);
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
            this.btnPrint.Location = new System.Drawing.Point(302, 6);
            this.btnPrint.Name = "btnPrint";
            this.btnPrint.Size = new System.Drawing.Size(65, 23);
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
            this.btnLoadSession.Location = new System.Drawing.Point(231, 6);
            this.btnLoadSession.Name = "btnLoadSession";
            this.btnLoadSession.Size = new System.Drawing.Size(65, 23);
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
            this.tabStats.Size = new System.Drawing.Size(519, 445);
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
            this.tableLayoutPanelStatistics.Size = new System.Drawing.Size(513, 439);
            this.tableLayoutPanelStatistics.TabIndex = 0;
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.chartScore);
            this.groupBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox1.Location = new System.Drawing.Point(3, 3);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(470, 194);
            this.groupBox1.TabIndex = 4;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Scores";
            // 
            // chartScore
            // 
            chartArea13.AxisX.Enabled = System.Windows.Forms.DataVisualization.Charting.AxisEnabled.False;
            chartArea13.AxisX.MajorGrid.Enabled = false;
            chartArea13.AxisX.MajorTickMark.Enabled = false;
            chartArea13.AxisY.Enabled = System.Windows.Forms.DataVisualization.Charting.AxisEnabled.False;
            chartArea13.Name = "ChartArea1";
            chartArea13.Position.Auto = false;
            chartArea13.Position.Height = 85F;
            chartArea13.Position.Width = 100F;
            this.chartScore.ChartAreas.Add(chartArea13);
            this.chartScore.Dock = System.Windows.Forms.DockStyle.Fill;
            legend13.Alignment = System.Drawing.StringAlignment.Center;
            legend13.DockedToChartArea = "ChartArea1";
            legend13.Docking = System.Windows.Forms.DataVisualization.Charting.Docking.Bottom;
            legend13.IsDockedInsideChartArea = false;
            legend13.Name = "Legend1";
            legend13.TextWrapThreshold = 100;
            this.chartScore.Legends.Add(legend13);
            this.chartScore.Location = new System.Drawing.Point(3, 16);
            this.chartScore.Name = "chartScore";
            series13.ChartArea = "ChartArea1";
            series13.CustomProperties = "LabelStyle=Bottom";
            series13.Font = new System.Drawing.Font("Microsoft Sans Serif", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            series13.IsValueShownAsLabel = true;
            series13.Legend = "Legend1";
            series13.LegendToolTip = "Average ToolTip";
            series13.Name = "Average";
            series13.SmartLabelStyle.AllowOutsidePlotArea = System.Windows.Forms.DataVisualization.Charting.LabelOutsidePlotAreaStyle.Yes;
            series13.SmartLabelStyle.CalloutLineColor = System.Drawing.Color.White;
            series13.SmartLabelStyle.CalloutLineWidth = 0;
            series13.SmartLabelStyle.Enabled = false;
            this.chartScore.Series.Add(series13);
            this.chartScore.Size = new System.Drawing.Size(464, 175);
            this.chartScore.TabIndex = 0;
            this.chartScore.Text = "chart1";
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.chartMeanRadius);
            this.groupBox2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox2.Location = new System.Drawing.Point(3, 203);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(470, 194);
            this.groupBox2.TabIndex = 5;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Mean Radius";
            // 
            // chartMeanRadius
            // 
            chartArea14.AxisX.Enabled = System.Windows.Forms.DataVisualization.Charting.AxisEnabled.False;
            chartArea14.AxisX.MajorGrid.Enabled = false;
            chartArea14.AxisX.MajorTickMark.Enabled = false;
            chartArea14.AxisY.Enabled = System.Windows.Forms.DataVisualization.Charting.AxisEnabled.False;
            chartArea14.Name = "ChartArea1";
            chartArea14.Position.Auto = false;
            chartArea14.Position.Height = 85F;
            chartArea14.Position.Width = 100F;
            this.chartMeanRadius.ChartAreas.Add(chartArea14);
            this.chartMeanRadius.Dock = System.Windows.Forms.DockStyle.Fill;
            legend14.Alignment = System.Drawing.StringAlignment.Center;
            legend14.DockedToChartArea = "ChartArea1";
            legend14.Docking = System.Windows.Forms.DataVisualization.Charting.Docking.Bottom;
            legend14.IsDockedInsideChartArea = false;
            legend14.Name = "Legend1";
            legend14.TextWrapThreshold = 100;
            this.chartMeanRadius.Legends.Add(legend14);
            this.chartMeanRadius.Location = new System.Drawing.Point(3, 16);
            this.chartMeanRadius.Name = "chartMeanRadius";
            series14.ChartArea = "ChartArea1";
            series14.CustomProperties = "LabelStyle=Bottom";
            series14.Font = new System.Drawing.Font("Microsoft Sans Serif", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            series14.IsValueShownAsLabel = true;
            series14.Legend = "Legend1";
            series14.Name = "Series1";
            series14.SmartLabelStyle.AllowOutsidePlotArea = System.Windows.Forms.DataVisualization.Charting.LabelOutsidePlotAreaStyle.Yes;
            series14.SmartLabelStyle.CalloutLineColor = System.Drawing.Color.White;
            series14.SmartLabelStyle.CalloutLineWidth = 0;
            series14.SmartLabelStyle.Enabled = false;
            this.chartMeanRadius.Series.Add(series14);
            this.chartMeanRadius.Size = new System.Drawing.Size(464, 175);
            this.chartMeanRadius.TabIndex = 1;
            this.chartMeanRadius.Text = "chart2";
            // 
            // groupBox3
            // 
            this.groupBox3.Controls.Add(this.chartWindage);
            this.groupBox3.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox3.Location = new System.Drawing.Point(3, 403);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new System.Drawing.Size(470, 194);
            this.groupBox3.TabIndex = 6;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "Windage";
            // 
            // chartWindage
            // 
            chartArea15.AxisX.Enabled = System.Windows.Forms.DataVisualization.Charting.AxisEnabled.False;
            chartArea15.AxisX.MajorGrid.Enabled = false;
            chartArea15.AxisX.MajorTickMark.Enabled = false;
            chartArea15.AxisY.Enabled = System.Windows.Forms.DataVisualization.Charting.AxisEnabled.False;
            chartArea15.Name = "ChartArea1";
            chartArea15.Position.Auto = false;
            chartArea15.Position.Height = 85F;
            chartArea15.Position.Width = 100F;
            this.chartWindage.ChartAreas.Add(chartArea15);
            this.chartWindage.Dock = System.Windows.Forms.DockStyle.Fill;
            legend15.Alignment = System.Drawing.StringAlignment.Center;
            legend15.DockedToChartArea = "ChartArea1";
            legend15.Docking = System.Windows.Forms.DataVisualization.Charting.Docking.Bottom;
            legend15.IsDockedInsideChartArea = false;
            legend15.Name = "Legend1";
            legend15.TextWrapThreshold = 100;
            this.chartWindage.Legends.Add(legend15);
            this.chartWindage.Location = new System.Drawing.Point(3, 16);
            this.chartWindage.Name = "chartWindage";
            series15.ChartArea = "ChartArea1";
            series15.CustomProperties = "LabelStyle=Bottom";
            series15.Font = new System.Drawing.Font("Microsoft Sans Serif", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            series15.IsValueShownAsLabel = true;
            series15.Legend = "Legend1";
            series15.Name = "Series1";
            series15.SmartLabelStyle.AllowOutsidePlotArea = System.Windows.Forms.DataVisualization.Charting.LabelOutsidePlotAreaStyle.Yes;
            series15.SmartLabelStyle.CalloutLineColor = System.Drawing.Color.White;
            series15.SmartLabelStyle.CalloutLineWidth = 0;
            series15.SmartLabelStyle.Enabled = false;
            this.chartWindage.Series.Add(series15);
            this.chartWindage.Size = new System.Drawing.Size(464, 175);
            this.chartWindage.TabIndex = 2;
            this.chartWindage.Text = "chart3";
            // 
            // groupBox4
            // 
            this.groupBox4.Controls.Add(this.chartElevation);
            this.groupBox4.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox4.Location = new System.Drawing.Point(3, 603);
            this.groupBox4.Name = "groupBox4";
            this.groupBox4.Size = new System.Drawing.Size(470, 194);
            this.groupBox4.TabIndex = 7;
            this.groupBox4.TabStop = false;
            this.groupBox4.Text = "Elevation";
            // 
            // chartElevation
            // 
            chartArea16.AxisX.Enabled = System.Windows.Forms.DataVisualization.Charting.AxisEnabled.False;
            chartArea16.AxisX.MajorGrid.Enabled = false;
            chartArea16.AxisX.MajorTickMark.Enabled = false;
            chartArea16.AxisY.Enabled = System.Windows.Forms.DataVisualization.Charting.AxisEnabled.False;
            chartArea16.Name = "ChartArea1";
            chartArea16.Position.Auto = false;
            chartArea16.Position.Height = 85F;
            chartArea16.Position.Width = 100F;
            this.chartElevation.ChartAreas.Add(chartArea16);
            this.chartElevation.Dock = System.Windows.Forms.DockStyle.Fill;
            legend16.Alignment = System.Drawing.StringAlignment.Center;
            legend16.DockedToChartArea = "ChartArea1";
            legend16.Docking = System.Windows.Forms.DataVisualization.Charting.Docking.Bottom;
            legend16.IsDockedInsideChartArea = false;
            legend16.Name = "Legend1";
            legend16.TextWrapThreshold = 100;
            this.chartElevation.Legends.Add(legend16);
            this.chartElevation.Location = new System.Drawing.Point(3, 16);
            this.chartElevation.Name = "chartElevation";
            series16.ChartArea = "ChartArea1";
            series16.CustomProperties = "LabelStyle=Bottom";
            series16.Font = new System.Drawing.Font("Microsoft Sans Serif", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            series16.IsValueShownAsLabel = true;
            series16.Legend = "Legend1";
            series16.Name = "Series1";
            series16.SmartLabelStyle.AllowOutsidePlotArea = System.Windows.Forms.DataVisualization.Charting.LabelOutsidePlotAreaStyle.Yes;
            series16.SmartLabelStyle.CalloutLineColor = System.Drawing.Color.White;
            series16.SmartLabelStyle.CalloutLineWidth = 0;
            series16.SmartLabelStyle.Enabled = false;
            this.chartElevation.Series.Add(series16);
            this.chartElevation.Size = new System.Drawing.Size(464, 175);
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
            this.ClientSize = new System.Drawing.Size(566, 591);
            this.Controls.Add(this.imgLogo);
            this.Controls.Add(this.tabDetails);
            this.Controls.Add(this.btnClose);
            this.Controls.Add(this.tabEvents);
            this.Controls.Add(this.cmbUsers);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MinimumSize = new System.Drawing.Size(582, 564);
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
        private System.Windows.Forms.Button btnGraph;
        private System.Windows.Forms.Button btnImport;
        private System.Windows.Forms.Button btnExport;
    }
}