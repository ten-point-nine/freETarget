using Avalonia;
using Avalonia.Controls;
using Avalonia.Layout;
using Avalonia.Media;
using Avalonia.Platform.Storage;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;

namespace freETarget.Desktop {

    /**
     * the saved sessions of a shooter in an event: details of a session, its diary, report and shot chart,
     * loading it into the main window, export and import, and statistics over all sessions
     */
    public class JournalWindow : Window {

        private readonly MainController controller;

        internal readonly ComboBox cmbUsers = new ComboBox { MinWidth = 180 };
        internal readonly ComboBox cmbEvents = new ComboBox { MinWidth = 200 };
        internal readonly ListBox lstbSessions = new ListBox { Width = 250, FontFamily = new FontFamily("Menlo, Consolas, DejaVu Sans Mono, monospace"), FontSize = 12 };
        internal readonly Grid details = new Grid { ColumnDefinitions = new ColumnDefinitions("Auto,*"), Margin = new Thickness(4, 0) };
        internal readonly TextBlock txtDiary = new TextBlock { TextWrapping = TextWrapping.Wrap, MaxHeight = 150 };

        internal readonly Button btnLoadSession = button("Load", "Show the session in the main window");
        internal readonly Button btnDiary = button("Diary", "Read and write the diary of the session");
        internal readonly Button btnPrint = button("Print", "Write the report of the session as PDF and open it");
        internal readonly Button btnGraph = button("Chart", "The scores of the shots over the time of the session");
        internal readonly Button btnExport = button("Export", "Write the session to a file that another computer can import");
        internal readonly Button btnImport = button("Import", "Read a session from an exported file");
        internal readonly Button btnDelete = button("Delete", "Delete the session permanently");

        internal readonly SimpleChart chartScore = chart();
        internal readonly SimpleChart chartMeanRadius = chart();
        internal readonly SimpleChart chartWindage = chart();
        internal readonly SimpleChart chartElevation = chart();

        internal Session currentSession;
        private bool filling = false;

        private static Button button(string text, string tip) {
            Button b = new Button { Content = text, MinWidth = 76, HorizontalContentAlignment = HorizontalAlignment.Center };
            ToolTip.SetTip(b, tip);
            return b;
        }

        private static SimpleChart chart() {
            return new SimpleChart { MinHeight = 180, MinWidth = 320 };
        }

        //for the designer
        public JournalWindow() : this(null) {
        }

        public JournalWindow(MainController controller) {
            this.controller = controller;
            Title = "Journal";
            Width = 900;
            Height = 640;
            MinWidth = 760;
            MinHeight = 480;
            WindowStartupLocation = WindowStartupLocation.CenterOwner;

            StackPanel top = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 10, Margin = new Thickness(0, 0, 0, 10), Children = {
                new TextBlock { Text = "Shooter", VerticalAlignment = VerticalAlignment.Center }, cmbUsers,
                new TextBlock { Text = "Event", VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(12, 0, 0, 0) }, cmbEvents
            } };

            //sessions tab
            StackPanel buttons = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 6, Children = { btnLoadSession, btnDiary, btnPrint, btnGraph, btnExport, btnDelete } };
            DockPanel right = new DockPanel { Margin = new Thickness(14, 0, 0, 0) };
            DockPanel.SetDock(buttons, Dock.Bottom);
            right.Children.Add(buttons);
            right.Children.Add(new ScrollViewer { Content = new StackPanel { Spacing = 12, Children = {
                details,
                new TextBlock { Text = "Diary", FontWeight = FontWeight.SemiBold, Margin = new Thickness(4, 0) },
                new Border { Padding = new Thickness(8), Margin = new Thickness(4, 0, 4, 8), BorderThickness = new Thickness(1), CornerRadius = new CornerRadius(4), Child = txtDiary }.withBrush(Border.BorderBrushProperty, "LineBrush")
            } } });

            DockPanel left = new DockPanel();
            DockPanel.SetDock(btnImport, Dock.Bottom);
            btnImport.Margin = new Thickness(0, 8, 0, 0);
            btnImport.HorizontalAlignment = HorizontalAlignment.Left;
            left.Children.Add(btnImport);
            left.Children.Add(lstbSessions);

            DockPanel sessions = new DockPanel { Margin = new Thickness(4, 10) };
            DockPanel.SetDock(left, Dock.Left);
            sessions.Children.Add(left);
            sessions.Children.Add(right);

            //statistics tab
            Grid statistics = new Grid { ColumnDefinitions = new ColumnDefinitions("*,*"), RowDefinitions = new RowDefinitions("*,*"), Margin = new Thickness(4, 10) };
            SimpleChart[] charts = { chartScore, chartMeanRadius, chartWindage, chartElevation };
            for (int i = 0; i < charts.Length; i++) {
                Grid.SetRow(charts[i], i / 2);
                Grid.SetColumn(charts[i], i % 2);
                statistics.Children.Add(charts[i]);
            }

