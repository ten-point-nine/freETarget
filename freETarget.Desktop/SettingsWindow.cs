using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.Templates;
using Avalonia.Layout;
using Avalonia.Media;
using freETarget.Properties;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO.Ports;
using System.Linq;

namespace freETarget.Desktop {

    /**
     * the application settings, on tabs: general, connection, colors, the events and which of them are active.
     * closes with true when the user pressed OK and the settings object was changed; the caller saves it to the database.
     * events are different: they are saved as soon as their Save button is pressed, independent of the OK button of the window
     */
    public class SettingsWindow : Window {

        private readonly MainController controller;

        //general
        internal readonly TextBox txtName = new TextBox();
        internal readonly ComboBox cmbDefaultEvent = stretched(new ComboBox());
        internal readonly TextBox txtDistance = new TextBox();
        internal readonly TextBox txtPdfFolder = new TextBox();
        private readonly CheckBox chkShowScoring = new CheckBox { Content = "Show scoring" };
        private readonly CheckBox chkOnlySeries = new CheckBox { Content = "Display only current series target" };
        private readonly CheckBox chkMeanGroup = new CheckBox { Content = "Draw Mean Group" };
        private readonly CheckBox chkIgnoreMiss = new CheckBox { Content = "Ignore misses" };
        private readonly CheckBox chkVoice = new CheckBox { Content = "Voice commands in finals" };
        private readonly CheckBox chkScoreVoice = new CheckBox { Content = "Score spoken" };
        private readonly CheckBox chkLogging = new CheckBox { Content = "Enable logging" };
        private readonly CheckBox chkConsole = new CheckBox { Content = "Display Debug Console" };

        //connection
        private readonly ComboBox cmbProtocol = stretched(new ComboBox());
        private readonly ComboBox cmbPort = stretched(new ComboBox());
        private readonly TextBox txtBaud = new TextBox();
        private readonly TextBox txtIP = new TextBox();
        private readonly TextBox txtTcpPort = new TextBox();
        private readonly ComboBox cmbBoard = stretched(new ComboBox());

        //colors
        internal readonly ComboBox cmbTargetColor = colorCombo();
        internal readonly ComboBox cmbOldBack = colorCombo(), cmbOldText = colorCombo();
        internal readonly ComboBox cmbLastBack = colorCombo(), cmbLastText = colorCombo();
        internal readonly ComboBox cmb9Back = colorCombo(), cmb9Text = colorCombo();
        internal readonly ComboBox cmb10Back = colorCombo(), cmb10Text = colorCombo();

        //events
        internal readonly ListBox lstEvents = new ListBox { Width = 190, Height = 420 };
        internal readonly Button btnAddEvent = new Button { Content = "Add New" };
        internal readonly Button btnSaveEvent = new Button { Content = "Save", MinWidth = 80, HorizontalContentAlignment = HorizontalAlignment.Center, IsEnabled = false };
        private readonly Button btnCancelEvent = new Button { Content = "Cancel", MinWidth = 80, HorizontalContentAlignment = HorizontalAlignment.Center, IsEnabled = false };
        private readonly StackPanel eventForm = new StackPanel { Spacing = 10, IsEnabled = false };
        internal readonly TextBox txtEventName = new TextBox();
        internal readonly ComboBox cmbEventType = stretched(new ComboBox());
        internal readonly CheckBox chkDecimalScoring = new CheckBox { Content = "Decimal Scoring" };
        internal readonly TextBox txtNoOfShots = small();
        internal readonly ComboBox cmbTarget = stretched(new ComboBox());
        internal readonly TextBox txtCaliber = small();
        internal readonly TextBox txtMinutes = small();
        internal readonly ComboBox cmbEventColor = colorCombo();
        internal readonly TextBox txtShotsPerSeries = small(), txtSeriesSeconds = small(), txtShotsInSeries = small(), txtShotsSingles = small(), txtSingleShotSeconds = small();
        internal readonly CheckBox chkRapidFire = new CheckBox { Content = "Rapid Fire" };
        internal readonly TextBox txtRFshots = small(), txtRFtimePerSerie = small(), txtRFtimePerShot = small(), txtRFpauseTime = small(), txtRFloadTime = small();
        internal readonly TextBlock txtEventError = new TextBlock { TextWrapping = TextWrapping.Wrap, MaxWidth = 420 }.withBrush(TextBlock.ForegroundProperty, "ErrorTextBrush");
        private Event editedEvent;      //the event in the form. null for a new one
        private bool loadingEvent = false;

