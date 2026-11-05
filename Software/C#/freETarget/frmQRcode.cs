using Net.Codecrete.QrCodeGenerator;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.Remoting.Messaging;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;

namespace freETarget {
    public partial class frmQRcode : Form {
        public frmQRcode() {
            InitializeComponent();
        }



        public void initPage(Session session) {
            //translate session shots to shotmetrix shots
            List<ShotmetrixShot> shots = new List<ShotmetrixShot>();
            foreach (Shot s in session.Shots) {
                ShotmetrixShot sms = new ShotmetrixShot() {
                    Value = (double)s.decimalScore,
                    X = (double)s.getX(),
                    Y = (double)s.getY(),
                    Time = s.timestamp,
                    Sighter = false
                };
                shots.Add(sms);
            }


            //determine the discipline for the session
            int disc = translateSessionDiscipline(session);
            if (disc == -1) {
                string error = "Unsupported discipline for QR code generation.";
                pbQRcode.Image = TextToBitmap(error);
                pbQRcode.SizeMode = PictureBoxSizeMode.Zoom;
                return;
            }

            //generate QR code
            try {
                string url = ShotmetrixQrCode.QrCodeUrl(
                    publicKey: Secrets.publicKeyBase64,
                    discipline: disc,
                    decimalScoring: true,
                    sessionStart: session.startTime,
                    shots: shots,
                    app: Secrets.freeTargetId,
                    appSecret: Convert.FromBase64String(Secrets.freeTargetKey)
                );
                txtLink.Text = url;
                pbQRcode.Image = GenerateQrBitmap(url);
                pbQRcode.SizeMode = PictureBoxSizeMode.Zoom;
            } catch (Exception ex) {
                pbQRcode.Image = TextToBitmap("Error generating QR code: " + ex.Message);
                pbQRcode.SizeMode = PictureBoxSizeMode.Zoom;
            }
        }

        public static Bitmap TextToBitmap( string text) {
            int width = 400;
            int height = 100;
            float fontSize = 16;

            var bitmap = new Bitmap(width, height);

            using (Graphics g = Graphics.FromImage(bitmap))
            using (Font font = new Font("Arial", fontSize))
            using (Brush brush = new SolidBrush(Color.Black))
            using (StringFormat format = new StringFormat()) {
                g.Clear(Color.White);
                g.SmoothingMode = SmoothingMode.AntiAlias;
                g.TextRenderingHint =
                    System.Drawing.Text.TextRenderingHint.AntiAliasGridFit;

                format.Alignment = StringAlignment.Center;
                format.LineAlignment = StringAlignment.Center;
                format.Trimming = StringTrimming.EllipsisWord;

                g.DrawString(
                    text ?? string.Empty,
                    font,
                    brush,
                    new RectangleF(5, 5, width - 10, height - 10),
                    format);
            }

            return bitmap;
        }

        private int translateSessionDiscipline(Session session) {
           int ret = -1;
           switch (session.targetType) {
                case "freETarget.targets.AirPistol":
                    ret = 3;
                    break;
                case "freETarget.targets.AirRifle":
                    ret = 1;
                    break;
                case "freETarget.targets.Rifle50m":
                    ret = 6;
                    break;
                case "freETarget.targets.Pistol50m":
                    ret = 11;
                    break;
                case "freETarget.targets.Pistol25m":
                    ret = 10;
                    break;
                case "freETarget.targets.Pistol25mRF":
                    ret = 9;
                    break;
                default:
                    ret = -1;
                    break;

            }
            return ret;
        }

        public static Bitmap GenerateQrBitmap(string url) {
            var qr = QrCode.EncodeTextAdvanced(
                url,
                QrCode.Ecc.Low,
                boostEcl: false,
                minVersion: 24,
                maxVersion: 24
            );

            // 4-module quiet zone, 5 pixels per module.
            byte[] pngBytes = qr.ToPngBitmap(
                border: 4,
                scale: 5
            );

            using (var ms = new MemoryStream(pngBytes))
            using (var temp = new Bitmap(ms)) {
                // Return an independent bitmap; the stream can now be disposed.
                return new Bitmap(temp);
            }
        }

        private void btnShowLink_Click(object sender, EventArgs e) {
            if (this.Height == 569) { 
                this.Height = 677;
                btnShowLink.Image=pbUp.Image;
            } else {
                btnShowLink.Image=pbDown.Image;
                this.Height = 569;
            }
        }

        private void button1_Click(object sender, EventArgs e) {
            if (txtLink.Text != null && txtLink.Text.Length > 0) {
                Process.Start(txtLink.Text);
            }
        }
    }
}
