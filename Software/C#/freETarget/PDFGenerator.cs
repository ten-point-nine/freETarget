using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using PdfSharp;
using PdfSharp.Drawing;
using PdfSharp.Drawing.Layout;
using PdfSharp.Pdf;
using PdfSharp.Pdf.IO;

namespace freETarget {
    class PDFGenerator {

        protected PDFGenerator() {

        }

        public static void generateAndSavePDF(Session session, string savePath, Stream imageStream) {

            //prepare session (load shots into series)
            session.AllSeries.Clear();
            foreach (Shot s in session.Shots) {
                session.addLoadedShot(s);
            }

                PdfDocument document = new PdfDocument();
            document.Info.Title = "Session " +session.id ;

            PdfPage firstPage = document.AddPage();
            int pageNo = 1;
            //dimensions:  595 x 792 - good both for A4 and for Letter

            int blackRings=session.getTarget().getBlackRings();
            bool solidInner= session.getTarget().isSolidInner();
            decimal[] rings= session.getTarget().getRings();

            XPen penBlack = new XPen(XColor.FromKnownColor(XKnownColor.Black), 0.75);
            XBrush brushBlack = new XSolidBrush(XColor.FromKnownColor(XKnownColor.Black));
            Font f = new Font("Arial", 12, GraphicsUnit.World);

            //session target

            paintTarget(250, 20, 20, blackRings, rings, session.getTarget().getPDFZoomFactor(null), solidInner, session.Shots, firstPage, session.getTarget().getSize(), session.getTarget().getProjectileCaliber());

            //session data

            printSessionData(firstPage, 280, 20, 295, 250, session);


            //draw series header
            XGraphics gfx = XGraphics.FromPdfPage(firstPage);
            gfx.DrawLine(penBlack, 20, 290, 575, 290);
            
            gfx.DrawString("Series (" + session.AllSeries.Count + ")", f, brushBlack, 25, 285);
            


            //draw footer
            XBrush brushLight = new XSolidBrush(XColor.FromKnownColor(XKnownColor.Linen));
            gfx.DrawRectangle(brushLight, 20, 770, 555, 10);
            Font f2 = new Font("Arial", 6, GraphicsUnit.World);
            gfx.DrawString("Page " + pageNo, f2, brushBlack, new XPoint(550, 777));
            XImage img = XImage.FromStream(imageStream);
            gfx.DrawImage(img, new XPoint(22,771));

            gfx.Dispose();

            //series 1
            if (session.AllSeries.Count > 0) {
                int y = 295;
                decimal zoom2 = session.getTarget().getPDFZoomFactor(session.AllSeries[0]);
                paintTarget(150, 20, y, blackRings, rings, zoom2, solidInner, session.AllSeries[0], firstPage, session.getTarget().getSize(), session.getTarget().getProjectileCaliber());
                printSeriesData(firstPage, 180, y, 395, 150, session.AllSeries[0], 1,session);
            }

            //series 2
            if (session.AllSeries.Count > 1) {
                int y = 455;
                decimal zoom2 = session.getTarget().getPDFZoomFactor(session.AllSeries[1]);
                paintTarget(150, 20, y, blackRings, rings, zoom2, solidInner, session.AllSeries[1], firstPage, session.getTarget().getSize(), session.getTarget().getProjectileCaliber());
                printSeriesData(firstPage, 180, y, 395, 150, session.AllSeries[1], 2, session);
            }

            //series 3
            if (session.AllSeries.Count > 2) {
                int y = 615;
                decimal zoom2 = session.getTarget().getPDFZoomFactor(session.AllSeries[2]);
                paintTarget(150, 20, y, blackRings, rings, zoom2, solidInner, session.AllSeries[2], firstPage, session.getTarget().getSize(), session.getTarget().getProjectileCaliber());
                printSeriesData(firstPage, 180, y, 395, 150, session.AllSeries[2], 3, session);
            }


            if (session.AllSeries.Count > 3) {
                //more pages needed
                PdfPage page = null;
                int y = 20;
                for (int p = 3; p < session.AllSeries.Count; p++) { 

                    if ((p + 1) % 4 == 0) {
                        page = document.AddPage();
                        pageNo++;
                        y = 20;
                    }

                    decimal zoom2 = session.getTarget().getPDFZoomFactor(session.AllSeries[p]);//getZoom(session.AllSeries[p], session.targetType);
                    paintTarget(150, 20, y, blackRings, rings, zoom2, solidInner, session.AllSeries[p], page, session.getTarget().getSize(), session.getTarget().getProjectileCaliber());
                    printSeriesData(page, 180, y, 395, 150, session.AllSeries[p], p+1, session);
                    y += 170;

                    //draw footer
                    gfx = XGraphics.FromPdfPage(page);
                    gfx.DrawRectangle(brushLight, 20, 770, 555, 10);
                    gfx.DrawString("Page " + pageNo, f2, brushBlack, new XPoint(550, 777));
                    gfx.DrawImage(img, new XPoint(22, 771));
                    gfx.Dispose();
                }
            }

            PdfPage notes = document.AddPage();
            pageNo++;
            gfx = XGraphics.FromPdfPage(notes);
            RichTextBox r = new RichTextBox();
            r.Rtf = session.diaryEntry;
            gfx.DrawLine(penBlack, 20, 30, 575, 30);
            gfx.DrawString("Diary", f, brushBlack, 30, 25);

            XTextFormatter tf = new XTextFormatter(gfx);
            tf.Alignment = XParagraphAlignment.Left;
            f = new Font("Arial", 12, GraphicsUnit.World);
            tf.DrawString(r.Text, f, brushBlack, new XRect(25, 40, 575 , 782));

            gfx.DrawRectangle(brushLight, 20, 770, 555, 10);
            gfx.DrawString("Page " + pageNo, f2, brushBlack, new XPoint(550, 777));
            gfx.DrawImage(img, new XPoint(22, 771));

            gfx.Dispose();

            string filename = savePath + Path.DirectorySeparatorChar + "Session " + session.id + ".pdf";

            try {
                document.Save(filename);
                Process.Start(filename);
            } catch(Exception ex) {
                MessageBox.Show("Error saving pdf!  " + Environment.NewLine + ex.Message,"PDF Export Error",MessageBoxButtons.OK,MessageBoxIcon.Error);
                Console.WriteLine("Error saving pdf " + ex.Message);
            }
           
        }