        //active events
        internal readonly ListBox lstAllEvents = new ListBox { Width = 200, Height = 360 };
        internal readonly ListBox lstActiveEvents = new ListBox { Width = 200, Height = 360 };
        internal readonly Button btnToActive = arrow(">", "Make the event active");
        internal readonly Button btnToAll = arrow("<", "Remove the event from the active ones");
        internal readonly Button btnUp = new Button { Content = "Up", MinWidth = 70, HorizontalContentAlignment = HorizontalAlignment.Center };
        internal readonly Button btnDown = new Button { Content = "Down", MinWidth = 70, HorizontalContentAlignment = HorizontalAlignment.Center };
        private bool activeChanged = false;

        internal readonly TabControl tabs = new TabControl();
        internal readonly TextBlock txtError = new TextBlock { TextWrapping = TextWrapping.Wrap, MaxWidth = 520 }.withBrush(TextBlock.ForegroundProperty, "ErrorTextBrush");
        internal readonly Button btnOK = new Button { Content = "OK", MinWidth = 80, HorizontalContentAlignment = HorizontalAlignment.Center, IsDefault = true };

        //---------------------------------------------------------------- construction

        private static T stretched<T>(T control) where T : Control {
            control.HorizontalAlignment = HorizontalAlignment.Stretch;
            return control;
        }

        private static TextBox small() {
            return new TextBox { Width = 90, HorizontalAlignment = HorizontalAlignment.Left, HorizontalContentAlignment = HorizontalAlignment.Right };
        }

        private static Button arrow(string text, string tip) {
            Button button = new Button { Content = text, Width = 44, HorizontalContentAlignment = HorizontalAlignment.Center };
            ToolTip.SetTip(button, tip);
            return button;
        }

        internal static Color toColor(string name) {
            System.Drawing.Color c = System.Drawing.Color.FromName(name);
            return Color.FromArgb(c.A, c.R, c.G, c.B);
        }

        //a list of all colors that have a name, each with a sample of the color
        private static ComboBox colorCombo() {
            ComboBox combo = stretched(new ComboBox { MaxDropDownHeight = 320 });
            combo.ItemsSource = NamedColors.All;
            combo.ItemTemplate = new FuncDataTemplate<string>((name, scope) => new StackPanel {
                Orientation = Orientation.Horizontal,
                Spacing = 8,
                Children = {
                    new Border { Width = 28, Height = 14, Background = new SolidColorBrush(toColor(name ?? "White")), BorderThickness = new Thickness(1), VerticalAlignment = VerticalAlignment.Center }
                        .withBrush(Border.BorderBrushProperty, "LineBrush"),
                    new TextBlock { Text = name, VerticalAlignment = VerticalAlignment.Center }
                }
            });
            return combo;
        }

        //for the designer
        public SettingsWindow() : this(null) {
        }

        public SettingsWindow(MainController controller) {
            this.controller = controller;
            Title = "Application Configuration";
            SizeToContent = SizeToContent.WidthAndHeight;
            CanResize = false;
            WindowStartupLocation = WindowStartupLocation.CenterOwner;

            tabs.Items.Add(new TabItem { Header = "General", Content = generalTab() });
            tabs.Items.Add(new TabItem { Header = "Connection", Content = connectionTab() });
            tabs.Items.Add(new TabItem { Header = "Colors", Content = colorsTab() });
            tabs.Items.Add(new TabItem { Header = "Events", Content = eventsTab() });
            tabs.Items.Add(new TabItem { Header = "Active Events", Content = activeEventsTab() });

            Button cancel = new Button { Content = "Cancel", MinWidth = 80, HorizontalContentAlignment = HorizontalAlignment.Center, IsCancel = true };
            btnOK.Click += (s, e) => {
                if (apply()) {
                    Close(true);
                }
            };
            cancel.Click += (s, e) => Close(false);
            tabs.SelectionChanged += (s, e) => {
                if (e.Source == tabs) {
                    txtError.Text = ""; //the message belonged to the tab that was shown with it
                }
            };

            Content = new StackPanel {
                Margin = new Thickness(16),
                Spacing = 10,
                Children = {
                    tabs,
                    txtError,
                    new StackPanel { Orientation = Orientation.Horizontal, HorizontalAlignment = HorizontalAlignment.Right, Spacing = 8, Children = { btnOK, cancel } }
                }
            };

            if (controller != null) {
                load();
                loadEventLists(null);
            }
        }

