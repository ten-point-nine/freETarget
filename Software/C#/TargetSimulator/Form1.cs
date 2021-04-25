using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using TargetSimulator.Properties;

namespace TargetSimulator
{

    public partial class Form1 : Form
    {
        public const decimal distanceBetweenSensors = 160; //in milimiters

        private int range = (int)Math.Round(distanceBetweenSensors * 10 / 2, 0);
        bool isConnected = false;
        int count = 1;
        String[] ports;


        public Form1()
        {
            InitializeComponent();

            getAvailablePorts();

            foreach (string port in ports)
            {
                comboBox1.Items.Add(port);
                Console.WriteLine(port);
                if (ports[0] != null)
                {
                    comboBox1.SelectedItem = ports[0];
                }
            }
        }


        void getAvailablePorts()
        {
            ports = SerialPort.GetPortNames();
        }

        private void btnConnect_Click(object sender, EventArgs e)
        {
            Console.WriteLine("Range " + range);
            if (isConnected == false)
            {
                string selectedPort = comboBox1.GetItemText(comboBox1.SelectedItem);
                serialPort1.PortName = selectedPort;
                serialPort1.BaudRate = Settings.Default.BaudRate; 
                serialPort1.WriteTimeout = 500;
                serialPort1.Open();
                btnConnect.Text = "Disconnect";
                isConnected = true;
                btnTimer.Enabled = true;
                btnShot.Enabled = true;

                btnBottom.Enabled = true;
                btnCenter.Enabled = true;
                btnHalfway.Enabled = true;
                btnLeft.Enabled = true;
                btnRight.Enabled = true;
                btnTop.Enabled = true;
                btnTopRight.Enabled = true;
                btnShoot.Enabled = true;
                btnImport.Enabled = true;
                btnImportLog.Enabled = true;
                btnMiss.Enabled = true;

                statusText.Text = "Connected";
                count = 1;

                serialPort1.Write("freETarget Simulator" + Environment.NewLine);


            } else {
                serialPort1.Close();
                btnConnect.Text = "Connect";
                isConnected = false;
                timer1.Enabled = false;
                btnTimer.Enabled = false;
                btnShot.Enabled = false;

                btnBottom.Enabled = false;
                btnCenter.Enabled = false;
                btnHalfway.Enabled = false;
                btnLeft.Enabled = false;
                btnRight.Enabled = false;
                btnTop.Enabled = false;
                btnTopRight.Enabled = false;
                btnShoot.Enabled = false;
                btnImport.Enabled = false;
                btnImportLog.Enabled = false;
                btnMiss.Enabled = false;

                timer1.Enabled = false;
                btnTimer.Text = "Start Timer";

                statusText.Text = "Disconnected";
            }
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            var rand = new Random();
            timer1.Interval = rand.Next(100, 300); //time between automated shots
            decimal xPos = rand.Next(-range, range) / 10m;
            decimal yPos = rand.Next(-range, range) / 10m;
            generateAndSend(xPos,yPos);
        }