        private static void paintTarget(int dimension, int xLoc, int yLoc, int blackRingCutoff, decimal[] rings, decimal zoomFactor, bool solidInner, List<Shot> shotsList, PdfPage page, decimal targetSize, decimal projectileCaliber) {
            XPen penBlack = new XPen(XColor.FromKnownColor(XKnownColor.Black), 0.5);
            XPen penWhite = new XPen(XColor.FromKnownColor(XKnownColor.Linen), 0.5);
            XBrush brushBlack = new XSolidBrush(XColor.FromKnownColor(XKnownColor.Black));
            XBrush brushWhite = new XSolidBrush(XColor.FromKnownColor(XKnownColor.Linen));

            XGraphics gfx = XGraphics.FromPdfPage(page);
            gfx.IntersectClip(new XRect(xLoc, yLoc, dimension, dimension));

            gfx.SmoothingMode = XSmoothingMode.AntiAlias;

            gfx.DrawRectangle(brushWhite, xLoc, yLoc, dimension, dimension);

            int r = 1;
            for (int i = 0; i < rings.Length; i++) {

                XPen p;
                XBrush b;
                XBrush bText;
                if (r < blackRingCutoff) {
                    p = penBlack;
                    b = brushWhite;
                    bText = brushBlack;
                } else {
                    p = penWhite;
                    b = brushBlack;
                    bText = brushWhite;
                }

                float circle = getDimension(dimension, rings[i], zoomFactor, targetSize);
                float centerX = (dimension / 2) + xLoc ;
                float centerY = (dimension / 2) + yLoc ;
                float x = centerX - (circle / 2);
                float y = centerY - (circle / 2);
                float xInvers = centerX + (circle / 2);
                float yInvers = centerY + (circle / 2);


                if (solidInner && i == rings.Length - 1) //rifle target - last ring (10) is a solid dot
                {
                    gfx.DrawEllipse(brushWhite, x, y, circle, circle);
                } else {
                    gfx.DrawEllipse(b, x, y, circle, circle);
                    gfx.DrawEllipse(p, x, y, circle, circle);
                }

                if (r < 9) //for ring 9 and after no text is displayed
                {
                    float nextCircle = getDimension(dimension, rings[i + 1], zoomFactor, targetSize);
                    float diff = circle - nextCircle;
                    float fontSize = diff / 6f;
                    Font f = new Font("Arial", fontSize, GraphicsUnit.World);

                    XStringFormat format = new XStringFormat();
                    format.LineAlignment = (XLineAlignment)XStringAlignment.Center;
                    format.Alignment = XStringAlignment.Center;

                    gfx.DrawString(r.ToString(), f, bText, centerX, y + (diff / 4), format);
                    gfx.DrawString(r.ToString(), f, bText, centerX, yInvers - (diff / 4), format);
                    gfx.DrawString(r.ToString(), f, bText, x + (diff / 4), centerY, format);
                    gfx.DrawString(r.ToString(), f, bText, xInvers - (diff / 4), centerY, format);
                }
                r++;
            }

            gfx.DrawRectangle(penBlack, xLoc, yLoc, dimension, dimension);

            foreach (Shot shot in shotsList) {
                drawShot(shot, gfx, xLoc, yLoc, dimension, zoomFactor,projectileCaliber,targetSize);
            }

            decimal localRbar;
            decimal localXbar;
            decimal localYbar;

            computeMeans(out localRbar, out localXbar, out localYbar, shotsList);

            drawMeanGroup(gfx, dimension, zoomFactor,shotsList, localRbar, localXbar, localYbar, xLoc,yLoc,targetSize);

            gfx.Dispose();

        }