        private static Grid form(double fieldWidth) {
            return new Grid { ColumnDefinitions = new ColumnDefinitions("Auto," + fieldWidth.ToString(CultureInfo.InvariantCulture)), Margin = new Thickness(8, 12) };
        }

        private static void addRow(Grid grid, string label, Control control) {
            int row = grid.RowDefinitions.Count;
            grid.RowDefinitions.Add(new RowDefinition(GridLength.Auto));
            control.Margin = new Thickness(0, 3);
            if (label != null) {
                TextBlock text = new TextBlock { Text = label, VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(0, 0, 16, 0) };
                Grid.SetRow(text, row);
                grid.Children.Add(text);
                Grid.SetColumn(control, 1);
            } else {
                Grid.SetColumnSpan(control, 2);
            }
            Grid.SetRow(control, row);
            grid.Children.Add(control);
        }

        private static Control group(string title, Control content) {
            return new StackPanel { Spacing = 4, Children = {
                new TextBlock { Text = title, FontWeight = FontWeight.SemiBold },
                new Border { BorderThickness = new Thickness(1), CornerRadius = new CornerRadius(4), Padding = new Thickness(10, 4), Child = content }.withBrush(Border.BorderBrushProperty, "LineBrush")
            } };
        }

        //---------------------------------------------------------------- tabs

        private Control generalTab() {
            Grid grid = form(280);
            addRow(grid, "Shooter name", txtName);
            addRow(grid, "Default Event", cmbDefaultEvent);
            addRow(grid, "Distance to target (%)", txtDistance);
            addRow(grid, "PDF location", txtPdfFolder);
            foreach (CheckBox box in new[] { chkShowScoring, chkOnlySeries, chkMeanGroup, chkIgnoreMiss, chkVoice, chkScoreVoice, chkLogging, chkConsole }) {
                addRow(grid, null, box);
            }
            return grid;
        }

        private Control connectionTab() {
            cmbProtocol.Items.Add("USB");
            cmbProtocol.Items.Add("TCP");
            cmbBoard.Items.Add(MainController.Arduino);
            cmbBoard.Items.Add(MainController.ESP32);

            Grid usb = form(240);
            usb.Margin = new Thickness(0);
            addRow(usb, "Port", cmbPort);
            addRow(usb, "Baud Rate", txtBaud);

            Grid tcp = form(240);
            tcp.Margin = new Thickness(0);
            addRow(tcp, "Target IP", txtIP);
            addRow(tcp, "Target Port", txtTcpPort);

            Grid top = form(240);
            top.Margin = new Thickness(0);
            addRow(top, "Communication protocol", cmbProtocol);
            addRow(top, "Board Type", cmbBoard);

            Control usbGroup = group("USB", usb);
            Control tcpGroup = group("TCP", tcp);
            cmbProtocol.SelectionChanged += (s, e) => {
                bool isTcp = (string)cmbProtocol.SelectedItem == "TCP";
                usbGroup.IsEnabled = !isTcp;
                tcpGroup.IsEnabled = isTcp;
            };

            return new StackPanel { Margin = new Thickness(8, 12), Spacing = 14, Children = { top, usbGroup, tcpGroup } };
        }

