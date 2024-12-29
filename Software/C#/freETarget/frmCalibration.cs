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
            txtXoffset.Text = mainWindow.calibrationX.ToString(CultureInfo.InvariantCulture);
            txtYoffset.Text = mainWindow.calibrationY.ToString(CultureInfo.InvariantCulture);
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
            txtXoffset.Text = mainWindow.calibrationX.ToString(CultureInfo.InvariantCulture);
            txtYoffset.Text = mainWindow.calibrationY.ToString(CultureInfo.InvariantCulture);
        }

        private void btnDown_Click(object sender, EventArgs e) {
            mainWindow.calibrateY(-getIncrement());
            txtXoffset.Text = mainWindow.calibrationX.ToString(CultureInfo.InvariantCulture);
            txtYoffset.Text = mainWindow.calibrationY.ToString(CultureInfo.InvariantCulture);
        }

        private void btnLeft_Click(object sender, EventArgs e) {
            mainWindow.calibrateX(-getIncrement());
            txtXoffset.Text = mainWindow.calibrationX.ToString(CultureInfo.InvariantCulture);
            txtYoffset.Text = mainWindow.calibrationY.ToString(CultureInfo.InvariantCulture);
        }

        private void btnRight_Click(object sender, EventArgs e) {
            mainWindow.calibrateX(getIncrement());
            txtXoffset.Text = mainWindow.calibrationX.ToString(CultureInfo.InvariantCulture);
            txtYoffset.Text = mainWindow.calibrationY.ToString(CultureInfo.InvariantCulture);
        }

        private void btnReset_Click(object sender, EventArgs e) {
            mainWindow.resetCalibration();
            txtXoffset.Text = mainWindow.calibrationX.ToString(CultureInfo.InvariantCulture);
            txtYoffset.Text = mainWindow.calibrationY.ToString(CultureInfo.InvariantCulture);
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
                txtIncrement.Text = ret.ToString();
            }

            if(ret > 10) {
                ret = 10;
                txtIncrement.Text = ret.ToString();
            }

            return ret;
        }

        private decimal getAngle() {
            string s = txtAngle.Text;
            decimal ret = 1m;
            try {
                ret = Decimal.Parse(s, CultureInfo.InvariantCulture);
            } catch (Exception ex) {
                Console.WriteLine("parse error " + ex.Message);
            }

            if (ret < 0.01m) {
                ret = 0.01m;
                txtAngle.Text = ret.ToString();
            }

            return ret;
        }

        private void frmCalibration_FormClosing(object sender, FormClosingEventArgs e) {
            e.Cancel = true;
            this.Hide();
        }

        private void btnClockwise_Click(object sender, EventArgs e) {
            mainWindow.calibrateAngle(-getAngle());
        }

        private void btnAntiClockwise_Click(object sender, EventArgs e) {
            mainWindow.calibrateAngle(getAngle());
        }
    }
}
