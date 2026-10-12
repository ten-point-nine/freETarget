using Avalonia;
using Avalonia.Controls;
using Avalonia.Layout;
using Avalonia.Media;
using Avalonia.Threading;
using System;

namespace freETarget.Desktop {

    /**
     * the settings that are stored in the target itself. they are read from the target when the window opens
     * and sent to it with the apply button
     */
    public class TargetSettingsWindow : Window {

        private readonly MainController controller;
        private readonly DispatcherTimer waitTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(5) };

        internal readonly TextBlock txtWait = new TextBlock { Text = "Please wait while retrieving target settings...", TextWrapping = TextWrapping.Wrap, Margin = new Thickness(0, 0, 0, 8) };
        private readonly Button btnRetry = new Button { Content = "Ask again", IsVisible = false };
        internal readonly StackPanel form = new StackPanel { Spacing = 14, IsEnabled = false };

        internal readonly ComboBox cmbName = new ComboBox { HorizontalAlignment = HorizontalAlignment.Stretch };
        internal readonly Slider trkLEDbright = new Slider { Minimum = 0, Maximum = TargetSettings.MaxLedBrightness, TickFrequency = 1, IsSnapToTickEnabled = true };
        private readonly TextBlock lblLED = new TextBlock { VerticalAlignment = VerticalAlignment.Center };
        internal readonly RadioButton rbDC = new RadioButton { Content = "Direct Current", GroupName = "motor", Margin = new Thickness(0, 0, 16, 0) };
        internal readonly RadioButton rbStepper = new RadioButton { Content = "Stepper", GroupName = "motor", Margin = new Thickness(0, 0, 16, 0) };
        internal readonly TextBox txtPaperTime = number();
        internal readonly TextBox txtSteps = number();
        internal readonly TextBox txtStepTime = number();
        internal readonly TextBox txtSensorDiameter = number();
        internal readonly TextBox txtXoffset = number();
        internal readonly TextBox txtYoffset = number();
        internal readonly TextBox txtZOffset = number();
        internal readonly TextBox txtNorthX = number(), txtNorthY = number(), txtEastX = number(), txtEastY = number();
        internal readonly TextBox txtSouthX = number(), txtSouthY = number(), txtWestX = number(), txtWestY = number();
        internal readonly TextBlock txtError = new TextBlock { TextWrapping = TextWrapping.Wrap }.withBrush(TextBlock.ForegroundProperty, "ErrorTextBrush");
        internal readonly Button btnApply = new Button { Content = "Apply settings to target", IsEnabled = false };
        private readonly Button btnClose = new Button { Content = "Close", MinWidth = 80, HorizontalContentAlignment = HorizontalAlignment.Center, IsCancel = true };

        private static TextBox number() {
            return new TextBox { Text = "0", Width = 80, HorizontalContentAlignment = HorizontalAlignment.Right };
        }

        //for the designer
        public TargetSettingsWindow() : this(null) {
        }

