using freETarget.Properties;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;

namespace freETarget {
    public partial class frmJournal : Form {

        private StorageController storage;
        private Session currentSession = null;
        frmMainWindow mainWindow;
        private bool isLoading = false;
        private int listSessionLastIndex = -1;


        public frmJournal(frmMainWindow mainWin) {
            InitializeComponent();
            storage = new StorageController(mainWin);
            this.mainWindow = mainWin;
        }

        private void frmJournal_Load(object sender, EventArgs e) {
            loadData();

        }

        private void loadData() {
            List<string> userList = storage.findAllUsers();
            cmbUsers.Items.Clear();
            cmbUsers.Items.AddRange(userList.ToArray());

            if (userList.Count > 0) {
                cmbUsers.SelectedIndex = 0;
                loadSessionsInList();
                loadStatistics();
            }
        }

        private void cmbUsers_SelectedIndexChanged(object sender, EventArgs e) {
            loadSessionsInList();
            loadStatistics();
            enableDisableButtons(false, true);
        }

        private void tabEvents_SelectedIndexChanged(object sender, EventArgs e) {
            tabDetails.SelectedIndex = 0;
            loadSessionsInList();
            loadStatistics();
            enableDisableButtons(false, true);
        }

        private void loadSessionsInList() {
            lstbSessions.Items.Clear();
            EventType currentCOF = EventType.GetEvent(tabEvents.SelectedTab.Text.Trim());
            if (cmbUsers.SelectedItem != null) {
                List<ListBoxSessionItem> list = storage.findSessionsForUser(cmbUsers.SelectedItem.ToString(), currentCOF);
                foreach (ListBoxSessionItem item in list) {
                    lstbSessions.Items.Add(item);
                }
            }
        }

        private void loadStatistics() {
            clearCharts();
            EventType currentEvent = EventType.GetEvent(tabEvents.SelectedTab.Text.Trim());
            if (cmbUsers.SelectedItem != null) {
                loadScoreStatistics(cmbUsers.SelectedItem.ToString(), currentEvent);
                loadMeanRadiusStatistics(cmbUsers.SelectedItem.ToString(), currentEvent);
                loadWindageStatistics(cmbUsers.SelectedItem.ToString(), currentEvent);
                loadElevationStatistics(cmbUsers.SelectedItem.ToString(), currentEvent);
            }
        }

        private void loadScoreStatistics(string user, EventType eventType) {
            List<decimal> list = storage.findScoresForUser(user, eventType);
            if (list.Count < 1) {
                return;
            }
            Series series = chartScore.Series[0];
            decimal sum = 0;
            decimal min = 1000000;
            int k = 0;
            int avgCount = 0;

            for (int i = list.Count-1; i >= 0; i--) {
                series.Points.AddXY(k++, list[i]);
                if (i >= list.Count - 10) {
                    sum += list[i];
                    avgCount++;
                }
                if (min > list[i]) {
                    min = list[i];
                }
            }
            decimal avg = sum / avgCount;
            chartScore.Series[0].Name = "Last 10 sessions average score: " + avg.ToString("F2",CultureInfo.InvariantCulture);
            chartScore.ChartAreas[0].AxisY.Minimum = (double)min - 1;
            chartScore.ChartAreas[0].AxisY.Maximum = 10;
            chartScore.ResetAutoValues();
            chartScore.Update();
        }

        private void loadMeanRadiusStatistics(string user, EventType eventType) {
            List<decimal> list = storage.findRBarForUser(user, eventType);
            if (list.Count < 1) {
                return;
            }
            Series series = chartMeanRadius.Series[0];
            decimal sum = 0;
            decimal min = 1000000;
            decimal max = -1000000;
            int k = 0;
            int avgCount = 0;

            for (int i = list.Count - 1; i >= 0; i--) {
                series.Points.AddXY(k, Math.Round(list[i],2));
                if (min > list[i]) {
                    min = list[i];
                }
                if(max < list[i]) {
                    max = list[i];
                }
                if (i >= list.Count - 10) {
                    sum += list[i];
                    avgCount++;
                }
            }
            decimal avg = sum / avgCount;
            chartMeanRadius.Series[0].Name = "Last 10 sessions average mean radius: " + avg.ToString("F2", CultureInfo.InvariantCulture);
            chartMeanRadius.ChartAreas[0].AxisY.Minimum = (double)min - 1;
            chartMeanRadius.ChartAreas[0].AxisY.Maximum = (double)max + 1;
            chartMeanRadius.ResetAutoValues();
            chartMeanRadius.Update();
        }

        private void loadWindageStatistics(string user, EventType eventType) {
            List<decimal> list = storage.findXBarForUser(user, eventType);
            if (list.Count < 1) {
                return;
            }
            Series series = chartWindage.Series[0];
            decimal sum = 0;
            decimal min = 1000000;
            decimal max = -1000000;
            int k = 0;
            int avgCount = 0;

            for (int i = list.Count - 1; i >= 0; i--) {
                series.Points.AddXY(k, Math.Round( list[i],2));
                if (i >= list.Count - 10) {
                    sum += list[i];
                    avgCount++;
                }
                if (min > list[i]) {
                    min = list[i];
                }
                if (max < list[i]) {
                    max = list[i];
                }
            }
            decimal avg = sum / avgCount;
            chartWindage.Series[0].Name = "Last 10 sessions average windage: " + avg.ToString("F2", CultureInfo.InvariantCulture);
            chartWindage.ChartAreas[0].AxisY.Minimum = (double)min - 1;
            chartWindage.ChartAreas[0].AxisY.Maximum = (double)max + 1;
            chartWindage.ResetAutoValues();
            chartWindage.Update();
        }

