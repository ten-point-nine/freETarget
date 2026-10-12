using freETarget.Properties;
using freETarget.targets;
using SkiaSharp;
using System;
using System.Collections.Generic;
using System.Drawing;

namespace freETarget {

    /**
     * draws a target and its shots with SkiaSharp, which works on every platform.
     * the geometry comes from the aTarget classes. the look follows the original Windows application of the freETarget project
     */
    public static class TargetRenderer {

        private const float pointsToPixels = 96f / 72f; //the sizes that the targets give for the texts are in points

        private static readonly SKTypeface typeface = SKTypeface.FromFamilyName("Arial") ?? SKTypeface.Default;

        private static SKColor toSK(Color c) {
            return new SKColor(c.R, c.G, c.B, c.A);
        }

        private static SKPaint fill(Color c) {
            return new SKPaint { Color = toSK(c), Style = SKPaintStyle.Fill, IsAntialias = true };
        }

        private static SKPaint stroke(Color c, float width) {
            return new SKPaint { Color = toSK(c), Style = SKPaintStyle.Stroke, StrokeWidth = width, IsAntialias = true };
        }

        //draws a text with its center at 0,0 of the current coordinates
        private static void drawCenteredText(SKCanvas canvas, string text, float fontSizeInPoints, SKPaint paint) {
            if (fontSizeInPoints <= 0) {
                return;
            }
            using (SKFont font = new SKFont(typeface, fontSizeInPoints * pointsToPixels)) {
                font.Subpixel = true;
                SKFontMetrics metrics = font.Metrics;
                float baseline = -(metrics.Ascent + metrics.Descent) / 2;
                canvas.DrawText(text, 0, baseline, SKTextAlign.Center, font, paint);
            }
        }

        /** PNG image of the target, for tests and exports */
        public static byte[] renderPng(aTarget target, int dimension, int zoomValue, bool notConnected, Session currentSession, List<Shot> shotList) {
            using (SKBitmap bitmap = new SKBitmap(new SKImageInfo(dimension, dimension, SKColorType.Bgra8888, SKAlphaType.Premul)))
            using (SKCanvas canvas = new SKCanvas(bitmap)) {
                paint(canvas, target, dimension, zoomValue, notConnected, currentSession, shotList);
                canvas.Flush();
                using (SKData data = bitmap.Encode(SKEncodedImageFormat.Png, 100)) {
                    return data.ToArray();
                }
            }
        }

        /**
         * paints the target in the square from 0,0 to dimension,dimension of the canvas.
         * a target that is not connected is painted in shades of gray
         */
        public static void paint(SKCanvas canvas, aTarget target, int dimension, int zoomValue, bool notConnected, Session currentSession, List<Shot> shotList) {
            if (dimension <= 0) { //window is minimized. nothing to paint
                return;
            }

            if (notConnected) {
                //the usual weights of red, green and blue for the brightness of a color
                float[] grayScale = new float[] {
                    .3f, .59f, .11f, 0, 0,
                    .3f, .59f, .11f, 0, 0,
                    .3f, .59f, .11f, 0, 0,
                    0, 0, 0, 1, 0
                };
                using (SKPaint layer = new SKPaint { ColorFilter = SKColorFilter.CreateColorMatrix(grayScale) }) {
                    canvas.SaveLayer(new SKRect(0, 0, dimension, dimension), layer);
                    paintInColor(canvas, target, dimension, zoomValue, currentSession, shotList);
                    canvas.Restore();
                }
            } else {
                paintInColor(canvas, target, dimension, zoomValue, currentSession, shotList);
            }
        }