        private static float getDimension(decimal currentTargetSize, decimal milimiters, decimal zoomFactor, decimal targetSize) {
            return (float)((currentTargetSize * milimiters) / (targetSize * zoomFactor));
        }

        private static void drawShot(Shot shot, XGraphics it, int xLoc, int yLoc, int targetSize, decimal zoomFactor, decimal projectileCaliber, decimal realTargetSize) {
            //transform shot coordinates to imagebox coordinates

            XPoint x = transform((float)getShotX(shot), (float)getShotY(shot), targetSize, zoomFactor, xLoc, yLoc, realTargetSize);

            //draw shot on target
            XBrush b = new XSolidBrush(XColor.FromArgb(200, 135, 206, 250)); //semitransparent shots
            XPen p = new XPen(XColor.FromKnownColor(XKnownColor.Blue), 0.5);
            XBrush bText = new XSolidBrush(XColor.FromKnownColor(XKnownColor.Blue));


            it.SmoothingMode = XSmoothingMode.AntiAlias;

            float peletSize = getDimension(targetSize, projectileCaliber, zoomFactor, realTargetSize);

            x.X -= peletSize / 2;
            x.Y -= peletSize / 2;

            it.DrawEllipse(b, new XRect(x, new XSize(peletSize, peletSize)));
            it.DrawEllipse(p, new XRect(x, new XSize(peletSize, peletSize)));

            XStringFormat format = new XStringFormat();
            format.LineAlignment = (XLineAlignment)XStringAlignment.Center;
            format.Alignment = XStringAlignment.Center;

            Font f = new Font("Arial", peletSize / 3, GraphicsUnit.World);

            it.DrawString((shot.index + 1).ToString(), f, bText, new XRect(x, new XSize(peletSize, peletSize)), format);
        }

        private static decimal getShotX(Shot shot) {
            return shot.getX();
        }

        private static decimal getShotY(Shot shot) {
            return shot.getY();
        }