        private Control colorsTab() {
            Grid target = form(220);
            target.Margin = new Thickness(0);
            addRow(target, "Target Color", cmbTargetColor);

            Grid shots = new Grid { ColumnDefinitions = new ColumnDefinitions("Auto,200,200") };
            string[] labels = { "", "Old shot", "Last shot", "Last shot 9", "Last shot 10" };
            ComboBox[][] combos = { null, new[] { cmbOldBack, cmbOldText }, new[] { cmbLastBack, cmbLastText }, new[] { cmb9Back, cmb9Text }, new[] { cmb10Back, cmb10Text } };
            for (int row = 0; row < labels.Length; row++) {
                shots.RowDefinitions.Add(new RowDefinition(GridLength.Auto));
                if (row == 0) {
                    place(shots, new TextBlock { Text = "Back", Margin = new Thickness(0, 0, 0, 2) }, row, 1);
                    place(shots, new TextBlock { Text = "Text", Margin = new Thickness(8, 0, 0, 2) }, row, 2);
                } else {
                    place(shots, new TextBlock { Text = labels[row], VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(0, 0, 16, 0) }, row, 0);
                    combos[row][0].Margin = new Thickness(0, 3);
                    combos[row][1].Margin = new Thickness(8, 3, 0, 3);
                    place(shots, combos[row][0], row, 1);
                    place(shots, combos[row][1], row, 2);
                }
            }

            return new StackPanel { Margin = new Thickness(8, 12), Spacing = 14, Children = { target, group("Shots color", shots) } };
        }

        private static void place(Grid grid, Control control, int row, int column) {
            Grid.SetRow(control, row);
            Grid.SetColumn(control, column);
            grid.Children.Add(control);
        }

        private Control eventsTab() {
            foreach (Event.EventType type in Enum.GetValues(typeof(Event.EventType))) {
                cmbEventType.Items.Add(type);
            }
            cmbTarget.ItemsSource = targets.aTarget.getTargetNames();
            //the targets are stored with the full name of their class. the last part is enough to recognize them
            cmbTarget.ItemTemplate = new FuncDataTemplate<string>((name, scope) => new TextBlock { Text = name == null ? "" : name.Substring(name.LastIndexOf('.') + 1) });

            Grid main = form(230);
            main.Margin = new Thickness(0);
            addRow(main, "Name", txtEventName);
            addRow(main, "Type", cmbEventType);
            addRow(main, null, chkDecimalScoring);
            addRow(main, "Target", cmbTarget);
            addRow(main, "Caliber (mm)", txtCaliber);
            addRow(main, "Color", cmbEventColor);
            addRow(main, "Number Of Shots", txtNoOfShots);
            addRow(main, "Duration (mins)", txtMinutes);

            Grid finals = form(100);
            finals.Margin = new Thickness(0);
            addRow(finals, "Shots per Serie", txtShotsPerSeries);
            addRow(finals, "Series Duration (secs)", txtSeriesSeconds);
            addRow(finals, "Shots in All Series", txtShotsInSeries);
            addRow(finals, "Shots in Single Series", txtShotsSingles);
            addRow(finals, "Single Shot Duration (s)", txtSingleShotSeconds);

            Grid rapid = form(100);
            rapid.Margin = new Thickness(0);
            addRow(rapid, null, chkRapidFire);
            addRow(rapid, "Nr shots", txtRFshots);
            addRow(rapid, "Time per serie", txtRFtimePerSerie);
            addRow(rapid, "Time per shot", txtRFtimePerShot);
            addRow(rapid, "Pause time", txtRFpauseTime);
            addRow(rapid, "Load time", txtRFloadTime);

            eventForm.Children.Add(new StackPanel { Orientation = Orientation.Horizontal, Spacing = 16, Children = {
                group("Event", main),
                new StackPanel { Spacing = 10, Children = { group("Final", finals), group("Rapid Fire", rapid) } }
            } });

            lstEvents.SelectionChanged += (s, e) => {
                if (!loadingEvent && lstEvents.SelectedItem is Event selected) {
                    showEvent(selected);
                }
            };
            btnAddEvent.Click += (s, e) => newEvent();
            btnSaveEvent.Click += (s, e) => saveEvent();
            btnCancelEvent.Click += (s, e) => {
                if (editedEvent != null) {
                    showEvent(editedEvent);
                } else {
                    closeEventForm();
                }
            };
            cmbEventType.SelectionChanged += (s, e) => enableEventFields();
            chkRapidFire.IsCheckedChanged += (s, e) => enableEventFields();

            //a rapid fire event has either a time for the whole series or a time per shot
            txtRFtimePerSerie.PropertyChanged += (s, e) => {
                if (e.Property == TextBox.TextProperty && !loadingEvent && txtRFtimePerSerie.IsEnabled && !isUnused(txtRFtimePerSerie.Text) && txtRFtimePerShot.Text != "-1") {
                    txtRFtimePerShot.Text = "-1";
                }
            };
            txtRFtimePerShot.PropertyChanged += (s, e) => {
                if (e.Property == TextBox.TextProperty && !loadingEvent && txtRFtimePerShot.IsEnabled && !isUnused(txtRFtimePerShot.Text) && txtRFtimePerSerie.Text != "-1") {
                    txtRFtimePerSerie.Text = "-1";
                }
            };

            return new StackPanel { Margin = new Thickness(8, 12), Orientation = Orientation.Horizontal, Spacing = 16, Children = {
                new StackPanel { Spacing = 8, Children = { new TextBlock { Text = "Events", FontWeight = FontWeight.SemiBold }, lstEvents, btnAddEvent } },
                new StackPanel { Spacing = 10, Children = {
                    eventForm,
                    txtEventError,
                    new StackPanel { Orientation = Orientation.Horizontal, Spacing = 8, Children = { btnSaveEvent, btnCancelEvent } }
                } }
            } };
        }

