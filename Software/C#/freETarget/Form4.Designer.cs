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
            this.cmbUsers = new System.Windows.Forms.ComboBox();
            this.tabCoursesOfFire = new System.Windows.Forms.TabControl();
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
            this.tabCoursesOfFire.SuspendLayout();
            this.tabDetails.SuspendLayout();
            this.tabSessionList.SuspendLayout();
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
            // tabCoursesOfFire
            // 
            this.tabCoursesOfFire.Alignment = System.Windows.Forms.TabAlignment.Left;
            this.tabCoursesOfFire.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.tabCoursesOfFire.Controls.Add(this.tabPistolPractice);
            this.tabCoursesOfFire.Controls.Add(this.tabPistolMatch);
            this.tabCoursesOfFire.Controls.Add(this.tabPistolFinal);
            this.tabCoursesOfFire.Controls.Add(this.tabRiflePractice);
            this.tabCoursesOfFire.Controls.Add(this.tabRifleMatch);
            this.tabCoursesOfFire.Controls.Add(this.tabRifleFinal);
            this.tabCoursesOfFire.DrawMode = System.Windows.Forms.TabDrawMode.OwnerDrawFixed;
            this.tabCoursesOfFire.Location = new System.Drawing.Point(514, 64);
            this.tabCoursesOfFire.Multiline = true;
            this.tabCoursesOfFire.Name = "tabCoursesOfFire";
            this.tabCoursesOfFire.SelectedIndex = 0;
            this.tabCoursesOfFire.Size = new System.Drawing.Size(56, 446);
            this.tabCoursesOfFire.TabIndex = 1;
            this.tabCoursesOfFire.DrawItem += new System.Windows.Forms.DrawItemEventHandler(this.tabSessions_DrawItem);
            this.tabCoursesOfFire.SelectedIndexChanged += new System.EventHandler(this.tabCoursesOfFire_SelectedIndexChanged);
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
            this.tabDetails.Size = new System.Drawing.Size(496, 471);
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
            this.tabSessionList.Size = new System.Drawing.Size(488, 445);
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
            this.pGridSession.Size = new System.Drawing.Size(322, 403);
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
            this.lstbSessions.Size = new System.Drawing.Size(147, 433);
            this.lstbSessions.TabIndex = 0;
            this.lstbSessions.SelectedIndexChanged += new System.EventHandler(this.lstbSessions_SelectedIndexChanged);
            // 
            // tabStats
            // 
            this.tabStats.Location = new System.Drawing.Point(4, 22);
            this.tabStats.Name = "tabStats";
            this.tabStats.Padding = new System.Windows.Forms.Padding(3);
            this.tabStats.Size = new System.Drawing.Size(488, 445);
            this.tabStats.TabIndex = 1;
            this.tabStats.Text = "Statistics";
            this.tabStats.UseVisualStyleBackColor = true;
            // 
            // frmJournal
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(535, 526);
            this.Controls.Add(this.tabDetails);
            this.Controls.Add(this.btnClose);
            this.Controls.Add(this.tabCoursesOfFire);
            this.Controls.Add(this.cmbUsers);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MinimumSize = new System.Drawing.Size(551, 564);
            this.Name = "frmJournal";
            this.Text = "Journal";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.frmJournal_FormClosing);
            this.Load += new System.EventHandler(this.frmJournal_Load);
            this.tabCoursesOfFire.ResumeLayout(false);
            this.tabDetails.ResumeLayout(false);
            this.tabSessionList.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.ComboBox cmbUsers;
        private System.Windows.Forms.TabControl tabCoursesOfFire;
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
    }
}