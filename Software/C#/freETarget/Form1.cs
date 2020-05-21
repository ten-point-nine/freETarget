using freETarget.Properties;
using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Globalization;
using System.IO.Ports;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace freETarget {
    public partial class frmMainWindow : Form {

        //target sizes per ISSF rules
        const decimal targetSize = 170; //mm


        const decimal outterRingPistol = 155.5m; //mm
        const decimal ring2Pistol = 139.5m; //mm
        const decimal ring3Pistol = 123.5m; //mm
        const decimal ring4Pistol = 107.5m; //mm
        const decimal ring5Pistol = 91.5m; //mm
        const decimal ring6Pistol = 75.5m; //mm
        const decimal ring7Pistol = 59.5m; //mm
        const decimal ring8Pistol = 43.5m; //mm
        const decimal ring9Pistol = 27.5m; //mm
        const decimal ring10Pistol = 11.5m; //mm
        const decimal innerRingPistol = 5m; //mm

        const decimal innerTenRadiusPistol = 4.75m;

        const float pistol1X = 5.6f;
        const float pistol1Y = 10.3f;
        const float pistol2X = 4f;
        const float pistol2Y = 10.5f;


        readonly decimal[] ringsPistol = new decimal[] { outterRingPistol, ring2Pistol, ring3Pistol, ring4Pistol, ring5Pistol, ring6Pistol, ring7Pistol, ring8Pistol, ring9Pistol, ring10Pistol, innerRingPistol };

        const decimal outterRingRifle = 45.5m; //mm
        const decimal ring2Rifle = 40.5m; //mm
        const decimal ring3Rifle = 35.5m; //mm
        const decimal ring4Rifle = 30.5m; //mm
        const decimal ring5Rifle = 25.5m; //mm
        const decimal ring6Rifle = 20.5m; //mm
        const decimal ring7Rifle = 15.5m; //mm
        const decimal ring8Rifle = 10.5m; //mm
        const decimal ring9Rifle = 5.5m; //mm
        const decimal ring10Rifle = 0.5m; //mm

        const decimal innerTenRadiusRifle = 2.0m;

        const float rifle1X = 2.50f;
        const float rifle1Y = 10.0f;
        const float rifle2X = 2.25f;
        const float rifle2Y = 10.1f;

        readonly decimal[] ringsRifle = new decimal[] { outterRingRifle, ring2Rifle, ring3Rifle, ring4Rifle, ring5Rifle, ring6Rifle, ring7Rifle, ring8Rifle, ring9Rifle, ring10Rifle };

        public const decimal pelletCaliber = 4.5m;

        private DateTime connectTime;

        public const string AirPistol = "Air Pistol";
        public const string AirRifle = "Air Rifle";

        public static string[] supportedTargets = new string[] { AirPistol, AirRifle };

        private bool isConnected = false;
        private delegate void SafeCallDelegate(string text, Shot shot);
        private delegate void SafeCallDelegate2(string text);
        private delegate void SafeCallDelegate3();

        private int score = 0;
        private decimal decimalScore = 0m;
        private int innerX = 0;
        private decimal xbar = 0;
        private decimal ybar = 0;
        private decimal rbar = 0;

        //private decimal currentRange = 0;
        private string currentTarget = Settings.Default.defaultTarget;

        private List<Shot> shots = new List<Shot>();

        public struct Shot {
            public int count;
            public decimal x;
            public decimal y;
            public decimal radius;
            public decimal angle;
            public int score;
            public decimal decimalScore;
            public bool innerTen;
        }

        public frmMainWindow() {
            InitializeComponent();

        }



        private void serialPort_DataReceived(object sender, System.IO.Ports.SerialDataReceivedEventArgs e) {
            //received data from serial port
            SerialPort sp = (SerialPort)sender;
            string indata = sp.ReadExisting();
            //Console.WriteLine(indata);

            //parse input data to shot structure and determine score
            Shot shot = parseJson(indata);
            if (shot.count >= 0) {
                shots.Add(shot);
                score += shot.score;
                decimalScore += shot.decimalScore;

                if (shot.innerTen) {
                    innerX++;
                }


                drawArrow(shot);
                writeShotData(indata, shot);
                var d = new SafeCallDelegate3(formResize); //draw shot
                this.Invoke(d);
            } else {

                displayError("Error parsing shot " + indata);
            }


        }

        private void btnConnect_Click(object sender, EventArgs e) {
            if (isConnected == false) {
                serialPort.PortName = Properties.Settings.Default.portName;
                serialPort.BaudRate = Properties.Settings.Default.baudRate;
                try {
                    serialPort.Open();

                    connectTime =  DateTime.Now;
                    timer.Enabled = true;

                    btnConnect.Text = "Disconnect";
                    isConnected = true;
                    statusText.Text = "Connected to " + serialPort.PortName;
                } catch (Exception ex) {
                    statusText.Text = "Error opening serial port: " + ex.Message;
                }

            } else {
                serialPort.Close();
                btnConnect.Text = "Connect";
                isConnected = false;
                clearShots();

                statusText.Text = "Disconnected";
                timer.Enabled = false;
            }
        }

        //output errors to the status bar at the bottom
        private void displayError(string text) {
            if (txtOutput.InvokeRequired) {
                var d = new SafeCallDelegate2(displayError);
                txtOutput.Invoke(d, new object[] { text });
                return;
            } else {
                txtOutput.AppendText(text);
                txtOutput.AppendText(Environment.NewLine);
                //statusText.Text = text;
            }
        }



        //write data to text box
        private void writeShotData(string json, Shot shot) {
            //special code for UI thread safety
            if (txtOutput.InvokeRequired) {
                var d = new SafeCallDelegate(writeShotData);
                txtOutput.Invoke(d, new object[] { json, shot });
                return;
            } else {
                string inner = "";
                if (shot.innerTen) {
                    inner = " *";
                }

                //write to console window (raw input string)
                txtOutput.AppendText(json);

                //write to total textbox
                txtTotal.Text = shots.Count + " shots : " +  score.ToString() + " ( " + decimalScore.ToString() + " ) - " + innerX.ToString() + "x";

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

                calculateMeanRadius(out rbar, out xbar, out ybar);
                txtMeanRadius.Text = Math.Round(rbar, 1).ToString();
                txtWindage.Text = Math.Round(Math.Abs(xbar), 1).ToString();
                drawWindageAndElevation(xbar, ybar);
                txtElevation.Text = Math.Round(Math.Abs(ybar), 1).ToString();
                txtMaxSpread.Text = Math.Round(calculateMaxSpread(), 1).ToString();

            }
        }

        private void drawWindageAndElevation(decimal xbar, decimal ybar) {
            Graphics g = imgWindage.CreateGraphics();
            g.Clear(this.BackColor);
            g.SmoothingMode = SmoothingMode.AntiAlias;
            Pen arr = new Pen(Color.Black);
            arr.CustomEndCap = new AdjustableArrowCap(3, 3, true);
            if (xbar < 0) {
                g.DrawLine(arr, imgWindage.Width - 2, (imgWindage.Height / 2) - 2, 2, (imgWindage.Height / 2) - 2);
            } else {
                g.DrawLine(arr, 2, (imgWindage.Height / 2) - 2, imgWindage.Width - 2, (imgWindage.Height / 2) - 2);
            }

            Graphics g2 = imgElevation.CreateGraphics();
            g2.Clear(this.BackColor);
            g2.SmoothingMode = SmoothingMode.AntiAlias;
            if (ybar < 0) {
                g2.DrawLine(arr, (imgElevation.Height / 2) - 2, 2, (imgElevation.Height / 2) - 2, imgElevation.Width - 2);
            } else {
                g2.DrawLine(arr, (imgElevation.Height / 2) - 2, imgElevation.Width - 2, (imgElevation.Height / 2) - 2, 2);
            }

        }

        private int getZoom() {
            if (trkZoom.InvokeRequired) {
                int r = 1;
                trkZoom.Invoke(new MethodInvoker(delegate {
                    r = trkZoom.Value;
                }));
                return r;
            } else {
                return trkZoom.Value;
            }
        }

        //draw shot on target imagebox
        private void drawShot(Shot shot, Graphics it, int targetSize, decimal zoomFactor, int l) {
            //transform shot coordinates to imagebox coordinates

            PointF x = transform((float)shot.x, (float)shot.y, targetSize, zoomFactor);

            //draw shot on target
            int count = shots.Count;
            Color c = Color.FromArgb(0, 0, 255);
            Pen p = new Pen(Color.LightSkyBlue);
            Brush bText = new SolidBrush(Color.LightSkyBlue);
            if (l == count - 1) {
                c = Color.Aqua;
                p = new Pen(Color.Blue);
                bText = new SolidBrush(Color.Blue);
            }


            Brush b = new SolidBrush(c);


            it.SmoothingMode = SmoothingMode.AntiAlias;
            it.InterpolationMode = InterpolationMode.HighQualityBicubic;

            float peletSize = getDimension(targetSize, pelletCaliber, zoomFactor);

            x.X -= peletSize / 2;
            x.Y -= peletSize / 2;

            it.FillEllipse(b, new RectangleF(x, new SizeF(peletSize, peletSize)));
            it.DrawEllipse(p, new RectangleF(x, new SizeF(peletSize, peletSize)));

            StringFormat format = new StringFormat();
            format.LineAlignment = StringAlignment.Center;
            format.Alignment = StringAlignment.Center;

            Font f = new Font("Arial", peletSize / 3);

            x.X += 0.2f; //small adjustment for the number to be centered
            x.Y += 0.3f;
            it.DrawString(shot.count.ToString(), f, bText, new RectangleF(x, new SizeF(peletSize, peletSize)), format);
        }

        private void drawArrow(Shot shot) {
            //draw direction arrow
            Bitmap bmp = new Bitmap(imgArrow.Image);
            Graphics g = Graphics.FromImage(bmp);
            g.SmoothingMode = SmoothingMode.AntiAlias;
            g.Clear(Color.White);

            if (shot.decimalScore < 10.9m) {
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
            } else { //if 10.9 draw a dot
                Brush br = new SolidBrush(Color.Black);
                g.FillEllipse(br, new Rectangle(new Point(13, 13), new Size(4, 4)));
            }

            imgArrow.Image = bmp;

            //save drawn image (arrow) to imagelist for listview column
            try {
                imgListDirections.Images.Add(shot.count.ToString(), imgArrow.Image);
            } catch (Exception ex) {
                Console.WriteLine("Error adding image to list " + ex.Message);
            }

            Thread.Sleep(100);
        }

        private PointF transform(float xp, float yp, float size, decimal zoomFactor) {
            //matrix magic from: https://docs.microsoft.com/en-us/previous-versions/windows/internet-explorer/ie-developer/samples/jj635757(v=vs.85)

            System.Numerics.Matrix4x4 M = new System.Numerics.Matrix4x4(0, 0, 1, 0,
                                                                        0, 0, 0, 1,
                                                                        size, size, 1, 0,
                                                                       -size, size, 0, 1);

            System.Numerics.Matrix4x4 Minverted = new System.Numerics.Matrix4x4();
            System.Numerics.Matrix4x4.Invert(M, out Minverted);

            float shotRange = (float)(targetSize * zoomFactor) / 2f;
            System.Numerics.Matrix4x4 xyPrime = new System.Numerics.Matrix4x4(-shotRange, 0, 0, 0,
                                                                                shotRange, 0, 0, 0,
                                                                                shotRange, 0, 0, 0,
                                                                               -shotRange, 0, 0, 0);

            System.Numerics.Matrix4x4 abcd = System.Numerics.Matrix4x4.Multiply(Minverted, xyPrime);

            float a = abcd.M11;
            float b = abcd.M21;
            float c = abcd.M31;
            float d = abcd.M41;

            float x = (a * xp + b * yp - b * d - a * c) / (a * a + b * b);
            float y = (b * xp - a * yp - b * c + a * d) / (a * a + b * b);

            PointF ret = new PointF(x, y);
            return ret;
        }


        private Shot parseJson(string json) {
            //parse json shot data
            Shot ret = new Shot();
            json = json.Trim();
            string t1 = json.Substring(1, json.Length - 2); //cut the bracket in front and the newline and bracket at the end
            string[] t2 = t1.Split(',');
            try {
                foreach (string t3 in t2) {
                    string[] t4 = t3.Split(':');
                    if (t4[0].Contains("shot")) {
                        ret.count = int.Parse(t4[1], CultureInfo.InvariantCulture);
                    } else if (t4[0].Contains("x")) {
                        ret.x = decimal.Parse(t4[1], CultureInfo.InvariantCulture);
                    } else if (t4[0].Contains("y")) {
                        ret.y = decimal.Parse(t4[1], CultureInfo.InvariantCulture);
                    } else if (t4[0].Contains("r")) {
                        ret.radius = decimal.Parse(t4[1], CultureInfo.InvariantCulture);
                    } else if (t4[0].Contains("a")) {
                        ret.angle = decimal.Parse(t4[1], CultureInfo.InvariantCulture);
                    }
                }
            } catch (FormatException ex) {
                Console.WriteLine("Could not parse: " + json + " Error: " + ex.Message);
                Shot err = new Shot();
                err.count = -1;
                return err;
            }

            determineScore(ref ret);

            return ret;
        }

        private void determineScore(ref Shot shot) {

            //using liner interpolation with the "official" values found here: http://targettalk.org/viewtopic.php?p=100591#p100591


            if (currentTarget == AirPistol) {
                double score = linearInterpolation(pistol1X, pistol1Y, pistol2X, pistol2Y, (float)shot.radius);

                shot.decimalScore = (decimal) (Math.Truncate(score*10)) / 10m;
                shot.decimalScore += 0.0m; //add a decimal is the result is an integer

                if (shot.decimalScore < 1) { //shot outside the target
                    shot.decimalScore = 0;
                }

                shot.score = (int)Math.Floor(shot.decimalScore);

                //determine if inner ten (X)
                if (shot.radius <= innerTenRadiusPistol) {
                    shot.innerTen = true;
                } else {
                    shot.innerTen = false;
                }
            } else if (currentTarget == AirRifle) {
                double score = linearInterpolation(rifle1X, rifle1Y, rifle2X, rifle2Y, (float)shot.radius);

                shot.decimalScore = (decimal)(Math.Truncate(score * 10)) / 10m;
                shot.decimalScore += 0.0m; //add a decimal is the result is an integer

                if (shot.decimalScore >= 11m) { //the linear interpolation returns 11.000003814 for 0 (dead centre)
                    shot.decimalScore = 10.9m;
                }

                if (shot.decimalScore < 1) {//shot outside the target
                    shot.decimalScore = 0;
                }

                shot.score = (int)Math.Floor(shot.decimalScore);

                //determine if inner ten (X)
                if (shot.radius <= innerTenRadiusRifle) {
                    shot.innerTen = true;
                } else {
                    shot.innerTen = false;
                }
            } else {
                Console.WriteLine("Unknown current target " + currentTarget);
            }

        }

        private void imgArrow_LoadCompleted(object sender, AsyncCompletedEventArgs e) {
            if (e.Error != null) {
                // You got the Error image, e.Error tells you why
                Console.WriteLine("Image load error! " + e.Error);
            }
        }

        private void btnConfig_Click(object sender, EventArgs e) {
            frmSettings settingsFrom = new frmSettings();
            if (settingsFrom.ShowDialog(this) == DialogResult.OK) {
                Properties.Settings.Default.name = settingsFrom.txtName.Text;
                Properties.Settings.Default.baudRate = int.Parse(settingsFrom.txtBaud.Text);
                Properties.Settings.Default.displayDebugConsole = settingsFrom.chkDisplayConsole.Checked;
                Properties.Settings.Default.portName = settingsFrom.cmbPorts.GetItemText(settingsFrom.cmbPorts.SelectedItem);
                Properties.Settings.Default.defaultTarget = settingsFrom.cmbWeapons.GetItemText(settingsFrom.cmbWeapons.SelectedItem);
                Properties.Settings.Default.targetColor = Color.FromName(settingsFrom.cmbColor.GetItemText(settingsFrom.cmbColor.SelectedItem));
                Properties.Settings.Default.drawMeanGroup = settingsFrom.chkDrawMeanG.Checked;
                Properties.Settings.Default.Save();

                imgTarget.BackColor = Settings.Default.targetColor;

                displayDebugConsole(Properties.Settings.Default.displayDebugConsole);
            }

            settingsFrom.Dispose();
        }

        private void displayDebugConsole(bool display) {
            txtOutput.Visible = display;
            formResize();
        }

        private void frmMainWindow_Load(object sender, EventArgs e) {
            cmbWeapon.Items.AddRange(supportedTargets);
            setTarget();
            formResize();
            imgTarget.BackColor = Settings.Default.targetColor;
        }

        private void frmMainWindow_FormClosing(object sender, FormClosingEventArgs e) {
            if (isConnected) {
                serialPort.Close();
                btnConnect.Text = "Connect";
                isConnected = false;

                statusText.Text = "Disconnected";
            }
        }

        private void frmMainWindow_Shown(object sender, EventArgs e) {
            displayDebugConsole(Properties.Settings.Default.displayDebugConsole);
            cmbWeapon.SelectedItem = Properties.Settings.Default.defaultTarget;
        }

        private void cmbWeapon_SelectedIndexChanged(object sender, EventArgs e) {
            setTarget();

        }

        private void setTarget() {
            if (cmbWeapon.GetItemText(cmbWeapon.SelectedItem) == AirPistol) {
                trkZoom.Minimum = 1;
                trkZoom.Maximum = 5;
                trkZoom.Value = 1;

               // currentRange = outterRingPistol / 2m + pelletCaliber / 2m; //maximum range that can score a point 155.5 / 2 + 4.5 / 2 = 80mm
                currentTarget = AirPistol;
            } else if (cmbWeapon.GetItemText(cmbWeapon.SelectedItem) == AirRifle) {
                trkZoom.Minimum = 0;
                trkZoom.Maximum = 5;
                trkZoom.Value = 0;

                //currentRange = outterRingRifle / 2m + pelletCaliber / 2m; //maximum range that can score a point = 25mm
                currentTarget = AirRifle;
            }

            clearShots();
            drawTarget();
        }

        private void trkZoom_ValueChanged(object sender, EventArgs e) {
            drawTarget();

        }

        private void imgTarget_Paint(object sender, PaintEventArgs e) {
            // drawTarget();
        }

        private void frmMainWindow_Resize(object sender, EventArgs e) {
            formResize();
        }

        private void formResize() {
            int rightBorder = 10;
            if (Settings.Default.displayDebugConsole == true) {
                rightBorder = txtOutput.Width + 10;
            }
            int height = this.ClientSize.Height - 10 - statusStrip1.Height - imgTarget.Top;
            int width = this.ClientSize.Width - rightBorder - imgTarget.Left;

            if (height < width) {
                imgTarget.Height = height;
                imgTarget.Width = height;
            } else {
                imgTarget.Height = width;
                imgTarget.Width = width;

            }

            drawTarget();
        }

        private void drawTarget() {
            if (cmbWeapon.GetItemText(cmbWeapon.SelectedItem) == "") {
                cmbWeapon.SelectedItem = Properties.Settings.Default.defaultTarget;
            }

            if (cmbWeapon.GetItemText(cmbWeapon.SelectedItem) == AirPistol) {
                decimal zoomFactor = (decimal)(1 / (decimal)getZoom());
                imgTarget.Image = paintTarget(imgTarget.Height, 7, ringsPistol, zoomFactor, false);
            } else if (cmbWeapon.GetItemText(cmbWeapon.SelectedItem) == AirRifle) {
                decimal zoomFactor = (decimal)(1 / Math.Pow(2, getZoom()));
                imgTarget.Image = paintTarget(imgTarget.Height, 4, ringsRifle, zoomFactor, true);
            }
        }

        private float getDimension(decimal currentTargetSize, decimal milimiters, decimal zoomFactor) {
            return (float)((currentTargetSize * milimiters) / (targetSize * zoomFactor));
        }

        private Bitmap paintTarget(int dimension, int blackRingCutoff, decimal[] rings, decimal zoomFactor, bool solidInner) {
            Pen penBlack = new Pen(Color.Black);
            Pen penWhite = new Pen(Settings.Default.targetColor);
            Brush brushBlack = new SolidBrush(Color.Black);
            Brush brushWhite = new SolidBrush(Settings.Default.targetColor);

            Bitmap bmpTarget = new Bitmap(dimension, dimension);
            Graphics it = Graphics.FromImage(bmpTarget);
            it.SmoothingMode = SmoothingMode.AntiAlias;

            int r = 1;
            for (int i = 0; i < rings.Length; i++) {

                Pen p = null;
                Brush b = null;
                Brush bText = null;
                if (r < blackRingCutoff) {
                    p = penBlack;
                    b = brushWhite;
                    bText = brushBlack;
                } else {
                    p = penWhite;
                    b = brushBlack;
                    bText = brushWhite;
                }
                //float circle = (float)((dimension * rings[i]) / (targetSize * zoomFactor));
                float circle = getDimension(dimension, rings[i], zoomFactor);
                float center = (float)(dimension / 2);
                float x = center - (circle / 2);
                float y = center + (circle / 2);

                if (solidInner && i == rings.Length - 1) //rifle target - last ring (10) is a solid dot
                {
                    it.FillEllipse(brushWhite, x, x, circle, circle);
                } else {
                    it.FillEllipse(b, x, x, circle, circle);
                    it.DrawEllipse(p, x, x, circle, circle);
                }

                if (r < 9) //for ring 9 and after no text is displayed
                {
                    float nextCircle = getDimension(dimension, rings[i + 1], zoomFactor);
                    float diff = circle - nextCircle;
                    float fontSize = diff / 8f; //8 is empirically determinted for best look
                    Font f = new Font("Arial", fontSize);

                    StringFormat format = new StringFormat();
                    format.LineAlignment = StringAlignment.Center;
                    format.Alignment = StringAlignment.Center;

                    it.DrawString(r.ToString(), f, bText, center, x + (diff / 4), format);
                    it.DrawString(r.ToString(), f, bText, center, y - (diff / 4), format);
                    it.DrawString(r.ToString(), f, bText, x + (diff / 4), center, format);
                    it.DrawString(r.ToString(), f, bText, y - (diff / 4), center, format);
                }
                r++;
            }

            it.DrawRectangle(penBlack, 0, 0, dimension - 1, dimension - 1);

            int index = 0;
            foreach (Shot shot in shots) {
                drawShot(shot, it, dimension, zoomFactor, index++);
            }

            if (Settings.Default.drawMeanGroup) {
                drawMeanGroup(it, dimension, zoomFactor);
            }

            return bmpTarget;
        }
        private void drawMeanGroup(Graphics it, decimal currentTargetSize, decimal zoomFactor) {
            if (shots.Count >= 2) {
                float circle = getDimension(currentTargetSize, rbar*2, zoomFactor);

                PointF x = transform((float)xbar, (float)ybar, (float)currentTargetSize, zoomFactor);
                Pen p = new Pen(Color.Red, 2);

                it.DrawEllipse(p, x.X - (circle / 2), x.Y - (circle / 2), circle, circle);

                float cross = 5; // center of group cross is always the same size - 5 pixels

                it.DrawLine(p, x.X - cross, x.Y, x.X + cross, x.Y);
                it.DrawLine(p, x.X, x.Y - cross, x.X, x.Y + cross);
            }
        }

        private void btnClear_Click(object sender, EventArgs e) {
            clearShots();
        }

        private void clearShots() {
            shots.Clear();
            formResize();
            shotsList.Items.Clear();
            shotsList.Refresh();
            txtLastShot.Text = "";
            imgArrow.Image = new Bitmap(imgArrow.Width, imgArrow.Height);
            imgArrow.Refresh();
            Application.DoEvents();
            txtTotal.Text = "";
            txtTime.Text = "";
            txtWindage.Text = "";
            txtElevation.Text = "";
            txtMaxSpread.Text = "";
            txtMeanRadius.Text = "";
            imgElevation.CreateGraphics().Clear(this.BackColor);
            imgWindage.CreateGraphics().Clear(this.BackColor);
            score = 0;
            decimalScore = 0;
            innerX = 0;
            xbar = 0;
            ybar = 0;
            rbar = 0;
        }

        private double linearInterpolation(float x1, float y1, float x2, float y2, float x) {
            double y = ((x2 - x) * y1 + (x - x1) * y2) / (x2 - x1);
            return y;
        }

        private void timer_Tick(object sender, EventArgs e) {
            DateTime now = DateTime.Now;
            TimeSpan ts = now - connectTime;
            txtTime.Text = ts.ToString(@"hh\:mm\:ss");
        }

        private void calculateMeanRadius( out decimal rbar, out decimal xbar, out decimal ybar) {

            decimal xsum = 0;
            decimal ysum = 0;

            foreach(Shot shot in shots) {
                xsum += shot.x;
                ysum += shot.y;
            }

            xbar = xsum / (decimal)shots.Count;
            ybar = ysum / (decimal)shots.Count;

            decimal[] r = new decimal[shots.Count];
            for(int i=0; i< shots.Count; i++) {
                r[i] = (decimal)Math.Sqrt((double)(((shots[i].x - xbar) * (shots[i].x - xbar)) + ((shots[i].y - ybar) * (shots[i].y - ybar))));
            }

            decimal rsum = 0;
            foreach(decimal ri in r) {
                rsum += ri;
            }

            rbar = rsum / shots.Count;

        }

        private decimal calculateMaxSpread() {
            List<double> spreads = new List<double>();
            for(int i = 0; i < shots.Count; i++) {
                for(int j = 0; j < shots.Count; j++) {
                    spreads.Add(Math.Sqrt( Math.Pow ((double)shots[i].x - (double)shots[j].x, 2) - Math.Pow((double)shots[i].y - (double)shots[j].y, 2)));
                }
            }

            return (decimal)spreads.Max();
        }
    }


}
