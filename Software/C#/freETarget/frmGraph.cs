using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;

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

            long totalSeconds = (long)(session.endTime - session.startTime).TotalSeconds;

            decimal[] x = new decimal[totalSeconds];

            foreach (Shot s in session.Shots) {
                long seconds = (long)(s.timestamp - session.startTime).TotalSeconds;
                //chart.Series[0].Points.AddXY(seconds, s.decimalScore);
                x[seconds] = s.decimalScore;
            }

            for(int i = 0; i < totalSeconds; i++) {
                chart.Series[0].Points.AddXY(i, x[i]);
            }

            chart.ResetAutoValues();
            chart.Update();
            //chart.SaveImage("chartTemp.jpg", System.Windows.Forms.DataVisualization.Charting.ChartImageFormat.Png);
        }

        public Chart getChart() {
            return chart;
        }
    }
}
