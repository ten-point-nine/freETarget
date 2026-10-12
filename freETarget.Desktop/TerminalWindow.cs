using Avalonia;
using Avalonia.Controls;
using Avalonia.Layout;
using Avalonia.Media;
using freETarget.Properties;
using System;

namespace freETarget.Desktop {

    /**
     * everything the target sends as text, and buttons to send commands to it by hand.
     * for setting up and troubleshooting a target
     */
    public class TerminalWindow : Window {

        private readonly MainController controller;

        internal readonly TextBox txtOutput = new TextBox {
            IsReadOnly = true, AcceptsReturn = true, TextWrapping = TextWrapping.NoWrap, FontSize = 12,
            FontFamily = new FontFamily("Menlo, Consolas, DejaVu Sans Mono, monospace")
        };
        internal readonly CheckBox ckbAutoscroll = new CheckBox { Content = "Autoscroll", IsChecked = true };
        internal readonly Button btnEcho = button("ECHO", "Ask the target for all its settings");
        internal readonly Button btnVersion = button("VERSION", "Ask the target for the version of its firmware");
        internal readonly Button btnCalibration = button("CAL", "Start the calibration of the target. Press again to stop it");
        internal readonly Button btnInit = button("INIT", "Reset the settings of the target. Asks for the key first");
        internal readonly ComboBox cmbCommands = new ComboBox { MinWidth = 170 };
        internal readonly TextBox txtParameter = new TextBox { Width = 100 };
        internal readonly Button btnSend = button("Send", null);
        internal readonly TextBox txtGenericCommand = new TextBox { Width = 290 };
        internal readonly Button btnSend2 = button("Send", null);
        internal readonly TextBlock txtError = new TextBlock { TextWrapping = TextWrapping.Wrap }.withBrush(TextBlock.ForegroundProperty, "ErrorTextBrush");

        private const string calStart = "CAL";
        private const string calStop = "STOP CAL";

        private static Button button(string text, string tip) {
            Button b = new Button { Content = text, MinWidth = 84, HorizontalContentAlignment = HorizontalAlignment.Center };
            if (tip != null) {
                ToolTip.SetTip(b, tip);
            }
            return b;
        }

        private static Control group(string title, Control content) {
            return new StackPanel { Spacing = 4, Children = {
                new TextBlock { Text = title, FontWeight = FontWeight.SemiBold },
                new Border { BorderThickness = new Thickness(1), CornerRadius = new CornerRadius(4), Padding = new Thickness(10), Child = content }.withBrush(Border.BorderBrushProperty, "LineBrush")
            } };
        }

        //for the designer
        public TerminalWindow() : this(null) {
        }