        private static XPoint transform(float xp, float yp, float size, decimal zoomFactor, int xLoc, int yLoc, decimal targetSize) {
            //matrix magic from: https://docs.microsoft.com/en-us/previous-versions/windows/internet-explorer/ie-developer/samples/jj635757(v=vs.85)

            System.Numerics.Matrix4x4 M = new System.Numerics.Matrix4x4(xLoc, yLoc, 1, 0,
                                                                        -yLoc, xLoc, 0, 1,
                                                                        xLoc+size, yLoc+size, 1, 0,
                                                                       -(yLoc+size), xLoc+size, 0, 1);

            System.Numerics.Matrix4x4 Minverted;
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

            XPoint ret = new XPoint(x, y);
            return ret;
        }

        private static void drawMeanGroup(XGraphics it, decimal currentTargetSize, decimal zoomFactor, List<Shot> shotsList, decimal rBar, decimal xBar, decimal yBar, int xLoc, int yLoc, decimal realTargetSize) {
            if (shotsList.Count >= 2) {
                float circle = getDimension(currentTargetSize, rBar * 2, zoomFactor, realTargetSize);

                XPoint x = transform((float)xBar, (float)yBar, (float)currentTargetSize, zoomFactor,xLoc, yLoc, realTargetSize);
                XPen p = new XPen(XColor.FromKnownColor(XKnownColor.Red), 0.5);

                it.DrawEllipse(p, x.X - (circle / 2), x.Y - (circle / 2), circle, circle);

                float cross = 4; // center of group cross is always the same size - 5 pixels

                it.DrawLine(p, x.X - cross, x.Y, x.X + cross, x.Y);
                it.DrawLine(p, x.X, x.Y - cross, x.X, x.Y + cross);
            }
        }

        private static void computeMeans(out decimal rbar, out decimal xbar, out decimal ybar, List<Shot> shots) {

            decimal xsum = 0;
            decimal ysum = 0;

            foreach (Shot shot in shots) {
                xsum += getShotX(shot);
                ysum += getShotY(shot);
            }

            xbar = xsum / (decimal)shots.Count;
            ybar = ysum / (decimal)shots.Count;

            decimal[] r = new decimal[shots.Count];
            for (int i = 0; i < shots.Count; i++) {
                r[i] = (decimal)Math.Sqrt((double)(((getShotX(shots[i]) - xbar) * (getShotX(shots[i]) - xbar)) + ((getShotY(shots[i]) - ybar) * (getShotY(shots[i]) - ybar))));
            }

            decimal rsum = 0;
            foreach (decimal ri in r) {
                rsum += ri;
            }

            rbar = rsum / shots.Count;

        }

