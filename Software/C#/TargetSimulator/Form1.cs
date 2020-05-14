using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Globalization;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace TargetSimulator
{

    public partial class Form1 : Form
    {
        bool isConnected = false;
        int count = 0;
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
            if (isConnected == false)
            {
                string selectedPort = comboBox1.GetItemText(comboBox1.SelectedItem);
                serialPort1.PortName = selectedPort;
                serialPort1.BaudRate = 115200; 
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

                statusText.Text = "Connected";
                count = 0;
                
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

                timer1.Enabled = false;
                btnTimer.Text = "Start Timer";

                statusText.Text = "Disconnected";
            }
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            var rand = new Random();
            timer1.Interval = rand.Next(100, 300);
            decimal xPos = rand.Next(-750, 750) / 10m;
            decimal yPos = rand.Next(-750, 750) / 10m;
            generateAndSend(xPos,yPos);
        }

        void generateAndSend(decimal xPos, decimal yPos)
        {

            decimal radius = (decimal)pitagora(xPos, yPos);
            decimal angle = (decimal)findDegree((float)yPos, (float)xPos);

            string command = "{\"shot\":" + count + ", \"x\":" + xPos.ToString("F1", CultureInfo.InvariantCulture) + ", \"y\":" + yPos.ToString("F1", CultureInfo.InvariantCulture) + ", \"r\":" + radius.ToString("F2", CultureInfo.InvariantCulture) + ", \"a\":" + angle.ToString("F2", CultureInfo.InvariantCulture) + "}";
            
          
            txtOutput.AppendText(DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss:ffff") + " | ");
            txtOutput.AppendText(command);
            txtOutput.AppendText(Environment.NewLine);

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
            decimal xPos = rand.Next(-750, 750) / 10m;
            decimal yPos = rand.Next(-750, 750) / 10m;
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
            decimal xPos = -75;
            decimal yPos = 0;
            generateAndSend(xPos, yPos);
        }

        private void btnTop_Click(object sender, EventArgs e)
        {
            decimal xPos = 0;
            decimal yPos = 75;
            generateAndSend(xPos, yPos);
        }

        private void btnBottom_Click(object sender, EventArgs e)
        {
            decimal xPos = 0;
            decimal yPos = -75;
            generateAndSend(xPos, yPos);
        }

        private void btnRight_Click(object sender, EventArgs e)
        {
            decimal xPos = 75;
            decimal yPos = 0;
            generateAndSend(xPos, yPos);
        }

        private void btnTopRight_Click(object sender, EventArgs e)
        {
            decimal xPos = 75;
            decimal yPos = 75;
            generateAndSend(xPos, yPos);
        }

        private void btnHalfway_Click(object sender, EventArgs e)
        {
            decimal xPos = 37.5m;
            decimal yPos = 37.5m;
            generateAndSend(xPos, yPos);
        }
    }
}
