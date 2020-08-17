using PdfSharp.Drawing.BarCodes;
using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace freETarget
{
    public partial class frmSettings : Form { 


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

            System.Reflection.Assembly assembly = System.Reflection.Assembly.GetExecutingAssembly();
            lblVersion.Text = "freETarget Project  -  v"+ assembly.GetName().Version.Major + "." + assembly.GetName().Version.Minor + "." + assembly.GetName().Version.Build + "   (c) 2020";
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
            txtPDFlocation.Text = Properties.Settings.Default.pdfPath;
            txtDistance.Text = Properties.Settings.Default.targetDistance.ToString();
            chkScoreVoice.Checked = Properties.Settings.Default.scoreVoice;
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

        private void btnBrowse_Click(object sender, EventArgs e) {
            folderBrowserDialog.SelectedPath = txtPDFlocation.Text;
            SendKeys.Send("{TAB}{TAB}{RIGHT}");
            if (folderBrowserDialog.ShowDialog() == DialogResult.OK) {
                txtPDFlocation.Text = folderBrowserDialog.SelectedPath;
            }
        }

        private void btnOK_Click(object sender, EventArgs e) {
            try {
                int.Parse(txtBaud.Text);
                
            }catch(Exception) {
                MessageBox.Show("Baud rate is not a number","Validation error",MessageBoxButtons.OK,MessageBoxIcon.Exclamation);
                return;
            }

            try {
                int.Parse(txtDistance.Text);

            } catch (Exception) {
                MessageBox.Show("Distance is not a number", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return;
            }

            if(int.Parse(txtDistance.Text) > 10 || int.Parse(txtDistance.Text)< 3) {
                MessageBox.Show("Target distance must be between 3 and 10 meters", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return;
            }

            if (!Directory.Exists(txtPDFlocation.Text)) {
                MessageBox.Show("PDF save location must exist", "Validation error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return;
            }
            

            DialogResult = DialogResult.OK;
            this.Close();
        }

        private void linkLabel1_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e) {
            System.Diagnostics.Process.Start(linkLabel.Text);
        }

        private void linkLabel1_LinkClicked_1(object sender, LinkLabelLinkClickedEventArgs e) {
            System.Diagnostics.Process.Start(linkLabel1.Text);
        }
    }
}
