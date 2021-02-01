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
    public partial class frmDiary : Form {
        public frmDiary() {
            InitializeComponent();
        }

        private void btnOK_Click(object sender, EventArgs e) {
            this.Close();
        }

        private void btnCancel_Click(object sender, EventArgs e) {
            this.Close();
        }

        public void initPage() {
            string text = "";
            text += "Gun:" + Environment.NewLine;
            text += "Gun adjustments:" + Environment.NewLine;
            text += "Pellets:" + Environment.NewLine;
            text += "Location:" + Environment.NewLine;
            text += "Weather:" + Environment.NewLine;
            text += "Goal:" + Environment.NewLine;
            text += "Influences:" + Environment.NewLine;
            text += "Exercises:" + Environment.NewLine;
            text += "Problems/Solutions:" + Environment.NewLine;
            text += "Positive/Learned:" + Environment.NewLine;

            trtbPage.Text = text;
        }

        private void miBold_Click(object sender, EventArgs e) {
            Font f = trtbPage.SelectionFont;
            if (f.Bold) {
                trtbPage.SelectionFont = new Font(f, FontStyle.Regular);
            } else {
                trtbPage.SelectionFont = new Font(f, FontStyle.Bold);
            }
        }

        private void miItalic_Click(object sender, EventArgs e) {
            Font f = trtbPage.SelectionFont;
            if (f.Italic) {
                trtbPage.SelectionFont = new Font(f, FontStyle.Regular);
            } else {
                trtbPage.SelectionFont = new Font(f, FontStyle.Italic);
            }
        }

        private void miIncreaseSize_Click(object sender, EventArgs e) {
            Font f = trtbPage.SelectionFont;
            trtbPage.SelectionFont = new Font(f.FontFamily, f.Size + 2);

        }

        private void miDecreaseSize_Click(object sender, EventArgs e) {
            Font f = trtbPage.SelectionFont;
            trtbPage.SelectionFont = new Font(f.FontFamily, f.Size - 2);
        }
    }
}