        public TerminalWindow(MainController controller) {
            this.controller = controller;
            Title = "Terminal " + Settings.Default.Board;
            Width = 860;
            Height = 600;
            MinWidth = 700;
            MinHeight = 420;
            WindowStartupLocation = WindowStartupLocation.CenterOwner;

            foreach (FirmwareCommand command in FirmwareCommand.All) {
                cmbCommands.Items.Add(command);
            }

            Control commands = group("Commands", new StackPanel { Orientation = Orientation.Horizontal, Spacing = 6, Children = { btnEcho, btnVersion, btnCalibration, btnInit } });
            Control parametrized = group("Parametrized commands", new StackPanel { Orientation = Orientation.Horizontal, Spacing = 6, Children = {
                new TextBlock { Text = "Command", VerticalAlignment = VerticalAlignment.Center }, cmbCommands,
                new TextBlock { Text = "Parameter", VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(8, 0, 0, 0) }, txtParameter, btnSend } });
            Control generic = group("Generic command", new StackPanel { Orientation = Orientation.Horizontal, Spacing = 6, Children = {
                new TextBlock { Text = "Command", VerticalAlignment = VerticalAlignment.Center }, txtGenericCommand, btnSend2 } });

            Button close = button("Close", null);
            close.IsCancel = true;
            close.Click += (s, e) => Close();

            DockPanel bottomRow = new DockPanel();
            DockPanel.SetDock(close, Dock.Right);
            bottomRow.Children.Add(close);
            bottomRow.Children.Add(ckbAutoscroll);

            StackPanel bottom = new StackPanel { Spacing = 10, Margin = new Thickness(0, 10, 0, 0), Children = {
                new WrapPanel { Children = { withMargin(commands), withMargin(generic) } },
                parametrized,
                txtError,
                bottomRow
            } };

            DockPanel root = new DockPanel { Margin = new Thickness(14) };
            DockPanel.SetDock(bottom, Dock.Bottom);
            root.Children.Add(bottom);
            root.Children.Add(txtOutput);
            Content = root;

            if (controller == null) {
                return;
            }

            cmbCommands.SelectionChanged += (s, e) => {
                if (cmbCommands.SelectedItem is FirmwareCommand command) {
                    txtParameter.Text = command.defaultValue;
                }
            };
            cmbCommands.SelectedIndex = 0;

            btnEcho.Click += (s, e) => show(controller.sendRaw("{\"ECHO\":0}"));
            btnVersion.Click += (s, e) => show(controller.sendRaw("{\"VERSION\":7}"));
            btnCalibration.Click += (s, e) => {
                if ((string)btnCalibration.Content == calStart) {
                    if (show(controller.sendRaw("{\"CAL\":0}"))) {
                        btnCalibration.Content = calStop;
                    }
                } else {
                    if (show(controller.sendRaw("!"))) {
                        btnCalibration.Content = calStart;
                    }
                }
            };
            btnInit.Click += async (s, e) => {
                string key = await new InputWindow("INIT key", "INIT KEY").ShowDialog<string>(this);
                if (key != null) {
                    sendInit(key);
                }
            };
            btnSend.Click += (s, e) => show(controller.sendCommand(cmbCommands.SelectedItem?.ToString(), txtParameter.Text));
            btnSend2.Click += (s, e) => show(controller.sendRaw(txtGenericCommand.Text));

            controller.TerminalChanged += showOutput;
            Closed += (s, e) => controller.TerminalChanged -= showOutput;
            showOutput();
        }

        private static Control withMargin(Control control) {
            control.Margin = new Thickness(0, 0, 14, 6);
            return control;
        }

        internal void sendInit(string key) {
            show(controller.sendCommand("INIT", key));
        }

        //shows why something was not sent. returns true if it was sent
        private bool show(string error) {
            txtError.Text = error ?? "";
            return error == null;
        }

        private void showOutput() {
            string text = controller.terminalText;
            txtOutput.Text = text;
            if (ckbAutoscroll.IsChecked == true) {
                txtOutput.CaretIndex = text.Length;
            }
        }
    }

    /** asks for one line of text. closes with the text, or with null if it was cancelled */
    public class InputWindow : Window {

        internal readonly TextBox txtInput = new TextBox { Width = 260 };

        //for the designer
        public InputWindow() : this("", "") {
        }

        public InputWindow(string title, string text) {
            Title = title;
            SizeToContent = SizeToContent.WidthAndHeight;
            CanResize = false;
            WindowStartupLocation = WindowStartupLocation.CenterOwner;
            txtInput.Text = text;

            Button ok = new Button { Content = "OK", MinWidth = 80, HorizontalContentAlignment = HorizontalAlignment.Center, IsDefault = true };
            Button cancel = new Button { Content = "Cancel", MinWidth = 80, HorizontalContentAlignment = HorizontalAlignment.Center, IsCancel = true };
            ok.Click += (s, e) => Close(txtInput.Text ?? "");
            cancel.Click += (s, e) => Close(null);

            Content = new StackPanel { Margin = new Thickness(16), Spacing = 12, Children = {
                txtInput,
                new StackPanel { Orientation = Orientation.Horizontal, Spacing = 8, HorizontalAlignment = HorizontalAlignment.Right, Children = { ok, cancel } }
            } };
            Opened += (s, e) => {
                txtInput.Focus();
                txtInput.SelectAll();
            };
        }
    }
}
