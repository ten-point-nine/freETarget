using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace freETarget {
    public partial class frmCalibration : Form {

        private static frmCalibration instance;

        frmMainWindow mainWindow;
        private frmCalibration(frmMainWindow mainWin) {
            InitializeComponent();
            this.mainWindow = mainWin;
        }

        public static frmCalibration getInstance(frmMainWindow mainWin) {
            if (instance != null) {
                return instance;
            } else {
                instance = new frmCalibration(mainWin);
                return instance;
            }
        }

        private void btnUp_Click(object sender, EventArgs e) {
            mainWindow.calibrateY(getIncrement());
        }

        private void btnDown_Click(object sender, EventArgs e) {
            mainWindow.calibrateY(-getIncrement());
        }

        private void btnLeft_Click(object sender, EventArgs e) {
            mainWindow.calibrateX(-getIncrement());
        }

        private void btnRight_Click(object sender, EventArgs e) {
            mainWindow.calibrateX(getIncrement());
        }

        private void btnReset_Click(object sender, EventArgs e) {
            mainWindow.resetCalibration();
        }

        private void btnClose_Click(object sender, EventArgs e) {
            mainWindow.saveCalibration();
            this.Close();
        }

        private decimal getIncrement() {
            string s = txtIncrement.Text;
            decimal ret = 0.5m;
            try {
                ret = Decimal.Parse(s, CultureInfo.InvariantCulture);
            }catch(Exception ex) {
                Console.WriteLine("parse error " + ex.Message);
            }

            if (ret < 0.01m) {
                ret = 0.01m;
            }

            if(ret > 10) {
                ret = 10;
            }

            return ret;
        }

        private void frmCalibration_FormClosing(object sender, FormClosingEventArgs e) {
            e.Cancel = true;
            this.Hide();
        }
    }
}