        private static bool isUnused(string text) {
            return string.IsNullOrEmpty(text) || text == "-1" || text == "-";
        }

        private Control activeEventsTab() {
            btnToActive.Click += (s, e) => move(lstAllEvents, lstActiveEvents);
            btnToAll.Click += (s, e) => move(lstActiveEvents, lstAllEvents);
            btnUp.Click += (s, e) => shift(-1);
            btnDown.Click += (s, e) => shift(1);
            lstAllEvents.SelectionChanged += (s, e) => enableActiveButtons();
            lstActiveEvents.SelectionChanged += (s, e) => enableActiveButtons();

            return new StackPanel { Margin = new Thickness(8, 12), Spacing = 10, Children = {
                new StackPanel { Orientation = Orientation.Horizontal, Spacing = 12, Children = {
                    new StackPanel { Spacing = 4, Children = { new TextBlock { Text = "All Events", FontWeight = FontWeight.SemiBold }, lstAllEvents } },
                    new StackPanel { Spacing = 8, VerticalAlignment = VerticalAlignment.Center, Children = { btnToActive, btnToAll } },
                    new StackPanel { Spacing = 4, Children = { new TextBlock { Text = "Active Events", FontWeight = FontWeight.SemiBold }, lstActiveEvents } },
                    new StackPanel { Spacing = 8, VerticalAlignment = VerticalAlignment.Center, Children = { btnUp, btnDown } }
                } },
                new TextBlock { Text = "The active events are the buttons in the main window, in this order." }.withBrush(TextBlock.ForegroundProperty, "MutedTextBrush")
            } };
        }

        //---------------------------------------------------------------- general, connection and colors

        private void load() {
            Settings s = Settings.Default;

            txtName.Text = s.name;
            txtDistance.Text = s.targetDistance.ToString(CultureInfo.InvariantCulture);
            txtPdfFolder.Text = PDFGenerator.getFolder();
            chkShowScoring.IsChecked = s.showScoring;
            chkOnlySeries.IsChecked = s.OnlySeries;
            chkMeanGroup.IsChecked = s.drawMeanGroup;
            chkIgnoreMiss.IsChecked = s.ignoreMiss;
            chkVoice.IsChecked = s.voiceCommands;
            chkScoreVoice.IsChecked = s.scoreVoice;
            chkLogging.IsChecked = s.fileLogging;
            chkConsole.IsChecked = s.displayDebugConsole;

            List<string> ports = new List<string>();
            try {
                ports.AddRange(SerialPort.GetPortNames());
            } catch (Exception ex) {
                Console.WriteLine("Could not list the serial ports: " + ex.Message);
            }
            if (!string.IsNullOrEmpty(s.portName) && !ports.Contains(s.portName)) {
                ports.Add(s.portName); //the configured port stays selectable while the target is unplugged
            }
            ports.Sort(StringComparer.Ordinal);
            foreach (string port in ports) {
                cmbPort.Items.Add(port);
            }
            cmbPort.SelectedItem = string.IsNullOrEmpty(s.portName) ? null : s.portName;
            txtBaud.Text = s.baudRate.ToString(CultureInfo.InvariantCulture);
            txtIP.Text = s.TcpIP;
            txtTcpPort.Text = s.TcpPort.ToString(CultureInfo.InvariantCulture);
            cmbBoard.SelectedItem = s.Board == MainController.ESP32 ? MainController.ESP32 : MainController.Arduino;
            cmbProtocol.SelectedItem = s.CommProtocol == "TCP" ? "TCP" : "USB";

            cmbTargetColor.SelectedItem = s.targetColor.Name;
            cmbOldBack.SelectedItem = s.scoreOldBackgroundColor.Name;
            cmbOldText.SelectedItem = s.scoreOldPenColor.Name;
            cmbLastBack.SelectedItem = s.scoreDefaultBackgroundColor.Name;
            cmbLastText.SelectedItem = s.scoreDefaultPenColor.Name;
            cmb9Back.SelectedItem = s.score9BackgroundColor.Name;
            cmb9Text.SelectedItem = s.score9PenColor.Name;
            cmb10Back.SelectedItem = s.score10BackgroundColor.Name;
            cmb10Text.SelectedItem = s.score10PenColor.Name;
        }