        private static void paintInColor(SKCanvas canvas, aTarget target, int dimension, int zoomValue, Session currentSession, List<Shot> shotList) {
            bool solidInner = target.isSolidInner();
            decimal zoomFactor = target.getZoomFactor(zoomValue);
            int blackRingCutoff = target.getBlackRings();
            decimal[] rings = target.getRings();

            float center = (float)(dimension / 2);

            using (SKPaint penBlack = stroke(Color.Black, 1))
            using (SKPaint penWhite = stroke(Settings.Default.targetColor, 1))
            using (SKPaint brushBlack = fill(Color.Black))
            using (SKPaint brushWhite = fill(Settings.Default.targetColor)) {

                canvas.Save();
                canvas.ClipRect(new SKRect(0, 0, dimension, dimension));

                canvas.DrawRect(0, 0, dimension, dimension, brushWhite);

                float circle = target.getDimension(dimension, target.getBlackDiameter(), zoomFactor);
                canvas.DrawCircle(center, center, circle / 2, brushBlack); //draw the black circle

                int r = target.getFirstRing();
                if (Settings.Default.showScoring) {
                    for (int i = 0; i < rings.Length; i++) {

                        SKPaint p;
                        SKPaint bText;
                        if (r < blackRingCutoff) {
                            p = penBlack;
                            bText = brushBlack;
                        } else {
                            p = penWhite;
                            bText = brushWhite;
                        }

                        circle = target.getDimension(dimension, rings[i], zoomFactor);

                        float x = center - (circle / 2);
                        float y = center + (circle / 2);

                        if (solidInner && i == rings.Length - 1) { //rifle target - last ring (10) is a solid dot
                            canvas.DrawCircle(center, center, circle / 2, brushWhite);
                        } else {
                            canvas.DrawCircle(center, center, circle / 2, p);
                        }

                        if (r <= target.getRingTextCutoff()) {
                            String txt = r.ToString();
                            int half = dimension / 2;

                            if (r <= 10) {
                                if (i + 1 == rings.Length) {
                                    //center 10 ring with no inner X
                                    canvas.Save();
                                    canvas.Translate(half, half); //set coordinates in the middle of the target
                                    canvas.RotateDegrees(target.getTextRotation());
                                    drawCenteredText(canvas, txt, target.getFontSize(0), bText);
                                    canvas.Restore();
                                } else {
                                    float nextCircle = target.getDimension(dimension, rings[i + 1], zoomFactor);
                                    float diff = circle - nextCircle;
                                    float fontSize = target.getFontSize(diff);

                                    if (target.drawNorthText()) {
                                        canvas.Save();
                                        canvas.Translate(half, half);
                                        canvas.RotateDegrees(target.getTextRotation());
                                        canvas.Translate(0, -half + x + (diff / 4) - target.getTextOffset(diff, r)); //move coordinates at the exact center point of the text
                                        drawCenteredText(canvas, txt, fontSize, bText);
                                        canvas.Restore();
                                    }

                                    if (target.drawSouthText()) {
                                        canvas.Save();
                                        canvas.Translate(half, half);
                                        canvas.RotateDegrees(target.getTextRotation());
                                        canvas.Translate(0, half - x - (diff / 4) + target.getTextOffset(diff, r));
                                        if (target.getTextRotation() > 0) {
                                            canvas.RotateDegrees(180);
                                        }
                                        drawCenteredText(canvas, txt, fontSize, bText);
                                        canvas.Restore();
                                    }

                                    if (target.drawEastText()) {
                                        canvas.Save();
                                        canvas.Translate(half, half);
                                        canvas.RotateDegrees(target.getTextRotation());
                                        canvas.Translate(-half + y - (diff / 4) + target.getTextOffset(diff, r), 0);
                                        if (target.getTextRotation() > 0) {
                                            canvas.RotateDegrees(90);
                                        }
                                        drawCenteredText(canvas, txt, fontSize, bText);
                                        canvas.Restore();
                                    }

                                    if (target.drawWestText()) {
                                        canvas.Save();
                                        canvas.Translate(half, half);
                                        canvas.RotateDegrees(target.getTextRotation());
                                        canvas.Translate(half - y + (diff / 4) - target.getTextOffset(diff, r), 0);
                                        if (target.getTextRotation() > 0) {
                                            canvas.RotateDegrees(270);
                                        }
                                        drawCenteredText(canvas, txt, fontSize, bText);
                                        canvas.Restore();
                                    }
                                }
                            } else {
                                //r = 11
                                canvas.Save();
                                canvas.Translate(half, half);
                                canvas.RotateDegrees(target.getTextRotation());
                                drawCenteredText(canvas, "X", target.getFontSize(0), bText);
                                canvas.Restore();
                            }
                        }
                        r++;
                    }
                }

                if (currentSession.sessionType == Event.EventType.Practice) {
                    //draw triangle in corner
                    float sixth = dimension / 6f;
                    using (SKPath triangle = new SKPath())
                    using (SKPaint brushBlue = fill(Color.DarkBlue)) {
                        triangle.MoveTo(5 * sixth, 0);
                        triangle.LineTo(dimension, sixth);
                        triangle.LineTo(dimension, 0);
                        triangle.Close();
                        canvas.DrawPath(triangle, brushBlue);
                    }
                }

                if (target.rapidFireBarDimensions() != (-1, -1)) {
                    //for the RF target, draw 2 lines on left and right 125mm x 5mm

                    (decimal, decimal) barDimension = target.rapidFireBarDimensions();

                    float bar_height = target.getDimension(dimension, barDimension.Item1, zoomFactor);
                    float bar_width = target.getDimension(dimension, barDimension.Item2, zoomFactor);

                    float bar_y = dimension / 2 - bar_height / 2;

                    float leftBar_x = center - target.getDimension(dimension, target.getOutterRing() / 2, zoomFactor);
                    float rightBar_x = center + target.getDimension(dimension, target.getOutterRing() / 2, zoomFactor) - bar_width;

                    canvas.DrawRect(leftBar_x, bar_y, bar_width, bar_height, brushWhite);
                    canvas.DrawRect(rightBar_x, bar_y, bar_width, bar_height, brushWhite);
                }

                canvas.DrawRect(0.5f, 0.5f, dimension - 1, dimension - 1, penBlack);

                int index = 0;
                foreach (Shot shot in shotList) {
                    drawShot(canvas, target, shot, dimension, zoomFactor, index++, shotList);
                }

                if (Settings.Default.drawMeanGroup) {
                    drawMeanGroup(canvas, target, dimension, zoomFactor, currentSession, shotList);
                }

                canvas.Restore();
            }
        }

