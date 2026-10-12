using Avalonia;
using Avalonia.Controls;
using Avalonia.Layout;
using Avalonia.Media;
using Avalonia.Threading;
using System;

namespace freETarget.Desktop {

    /**
     * message boxes. the controller asks its questions in the middle of what it is doing, so these block until the user answered,
     * like the message boxes of Windows do
     */
    public class Dialogs : IUserDialogs {

        /** the window that the message boxes belong to. null before the main window exists */
        public Window owner;

        public void showError(string text, string title) {
            show(text, title, "OK");
        }

        public void showWarning(string text, string title) {
            show(text, title, "OK");
        }

        public bool confirm(string text, string title) {
            return show(text, title, "OK", "Cancel") == "OK";
        }

        public bool askYesNo(string text, string title) {
            return show(text, title, "Yes", "No") == "Yes";
        }

        public bool? askYesNoCancel(string text, string title) {
            string answer = show(text, title, "Yes", "No", "Cancel");
            if (answer == "Yes") {
                return true;
            } else if (answer == "No") {
                return false;
            } else {
                return null;
            }
        }

        /** shows the message and waits for the answer. returns the text of the button, or the last button if the window was closed */
        public string show(string text, string title, params string[] buttons) {
            if (!Dispatcher.UIThread.CheckAccess()) {
                return Dispatcher.UIThread.Invoke(() => show(text, title, buttons));
            }

            string answer = buttons[buttons.Length - 1];

            Window box = new Window {
                Title = title,
                SizeToContent = SizeToContent.WidthAndHeight,
                CanResize = false,
                MinWidth = 320,
                MaxWidth = 640,
                WindowStartupLocation = owner != null && owner.IsVisible ? WindowStartupLocation.CenterOwner : WindowStartupLocation.CenterScreen
            };

            StackPanel buttonRow = new StackPanel { Orientation = Orientation.Horizontal, HorizontalAlignment = HorizontalAlignment.Right, Spacing = 8 };
            for (int i = 0; i < buttons.Length; i++) {
                string name = buttons[i];
                Button button = new Button { Content = name, MinWidth = 80, HorizontalContentAlignment = HorizontalAlignment.Center, IsDefault = i == 0, IsCancel = i == buttons.Length - 1 };
                button.Click += (s, e) => {
                    answer = name;
                    box.Close();
                };
                buttonRow.Children.Add(button);
            }

            box.Content = new StackPanel {
                Margin = new Thickness(20),
                Spacing = 20,
                Children = {
                    new TextBlock { Text = text, TextWrapping = TextWrapping.Wrap },
                    buttonRow
                }
            };

            //wait here, with the UI alive, until the box is closed
            DispatcherFrame frame = new DispatcherFrame();
            box.Closed += (s, e) => frame.Continue = false;
            if (owner != null && owner.IsVisible) {
                box.ShowDialog(owner);
            } else {
                box.Show();
            }
            Dispatcher.UIThread.PushFrame(frame);

            return answer;
        }
    }
}
