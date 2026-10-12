using Avalonia;
using Avalonia.Controls;
using Avalonia.Input.Platform;
using Avalonia.Layout;
using Avalonia.Media;
using System;
using System.Globalization;

namespace freETarget.Desktop {

    /**
     * moves and turns the shots on the target to correct a target that does not measure exactly.
     * every click is applied at once to the shots of the session, so the effect is visible on the target behind this window
     */
    public class CalibrationWindow : Window {

        private readonly MainController controller;

        private readonly TextBox txtIncrement = new TextBox { Text = "0.5", Width = 70, HorizontalContentAlignment = HorizontalAlignment.Right };
        private readonly TextBox txtAngle = new TextBox { Text = "1", Width = 70, HorizontalContentAlignment = HorizontalAlignment.Right };
        internal readonly TextBlock txtXoffset = new TextBlock { FontWeight = FontWeight.SemiBold };
        internal readonly TextBlock txtYoffset = new TextBlock { FontWeight = FontWeight.SemiBold };
        internal readonly TextBlock txtAngleValue = new TextBlock { FontWeight = FontWeight.SemiBold };

        internal readonly Button btnUp = arrowButton("↑", "Move the shots up");
        internal readonly Button btnDown = arrowButton("↓", "Move the shots down");
        internal readonly Button btnLeft = arrowButton("←", "Move the shots to the left");
        internal readonly Button btnRight = arrowButton("→", "Move the shots to the right");
        internal readonly Button btnClockwise = arrowButton("↻", "Turn the shots clockwise");
        internal readonly Button btnAntiClockwise = arrowButton("↺", "Turn the shots anticlockwise");
        internal readonly Button btnReset = new Button { Content = "Reset", MinWidth = 80, HorizontalContentAlignment = HorizontalAlignment.Center };
        private readonly Button btnCopy = new Button { Content = "Copy" };
        private readonly Button btnClose = new Button { Content = "Save & Close", MinWidth = 110, HorizontalContentAlignment = HorizontalAlignment.Center, IsDefault = true };

        private static Button arrowButton(string arrow, string tip) {
            Button button = new Button { Content = arrow, Width = 44, Height = 44, FontSize = 20, HorizontalContentAlignment = HorizontalAlignment.Center, VerticalContentAlignment = VerticalAlignment.Center };
            ToolTip.SetTip(button, tip);
            return button;
        }

        //for the designer
        public CalibrationWindow() : this(null) {
        }

        public CalibrationWindow(MainController controller) {
            this.controller = controller;
            Title = "Calibration";
            SizeToContent = SizeToContent.WidthAndHeight;
            CanResize = false;
            WindowStartupLocation = WindowStartupLocation.CenterOwner;

            //the four directions around the size of a step
            Grid arrows = new Grid { ColumnDefinitions = new ColumnDefinitions("Auto,Auto,Auto"), RowDefinitions = new RowDefinitions("Auto,Auto,Auto"), HorizontalAlignment = HorizontalAlignment.Center };
            place(arrows, btnUp, 0, 1);
            place(arrows, btnLeft, 1, 0);
            place(arrows, new StackPanel { Margin = new Thickness(8), Spacing = 2, Children = { txtIncrement, new TextBlock { Text = "mm", HorizontalAlignment = HorizontalAlignment.Center, FontSize = 11 } } }, 1, 1);
            place(arrows, btnRight, 1, 2);
            place(arrows, btnDown, 2, 1);

            StackPanel turn = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 8, HorizontalAlignment = HorizontalAlignment.Center, Children = {
                btnAntiClockwise,
                new StackPanel { Spacing = 2, VerticalAlignment = VerticalAlignment.Center, Children = { txtAngle, new TextBlock { Text = "degrees", HorizontalAlignment = HorizontalAlignment.Center, FontSize = 11 } } },
                btnClockwise } };

            Grid values = new Grid { ColumnDefinitions = new ColumnDefinitions("Auto,*,Auto"), RowDefinitions = new RowDefinitions("Auto,Auto,Auto") };
            place(values, new TextBlock { Text = "X_OFFSET", Margin = new Thickness(0, 2, 16, 2) }, 0, 0);
            place(values, txtXoffset, 0, 1);
            place(values, new TextBlock { Text = "Y_OFFSET", Margin = new Thickness(0, 2, 16, 2) }, 1, 0);
            place(values, txtYoffset, 1, 1);
            place(values, new TextBlock { Text = "Angle", Margin = new Thickness(0, 2, 16, 2) }, 2, 0);
            place(values, txtAngleValue, 2, 1);
            Grid.SetRowSpan(btnCopy, 2);
            btnCopy.VerticalAlignment = VerticalAlignment.Center;
            place(values, btnCopy, 0, 2);

            Content = new StackPanel {
                Margin = new Thickness(20),
                Spacing = 16,
                MinWidth = 260,
                Children = {
                    arrows,
                    turn,
                    values,
                    new StackPanel { Orientation = Orientation.Horizontal, Spacing = 8, HorizontalAlignment = HorizontalAlignment.Right, Children = { btnReset, btnClose } }
                }
            };

            if (controller == null) {
                return;
            }

            btnUp.Click += (s, e) => controller.calibrate(0, getIncrement(), 0);
            btnDown.Click += (s, e) => controller.calibrate(0, -getIncrement(), 0);
            btnLeft.Click += (s, e) => controller.calibrate(-getIncrement(), 0, 0);
            btnRight.Click += (s, e) => controller.calibrate(getIncrement(), 0, 0);
            btnClockwise.Click += (s, e) => controller.calibrate(0, 0, -getAngle());
            btnAntiClockwise.Click += (s, e) => controller.calibrate(0, 0, getAngle());
            btnReset.Click += (s, e) => controller.resetCalibration();
            btnCopy.Click += async (s, e) => {
                if (Clipboard != null) {
                    await Clipboard.SetTextAsync("{\"X_OFFSET\":" + txtXoffset.Text + ", \"Y_OFFSET\":" + txtYoffset.Text + "}");
                }
            };
            btnClose.Click += (s, e) => Close();
            ToolTip.SetTip(btnCopy, "Copy the offsets as a command for the target");

            controller.CalibrationChanged += showValues;
            Closed += (s, e) => {
                controller.CalibrationChanged -= showValues;
                controller.saveCalibration();
            };
            showValues();
        }

        private static void place(Grid grid, Control control, int row, int column) {
            Grid.SetRow(control, row);
            Grid.SetColumn(control, column);
            grid.Children.Add(control);
        }

        private void showValues() {
            txtXoffset.Text = controller.calibrationX.ToString(CultureInfo.InvariantCulture);
            txtYoffset.Text = controller.calibrationY.ToString(CultureInfo.InvariantCulture);
            txtAngleValue.Text = controller.calibrationAngle.ToString(CultureInfo.InvariantCulture);
        }

        //a number from a text box, with a decimal point or comma, kept inside its limits. the box shows what is used
        private static decimal read(TextBox box, decimal fallback, decimal min, decimal max) {
            decimal value;
            if (!decimal.TryParse((box.Text ?? "").Trim().Replace(',', '.'), NumberStyles.Number, CultureInfo.InvariantCulture, out value)) {
                value = fallback;
            }
            value = Math.Max(min, Math.Min(max, value));
            box.Text = value.ToString(CultureInfo.InvariantCulture);
            return value;
        }

        private decimal getIncrement() {
            return read(txtIncrement, 0.5m, 0.01m, 10m);
        }

        private decimal getAngle() {
            return read(txtAngle, 1m, 0.01m, 180m);
        }
    }
}
