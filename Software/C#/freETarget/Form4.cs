using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace freETarget {
    public partial class frmJournal : Form {

        private StorageController storage;
        private Session currentSession = null;
        frmMainWindow mainWindow;
        private bool isLoading = false;

        private static frmJournal instance;

        public static frmJournal getInstance(frmMainWindow mainWin) {
            if (instance != null) {
                return instance;
            } else {
                instance = new frmJournal(mainWin);
                return instance;
            }
        }

        private frmJournal(frmMainWindow mainWin) {
            InitializeComponent();
            storage = new StorageController();
            this.mainWindow = mainWin;
        }

        private void frmJournal_Load(object sender, EventArgs e) {
            List<string> userList = storage.findAllUsers();

            cmbUsers.Items.AddRange(userList.ToArray());

            if (userList.Count > 0) {
                cmbUsers.SelectedIndex = 0;
                loadSessionsInList();
            }
        }

        private void cmbUsers_SelectedIndexChanged(object sender, EventArgs e) {
            loadSessionsInList();
            enableDisableButtons(false, true);
        }

        private void tabCoursesOfFire_SelectedIndexChanged(object sender, EventArgs e) {
            loadSessionsInList();
            enableDisableButtons(false, true);
        }

        private void loadSessionsInList() {
            lstbSessions.Items.Clear();
            EventType currentCOF = EventType.GetCourseOfFire(tabCoursesOfFire.SelectedTab.Text.Trim());
            if (cmbUsers.SelectedItem != null) {
                List<ListBoxSessionItem> list = storage.findSessionsForUser(cmbUsers.SelectedItem.ToString(), currentCOF);
                foreach (ListBoxSessionItem item in list) {
                    lstbSessions.Items.Add(item);
                }
            }
        }

        private void tabSessions_DrawItem(object sender, DrawItemEventArgs e) {
            Color backC = tabCoursesOfFire.TabPages[e.Index].BackColor;
            Color foreC = tabCoursesOfFire.TabPages[e.Index].ForeColor;
            if (tabCoursesOfFire.Enabled == false) {
                int grayScale = (int)((backC.R * 0.3) + (backC.G * 0.59) + (backC.B * 0.11));
                backC = Color.FromArgb(backC.A, grayScale, grayScale, grayScale);
            }

            e.Graphics.FillRectangle(new SolidBrush(backC), e.Bounds);
            Rectangle paddedBounds = e.Bounds;
            paddedBounds.Inflate(-2, -2);
            StringFormat format1h = new StringFormat(StringFormatFlags.DirectionVertical | StringFormatFlags.DirectionRightToLeft);
            e.Graphics.DrawString(tabCoursesOfFire.TabPages[e.Index].Text, this.Font, new SolidBrush(foreC), paddedBounds, format1h);
        }

        private void btnClose_Click(object sender, EventArgs e) {
            mainWindow.clearSession();
            mainWindow.btnConnect.Enabled = true;
            this.Hide();
        }

        private void lstbSessions_SelectedIndexChanged(object sender, EventArgs e) {
            ListBoxSessionItem item = (ListBoxSessionItem)lstbSessions.SelectedItem;
            if (item != null) {
                Session session = storage.findSession(item.id);
                pGridSession.SelectedObject = session;
                currentSession = session;
                enableDisableButtons(true, true);
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
            e.Cancel = true;
            mainWindow.clearSession();
            mainWindow.btnConnect.Enabled = true;
            this.Hide();
        }

        private void btnDelete_Click(object sender, EventArgs e) {
            DialogResult result = MessageBox.Show("Are you sure you want to permanently delete this session?", "Delete session", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
            if(result == DialogResult.Yes) {
                ListBoxSessionItem item = (ListBoxSessionItem)lstbSessions.SelectedItem;
                if (item != null) {
                    storage.deleteSession(item.id);
                    loadSessionsInList();
                    enableDisableButtons(false, true);
                }
                
            }
        }

        private void btnPrint_Click(object sender, EventArgs e) {

        }
    }
}