            TabControl tabs = new TabControl();
            tabs.Items.Add(new TabItem { Header = "Sessions", Content = sessions });
            tabs.Items.Add(new TabItem { Header = "Statistics", Content = statistics });

            Button close = new Button { Content = "Close", MinWidth = 80, HorizontalContentAlignment = HorizontalAlignment.Center, HorizontalAlignment = HorizontalAlignment.Right, IsCancel = true };
            close.Click += (s, e) => Close();

            DockPanel root = new DockPanel { Margin = new Thickness(14) };
            DockPanel.SetDock(top, Dock.Top);
            DockPanel.SetDock(close, Dock.Bottom);
            root.Children.Add(top);
            root.Children.Add(close);
            root.Children.Add(tabs);
            Content = root;

            if (controller == null) {
                return;
            }

            cmbUsers.SelectionChanged += (s, e) => { if (!filling) { loadSessions(); } };
            cmbEvents.SelectionChanged += (s, e) => { if (!filling) { loadSessions(); } };
            lstbSessions.SelectionChanged += (s, e) => { if (!filling) { showSelectedSession(); } };
            lstbSessions.DoubleTapped += (s, e) => loadIntoMainWindow();
            btnLoadSession.Click += (s, e) => loadIntoMainWindow();
            btnDiary.Click += async (s, e) => await editDiary();
            btnPrint.Click += (s, e) => print();
            btnGraph.Click += async (s, e) => { if (currentSession != null) { await new SessionChartWindow(currentSession).ShowDialog(this); } };
            btnExport.Click += (s, e) => { if (currentSession != null) { controller.exportSessionFile(currentSession); } };
            btnImport.Click += async (s, e) => await importFile();
            btnDelete.Click += (s, e) => delete();