        void generateAndSend(decimal xPos, decimal yPos)
        {

            decimal radius = (decimal)pitagora(xPos, yPos);
            decimal angle = (decimal)findDegree((float)yPos, (float)xPos);

            //Console.WriteLine("X: " + xPos + " Y: " + yPos + " Radius: " + radius);

            string command = "{\"shot\":" + count + ", \"x\":" + xPos.ToString("F2", CultureInfo.InvariantCulture) + ", \"y\":" + yPos.ToString("F2", CultureInfo.InvariantCulture) + ", \"r\":" + radius.ToString("F2", CultureInfo.InvariantCulture) + ", \"a\":" + angle.ToString("F2", CultureInfo.InvariantCulture) +"}" + Environment.NewLine;
            
          
            txtOutput.AppendText(DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss:ffff") + " | ");
            txtOutput.AppendText(command);

            try
            {
                serialPort1.WriteLine(command);
            }catch(TimeoutException ex)
            {
                statusText.Text = "Error writing to port: (" + count + ") " + ex.Message;
                Console.WriteLine("ERROR: ("+ count +") " + ex.Message);
                timer1.Enabled = false;
                btnTimer.Text = "Start Timer";
            }

            count++;
        }

        void generateMissAndSend() {
            string command = "{\"shot\":0, \"miss\": 1,\"name\":\"BOSS\", \"x\": 0, \"y\": 0}" + Environment.NewLine;


            txtOutput.AppendText(DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss:ffff") + " | ");
            txtOutput.AppendText(command);

            try {
                serialPort1.WriteLine(command);
            } catch (TimeoutException ex) {
                statusText.Text = "Error writing to port: (" + count + ") " + ex.Message;
                Console.WriteLine("ERROR: (" + count + ") " + ex.Message);
                timer1.Enabled = false;
                btnTimer.Text = "Start Timer";
            }

        }

        public float findDegree(float x, float y)
        {
            float value = (float)((System.Math.Atan2(x, y) / System.Math.PI) * 180f);
            if (value < 0) value += 360f;
            return value;
        }

        double pitagora(decimal x, decimal y)
        {
            return Math.Sqrt(Math.Pow((double)Math.Abs(x), 2) + Math.Pow((double)Math.Abs(y), 2));
        }

        private void btnTimer_Click(object sender, EventArgs e)
        {
            if (timer1.Enabled==false)
            {
                timer1.Enabled = true;
                btnTimer.Text = "Stop Timer";
            } else {
                timer1.Enabled = false;
                btnTimer.Text = "Start Timer";
            }
            
        }

        private void btnShot_Click(object sender, EventArgs e)
        {
            var rand = new Random();
            decimal xPos = rand.Next(-range, range) / 10m;
            decimal yPos = rand.Next(-range, range) / 10m;
            generateAndSend(xPos, yPos);
        }

        private void btnCenter_Click(object sender, EventArgs e)
        {
            decimal xPos = 0;
            decimal yPos = 0;
            generateAndSend(xPos, yPos);
        }

        private void btnLeft_Click(object sender, EventArgs e)
        {
            decimal xPos = -range / 10m;
            decimal yPos = 0;
            generateAndSend(xPos, yPos);
        }

        private void btnTop_Click(object sender, EventArgs e)
        {
            decimal xPos = 0;
            decimal yPos = range / 10m;
            generateAndSend(xPos, yPos);
        }

        private void btnBottom_Click(object sender, EventArgs e)
        {
            decimal xPos = 0;
            decimal yPos = -range / 10m;
            generateAndSend(xPos, yPos);
        }

        private void btnRight_Click(object sender, EventArgs e)
        {
            decimal xPos = range / 10m;
            decimal yPos = 0;
            generateAndSend(xPos, yPos);
        }

        private void btnTopRight_Click(object sender, EventArgs e)
        {
            decimal xPos = range / 10m;
            decimal yPos = range / 10m;
            generateAndSend(xPos, yPos);
        }

        private void btnHalfway_Click(object sender, EventArgs e)
        {
            decimal xPos = range / 20m;
            decimal yPos = range / 20m;
            generateAndSend(xPos, yPos);
        }

        private void bthShoot_Click(object sender, EventArgs e)
        {

            try
            {
                decimal xPos = Decimal.Parse(txtX.Text, CultureInfo.InvariantCulture);
                decimal yPos = Decimal.Parse(txtY.Text, CultureInfo.InvariantCulture);
                generateAndSend(xPos, yPos);
            }catch(Exception ex)
            {
                Console.WriteLine("Parse error: " + ex.Message);
            }
        }

        private void btnImport_Click(object sender, EventArgs e) {
            openFileDialog.Filter = "TargetScan App file|*.csv";
            DialogResult r = openFileDialog.ShowDialog();
            if ( r == DialogResult.OK) {
                StreamReader sr = new StreamReader(openFileDialog.FileName);
                string fileData = sr.ReadToEnd();
                string[] lines = fileData.Split( '\n');
                for (int i = 1; i < lines.Length; i++) {
                    string line = lines[i];
                    if (line != "") {
                        string[] items = line.Split(',');
                        decimal x = Decimal.Parse(items[3].Substring(1, items[3].Length - 2), CultureInfo.InvariantCulture);
                        decimal y = Decimal.Parse(items[4].Substring(1, items[4].Length - 2), CultureInfo.InvariantCulture);
                        string s = items[0].Substring(1, items[0].Length - 2);
                        Console.WriteLine("Shot: " + s + " Score: " + items[2] + " x: " + x + " y: " + y);

                        generateAndSend(x, y);
                        Thread.Sleep(300);
                        Application.DoEvents();
                    }
                }
            }
        }

        private void chkChamp_CheckedChanged(object sender, EventArgs e) {
            if (chkChamp.Checked) {
                range = 150;
            } else {
                range = (int)Math.Round(distanceBetweenSensors * 10 / 2, 0);
            }
        }

        private void btnImportLog_Click(object sender, EventArgs e) {
            openFileDialog.Filter = "Cleaned log file|*.logc";
            DialogResult r = openFileDialog.ShowDialog();
            if (r == DialogResult.OK) {
                StreamReader sr = new StreamReader(openFileDialog.FileName);
                string fileData = sr.ReadToEnd();
                string[] lines = fileData.Split('\n');
                for (int i = 0; i < lines.Length; i++) {
                    string line = lines[i];
                    if (line != "") {

                        line += Environment.NewLine;
                        txtOutput.AppendText(DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss:ffff") + " | ");
                        txtOutput.AppendText(line);

                        try {
                            serialPort1.WriteLine(line);
                        } catch (TimeoutException ex) {
                            statusText.Text = "Error writing to port: (" + count + ") " + ex.Message;
                            Console.WriteLine("ERROR: (" + count + ") " + ex.Message);
                            timer1.Enabled = false;
                            btnTimer.Text = "Start Timer";
                        }

                        Thread.Sleep(300);
                        Application.DoEvents();
                    }
                }
            }
        }

        private void btnMiss_Click(object sender, EventArgs e) {
            generateMissAndSend();
        }
    }
}
