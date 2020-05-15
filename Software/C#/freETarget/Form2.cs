using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO.Ports;
using System.Linq;
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
                Console.WriteLine(port);
                if (ports[0] != null)
                {
                    cmbPorts.SelectedItem = ports[0];
                }
            }

            cmbWeapons.Items.AddRange(frmMainWindow.supportedTargets);

            loadSettings();
        }

        private void loadSettings()
        {
            txtName.Text = Properties.Settings.Default.name;
            txtBaud.Text = Properties.Settings.Default.baudRate.ToString();
            chkDisplayConsole.Checked = Properties.Settings.Default.displayDebugConsole;
            cmbPorts.SelectedItem = Properties.Settings.Default.portName;
            cmbWeapons.SelectedItem = Properties.Settings.Default.defaultTarget;

        }
    }
}