        private static System.Drawing.Color color(ComboBox combo, System.Drawing.Color current) {
            string name = combo.SelectedItem as string;
            return name == null ? current : System.Drawing.Color.FromName(name);
        }

        //checks the input and writes it to the settings. returns false if something is wrong
        private bool apply() {
            int baud;
            int tcpPort;
            decimal distance;

            if (string.IsNullOrWhiteSpace(txtName.Text)) {
                return fail(0, "Shooter name empty. Please fill in your name.");
            }
            if (!decimal.TryParse((txtDistance.Text ?? "").Replace(',', '.'), NumberStyles.Number, CultureInfo.InvariantCulture, out distance) || distance <= 0 || distance > 100) {
                return fail(0, "The target distance is a percentage of the full distance, between 1 and 100.");
            }
            if (!int.TryParse(txtBaud.Text, NumberStyles.Integer, CultureInfo.InvariantCulture, out baud) || baud <= 0) {
                return fail(1, "The baud rate must be a number, for example 115200.");
            }
            if (!int.TryParse(txtTcpPort.Text, NumberStyles.Integer, CultureInfo.InvariantCulture, out tcpPort) || tcpPort <= 0 || tcpPort > 65535) {
                return fail(1, "The TCP port must be a number between 1 and 65535.");
            }
            if (lstActiveEvents.ItemCount == 0) {
                return fail(4, "At least one event has to be active.");
            }

            Settings s = Settings.Default;
            s.name = txtName.Text.Trim();
            s.targetDistance = distance;
            s.pdfPath = (txtPdfFolder.Text ?? "").Trim();
            s.showScoring = chkShowScoring.IsChecked == true;
            s.OnlySeries = chkOnlySeries.IsChecked == true;
            s.drawMeanGroup = chkMeanGroup.IsChecked == true;
            s.ignoreMiss = chkIgnoreMiss.IsChecked == true;
            s.voiceCommands = chkVoice.IsChecked == true;
            s.scoreVoice = chkScoreVoice.IsChecked == true;
            s.fileLogging = chkLogging.IsChecked == true;
            s.displayDebugConsole = chkConsole.IsChecked == true;

            s.CommProtocol = (string)cmbProtocol.SelectedItem ?? "USB";
            s.portName = (string)cmbPort.SelectedItem ?? "";
            s.baudRate = baud;
            s.TcpIP = (txtIP.Text ?? "").Trim();
            s.TcpPort = tcpPort;
            s.Board = (string)cmbBoard.SelectedItem ?? MainController.Arduino;

            s.targetColor = color(cmbTargetColor, s.targetColor);
            s.scoreOldBackgroundColor = color(cmbOldBack, s.scoreOldBackgroundColor);
            s.scoreOldPenColor = color(cmbOldText, s.scoreOldPenColor);
            s.scoreDefaultBackgroundColor = color(cmbLastBack, s.scoreDefaultBackgroundColor);
            s.scoreDefaultPenColor = color(cmbLastText, s.scoreDefaultPenColor);
            s.score9BackgroundColor = color(cmb9Back, s.score9BackgroundColor);
            s.score9PenColor = color(cmb9Text, s.score9PenColor);
            s.score10BackgroundColor = color(cmb10Back, s.score10BackgroundColor);
            s.score10PenColor = color(cmb10Text, s.score10PenColor);

            //the event at startup has to be one of the active ones
            List<Event> active = lstActiveEvents.Items.Cast<Event>().ToList();
            string defaultEvent = cmbDefaultEvent.SelectedItem as string;
            if (defaultEvent == null || !active.Any(ev => ev.Name == defaultEvent)) {
                defaultEvent = active[0].Name;
            }
            s.defaultTarget = defaultEvent;

            if (activeChanged) {
                controller.setActiveEvents(active);
            }
            return true;
        }

