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
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Binary;

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

            loadEventsOnTabs();
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
            Event currentCOF = mainWindow.eventManager.findEventByName(tabEvents.SelectedTab.Text.Trim());
            if (cmbUsers.SelectedItem != null) {
                List<ListBoxSessionItem> list = storage.findSessionsForUser(cmbUsers.SelectedItem.ToString(), currentCOF);
                foreach (ListBoxSessionItem item in list) {
                    lstbSessions.Items.Add(item);
                }
            }
        }

        private void loadStatistics() {
            clearCharts();
            Event currentEvent = mainWindow.eventManager.findEventByName(tabEvents.SelectedTab.Text.Trim());
            if (cmbUsers.SelectedItem != null) {
                loadScoreStatistics(cmbUsers.SelectedItem.ToString(), currentEvent);
                loadMeanRadiusStatistics(cmbUsers.SelectedItem.ToString(), currentEvent);
                loadWindageStatistics(cmbUsers.SelectedItem.ToString(), currentEvent);
                loadElevationStatistics(cmbUsers.SelectedItem.ToString(), currentEvent);
            }
        }

        private void loadScoreStatistics(string user, Event eventType) {
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

        private void loadMeanRadiusStatistics(string user, Event eventType) {
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

        private void loadWindageStatistics(string user, Event eventType) {
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

        private void loadElevationStatistics(string user, Event eventType) {
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

                Session clonedSession = Clone(session);
                pGridSession.SelectedObject = clonedSession;
                currentSession = session;
                enableDisableButtons(true, true);
                listSessionLastIndex = lstbSessions.SelectedIndex;

                btnExport.Enabled = true;
            } else {
                btnExport.Enabled = false;
            }
        }

        private T Clone<T>(T source) {
            if (!typeof(T).IsSerializable) {
                throw new ArgumentException("The type must be serializable.", nameof(source));
            }

            // Don't serialize a null object, simply return the default for that object
            if (ReferenceEquals(source, null)) return default;

            Stream stream = new MemoryStream();
            IFormatter formatter = new BinaryFormatter();
            formatter.Serialize(stream, source);
            stream.Seek(0, SeekOrigin.Begin);
            return (T)formatter.Deserialize(stream);
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
            mainWindow.loadSessionFromJournal(currentSession);
            enableDisableButtons(true, false);
            isLoading = false;
        }

        private void frmJournal_FormClosing(object sender, FormClosingEventArgs e) {
            if (isLoading) {
                e.Cancel = true;
                return;
            }
            mainWindow.unloadJournalSession();
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

            if (Directory.Exists(Settings.Default.pdfPath) == false) {
                try {
                    Directory.CreateDirectory(Settings.Default.pdfPath);
                }catch(Exception ex) {
                    mainWindow.log(ex.Message);
                    MessageBox.Show("Path " + Settings.Default.pdfPath + " not valid. Cannot save PDF","Error saving PDF", MessageBoxButtons.OK,MessageBoxIcon.Error);
                    return;
                }
            }

            var stream = new System.IO.MemoryStream();
            imgLogo.Image.Save(stream, ImageFormat.Png);
            stream.Position = 0;

            //print session from DB, not the one currently loaded
            Session dbSession = storage.findSession(currentSession.id);
            PDFGenerator.generateAndSavePDF(dbSession, Settings.Default.pdfPath, stream);
        }

        public bool isSessionLoading() {
            return isLoading;
        }

        private void btnGraph_Click(object sender, EventArgs e) {
            frmGraph graph = new frmGraph(currentSession);
            graph.ShowDialog();
        }


        private void loadEventsOnTabs() {

            this.tabEvents.Controls.Clear();
            int index = 0;
            foreach (Event ev in mainWindow.eventManager.getActiveEventsList()) {

                TabPage tab = new System.Windows.Forms.TabPage();

                tab.BackColor = ev.TabColor;
                //tab.Location = new System.Drawing.Point(52, 4);
                tab.Name = ev.ID.ToString();
                //tab.Padding = new System.Windows.Forms.Padding(5);
                //tab.Size = new System.Drawing.Size(250, 522);
                tab.TabIndex = index++;
                tab.Text = ev.Name;

                this.tabEvents.Controls.Add(tab);
            }
        }

        private void btnExport_Click(object sender, EventArgs e) {
            if (currentSession != null) {
                StringBuilder sb = new StringBuilder();
                sb.Append(currentSession.eventType.Name + "~");
                sb.Append(currentSession.user + "~");
                sb.Append(currentSession.score.ToString(CultureInfo.InvariantCulture) + "~");
                sb.Append(currentSession.decimalScore.ToString(CultureInfo.InvariantCulture) + "~");
                sb.Append(currentSession.innerX.ToString(CultureInfo.InvariantCulture) + "~");
                sb.Append(currentSession.xbar.ToString(CultureInfo.InvariantCulture) + "~");
                sb.Append(currentSession.ybar.ToString(CultureInfo.InvariantCulture) + "~");
                sb.Append(currentSession.rbar.ToString(CultureInfo.InvariantCulture) + "~");
                sb.Append(currentSession.groupSize.ToString(CultureInfo.InvariantCulture) + "~");
                sb.Append(StorageController.convertListOfShotsToString(currentSession.Shots) + "~");
                sb.Append(StorageController.convertDatetimeToString(currentSession.startTime) + "~");
                sb.Append(StorageController.convertDatetimeToString(currentSession.endTime) + "~");
                sb.Append(currentSession.averageScore.ToString("F2", CultureInfo.InvariantCulture) + "~");
                sb.Append(currentSession.actualNumberOfShots.ToString(CultureInfo.InvariantCulture) + "~");
                sb.Append(currentSession.diaryEntry + "~");
                sb.Append(StorageController.convertTimespanToDecimal(currentSession.averageTimePerShot).ToString("F3", CultureInfo.InvariantCulture) + "~");
                sb.Append(StorageController.convertTimespanToDecimal(currentSession.longestShot).ToString("F3", CultureInfo.InvariantCulture) + "~");
                sb.Append(StorageController.convertTimespanToDecimal(currentSession.shortestShot).ToString("F3", CultureInfo.InvariantCulture) + "~");
                string controlString = StorageController.getControlString(currentSession);
                sb.Append(StorageController.GetMd5Hash(controlString));

                string exportDirectory = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments) + @"\freETarget\sessionExport\";

                if (!Directory.Exists(exportDirectory)) {
                    try {
                        Directory.CreateDirectory(exportDirectory);
                    } catch (Exception ex) {
                        Console.WriteLine(ex.Message);

                        //if there is no write permission at exe location, write log in C:\Users\<user>\AppData\Roaming\freETarget\sessionExport
                        exportDirectory = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) + @"\freETarget\sessionExport\";
                        Directory.CreateDirectory(exportDirectory);
                    }
                }
                string sessionfilename = "freETargetSession_" + currentSession.user + "-" + currentSession.eventType.Name + "_" + currentSession.id + ".exp";

                string sesFile;
                if (!File.Exists(exportDirectory + sessionfilename)) {
                    FileStream log = null;
                    try {
                        log = File.Create(exportDirectory + sessionfilename);
                        sesFile = exportDirectory + sessionfilename;
                    } catch (Exception ex) {
                        Console.WriteLine(ex.Message);
                        sesFile = null;
                    } finally {
                        log.Close();
                    }

                } else {
                    sesFile = exportDirectory + sessionfilename;
                }

                try {
                    //Opens a new file stream which allows asynchronous reading and writing
                    using (StreamWriter sw = new StreamWriter(new FileStream(sesFile, FileMode.Append, FileAccess.Write, FileShare.ReadWrite))) {

                        sw.WriteLine(sb.ToString());
                        mainWindow.log("Session " + currentSession.id + " exported to " + sesFile);
                        mainWindow.displayMessage("Session " + currentSession.id + " exported to " + sesFile, false);

                    }
                } catch (Exception ex) {
                    //oh well...
                    mainWindow.log("Error exporting session: " + currentSession.id + Environment.NewLine + ex.Message);
                }
            }
        }

        private void btnImport_Click(object sender, EventArgs e) {
            using (OpenFileDialog openFileDialog = new OpenFileDialog()) {
                openFileDialog.InitialDirectory = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments) + @"\freETarget\sessionExport\"; ;
                openFileDialog.Filter = "exp files (*.exp)|*.exp|All files (*.*)|*.*";
                openFileDialog.FilterIndex = 1;
                openFileDialog.RestoreDirectory = true;

                if (openFileDialog.ShowDialog() == DialogResult.OK) {
                    string filePath = openFileDialog.FileName;
                    StreamReader sr = new StreamReader(filePath);
                    string fileData = sr.ReadToEnd();
                    string[] items = fileData.Split('~');

                    string eventName = items[0];
                    string user = items[1];
                    Event cof = mainWindow.eventManager.findEventByName(eventName);
                    if (cof == null) {
                        MessageBox.Show("Could not find event '"+eventName+"'. Please create it before importing.", "Error loading session", MessageBoxButtons.OK, MessageBoxIcon.Stop);
                        return;
                    }

                    Session session = Session.createNewSession(cof, user);

                    session.score = int.Parse(items[2], CultureInfo.InvariantCulture);
                    session.decimalScore = decimal.Parse(items[3], CultureInfo.InvariantCulture);
                    session.innerX = int.Parse(items[4], CultureInfo.InvariantCulture);
                    session.xbar = decimal.Parse(items[5], CultureInfo.InvariantCulture);
                    session.ybar = decimal.Parse(items[6], CultureInfo.InvariantCulture);
                    session.rbar = decimal.Parse(items[7], CultureInfo.InvariantCulture);
                    session.groupSize = decimal.Parse(items[8], CultureInfo.InvariantCulture);
                    session.Shots = StorageController.convertStringToListOfShots(items[9]);
                    session.startTime = StorageController.convertStringToDate(items[10]);
                    session.endTime = StorageController.convertStringToDate(items[11]);
                    session.averageScore = decimal.Parse(items[12], CultureInfo.InvariantCulture);
                    session.actualNumberOfShots = int.Parse(items[13], CultureInfo.InvariantCulture);
                    session.diaryEntry = items[14];
                    session.averageTimePerShot = StorageController.convertDecimalToTimespan(decimal.Parse(items[15], CultureInfo.InvariantCulture));
                    session.longestShot = StorageController.convertDecimalToTimespan(decimal.Parse(items[16], CultureInfo.InvariantCulture));
                    session.shortestShot = StorageController.convertDecimalToTimespan(decimal.Parse(items[17], CultureInfo.InvariantCulture));
                    string hash = items[18].Trim();
                    string controlString = StorageController.getControlString(session);
                    if (StorageController.VerifyMd5Hash(controlString, hash) == false) {
                        MessageBox.Show("MD5 check failed. Session data corrupted/modified.", "Error loading session", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    } else {
                        //session valid. store it
                        mainWindow.storage.storeSession(session, false);
                        loadData();
                        mainWindow.log("Session from file "+ filePath + " imported succesfully.");
                        mainWindow.displayMessage("Session from file " + filePath + " imported succesfully.", false);
                    }
                }
            }
        }
    }
}
