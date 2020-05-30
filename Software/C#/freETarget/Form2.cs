using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO.Ports;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace freETarget
{
    public partial class frmSettings : Form
    {
        public frmSettings()
        {
            InitializeComponent();
        }

        private void frmSettings_Load(object sender, EventArgs e)
        {
            string[] ports = SerialPort.GetPortNames();

            foreach (string port in ports)
            {
                cmbPorts.Items.Add(port);
                if (ports[0] != null)
                {
                    cmbPorts.SelectedItem = ports[0];
                }
            }

            ArrayList ColorList = new ArrayList();
            Type colorType = typeof(System.Drawing.Color);
            PropertyInfo[] propInfoList = colorType.GetProperties(BindingFlags.Static | BindingFlags.DeclaredOnly | BindingFlags.Public);
            foreach (PropertyInfo c in propInfoList)
            {
                this.cmbColor.Items.Add(c.Name);
            }

            cmbWeapons.Items.Add("Air Pistol Practice");
            cmbWeapons.Items.Add("Air Rifle Practice");

            loadSettings();
        }

        private void loadSettings()
        {
            txtName.Text = Properties.Settings.Default.name;
            txtBaud.Text = Properties.Settings.Default.baudRate.ToString();
            chkDisplayConsole.Checked = Properties.Settings.Default.displayDebugConsole;
            cmbPorts.SelectedItem = Properties.Settings.Default.portName;
            cmbWeapons.SelectedItem = Properties.Settings.Default.defaultTarget;
            cmbColor.SelectedItem = Properties.Settings.Default.targetColor.Name;
            chkDrawMeanG.Checked = Properties.Settings.Default.drawMeanGroup;
            chkSeries.Checked = Properties.Settings.Default.OnlySeries;
            chkVoice.Checked = Properties.Settings.Default.voiceCommands;
            if (Properties.Settings.Default.MatchShots == 60) {
                rdb60.Checked = true;
                rdb40.Checked = false;
            } else if (Properties.Settings.Default.MatchShots == 40) {
                rdb60.Checked = false;
                rdb40.Checked = true;
            } else {
                rdb60.Checked = false;
                rdb40.Checked = false;
            }
        }

        private void cmbColor_DrawItem(object sender, DrawItemEventArgs e)
        {
            e.DrawBackground();

            Graphics g = e.Graphics;
            Rectangle rect = e.Bounds;
            if (e.Index >= 0)
            {
                string n = ((ComboBox)sender).Items[e.Index].ToString();
                Font f = new Font("Microsoft Sans Serif", 8.25f, FontStyle.Regular);
                Color c = Color.FromName(n);
                Brush b = new SolidBrush(c);
                g.DrawString(n, f, Brushes.Black, rect.X, rect.Top);
                g.FillRectangle(b, rect.X + 130, rect.Y + 5, rect.Width - 5, rect.Height - 7);
            }
        }
    }
}
