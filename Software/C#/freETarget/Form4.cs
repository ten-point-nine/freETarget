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
        public frmJournal() {
            InitializeComponent();
            storage = new StorageController();
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
            enableDisableButtons(false);
        }

        private void tabCoursesOfFire_SelectedIndexChanged(object sender, EventArgs e) {
            loadSessionsInList();
            enableDisableButtons(false);
        }

        private void loadSessionsInList() {
            lstbSessions.Items.Clear();
            CourseOfFire currentCOF = CourseOfFire.GetCourseOfFire(tabCoursesOfFire.SelectedTab.Text.Trim());
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
            this.Close();
        }

        private void lstbSessions_SelectedIndexChanged(object sender, EventArgs e) {
            ListBoxSessionItem item = (ListBoxSessionItem)lstbSessions.SelectedItem;
            if (item != null) {
                Console.WriteLine("id: " + item.id);
                Session session = storage.findSession(item.id);
                pGridSession.SelectedObject = session;
                currentSession = session;
                enableDisableButtons(true);
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
                    MemoryStream ms = new MemoryStream();

                    string s = diary.trtbPage.Rtf;
                    Console.WriteLine("rich text: "+s);
                    currentSession.diaryEntry = s;
                    storage.updateDiary(currentSession.id, currentSession.diaryEntry);
                }
            }
        }

        private void enableDisableButtons(bool input) {
            btnDelete.Enabled = input;
            btnDiary.Enabled = input;
            btnLoadSession.Enabled = input;
            btnPrint.Enabled = input;
            if (!input) {
                currentSession = null;
                pGridSession.SelectedObject = null;
            } 
        }
    }
}