        private bool fail(int tab, string message) {
            tabs.SelectedIndex = tab;
            txtError.Text = message;
            return false;
        }

        //---------------------------------------------------------------- events

        //fills the three lists of events from the controller, keeping the order of the active list on the screen
        private void loadEventLists(string selectName) {
            List<long> activeIds = activeChanged
                ? lstActiveEvents.Items.Cast<Event>().Select(ev => ev.ID).ToList()
                : controller.eventManager.getActiveEventsList().Select(ev => ev.ID).ToList();

            loadingEvent = true;
            lstEvents.Items.Clear();
            lstAllEvents.Items.Clear();
            lstActiveEvents.Items.Clear();
            List<Event> all = controller.eventManager.getEventsList();
            foreach (Event ev in all) {
                lstEvents.Items.Add(ev);
                if (!activeIds.Contains(ev.ID)) {
                    lstAllEvents.Items.Add(ev);
                }
            }
            foreach (long id in activeIds) {
                Event ev = all.FirstOrDefault(x => x.ID == id);
                if (ev != null) {
                    lstActiveEvents.Items.Add(ev);
                }
            }
            loadingEvent = false;

            loadDefaultEvents();
            enableActiveButtons();

            if (selectName != null) {
                Event saved = all.FirstOrDefault(ev => ev.Name == selectName);
                if (saved != null) {
                    lstEvents.SelectedItem = saved;
                }
            }
        }

        private void loadDefaultEvents() {
            string selected = cmbDefaultEvent.SelectedItem as string ?? Settings.Default.defaultTarget;
            cmbDefaultEvent.Items.Clear();
            foreach (Event ev in lstActiveEvents.Items.Cast<Event>()) {
                cmbDefaultEvent.Items.Add(ev.Name);
            }
            cmbDefaultEvent.SelectedItem = cmbDefaultEvent.Items.Contains(selected) ? selected : null;
        }

        private void showEvent(Event ev) {
            editedEvent = ev;
            fill(EventDraft.from(ev));
            openEventForm();
        }

        private void newEvent() {
            loadingEvent = true;
            lstEvents.SelectedItem = null;
            loadingEvent = false;
            editedEvent = null;
            fill(new EventDraft { caliber = "4.5", color = "Gold", type = Event.EventType.Practice, target = typeof(targets.AirPistol).FullName });
            openEventForm();
            txtEventName.Focus();
        }

        private void openEventForm() {
            eventForm.IsEnabled = true;
            btnSaveEvent.IsEnabled = true;
            btnCancelEvent.IsEnabled = true;
            txtEventError.Text = "";
            enableEventFields();
        }

        private void closeEventForm() {
            editedEvent = null;
            eventForm.IsEnabled = false;
            btnSaveEvent.IsEnabled = false;
            btnCancelEvent.IsEnabled = false;
            txtEventError.Text = "";
        }

        private void fill(EventDraft d) {
            loadingEvent = true;
            txtEventName.Text = d.name;
            cmbEventType.SelectedItem = d.type;
            chkDecimalScoring.IsChecked = d.decimalScoring;
            txtNoOfShots.Text = d.numberOfShots;
            cmbTarget.SelectedItem = d.target;
            txtCaliber.Text = d.caliber;
            txtMinutes.Text = d.minutes;
            cmbEventColor.SelectedItem = d.color;
            txtShotsPerSeries.Text = d.shotsPerSeries;
            txtSeriesSeconds.Text = d.seriesSeconds;
            txtShotsInSeries.Text = d.shotsInSeries;
            txtSingleShotSeconds.Text = d.singleShotSeconds;
            txtShotsSingles.Text = d.shotsInSingleSeries;
            chkRapidFire.IsChecked = d.rapidFire;
            txtRFshots.Text = d.rfNumberOfShots;
            txtRFtimePerSerie.Text = d.rfTimePerSerie;
            txtRFtimePerShot.Text = d.rfTimePerShot;
            txtRFpauseTime.Text = d.rfPauseTime;
            txtRFloadTime.Text = d.rfLoadTime;
            loadingEvent = false;
        }

