using Avalonia;
using Avalonia.Controls;
using Avalonia.Layout;
using Avalonia.Media;
using System.Collections.Generic;

namespace freETarget.Desktop {

    /**
     * lists the saved sessions of the current shooter in the current event.
     * closes with the session to continue, or with null
     */
    public class ResumeWindow : Window {

        internal readonly ListBox lstbSessions = new ListBox { Height = 260, FontFamily = new FontFamily("Menlo, Consolas, DejaVu Sans Mono, monospace") };
        internal readonly Button btnOK = new Button { Content = "OK", MinWidth = 80, HorizontalContentAlignment = HorizontalAlignment.Center, IsDefault = true, IsEnabled = false };
        private readonly Button btnCancel = new Button { Content = "Cancel", MinWidth = 80, HorizontalContentAlignment = HorizontalAlignment.Center, IsCancel = true };

        //for the designer
        public ResumeWindow() : this(null) {
        }

        public ResumeWindow(MainController controller) {
            Title = "Resume Session";
            Width = 380;
            SizeToContent = SizeToContent.Height;
            CanResize = false;
            WindowStartupLocation = WindowStartupLocation.CenterOwner;

            TextBlock lblEvent = new TextBlock { FontWeight = FontWeight.SemiBold };
            TextBlock lblUser = new TextBlock { FontWeight = FontWeight.SemiBold };
            TextBlock lblEmpty = new TextBlock { Text = "There are no saved sessions of this event.", IsVisible = false }.withBrush(TextBlock.ForegroundProperty, "MutedTextBrush");

            Grid header = new Grid { ColumnDefinitions = new ColumnDefinitions("Auto,*"), RowDefinitions = new RowDefinitions("Auto,Auto") };
            TextBlock eventLabel = new TextBlock { Text = "Event", Margin = new Thickness(0, 0, 16, 2) };
            TextBlock userLabel = new TextBlock { Text = "User", Margin = new Thickness(0, 0, 16, 2) };
            Grid.SetColumn(lblEvent, 1);
            Grid.SetRow(userLabel, 1);
            Grid.SetRow(lblUser, 1);
            Grid.SetColumn(lblUser, 1);
            header.Children.Add(eventLabel);
            header.Children.Add(lblEvent);
            header.Children.Add(userLabel);
            header.Children.Add(lblUser);

            Content = new StackPanel {
                Margin = new Thickness(20),
                Spacing = 12,
                Children = {
                    header,
                    lstbSessions,
                    lblEmpty,
                    new StackPanel { Orientation = Orientation.Horizontal, Spacing = 8, HorizontalAlignment = HorizontalAlignment.Right, Children = { btnOK, btnCancel } }
                }
            };

            lstbSessions.SelectionChanged += (s, e) => btnOK.IsEnabled = lstbSessions.SelectedItem != null;
            lstbSessions.DoubleTapped += (s, e) => choose();
            btnOK.Click += (s, e) => choose();
            btnCancel.Click += (s, e) => Close(null);

            if (controller != null && controller.currentSession != null) {
                lblEvent.Text = controller.currentSession.eventType.ToString();
                lblUser.Text = controller.currentSession.user;

                List<ListBoxSessionItem> sessions = controller.getResumableSessions();
                foreach (ListBoxSessionItem item in sessions) {
                    lstbSessions.Items.Add(item); //shown with its date and score
                }
                lblEmpty.IsVisible = sessions.Count == 0;
            }
        }

        private void choose() {
            if (lstbSessions.SelectedItem != null) {
                Close(lstbSessions.SelectedItem);
            }
        }
    }
}