        public TargetSettingsWindow(MainController controller) {
            this.controller = controller;
            Title = "Target Settings";
            SizeToContent = SizeToContent.WidthAndHeight;
            CanResize = false;
            WindowStartupLocation = WindowStartupLocation.CenterOwner;

            foreach (string name in TargetSettings.TargetNames) {
                cmbName.Items.Add(name);
            }

            //hardware
            Grid hardware = grid("Auto,200");
            row(hardware, "Target Name", cmbName);
            row(hardware, lblLED, trkLEDbright);

            Grid motor = grid("Auto,Auto,Auto");
            add(motor, rbDC, 0, 0);
            add(motor, label("Time (ms)"), 0, 1);
            add(motor, txtPaperTime, 0, 2);
            add(motor, rbStepper, 1, 0);
            add(motor, label("Steps"), 1, 1);
            add(motor, txtSteps, 1, 2);
            add(motor, label("Step Duration (ms)"), 2, 1);
            add(motor, txtStepTime, 2, 2);

            Grid offsets = grid("Auto,Auto");
            row(offsets, "X_OFFSET", txtXoffset);
            row(offsets, "Y_OFFSET", txtYoffset);

            //sensors
            Grid sensors = grid("Auto,Auto,Auto");
            add(sensors, label("X"), 0, 1);
            add(sensors, label("Y"), 0, 2);
            sensorRow(sensors, 1, "North", txtNorthX, txtNorthY);
            sensorRow(sensors, 2, "East", txtEastX, txtEastY);
            sensorRow(sensors, 3, "South", txtSouthX, txtSouthY);
            sensorRow(sensors, 4, "West", txtWestX, txtWestY);

            Grid geometry = grid("Auto,Auto");
            row(geometry, "Sensor diameter (mm)", txtSensorDiameter);
            row(geometry, "Sensor Z Offset (mm)", txtZOffset);

            form.Children.Add(new StackPanel { Orientation = Orientation.Horizontal, Spacing = 20, Children = {
                new StackPanel { Spacing = 14, Children = { group("Hardware", hardware), group("Motor", motor), group("Calibration offset", offsets) } },
                new StackPanel { Spacing = 14, Children = { group("Sensors", geometry), group("Sensor offsets", sensors) } }
            } });
            form.Children.Add(new TextBlock {
                Text = "All modifications on this page are sent directly to the target and stored in its memory.",
                TextWrapping = TextWrapping.Wrap, MaxWidth = 560, HorizontalAlignment = HorizontalAlignment.Left
            }.withBrush(TextBlock.ForegroundProperty, "WarningTextBrush"));

            Content = new StackPanel {
                Margin = new Thickness(20),
                Spacing = 10,
                Children = {
                    new StackPanel { Orientation = Orientation.Horizontal, Spacing = 12, Children = { txtWait, btnRetry } },
                    form,
                    txtError,
                    new StackPanel { Orientation = Orientation.Horizontal, Spacing = 8, HorizontalAlignment = HorizontalAlignment.Right, Children = { btnApply, btnClose } }
                }
            };

            trkLEDbright.PropertyChanged += (s, e) => {
                if (e.Property == Slider.ValueProperty) {
                    showBrightness();
                }
            };
            rbDC.IsCheckedChanged += (s, e) => radios();
            rbStepper.IsCheckedChanged += (s, e) => radios();
            btnClose.Click += (s, e) => Close();
            showBrightness();

            if (controller == null) {
                return;
            }

            btnApply.Click += (s, e) => apply();
            btnRetry.Click += (s, e) => ask();
            waitTimer.Tick += (s, e) => {
                waitTimer.Stop();
                txtWait.Text = "The target did not answer.";
                btnRetry.IsVisible = true;
            };
            controller.EchoReceived += onEcho;
            Closed += (s, e) => {
                waitTimer.Stop();
                controller.EchoReceived -= onEcho;
            };
            Opened += (s, e) => ask();
        }

        //---------------------------------------------------------------- layout helpers

        private static Grid grid(string columns) {
            return new Grid { ColumnDefinitions = new ColumnDefinitions(columns) };
        }

        private static TextBlock label(string text) {
            return new TextBlock { Text = text, VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(0, 0, 12, 0) };
        }

        private static void add(Grid grid, Control control, int row, int column) {
            while (grid.RowDefinitions.Count <= row) {
                grid.RowDefinitions.Add(new RowDefinition(GridLength.Auto));
            }
            control.Margin = new Thickness(control.Margin.Left, 3, control.Margin.Right, 3);
            Grid.SetRow(control, row);
            Grid.SetColumn(control, column);
            grid.Children.Add(control);
        }

        private static void row(Grid grid, string text, Control control) {
            row(grid, label(text), control);
        }

        private static void row(Grid grid, Control first, Control control) {
            int index = grid.RowDefinitions.Count;
            first.Margin = new Thickness(0, 0, 12, 0);
            add(grid, first, index, 0);
            add(grid, control, index, 1);
        }

        private static void sensorRow(Grid grid, int index, string name, TextBox x, TextBox y) {
            add(grid, label(name), index, 0);
            x.Margin = new Thickness(0, 0, 8, 0);
            add(grid, x, index, 1);
            add(grid, y, index, 2);
        }