        private void loadElevationStatistics(string user, EventType eventType) {
            List<decimal> list = storage.findYBarForUser(user, eventType);
            if (list.Count < 1) {
                return;
            }
            Series series = chartElevation.Series[0];
            decimal sum = 0;
            decimal min = 1000000;
            decimal max = -1000000;
            int k = 0;
            int avgCount = 0;

            for (int i = list.Count - 1; i >= 0; i--) {
                series.Points.AddXY(k, Math.Round(list[i],2));
                if (i >= list.Count - 10) {
                    sum += list[i];
                    avgCount++;
                }
                if (min > list[i]) {
                    min = list[i];
                }
                if (max < list[i]) {
                    max = list[i];
                }
            }
            decimal avg = sum / avgCount;
            chartElevation.Series[0].Name = "Last 10 sessions average elevation: " + avg.ToString("F2", CultureInfo.InvariantCulture);
            chartElevation.ChartAreas[0].AxisY.Minimum = (double)min - 1;
            chartElevation.ChartAreas[0].AxisY.Maximum = (double)max + 1;
            chartElevation.ResetAutoValues();
            chartElevation.Update();
        }

        private void clearCharts() {

            Chart[] charts = new Chart[] { chartScore, chartMeanRadius, chartWindage, chartElevation };

            foreach(Chart c in charts) {
                c.Series[0].Name = "Last 10 sessions average";
                c.Series[0].Points.Clear();
                c.ChartAreas[0].AxisY.Minimum = 0;
                c.ResetAutoValues();
                c.Update();
            }
        }

        private void tabSessions_DrawItem(object sender, DrawItemEventArgs e) {
            Color backC = tabEvents.TabPages[e.Index].BackColor;
            Color foreC = tabEvents.TabPages[e.Index].ForeColor;
            if (tabEvents.Enabled == false) {
                int grayScale = (int)((backC.R * 0.3) + (backC.G * 0.59) + (backC.B * 0.11));
                backC = Color.FromArgb(backC.A, grayScale, grayScale, grayScale);
            }

            e.Graphics.FillRectangle(new SolidBrush(backC), e.Bounds);
            Rectangle paddedBounds = e.Bounds;
            paddedBounds.Inflate(-2, -2);
            StringFormat format1h = new StringFormat(StringFormatFlags.DirectionVertical | StringFormatFlags.DirectionRightToLeft);
            e.Graphics.DrawString(tabEvents.TabPages[e.Index].Text, this.Font, new SolidBrush(foreC), paddedBounds, format1h);
        }

        private void btnClose_Click(object sender, EventArgs e) {
            isLoading = false;
            this.Close();
        }

        private void lstbSessions_SelectedIndexChanged(object sender, EventArgs e) {
            ListBoxSessionItem item = (ListBoxSessionItem)lstbSessions.SelectedItem;
            if (item != null) {
                Session session = storage.findSession(item.id);
                if (session == null) {
                    lstbSessions.SelectedIndex = listSessionLastIndex;
                    return;
                }
                pGridSession.SelectedObject = session;
                currentSession = session;
                enableDisableButtons(true, true);
                listSessionLastIndex = lstbSessions.SelectedIndex;
            }
        }

        private void btnDiary_Click(object sender, EventArgs e) {
            frmDiary diary = new frmDiary();
            if (currentSession != null) {
                if (currentSession.diaryEntry == null || currentSession.diaryEntry == "") {
                    diary.initPage();
                } else {
                    string rtfText= currentSession.diaryEntry;
                    diary.trtbPage.Rtf = rtfText;
                }
                if (diary.ShowDialog() == DialogResult.OK) {
                    string s = diary.trtbPage.Rtf;
                    currentSession.diaryEntry = s;
                    storage.updateDiary(currentSession.id, currentSession.diaryEntry);
                }
            }
        }

        private void enableDisableButtons(bool input, bool clearSession) {
            btnDelete.Enabled = input;
            btnDiary.Enabled = input;
            btnLoadSession.Enabled = input;
            btnPrint.Enabled = input;
            btnGraph.Enabled = input;
            if (clearSession) {
                if (!input) {
                    currentSession = null;
                    pGridSession.SelectedObject = null;
                }
            }
        }

        private void btnLoadSession_Click(object sender, EventArgs e) {
            isLoading = true;
            enableDisableButtons(false, false);
            Application.DoEvents();
            mainWindow.loadSession(currentSession);
            enableDisableButtons(true, false);
            isLoading = false;
        }

        private void frmJournal_FormClosing(object sender, FormClosingEventArgs e) {
            if (isLoading) {
                e.Cancel = true;
                return;
            }
            mainWindow.clearSession();
            mainWindow.btnConnect.Enabled = true;
        }

        private void btnDelete_Click(object sender, EventArgs e) {
            DialogResult result = MessageBox.Show("Are you sure you want to permanently delete this session?", "Delete session", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
            if(result == DialogResult.Yes) {
                ListBoxSessionItem item = (ListBoxSessionItem)lstbSessions.SelectedItem;
                if (item != null) {
                    storage.deleteSession(item.id);
                    loadSessionsInList();
                    loadStatistics();
                    enableDisableButtons(false, true);
                }
                
            }
        }

        private void btnPrint_Click(object sender, EventArgs e) {
            var stream = new System.IO.MemoryStream();
            imgLogo.Image.Save(stream, ImageFormat.Png);
            stream.Position = 0;
            PDFGenerator.generateAndSavePDF(currentSession, Settings.Default.pdfPath, stream);
        }

        public bool isSessionLoading() {
            return isLoading;
        }

        private void btnGraph_Click(object sender, EventArgs e) {
            frmGraph graph = new frmGraph(currentSession);
            graph.ShowDialog();
        }
    }
}
