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
    public partial class frmGraph : Form {

        Session session;

        public frmGraph(Session ses) {
            InitializeComponent();
            this.session = ses;
        }

        private void btnClose_Click(object sender, EventArgs e) {
            this.Close();
        }

        private void frmGraph_Load(object sender, EventArgs e) {



            foreach (Shot s in session.Shots) {
                long seconds = (long)(s.timestamp - session.startTime).TotalSeconds;
                    chart.Series[0].Points.AddXY(seconds, s.decimalScore);
            }

            chart.ResetAutoValues();
            chart.Update();
        }
    }
}
