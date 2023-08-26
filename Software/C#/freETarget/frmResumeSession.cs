using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace freETarget {
    public partial class frmResumeSession : Form {

        frmMainWindow mainWindow;
        private StorageController storage;
        public ListBoxSessionItem selectedSession;

        public frmResumeSession(frmMainWindow mainWin) {
            InitializeComponent();
            this.mainWindow = mainWin;
            storage = new StorageController(mainWin);
        }

        private void frmResumeSession_Load(object sender, EventArgs e) {
            lstbSessions.Items.Clear();
            Event currentCOF = mainWindow.getCurrentSession().eventType;
            String currentUser = mainWindow.getCurrentSession().user;

            lblEvent.Text = currentCOF.ToString();
            lblUser.Text = currentUser;

            if (currentUser != null) {
                List<ListBoxSessionItem> list = storage.findSessionsForUser(currentUser, currentCOF);
                foreach (ListBoxSessionItem item in list) {
                    lstbSessions.Items.Add(item);
                }
            }
        }

        private void btnOK_Click(object sender, EventArgs e) {
            selectedSession = (ListBoxSessionItem)lstbSessions.SelectedItem;

            DialogResult = DialogResult.OK;
            this.Close();
        }
    }
}