        private static void drawShot(SKCanvas canvas, aTarget target, Shot shot, int targetSize, decimal zoomFactor, int l, List<Shot> shotList) {
            if (shot.miss == true) {
                return;
            }

            //transform shot coordinates to image coordinates
            PointF x = target.transform((float)shot.getX(), (float)shot.getY(), targetSize, zoomFactor);

            Color c = Color.FromArgb(200, Settings.Default.scoreOldBackgroundColor); //semitransparent old shots
            Color pen = Settings.Default.scoreOldPenColor;

            if (l == shotList.Count - 1) { //last (current) shot
                if (shot.decimalScore > 9.9m) {
                    c = Settings.Default.score10BackgroundColor;
                    pen = Settings.Default.score10PenColor;
                } else if (shot.decimalScore > 8.9m && shot.decimalScore <= 9.9m) {
                    c = Settings.Default.score9BackgroundColor;
                    pen = Settings.Default.score9PenColor;
                } else {
                    c = Settings.Default.scoreDefaultBackgroundColor;
                    pen = Settings.Default.scoreDefaultPenColor;
                }
            }

            float peletSize = target.getDimension(targetSize, target.getProjectileCaliber(), zoomFactor);

            using (SKPaint b = fill(c))
            using (SKPaint p = stroke(pen, 1))
            using (SKPaint bText = fill(pen)) {
                canvas.DrawCircle(x.X, x.Y, peletSize / 2, b);
                canvas.DrawCircle(x.X, x.Y, peletSize / 2, p);

                canvas.Save();
                canvas.Translate(x.X, x.Y);
                drawCenteredText(canvas, (shot.index + 1).ToString(), peletSize / 3, bText);
                canvas.Restore();
            }
        }

        private static void drawMeanGroup(SKCanvas canvas, aTarget target, decimal currentTargetSize, decimal zoomFactor, Session currentSession, List<Shot> shotList) {
            if (shotList.Count >= 2) {
                float circle = target.getDimension(currentTargetSize, currentSession.rbar * 2, zoomFactor);

                PointF x = target.transform((float)currentSession.xbar, (float)currentSession.ybar, (float)currentTargetSize, zoomFactor);

                using (SKPaint p = stroke(Color.Red, 2)) {
                    canvas.DrawCircle(x.X, x.Y, circle / 2, p);

                    float cross = 5; // center of group cross is always the same size - 5 pixels

                    canvas.DrawLine(x.X - cross, x.Y, x.X + cross, x.Y, p);
                    canvas.DrawLine(x.X, x.Y - cross, x.X, x.Y + cross, p);
                }
            }
        }
    }
}