        private static void printSessionData(PdfPage page, int x, int y, int width, int height, Session session) {
            XGraphics gfx = XGraphics.FromPdfPage(page);
            gfx.IntersectClip(new XRect(x, y, width, height));
            XPen penBlack = new XPen(XColor.FromKnownColor(XKnownColor.Black), 0.5);
            XBrush brushBlack = new XSolidBrush(XColor.FromKnownColor(XKnownColor.Black));


            XTextFormatter tf = new XTextFormatter(gfx);
            tf.Alignment = XParagraphAlignment.Left;
            Font fLarge = new Font("Arial", 12, FontStyle.Bold, GraphicsUnit.World);
            Font fMedium = new Font("Arial", 10, GraphicsUnit.World);
            Font fMediumBold = new Font("Arial", 10, FontStyle.Bold, GraphicsUnit.World);
            Font fSmall = new Font("Arial", 8, GraphicsUnit.World);

            String s = "Match summary";

            XPoint point = new XPoint(x, y);
            point.Offset(5, 5);
            s = "Session ID:";
            XSize rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 5, 0);
            s = session.id.ToString();
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));



            //new line
            point = new XPoint(x, y);
            point.Offset(5, 5);
            point.Offset(0,rect.Height+5);
            s = "Event:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 5, 0);
            s = session.eventType.Name;
            if (session.numberOfShots > 0) {
                s +=  " " + session.numberOfShots;
            } 
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));


            //new line
            point = new XPoint(x + 5, point.Y + rect.Height + 5);
            s = "Shooter:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 5, 0);
            s = session.user;
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));


            //new line
            point = new XPoint(x+5, point.Y + rect.Height + 5);
            s = "Start Time: ";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width, 0);
            s = session.startTime.ToString("yyyy-MM-dd HH:mm:ss");
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));

            //new line
            point = new XPoint(x + 5, point.Y + rect.Height + 5);
            s = "End Time: ";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width, 0);
            s = session.endTime.ToString("yyyy-MM-dd HH:mm:ss");
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 15, 0);
            s = "Duration:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 1, 0);
            s = (session.endTime - session.startTime).TotalMinutes.ToString("F2", CultureInfo.InvariantCulture) + " mins";
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));

            //new line
            point = new XPoint(x + 5, point.Y + rect.Height + 5);
            s = "Avg Shot time:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width+1, 0);
            s = session.averageTimePerShot.TotalSeconds.ToString("F2", CultureInfo.InvariantCulture) + "s";
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 5, 0);
            s = "Shortest:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width+1, 0);
            s = session.shortestShot.TotalSeconds.ToString("F2", CultureInfo.InvariantCulture) + "s";
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 5, 0);
            s = "Longest:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width+1, 0);
            s = session.longestShot.TotalSeconds.ToString("F2", CultureInfo.InvariantCulture) + "s";
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));


            //new line
            point = new XPoint(x + 5, point.Y + rect.Height + 5);
            s = "MPI: ";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 1, 0);
            string W,E;
            if (session.xbar < 0) {
                W = "\u2190";
            } else {
                W = "\u2192";
            }

            if(session.ybar < 0) {
                E = "\u2193";
            } else {
                E = "\u2191";
            }
            s = W +Math.Abs(session.xbar).ToString("F2", CultureInfo.InvariantCulture) + "mm" + "  " + E + Math.Abs(session.ybar).ToString("F2", CultureInfo.InvariantCulture) + "mm";
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 5, 0);
            s = "MR:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 1, 0);
            s = session.rbar.ToString("F2", CultureInfo.InvariantCulture) + "mm";
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 5, 0);
            s = "Group size:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 1, 0);
            s = session.groupSize.ToString("F2", CultureInfo.InvariantCulture) + "mm";
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));

            //new line
            point = new XPoint(x + 5, point.Y + rect.Height + 5);
            s = "Total Score:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 5, 0);
            if (session.decimalScoring) {
                s = session.decimalScore.ToString("F1", CultureInfo.InvariantCulture) + " - " + session.innerX + "x";
            } else {
                s = session.score.ToString() + " - " + session.innerX + "x";
            }
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 10, 0);
            s = "Average score:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 1, 0);
            s = session.averageScore.ToString("F2", CultureInfo.InvariantCulture);
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));

            //new line
            point = new XPoint(x + 5, point.Y + rect.Height + 5);
            s = "Number of shots in session:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 2, 0);
            s = session.actualNumberOfShots.ToString();
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));

            //new line
            point = new XPoint(x + 5, point.Y + rect.Height + 5);
            s = "Series:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 2, 0);
            s = "";
            int X;
            foreach(List<Shot> shots in session.AllSeries) {
                if (session.decimalScoring) {
                    s += sumScores(session, shots, out X).ToString("F1", CultureInfo.InvariantCulture) + " ";
                } else {
                    s += sumScores(session, shots, out X).ToString("F0", CultureInfo.InvariantCulture) + " ";
                }
            }
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));

            //new line
            point = new XPoint(x + 5, point.Y + rect.Height + 5);
            drawBreakdownGraph(gfx, point.X, point.Y, width - 10, height - point.Y + 15, session);

            gfx.DrawRectangle(penBlack, x, y, width, height);
            gfx.Dispose();

        }

        private static void drawBreakdownGraph(XGraphics gfx, double x, double y, double width, double height, Session session) {
            XPen penBlack = new XPen(XColor.FromKnownColor(XKnownColor.Black), 0.5);
            XBrush brushBlack = new XSolidBrush(XColor.FromKnownColor(XKnownColor.Black));
            gfx.DrawRectangle(penBlack, x, y, width, height);
            Font f1 = new Font("Arial", 8, GraphicsUnit.World);
            Font f2 = new Font("Arial", 5, GraphicsUnit.World);

            List<int> breakdown = new List<int> { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
            foreach(Shot s in session.Shots) {
                if (s.score == 10) {
                    if (s.innerTen) {
                        breakdown[11]++;
                    } else {
                        breakdown[s.score]++;
                    }
                } else {
                    breakdown[s.score]++;
                }
            }

            int bigest = -100;

            double offset = x+7;
            for(int i = breakdown.Count-1;i >= 0;i--) {
                int c = breakdown[i];
                if (c > bigest) {
                    bigest = c;
                }
                XPoint point = new XPoint(offset, y + height - 10);
                string cs = c.ToString();
                if (c == 0) {
                    cs = "-";
                }

                gfx.DrawString(cs, f1, brushBlack, point);
               

                XPoint point2 = new XPoint(offset+1, y + height-3);
                cs = i.ToString();
                if (i == 11) {
                    cs = "X";
                }
                gfx.DrawString(cs, f2, brushBlack, point2);

                offset += width / 12;
            }

            double tallest = height - 30;
            offset = x + 7;
            for (int i = breakdown.Count - 1; i >= 0; i--) {
                int c = breakdown[i];
                double h = (c * tallest) / bigest;
                gfx.DrawRectangle(brushBlack, new XRect(new XPoint(offset-2, y + height - 25), new XPoint(offset+ 8, y + height - 25 - h)));
                offset += width / 12;
            }
        }

        private static void printSeriesData(PdfPage page, int x, int y, int width, int height, List<Shot> shotList, int index, Session session) {
            XGraphics gfx = XGraphics.FromPdfPage(page);
            gfx.IntersectClip(new XRect(x, y, width, height));
            XPen penBlack = new XPen(XColor.FromKnownColor(XKnownColor.Black), 0.5);
            XBrush brushBlack = new XSolidBrush(XColor.FromKnownColor(XKnownColor.Black));

            XTextFormatter tf = new XTextFormatter(gfx);
            tf.Alignment = XParagraphAlignment.Left;
            Font fMedium = new Font("Arial", 10, GraphicsUnit.World);
            Font fMediumBold = new Font("Arial", 10, FontStyle.Bold, GraphicsUnit.World);

            XPoint point = new XPoint(x, y);
            point.Offset(5, 5);
            String s = "Series " + index;
            XSize rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            //new line
            point = new XPoint(x + 5, point.Y + rect.Height + 5);
            s = "Score:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            int X;
            point.Offset(rect.Width + 2, 0);
            decimal score = sumScores(session, shotList, out X);
            if (session.decimalScoring) {
                s = score.ToString("F1", CultureInfo.InvariantCulture) + " - " + X + "x";
            } else {
                s = score.ToString("F0", CultureInfo.InvariantCulture) + " - " + X + "x";
            }
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));

            //new line
            point = new XPoint(x + 5, point.Y + rect.Height + 5);
            s = "Shots:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 2, 0);
            s = "";
            foreach (Shot shot in shotList) {
                string t = shot.innerTen ? "*" : "";
                if (session.decimalScoring) {

                    s += shot.decimalScore.ToString("F1", CultureInfo.InvariantCulture) + t + "   ";
                } else {
                    s += shot.score + t + "   ";
                }
            }
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));


            //new line
            point = new XPoint(x + 5, point.Y + rect.Height + 5);
            s = "Average score:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 1, 0);
            decimal avg = score / shotList.Count;
            s = avg.ToString("F2", CultureInfo.InvariantCulture);
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));



            decimal localRbar;
            decimal localXbar;
            decimal localYbar;

            computeMeans(out localRbar, out localXbar, out localYbar, shotList);

            //new line
            point = new XPoint(x + 5, point.Y + rect.Height + 5);
            s = "MPI: ";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 1, 0);
            string W, E;
            if (localXbar < 0) {
                W = "\u2190";
            } else {
                W = "\u2192";
            }

            if (localYbar < 0) {
                E = "\u2193";
            } else {
                E = "\u2191";
            }
            s = W + Math.Abs(localXbar).ToString("F2", CultureInfo.InvariantCulture) + "mm" + "  " + E + Math.Abs(localYbar).ToString("F2", CultureInfo.InvariantCulture) + "mm";
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 5, 0);
            s = "MR:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 1, 0);
            s = localRbar.ToString("F2", CultureInfo.InvariantCulture) + "mm";
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 5, 0);
            s = "Group size:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 1, 0);
            s = calculateMaxSpread(shotList).ToString("F2", CultureInfo.InvariantCulture) + "mm";
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));

            //new line
            point = new XPoint(x + 5, point.Y + rect.Height + 5);
            s = "First shot:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 1, 0);
            s = shotList[0].timestamp.ToString("HH:mm:ss");
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 5, 0);
            s = "Last shot:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 1, 0);
            s = shotList[shotList.Count-1].timestamp.ToString("HH:mm:ss");
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));

            TimeSpan shortest;
            TimeSpan longest;
            TimeSpan avgTime;
            calculateTimeStats(out shortest, out longest, out avgTime, shotList);
            //new line
            point = new XPoint(x + 5, point.Y + rect.Height + 5);
            s = "Avg Shot time:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 1, 0);
            s = avgTime.TotalSeconds.ToString("F2", CultureInfo.InvariantCulture) + "s";
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 5, 0);
            s = "Shortest:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 1, 0);
            s = shortest.TotalSeconds.ToString("F2", CultureInfo.InvariantCulture) + "s";
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 5, 0);
            s = "Longest:";
            rect = gfx.MeasureString(s, fMediumBold);
            tf.DrawString(s, fMediumBold, brushBlack, new XRect(point, rect));

            point.Offset(rect.Width + 1, 0);
            s = longest.TotalSeconds.ToString("F2", CultureInfo.InvariantCulture) + "s";
            rect = gfx.MeasureString(s, fMedium);
            tf.DrawString(s, fMedium, brushBlack, new XRect(point, rect));


            gfx.DrawRectangle(penBlack, x, y, width, height);
            gfx.Dispose();

        }

        private static void calculateTimeStats(out TimeSpan shortest, out TimeSpan longest, out TimeSpan avg, List<Shot> shots) {
            shortest = TimeSpan.MaxValue;
            longest = TimeSpan.MinValue;

            for (int i = 0; i < shots.Count; i++) {
                Shot s = shots[i];
                if (s.shotDuration > longest) {
                    longest = s.shotDuration;
                }

                if (s.shotDuration < shortest) {
                    shortest = s.shotDuration;
                }
            }

            avg = TimeSpan.FromTicks((shots[shots.Count - 1].timestamp - shots[0].timestamp).Ticks / shots.Count);

        }

        //computes group size (maxium distance between 2 shots), but measuring from the center of the shot, not outside
        private static decimal calculateMaxSpread(List<Shot> shots) {
            List<double> spreads = new List<double>();
            for (int i = 0; i < shots.Count; i++) {
                for (int j = 0; j < shots.Count; j++) {
                    spreads.Add(Math.Sqrt(Math.Pow((double)getShotX(shots[i]) - (double)getShotX(shots[j]), 2) - Math.Pow((double)getShotY(shots[i]) - (double)getShotY(shots[j]), 2)));
                }
            }

            return (decimal)spreads.Max();
        }

        private static decimal sumScores(Session session, List<Shot> shotList, out int innerXs) {
            int Xs = 0;
            if (session.decimalScoring) {
                decimal scoreSum = 0;
                foreach (Shot s in shotList) {
                    scoreSum += s.decimalScore;
                    if (s.innerTen) {
                        Xs++;
                    }
                }
                innerXs = Xs;
                return scoreSum;
            } else {
                int scoreSum = 0;
                foreach (Shot s in shotList) {
                    scoreSum += s.score;
                    if (s.innerTen) {
                        Xs++;
                    }
                }
                innerXs = Xs;
                return scoreSum;
            }
        }

    }
}