        private static Control group(string title, Control content) {
            return new StackPanel { Spacing = 4, Children = {
                new TextBlock { Text = title, FontWeight = FontWeight.SemiBold },
                new Border { BorderThickness = new Thickness(1), CornerRadius = new CornerRadius(4), Padding = new Thickness(10), Child = content }.withBrush(Border.BorderBrushProperty, "LineBrush")
            } };
        }

        //---------------------------------------------------------------- behaviour

        private void showBrightness() {
            lblLED.Text = "LED Brightness (" + (int)trkLEDbright.Value + ")";
        }

        //only the fields of the selected kind of motor can be edited, the others are zero
        private void radios() {
            if (rbDC.IsChecked == true) {
                txtPaperTime.IsEnabled = true;
                txtSteps.IsEnabled = false;
                txtStepTime.IsEnabled = false;
                txtSteps.Text = "0";
                txtStepTime.Text = "0";
            } else if (rbStepper.IsChecked == true) {
                txtPaperTime.IsEnabled = false;
                txtPaperTime.Text = "0";
                txtSteps.IsEnabled = true;
                txtStepTime.IsEnabled = true;
            }
        }

        private void ask() {
            txtWait.Text = "Please wait while retrieving target settings...";
            btnRetry.IsVisible = false;
            waitTimer.Stop();
            waitTimer.Start();
            controller.requestEcho();
        }

        private void onEcho() {
            if (controller.lastEcho == null) {
                return; //some other message of the target
            }
            waitTimer.Stop();
            btnRetry.IsVisible = false;
            txtWait.Text = "Settings of target " + (controller.lastEcho.NAME ?? "").Trim().Trim('"') + " received.";
            load(TargetSettings.fromEcho(controller.lastEcho));
            form.IsEnabled = true;
            btnApply.IsEnabled = true;
        }

        private void load(TargetSettings settings) {
            cmbName.SelectedIndex = settings.nameId;
            trkLEDbright.Value = settings.ledBrightness;

            txtPaperTime.Text = settings.paperTime;
            txtStepTime.Text = settings.stepTime;
            txtSteps.Text = settings.stepCount;
            if (settings.isDirectCurrent()) {
                rbDC.IsChecked = true;
            } else {
                rbStepper.IsChecked = true;
            }
            radios();

            txtSensorDiameter.Text = settings.sensorDiameter;
            txtXoffset.Text = settings.xOffset;
            txtYoffset.Text = settings.yOffset;
            txtZOffset.Text = settings.zOffset;
            txtNorthX.Text = settings.northX;
            txtNorthY.Text = settings.northY;
            txtEastX.Text = settings.eastX;
            txtEastY.Text = settings.eastY;
            txtSouthX.Text = settings.southX;
            txtSouthY.Text = settings.southY;
            txtWestX.Text = settings.westX;
            txtWestY.Text = settings.westY;
            txtError.Text = "";
        }

        private static string text(TextBox box) {
            return (box.Text ?? "").Trim();
        }

        private void apply() {
            TargetSettings settings = new TargetSettings {
                nameId = cmbName.SelectedIndex,
                ledBrightness = (int)trkLEDbright.Value,
                paperTime = text(txtPaperTime),
                stepTime = text(txtStepTime),
                stepCount = text(txtSteps),
                sensorDiameter = text(txtSensorDiameter),
                xOffset = text(txtXoffset),
                yOffset = text(txtYoffset),
                zOffset = text(txtZOffset),
                northX = text(txtNorthX),
                northY = text(txtNorthY),
                eastX = text(txtEastX),
                eastY = text(txtEastY),
                southX = text(txtSouthX),
                southY = text(txtSouthY),
                westX = text(txtWestX),
                westY = text(txtWestY)
            };

            string error = controller.applyTargetSettings(settings);
            if (error != null) {
                txtError.Text = error;
                return;
            }

            //the target answers with all its settings, which fills the window again
            txtError.Text = "";
            txtWait.Text = "Settings sent. Waiting for the target to confirm...";
            waitTimer.Stop();
            waitTimer.Start();
        }
    }
}
