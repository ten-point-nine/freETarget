using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Globalization;
using System.IO.Ports;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace freETarget
{
    public partial class frmMainWindow : Form
    {
        bool isConnected = false;
        private delegate void SafeCallDelegate(string text, Shot shot);
        private delegate void SafeCallDelegate2(string text);
        List<Shot> shots = new List<Shot>();
        int score = 0;
        decimal decimalScore = 0m;
        int innerX = 0; 

        public struct Shot
        {
            public int count;
            public decimal x;
            public decimal y;
            public decimal radius;
            public decimal angle;
            public int score;
            public decimal decimalScore;
        }

        public frmMainWindow()
        {
            InitializeComponent();
            
        }



        private void serialPort_DataReceived(object sender, System.IO.Ports.SerialDataReceivedEventArgs e)
        {
            //received data from serial port
            SerialPort sp = (SerialPort)sender;
            string indata = sp.ReadExisting();
            //Console.WriteLine(indata);

            //parse input data to shot structure and determine score
            Shot shot = parseJson(indata);
            if (shot.count >= 0)
            {
                shots.Add(shot);
                score += shot.score;
                decimalScore += shot.decimalScore;

                if (shot.decimalScore >= 10.5m)
                {
                    innerX++;
                }

                drawShot(shot);
                writeShotData(indata, shot);
            }
            else
            {
                statusText.Text = "Error parsing shot";
                displayError("Error parsing shot");
            }
        
            
        }

        private void btnConnect_Click(object sender, EventArgs e)
        {
            if (isConnected == false)
            {
                serialPort.PortName = Properties.Settings.Default.portName;
                serialPort.BaudRate = Properties.Settings.Default.baudRate;
                try { 
                    serialPort.Open();


                    btnConnect.Text = "Disconnect";
                    isConnected = true;
                    statusText.Text = "Connected to " + serialPort.PortName;
                }
                catch (Exception ex) {
                    statusText.Text = "Error opening serial port: " + ex.Message;
                }

            }
            else
            {
                serialPort.Close();
                btnConnect.Text = "Connect";
                isConnected = false;

                statusText.Text = "Disconnected";
            }
        }

        //output errors to the status bar at the bottom
        private void displayError(string text)
        {
            if (txtOutput.InvokeRequired)
            {
                var d = new SafeCallDelegate2(displayError);
                txtOutput.Invoke(d, new object[] { text });
                return;
            }
            else
            {
                txtOutput.AppendText(text);
                txtOutput.AppendText(Environment.NewLine);
            }
        }

        //write data to text box
        private void writeShotData (string json, Shot shot)
        {
            //special code for UI thread safety
            if (txtOutput.InvokeRequired)
            {
                var d = new SafeCallDelegate(writeShotData);
                txtOutput.Invoke(d ,new object[] {json, shot});
                return;
            }
            else
            {
                string inner = "";
                if (shot.decimalScore >= 10.5m)
                {
                    inner = " *";
                }

                //write to console window (raw input string)
                txtOutput.AppendText(json);

                //write to total textbox
                txtTotal.Text = score.ToString() + " ( " + decimalScore.ToString() + " ) - " + innerX.ToString() + "x";

                //write to last shot textbox
                string lastShot = shot.decimalScore.ToString();
                txtLastShot.Text = lastShot + inner;

                //write score to listview
                ListViewItem item = new ListViewItem(new string[] { "" }, shot.count.ToString());
                item.UseItemStyleForSubItems = false;
                ListViewItem.ListViewSubItem countItem = item.SubItems.Add(shot.count.ToString());
                countItem.Font = new Font("MS Sans Serif", 7, FontStyle.Italic);
                ListViewItem.ListViewSubItem scoreItem = item.SubItems.Add(shot.score.ToString());
                ListViewItem.ListViewSubItem decimalItem = item.SubItems.Add(shot.decimalScore.ToString() + inner);

                shotsList.Items.Add(item);
                shotsList.EnsureVisible(shotsList.Items.Count - 1);

            }
        }

        //draw shot on target imagebox
        private void drawShot(Shot shot)
        {
            //transform shot coordinates to imagebox coordinates
            decimal x = transformX(shot.x, shot.y);
            decimal y = transformY(shot.x, shot.y);
            //Console.WriteLine("X:" + x + " - Y: " + y);

            //draw shot on target
            Brush b = new SolidBrush(Color.Blue);
            Pen p = new Pen(Color.LightSkyBlue);
            Graphics it = imgTarget.CreateGraphics();
            it.SmoothingMode = SmoothingMode.AntiAlias;
            it.FillEllipse(b, new Rectangle(new Point((int)Math.Round(x - 8), (int)Math.Round(y - 8)), new Size(11, 11)));
            it.DrawEllipse(p, new Rectangle(new Point((int)Math.Round(x - 8), (int)Math.Round(y - 8)), new Size(11, 11)));


            //draw direction arrow
            Bitmap bmp = new Bitmap(imgArrow.Image);
            Graphics g = Graphics.FromImage(bmp);
            g.SmoothingMode = SmoothingMode.AntiAlias;
            g.Clear(Color.White);

            if (shot.decimalScore < 10.9m)
            {
                RectangleF range = new RectangleF(5, 5, 19, 19);
                double xp = 14.0, yp = 14.0;
                double θ = (double)shot.angle * (Math.PI / 180);

                double[] t = new double[4];
                t[0] = (range.Left - xp) / Math.Cos(θ);
                t[1] = (range.Right - xp) / Math.Cos(θ);
                t[2] = (range.Top - yp) / Math.Sin(θ);
                t[3] = (range.Bottom - yp) / Math.Sin(θ);
                Array.Sort(t);
                // pick middle two points
                var X1 = xp + t[1] * Math.Cos(θ);
                var Y1 = yp + t[1] * Math.Sin(θ);
                var X2 = xp + t[2] * Math.Cos(θ);
                var Y2 = yp + t[2] * Math.Sin(θ);

                Pen arr = new Pen(Color.Black);
                arr.CustomEndCap = new AdjustableArrowCap(3, 3, true);
                g.DrawLine(arr, (float)X1, (float)Y2, (float)X2, (float)Y1);
            }
            else //if 10.9 draw a dot
            {
                Brush br = new SolidBrush(Color.Black);
                g.FillEllipse(br, new Rectangle(new Point(13, 13), new Size(4, 4)));
            }

            imgArrow.Image = bmp;

            //save drawn image (arrow) to imagelist for listview column
            imgListDirections.Images.Add(shot.count.ToString(),imgArrow.Image);
        }

        private decimal transformX (decimal xp, decimal yp)
        {
            //matrix magic from: https://docs.microsoft.com/en-us/previous-versions/windows/internet-explorer/ie-developer/samples/jj635757(v=vs.85)
            decimal a = 0.3m;
            decimal b = 0;
            decimal c = -75;
            decimal d = 75;

            decimal ret = (a * xp + b * yp - b * d - a * c) / (a * a + b * b);
            return ret;
        }

        private decimal transformY (decimal xp, decimal yp)
        {
            decimal a = 0.3m;
            decimal b = 0;
            decimal c = -75;
            decimal d = 75;

            decimal ret = (b * xp - a * yp - b * c + a * d) / (a * a + b * b);
            return ret;
        }


        private Shot parseJson(string json)
        {
            //parse json shot data
            Shot ret = new Shot();
            json = json.Trim();
            string t1 = json.Substring(1, json.Length - 2); //cut the bracket in front and the newline and bracket at the end
            string[] t2 = t1.Split(',');
            try
            {
                foreach (string t3 in t2)
                {
                    string[] t4 = t3.Split(':');
                    if (t4[0].Contains("shot"))
                    {
                        ret.count = int.Parse(t4[1], CultureInfo.InvariantCulture);
                    }
                    else if (t4[0].Contains("x"))
                    {
                        ret.x = decimal.Parse(t4[1], CultureInfo.InvariantCulture);
                    }
                    else if (t4[0].Contains("y"))
                    {
                        ret.y = decimal.Parse(t4[1], CultureInfo.InvariantCulture);
                    }
                    else if (t4[0].Contains("r"))
                    {
                        ret.radius = decimal.Parse(t4[1], CultureInfo.InvariantCulture);
                    }
                    else if (t4[0].Contains("a"))
                    {
                        ret.angle = decimal.Parse(t4[1], CultureInfo.InvariantCulture);
                    }
                }
            }catch(FormatException ex)
            {
                Console.WriteLine(ex.Message);
                Shot err = new Shot();
                err.count = -1;
                return err;
            }

            determineScore(ref ret);

            return ret;
        }

        private void determineScore(ref Shot shot)
        {
            //75 is the radius of the target circle
            if (shot.radius <= 75)
            {
                decimal t = shot.radius;
                decimal t2 = (t / 7.73m); //empirically determined constant
                decimal score = 10.9m - t2;
               
                shot.decimalScore = Math.Round(score, 1);
                shot.score = (int)Math.Floor(shot.decimalScore);
            }
            else //if outside the target, score is zero
            {
                shot.score = 0;
                shot.decimalScore = 0;
            }
        }

        private void imgArrow_LoadCompleted(object sender, AsyncCompletedEventArgs e)
        {
            if (e.Error != null)
            {
                // You got the Error image, e.Error tells you why
                Console.WriteLine(e.Error);
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            frmSettings settingsFrom = new frmSettings();
            if (settingsFrom.ShowDialog(this) == DialogResult.OK)
            {
                Properties.Settings.Default.name = settingsFrom.txtName.Text;
                Properties.Settings.Default.baudRate = int.Parse(settingsFrom.txtBaud.Text);
                Properties.Settings.Default.displayDebugConsole = settingsFrom.chkDisplayConsole.Checked;
                Properties.Settings.Default.portName = settingsFrom.cmbPorts.GetItemText(settingsFrom.cmbPorts.SelectedItem);
                Properties.Settings.Default.Save();

                displayDebugConsole(Properties.Settings.Default.displayDebugConsole);
            }

            settingsFrom.Dispose();
        }

        private void displayDebugConsole(bool display)
        {
            if (display)
            {
                frmMainWindow.ActiveForm.Width = 1061;
            }
            else
            {
                frmMainWindow.ActiveForm.Width = 738;
            }
        }

        private void frmMainWindow_Load(object sender, EventArgs e)
        {
            
        }

        private void frmMainWindow_Shown(object sender, EventArgs e)
        {
            displayDebugConsole(Properties.Settings.Default.displayDebugConsole);
        }

        private void frmMainWindow_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (isConnected)
            {
                serialPort.Close();
                btnConnect.Text = "Connect";
                isConnected = false;

                statusText.Text = "Disconnected";
            }
        }
    }


}
