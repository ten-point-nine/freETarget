using Avalonia;
using Avalonia.Controls;
using Avalonia.Media;
using System;
using System.Collections.Generic;
using System.Globalization;

namespace freETarget.Desktop {

    /**
     * a small chart for the journal: either a line through one value per session, or one stem per shot over the time of a session.
     * drawn with the colors of the current appearance
     */
    public class SimpleChart : Control {

        private string title = "";
        private List<Point> points = new List<Point>();
        private bool stems = false;
        private double yMin = 0;
        private double yMax = 1;
        private double xMax = 1;

        /** one value per position, connected by a line */
        public void setLine(string title, IReadOnlyList<double> values, double yMin, double yMax) {
            this.title = title;
            this.stems = false;
            this.points = new List<Point>();
            for (int i = 0; i < values.Count; i++) {
                this.points.Add(new Point(i, values[i]));
            }
            this.xMax = Math.Max(1, values.Count - 1);
            setRange(yMin, yMax);
        }

        /** a vertical line with a dot for every point */
        public void setStems(string title, IReadOnlyList<Point> points, double xMax, double yMin, double yMax) {
            this.title = title;
            this.stems = true;
            this.points = new List<Point>(points);
            this.xMax = Math.Max(1, xMax);
            setRange(yMin, yMax);
        }

        public int PointCount {
            get { return points.Count; }
        }

        public string Title {
            get { return title; }
        }

        private void setRange(double min, double max) {
            if (max <= min) {
                max = min + 1;
            }
            yMin = min;
            yMax = max;
            InvalidateVisual();
        }

        private IBrush brush(string resource, IBrush fallback) {
            object found;
            if (this.TryFindResource(resource, ActualThemeVariant, out found) && found is IBrush) {
                return (IBrush)found;
            }
            return fallback;
        }

        private static FormattedText text(string s, double size, IBrush brush) {
            return new FormattedText(s, CultureInfo.InvariantCulture, FlowDirection.LeftToRight, Typeface.Default, size, brush);
        }

        public override void Render(DrawingContext context) {
            base.Render(context);

            IBrush textBrush = brush("MutedTextBrush", Brushes.Gray);
            IBrush lineBrush = brush("LineBrush", Brushes.LightGray);
            IBrush dataBrush = Brushes.DodgerBlue;
            Pen gridPen = new Pen(lineBrush, 1);
            Pen dataPen = new Pen(dataBrush, 2);

            double left = 44;
            double top = 26;
            double right = Bounds.Width - 12;
            double bottom = Bounds.Height - 12;
            if (right - left < 20 || bottom - top < 20) {
                return;
            }

            context.DrawText(text(title, 12, textBrush), new Point(left, 4));

            //horizontal lines with their values. at most six of them
            int lines = 5;
            for (int i = 0; i <= lines; i++) {
                double value = yMin + (yMax - yMin) * i / lines;
                double y = bottom - (bottom - top) * i / lines;
                context.DrawLine(gridPen, new Point(left, y), new Point(right, y));
                FormattedText label = text(value.ToString("0.##", CultureInfo.InvariantCulture), 10, textBrush);
                context.DrawText(label, new Point(left - 6 - label.Width, y - label.Height / 2));
            }

            if (points.Count == 0) {
                FormattedText empty = text("no data", 11, textBrush);
                context.DrawText(empty, new Point((left + right - empty.Width) / 2, (top + bottom - empty.Height) / 2));
                return;
            }

            Point? previous = null;
            foreach (Point p in points) {
                double x = left + (right - left) * Math.Max(0, Math.Min(xMax, p.X)) / xMax;
                double y = bottom - (bottom - top) * (Math.Max(yMin, Math.Min(yMax, p.Y)) - yMin) / (yMax - yMin);
                Point at = new Point(x, y);

                if (stems) {
                    context.DrawLine(dataPen, new Point(x, bottom), at);
                } else if (previous != null) {
                    context.DrawLine(dataPen, previous.Value, at);
                }
                context.DrawEllipse(dataBrush, null, at, 3, 3);
                previous = at;
            }
        }
    }
}