            loadData();
        }

        //---------------------------------------------------------------- lists

        /** fills shooters and events and shows the sessions of the first ones (or of the given ones) */
        internal void loadData(string user = null, Event ev = null) {
            filling = true;
            cmbUsers.Items.Clear();
            List<string> users = controller.storage.findAllUsers();
            users.Sort(StringComparer.CurrentCultureIgnoreCase);
            foreach (string u in users) {
                cmbUsers.Items.Add(u);
            }

            //the active events first, like the buttons of the main window, then the others
            cmbEvents.Items.Clear();
            foreach (Event active in controller.eventManager.getActiveEventsList()) {
                cmbEvents.Items.Add(active);
            }
            foreach (Event other in controller.eventManager.getEventsList()) {
                if (!cmbEvents.Items.Contains(other)) {
                    cmbEvents.Items.Add(other);
                }
            }

            string wantedUser = user ?? Properties.Settings.Default.name;
            cmbUsers.SelectedItem = users.Contains(wantedUser) ? wantedUser : users.FirstOrDefault();
            Event wantedEvent = ev ?? controller.selectedEvent;
            cmbEvents.SelectedItem = wantedEvent != null && cmbEvents.Items.Contains(wantedEvent) ? wantedEvent : (cmbEvents.ItemCount > 0 ? cmbEvents.Items[0] : null);
            filling = false;

            loadSessions();
        }

        private void loadSessions() {
            filling = true;
            lstbSessions.Items.Clear();
            string user = cmbUsers.SelectedItem as string;
            Event ev = cmbEvents.SelectedItem as Event;
            if (user != null && ev != null) {
                foreach (ListBoxSessionItem item in controller.storage.findSessionsForUser(user, ev)) {
                    lstbSessions.Items.Add(item);
                }
            }
            filling = false;

            showSelectedSession();
            loadStatistics(user, ev);
        }

        private void showSelectedSession() {
            ListBoxSessionItem item = lstbSessions.SelectedItem as ListBoxSessionItem;
            currentSession = item == null ? null : controller.storage.findSession(item.id);
            if (item != null && currentSession == null) {
                //could not be loaded, the reason was shown
                filling = true;
                lstbSessions.SelectedItem = null;
                filling = false;
            }

            bool selected = currentSession != null;
            foreach (Button b in new[] { btnLoadSession, btnDiary, btnPrint, btnGraph, btnExport, btnDelete }) {
                b.IsEnabled = selected;
            }
            showDetails();
        }

        //---------------------------------------------------------------- details of the selected session

        private void addDetail(string label, string value) {
            int row = details.RowDefinitions.Count;
            details.RowDefinitions.Add(new RowDefinition(GridLength.Auto));
            TextBlock l = new TextBlock { Text = label, Margin = new Thickness(0, 2, 20, 2) }.withBrush(TextBlock.ForegroundProperty, "MutedTextBrush");
            TextBlock v = new TextBlock { Text = value, Margin = new Thickness(0, 2), TextWrapping = TextWrapping.Wrap };
            Grid.SetRow(l, row);
            Grid.SetRow(v, row);
            Grid.SetColumn(v, 1);
            details.Children.Add(l);
            details.Children.Add(v);
        }

        private static string number(decimal value, string format) {
            return value.ToString(format, CultureInfo.InvariantCulture);
        }

        private static string seconds(TimeSpan time) {
            return time.TotalSeconds.ToString("F1", CultureInfo.InvariantCulture) + " s";
        }

        private void showDetails() {
            details.Children.Clear();
            details.RowDefinitions.Clear();
            Session s = currentSession;
            if (s == null) {
                txtDiary.Text = "";
                addDetail("Select a session", "");
                return;
            }

            addDetail("Session ID", s.id.ToString(CultureInfo.InvariantCulture));
            addDetail("Event", s.eventType.Name);
            addDetail("Shooter", s.user);
            addDetail("Start", s.startTime.ToString("yyyy-MM-dd HH:mm:ss"));
            addDetail("End", s.endTime.ToString("yyyy-MM-dd HH:mm:ss"));
            addDetail("Shots", s.actualNumberOfShots.ToString(CultureInfo.InvariantCulture) + (s.numberOfShots > 0 ? " of " + s.numberOfShots : ""));
            addDetail("Score", s.score + " (" + number(s.decimalScore, "F1") + ")  -  " + s.innerX + "x");
            addDetail("Average score", number(s.averageScore, "F2"));
            addDetail("Mean radius", number(s.rbar, "F2") + " mm");
            addDetail("Windage", number(Math.Abs(s.xbar), "F2") + " mm " + (s.xbar < 0 ? "←" : "→"));
            addDetail("Elevation", number(Math.Abs(s.ybar), "F2") + " mm " + (s.ybar < 0 ? "↓" : "↑"));
            addDetail("Group size", number(s.groupSize, "F1") + " mm");
            addDetail("Time per shot", seconds(s.averageTimePerShot) + " average, " + seconds(s.shortestShot) + " shortest, " + seconds(s.longestShot) + " longest");

            string diary = RtfText.toPlainText(s.diaryEntry);
            txtDiary.Text = diary == "" ? "(empty)" : diary;
        }

        //---------------------------------------------------------------- statistics

        private void loadStatistics(string user, Event ev) {
            StatisticsSeries scores = new StatisticsSeries();
            StatisticsSeries radius = new StatisticsSeries();
            StatisticsSeries windage = new StatisticsSeries();
            StatisticsSeries elevation = new StatisticsSeries();
            if (user != null && ev != null) {
                scores = StatisticsSeries.from(controller.storage.findScoresForUser(user, ev));
                radius = StatisticsSeries.from(controller.storage.findRBarForUser(user, ev));
                windage = StatisticsSeries.from(controller.storage.findXBarForUser(user, ev));
                elevation = StatisticsSeries.from(controller.storage.findYBarForUser(user, ev));
            }

            chartScore.setLine(title("Scores", "average score", scores), scores.values, Math.Floor(scores.min - 1), 10.9);
            chartMeanRadius.setLine(title("Mean Radius", "average mean radius", radius), radius.values, Math.Floor(radius.min - 1), Math.Ceiling(radius.max + 1));
            chartWindage.setLine(title("Windage", "average windage", windage), windage.values, Math.Floor(windage.min - 1), Math.Ceiling(windage.max + 1));
            chartElevation.setLine(title("Elevation", "average elevation", elevation), elevation.values, Math.Floor(elevation.min - 1), Math.Ceiling(elevation.max + 1));
        }

        private static string title(string name, string what, StatisticsSeries series) {
            if (series.values.Count == 0) {
                return name;
            }
            return name + "  -  last 10 sessions " + what + ": " + series.lastTenAverage.ToString("F2", CultureInfo.InvariantCulture);
        }

        //---------------------------------------------------------------- actions

        private void loadIntoMainWindow() {
            if (currentSession != null) {
                //a fresh copy from the database: the main window changes the series of the session it shows
                Session session = controller.storage.findSession(currentSession.id);
                if (session != null) {
                    controller.loadSessionFromJournal(session);
                }
            }
        }

        private async System.Threading.Tasks.Task editDiary() {
            if (currentSession == null) {
                return;
            }
            string before = RtfText.toPlainText(currentSession.diaryEntry);
            DiaryWindow diary = new DiaryWindow(before == "" ? RtfText.diaryTemplate() : before);
            string text = await diary.ShowDialog<string>(this);
            if (text != null) {
                saveDiary(text);
            }
        }

        internal void saveDiary(string text) {
            //stored as RTF, which is what the original Windows application reads
            currentSession.diaryEntry = RtfText.fromPlainText(text);
            controller.storage.updateDiary(currentSession.id, currentSession.diaryEntry);
            showDetails();
        }

        private void print() {
            if (currentSession == null) {
                return;
            }
            string file = controller.exportSavedSessionPdf(currentSession.id);
            if (file != null) {
                try {
                    PDFGenerator.open(file);
                } catch (Exception ex) {
                    controller.showWarning("The report was saved as " + file + " but could not be opened: " + ex.Message, "PDF export");
                }
            }
        }

        private async System.Threading.Tasks.Task importFile() {
            IStorageFolder start = await StorageProvider.TryGetFolderFromPathAsync(SessionFile.getExportFolder());
            IReadOnlyList<IStorageFile> files = await StorageProvider.OpenFilePickerAsync(new FilePickerOpenOptions {
                Title = "Import a session",
                AllowMultiple = false,
                SuggestedStartLocation = start,
                FileTypeFilter = new[] { new FilePickerFileType("freETarget sessions") { Patterns = new[] { "*.exp" } }, FilePickerFileTypes.All }
            });
            if (files.Count == 1) {
                string path = files[0].TryGetLocalPath();
                if (path != null) {
                    import(path);
                }
            }
        }

        internal void import(string path) {
            if (controller.importSessionFile(path)) {
                loadData(cmbUsers.SelectedItem as string, cmbEvents.SelectedItem as Event);
            }
        }

        private void delete() {
            if (currentSession == null) {
                return;
            }
            if (controller.confirm("Are you sure you want to permanently delete this session?", "Delete session")) {
                controller.storage.deleteSession(currentSession.id);
                loadData(cmbUsers.SelectedItem as string, cmbEvents.SelectedItem as Event);
            }
        }
    }

    /** the diary of a session as text. closes with the text, or with null if it was cancelled */
    public class DiaryWindow : Window {

        internal readonly TextBox txtPage = new TextBox { AcceptsReturn = true, AcceptsTab = true, TextWrapping = TextWrapping.Wrap, Width = 520, Height = 420 };

        //for the designer
        public DiaryWindow() : this("") {
        }

        public DiaryWindow(string text) {
            Title = "Diary";
            SizeToContent = SizeToContent.WidthAndHeight;
            CanResize = false;
            WindowStartupLocation = WindowStartupLocation.CenterOwner;
            txtPage.Text = text;

            Button ok = new Button { Content = "OK", MinWidth = 80, HorizontalContentAlignment = HorizontalAlignment.Center };
            Button cancel = new Button { Content = "Cancel", MinWidth = 80, HorizontalContentAlignment = HorizontalAlignment.Center, IsCancel = true };
            ok.Click += (s, e) => Close(txtPage.Text ?? "");
            cancel.Click += (s, e) => Close(null);

            Content = new StackPanel { Margin = new Thickness(16), Spacing = 10, Children = {
                txtPage,
                new StackPanel { Orientation = Orientation.Horizontal, Spacing = 8, HorizontalAlignment = HorizontalAlignment.Right, Children = { ok, cancel } }
            } };
        }
    }

    /** the decimal scores of the shots of a session over its time */
    public class SessionChartWindow : Window {

        internal readonly SimpleChart chart = new SimpleChart();

        //for the designer
        public SessionChartWindow() : this(null) {
        }

        public SessionChartWindow(Session session) {
            Title = "Chart";
            Width = 760;
            Height = 360;
            WindowStartupLocation = WindowStartupLocation.CenterOwner;

            Button close = new Button { Content = "Close", MinWidth = 80, HorizontalContentAlignment = HorizontalAlignment.Center, HorizontalAlignment = HorizontalAlignment.Right, IsCancel = true, IsDefault = true };
            close.Click += (s, e) => Close();
            DockPanel root = new DockPanel { Margin = new Thickness(14) };
            DockPanel.SetDock(close, Dock.Bottom);
            root.Children.Add(close);
            root.Children.Add(chart);
            Content = root;

            if (session == null) {
                return;
            }

            List<Point> points = new List<Point>();
            DateTime end = session.endTime;
            foreach (Shot shot in session.Shots) {
                if (shot.timestamp > end) {
                    end = shot.timestamp;
                }
                points.Add(new Point(Math.Max(0, (shot.timestamp - session.startTime).TotalSeconds), (double)shot.decimalScore));
            }
            chart.setStems("Score (decimal) over the time of session " + session.id, points, (end - session.startTime).TotalSeconds, 1, 11);
        }
    }
}