        private static string text(TextBox box) {
            return (box.Text ?? "").Trim();
        }

        private EventDraft readEvent() {
            return new EventDraft {
                name = text(txtEventName),
                type = cmbEventType.SelectedItem as Event.EventType?,
                decimalScoring = chkDecimalScoring.IsChecked == true,
                numberOfShots = text(txtNoOfShots),
                target = cmbTarget.SelectedItem as string,
                caliber = text(txtCaliber),
                minutes = text(txtMinutes),
                color = cmbEventColor.SelectedItem as string,
                shotsPerSeries = text(txtShotsPerSeries),
                seriesSeconds = text(txtSeriesSeconds),
                shotsInSeries = text(txtShotsInSeries),
                singleShotSeconds = text(txtSingleShotSeconds),
                shotsInSingleSeries = text(txtShotsSingles),
                rapidFire = chkRapidFire.IsChecked == true,
                rfNumberOfShots = text(txtRFshots),
                rfTimePerSerie = text(txtRFtimePerSerie),
                rfTimePerShot = text(txtRFtimePerShot),
                rfPauseTime = text(txtRFpauseTime),
                rfLoadTime = text(txtRFloadTime)
            };
        }

        //only the fields that the type of the event uses can be edited
        private void enableEventFields() {
            Event.EventType? type = cmbEventType.SelectedItem as Event.EventType?;
            bool shots = EventDraft.usesShotsAndMinutes(type);
            bool finals = EventDraft.usesFinalFields(type);
            bool rapid = chkRapidFire.IsChecked == true;

            txtNoOfShots.IsEnabled = shots;
            txtMinutes.IsEnabled = shots;
            foreach (TextBox box in new[] { txtShotsPerSeries, txtSeriesSeconds, txtShotsInSeries, txtShotsSingles, txtSingleShotSeconds }) {
                box.IsEnabled = finals;
            }
            foreach (TextBox box in new[] { txtRFshots, txtRFtimePerSerie, txtRFtimePerShot, txtRFpauseTime, txtRFloadTime }) {
                box.IsEnabled = rapid;
            }
        }

        private void saveEvent() {
            EventDraft draft = readEvent();
            string errors = controller.saveEvent(draft, editedEvent);
            if (errors.Length > 0) {
                txtEventError.Text = "There are some validation errors in the Event. Please fix and save again:" + Environment.NewLine + errors;
                return;
            }

            //the events were read from the database again: show the new objects
            closeEventForm();
            loadEventLists(draft.name.Trim());
        }

        //---------------------------------------------------------------- active events

        private void enableActiveButtons() {
            btnToActive.IsEnabled = lstAllEvents.SelectedItem != null;
            btnToAll.IsEnabled = lstActiveEvents.SelectedItem != null;
            btnUp.IsEnabled = lstActiveEvents.SelectedIndex > 0;
            btnDown.IsEnabled = lstActiveEvents.SelectedIndex >= 0 && lstActiveEvents.SelectedIndex < lstActiveEvents.ItemCount - 1;
        }

        private void move(ListBox from, ListBox to) {
            object ev = from.SelectedItem;
            if (ev == null) {
                return;
            }
            from.Items.Remove(ev);
            to.Items.Add(ev);
            to.SelectedItem = ev;
            activeChanged = true;
            loadDefaultEvents();
            enableActiveButtons();
        }

        private void shift(int by) {
            int index = lstActiveEvents.SelectedIndex;
            int target = index + by;
            if (index < 0 || target < 0 || target >= lstActiveEvents.ItemCount) {
                return;
            }
            object ev = lstActiveEvents.Items[index];
            lstActiveEvents.Items.RemoveAt(index);
            lstActiveEvents.Items.Insert(target, ev);
            lstActiveEvents.SelectedIndex = target;
            activeChanged = true;
            loadDefaultEvents();
            enableActiveButtons();
        }
    }
}
