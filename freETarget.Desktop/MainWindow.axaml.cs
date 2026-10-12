using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Media;
using Avalonia.Threading;
using freETarget.Properties;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;

namespace freETarget.Desktop {

    public partial class MainWindow : Window {

        private readonly MainController controller;
        private readonly DispatcherTimer timer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(500) };
        private readonly DispatcherTimer hintTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(2) };
        private readonly Dictionary<long, Button> eventButtons = new Dictionary<long, Button>();
        private bool updating = false; //true while the window itself changes selections
        private CalibrationWindow calibrationWindow;
        internal JournalWindow journalWindow;
        internal TerminalWindow terminalWindow;

        private static readonly string[] arrows = { "→", "↗", "↑", "↖", "←", "↙", "↓", "↘" };

        //for the designer
        public MainWindow() {
            InitializeComponent();
        }

        public MainWindow(MainController controller, string versionText) : this() {
            this.controller = controller;
            txtVersion.Text = versionText;
            targetView.attach(controller);

            controller.StatusChanged += updateStatus;
            controller.SessionStarted += onSessionStarted;
            controller.ShotsCleared += onShotsCleared;
            controller.ShotAdded += onShotAdded;
            controller.ShotListChanged += onShotListChanged;
            controller.MessageDisplayed += onMessage;
            controller.ShotsRecalculated += showShots;
            controller.SessionResumed += onSessionResumed;
            controller.SessionLoaded += onSessionLoaded;
            controller.CalibrationChanged += showCalibration;
            controller.EventsChanged += loadEventButtons;

            btnConnect.Click += async (s, e) => await controller.toggleConnection();
            mnuSettings.Click += btnSettings_Click;
            btnExport.Click += (s, e) => exportPdf();
            mnuCalibration.Click += (s, e) => openCalibration();
            mnuResume.Click += btnResume_Click;
            mnuJournal.Click += (s, e) => openJournal();
            mnuTerminal.Click += (s, e) => openTerminal();
            mnuThemeSystem.Click += (s, e) => setTheme(ThemeSetting.System);
            mnuThemeLight.Click += (s, e) => setTheme(ThemeSetting.Light);
            mnuThemeDark.Click += (s, e) => setTheme(ThemeSetting.Dark);
            mnuTargetSettings.Click += async (s, e) => await new TargetSettingsWindow(controller).ShowDialog(this);
            btnStart.Click += (s, e) => controller.startRapidFire();
            btnAllShots.Click += (s, e) => controller.selectSeries(-1);
            trkZoom.PropertyChanged += (s, e) => {
                if (e.Property == Slider.ValueProperty && !updating) {
                    targetView.Zoom = (int)trkZoom.Value;
                }
            };
            seriesList.SelectionChanged += (s, e) => {
                if (!updating && seriesList.SelectedIndex >= 0) {
                    //not from inside the selection event: showing the series fills the list again
                    int index = seriesList.SelectedIndex;
                    Dispatcher.UIThread.Post(() => controller.selectSeries(index));
                }
            };
            targetView.PointerWheelChanged += (s, e) => {
                trkZoom.Value = Math.Clamp(trkZoom.Value + (e.Delta.Y > 0 ? -1 : 1), trkZoom.Minimum, trkZoom.Maximum);
            };

            timer.Tick += (s, e) => updateClock();
            hintTimer.Tick += (s, e) => {
                hintTimer.Stop();
                hintBox.IsVisible = false;
            };

            Opened += (s, e) => {
                txtOutput.IsVisible = Settings.Default.displayDebugConsole;
                if (Settings.Default.fileLogging) {
                    onMessage("Log location: " + controller.logger.logFile, false);
                }
                controller.startDefaultSession();
                updateStatus();
                timer.Start();
            };
            Closing += (s, e) => {
                if (!controller.shutdown()) {
                    e.Cancel = true; //the user wants to keep the session
                    return;
                }
                timer.Stop();
                controller.Dispose();
            };

            loadEventButtons();
            showCalibration();
            showTheme();
        }

        //---------------------------------------------------------------- events of the range (pistol practice, rifle match ...)

        private void loadEventButtons() {
            eventTabs.Children.Clear();
            eventButtons.Clear();
            foreach (Event ev in controller.eventManager.getActiveEventsList()) {
                Event selected = ev;
                System.Drawing.Color c = ev.TabColor;
                Button button = new Button {
                    Content = ev.Name,
                    Background = new SolidColorBrush(Color.FromArgb(c.A, c.R, c.G, c.B)),
                    Foreground = Brushes.Black, //the colors of the events are bright ones
                    Padding = new Thickness(12, 5)
                };
                button.withBrush(Button.BorderBrushProperty, "SelectedEventBorderBrush");
                button.Click += (s, e) => {
                    if (controller.selectedEvent != selected) {
                        controller.selectEvent(selected);
                    }
                };
                eventButtons[ev.ID] = button;
                eventTabs.Children.Add(button);
            }
            markSelectedEvent();
        }

        private void markSelectedEvent() {
            foreach (KeyValuePair<long, Button> entry in eventButtons) {
                bool selected = controller.selectedEvent != null && controller.selectedEvent.ID == entry.Key;
                entry.Value.BorderThickness = new Thickness(selected ? 3 : 0);
                entry.Value.FontWeight = selected ? FontWeight.Bold : FontWeight.Normal;
            }
        }

        //---------------------------------------------------------------- what the controller reports

        private void updateStatus() {
            MainController.Status status = controller.currentStatus;

            bool idle = status == MainController.Status.NOT_CONNECTED || status == MainController.Status.LOADED;
            btnConnect.Content = idle ? "Connect" : (status == MainController.Status.CONECTING ? "Cancel" : "Disconnect");
            statusText.Text = controller.statusText == "" ? "Not connected" : controller.statusText;

            IBrush light = Brushes.Gray;
            if (controller.connectionLost) {
                light = Brushes.Red;
            } else if (status == MainController.Status.CONNECTED) {
                light = Brushes.LimeGreen;
            } else if (status == MainController.Status.CONECTING) {
                light = Brushes.Orange;
            }
            connectionStatus.Fill = light;

            bool connected = status == MainController.Status.CONNECTED;
            btnConnect.IsEnabled = journalWindow == null; //no target while saved sessions are shown
            eventTabs.IsEnabled = status != MainController.Status.CONECTING;
            mnuSettings.IsEnabled = idle;
            btnStart.IsEnabled = connected && !controller.rapidFireRunning;
            mnuCalibration.IsEnabled = connected;
            mnuResume.IsEnabled = controller.canResume();
            mnuTargetSettings.IsEnabled = connected;
            mnuTerminal.IsEnabled = connected;
            if (!connected && calibrationWindow != null) {
                calibrationWindow.Close();
            }
            if (!connected && terminalWindow != null) {
                terminalWindow.Close();
            }

            updateClock();

            targetView.refresh(); //a target that is not connected is gray
        }

        private void onSessionStarted() {
            Session session = controller.currentSession;
            targets.aTarget target = session.getTarget();

            updating = true;
            trkZoom.Minimum = target.getTrkZoomMinimum();
            trkZoom.Maximum = target.getTrkZoomMaximum();
            trkZoom.Value = target.getTrkZoomValue();
            updating = false;
            targetView.Zoom = (int)trkZoom.Value;

            txtSessionName.Text = session.ToString();
            btnStart.IsVisible = session.eventType.RapidFire;
            markSelectedEvent();
            onShotsCleared();
            updateStatus();
        }

        private void onShotsCleared() {
            shotsList.Items.Clear();
            txtLastShot.Text = "";
            txtLastArrow.Text = "";
            txtTotal.Text = "";
            txtClock.Text = "";
            txtClock.Foreground = Brushes.White;
            updateSeries();
            updateStatistics();
            targetView.refresh();
        }

        private void onShotAdded(Shot shot) {
            mnuResume.IsEnabled = false; //the session has shots of its own now
            addShotLine(shot);
            showTotals(shot);
            updateSeries();
            updateStatistics();
            targetView.refresh();
        }

        //fills everything that shows shots again, after their scores changed
        private void showShots() {
            shotsList.Items.Clear();
            Shot last = null;
            foreach (Shot shot in controller.getShots()) {
                addShotLine(shot);
                last = shot;
            }
            if (last != null) {
                showTotals(last);
            }
            updateSeries();
            updateStatistics();
            targetView.refresh();
        }

        private void showTotals(Shot lastShot) {
            Session session = controller.currentSession;
            if (Settings.Default.showScoring) {
                txtTotal.Text = controller.getShots().Count + ": " + session.score.ToString() + " (" + session.decimalScore.ToString(CultureInfo.InvariantCulture) + ") -" + session.innerX.ToString() + "x";
                txtLastShot.Text = lastShot.decimalScore.ToString(CultureInfo.InvariantCulture) + (lastShot.innerTen ? " *" : "");
            } else {
                txtTotal.Text = "XXX";
                txtLastShot.Text = "XX";
            }
            txtLastArrow.Text = lastShot.miss ? " " : getArrow(lastShot);
        }

        private void addShotLine(Shot shot) {
            string inner = shot.innerTen ? " *" : "";
            string arrow = shot.miss ? " " : getArrow(shot);
            string number = (shot.index + 1).ToString().PadLeft(3);
            string line;
            if (shot.miss) {
                line = number + "  " + arrow + "  miss";
            } else if (Settings.Default.showScoring) {
                line = number + "  " + arrow + "  " + shot.score.ToString().PadLeft(2) + "  " + shot.decimalScore.ToString("F1", CultureInfo.InvariantCulture).PadLeft(4) + inner;
            } else {
                line = number + "  " + arrow + "  XX  XX.X";
            }
            shotsList.Items.Add(line);
            shotsList.ScrollIntoView(shotsList.Items.Count - 1);
        }

        //arrow that points from the center of the target to the shot
        private static string getArrow(Shot shot) {
            int index = (int)Math.Round((double)shot.angle / 45.0, MidpointRounding.AwayFromZero);
            return arrows[((index % 8) + 8) % 8];
        }

        private void onShotListChanged() {
            updateSeries();
            updateStatistics();
            targetView.refresh();
        }

        private void updateStatistics() {
            if (controller.hasStatistics()) {
                Session session = controller.currentSession;
                txtMeanRadius.Text = Math.Round(session.rbar, 1).ToString(CultureInfo.InvariantCulture) + " mm";
                txtWindage.Text = Math.Round(Math.Abs(session.xbar), 1).ToString(CultureInfo.InvariantCulture) + " mm " + (session.xbar < 0 ? "←" : "→");
                txtElevation.Text = Math.Round(Math.Abs(session.ybar), 1).ToString(CultureInfo.InvariantCulture) + " mm " + (session.ybar < 0 ? "↓" : "↑");
                txtMaxSpread.Text = session.groupSize.ToString(CultureInfo.InvariantCulture) + " mm";
            } else {
                txtMeanRadius.Text = "";
                txtWindage.Text = "";
                txtElevation.Text = "";
                txtMaxSpread.Text = "";
            }
        }

        //one line per series with the scores of its shots and their sum
        private void updateSeries() {
            Session session = controller.currentSession;
            bool showScoring = Settings.Default.showScoring;
            int selected = -1;

            updating = true;
            seriesList.Items.Clear();
            if (session != null) {
                for (int i = 0; i < session.AllSeries.Count; i++) {
                    List<Shot> series = session.AllSeries[i];
                    if (series.Count == 0) {
                        continue;
                    }
                    StringBuilder line = new StringBuilder((i + 1).ToString().PadLeft(2) + ": ");
                    decimal sum = 0;
                    foreach (Shot shot in series) {
                        if (session.decimalScoring) {
                            line.Append(showScoring ? shot.decimalScore.ToString("F1", CultureInfo.InvariantCulture).PadLeft(5) : "   XX");
                            sum += shot.decimalScore;
                        } else {
                            line.Append(showScoring ? shot.score.ToString().PadLeft(3) : " XX");
                            sum += shot.score;
                        }
                    }
                    line.Append(showScoring ? "  = " + sum.ToString(CultureInfo.InvariantCulture) : "");
                    seriesList.Items.Add(line.ToString());

                    if (ReferenceEquals(series, session.CurrentSeries)) {
                        selected = seriesList.Items.Count - 1;
                    }
                }
            }

            bool oneSeriesShown = session != null && !ReferenceEquals(controller.getShotList(), controller.getShots()) && !Settings.Default.OnlySeries;
            seriesList.SelectedIndex = oneSeriesShown ? selected : -1;
            btnAllShots.IsEnabled = oneSeriesShown;
            updating = false;
        }

        private void updateClock() {
            txtTime.Text = DateTime.Now.ToString("yyyy-MM-dd  HH:mm:ss");
            if (controller.currentSession == null || controller.currentStatus != MainController.Status.CONNECTED) {
                return; //the session clock runs while connected
            }

            System.Drawing.Color c;
            string time = controller.currentSession.getTime(out c);

            if (time.StartsWith("@", StringComparison.Ordinal)) {
                txtClock.Text = time.Substring(1); //a command of the range officer instead of a time
            } else if (time.Length == 6) {
                txtClock.Text = time.Substring(0, 2) + ":" + time.Substring(2, 2) + ":" + time.Substring(4, 2);
            } else {
                txtClock.Text = time;
            }
            txtClock.Foreground = new SolidColorBrush(Color.FromArgb(c.A, c.R, c.G, c.B));
        }

        private void onMessage(string text, bool showHint) {
            const int maxLength = 100000;
            string all = (txtOutput.Text ?? "") + text + Environment.NewLine;
            if (all.Length > maxLength) {
                all = all.Substring(all.Length - maxLength / 2);
            }
            txtOutput.Text = all;
            txtOutput.CaretIndex = all.Length;

            if (showHint) {
                txtHint.Text = text;
                hintBox.IsVisible = true;
                hintTimer.Stop();
                hintTimer.Start();
            }
        }

        //---------------------------------------------------------------- appearance

        internal void setTheme(string theme) {
            Settings.Default.theme = theme;
            controller.saveSettings();
            ThemeSetting.apply();
            showTheme();
        }

        private void showTheme() {
            string theme = Settings.Default.theme;
            mnuThemeLight.IsChecked = theme == ThemeSetting.Light;
            mnuThemeDark.IsChecked = theme == ThemeSetting.Dark;
            mnuThemeSystem.IsChecked = !mnuThemeLight.IsChecked && !mnuThemeDark.IsChecked;
        }

        //---------------------------------------------------------------- terminal

        internal void openTerminal() {
            if (terminalWindow != null) {
                terminalWindow.Activate();
                return;
            }
            //not modal: it is watched while shooting
            terminalWindow = new TerminalWindow(controller);
            terminalWindow.Closed += (s, e) => terminalWindow = null;
            terminalWindow.Show(this);
        }

        //---------------------------------------------------------------- journal

        internal void openJournal() {
            if (journalWindow != null) {
                journalWindow.Activate();
                return;
            }
            if (!controller.prepareForJournal()) {
                return; //the user keeps the running session
            }

            journalWindow = new JournalWindow(controller);
            journalWindow.Closed += (s, e) => {
                //a session that was loaded stays in the main window. connecting or choosing an event puts it away
                journalWindow = null;
                updateStatus();
            };
            journalWindow.Show(this);
            updateStatus();
        }

        //a saved session is shown for viewing
        private void onSessionLoaded() {
            Session session = controller.currentSession;
            targets.aTarget target = session.getTarget();

            updating = true;
            trkZoom.Minimum = target.getTrkZoomMinimum();
            trkZoom.Maximum = target.getTrkZoomMaximum();
            trkZoom.Value = target.getTrkZoomValue();
            updating = false;
            targetView.Zoom = (int)trkZoom.Value;

            txtSessionName.Text = session.ToString() + "  (" + session.startTime.ToString("yyyy-MM-dd") + ")";
            txtClock.Text = "";
            btnStart.IsVisible = false;
            markSelectedEvent();
            showShots();
            updateStatus();
        }

        //---------------------------------------------------------------- resume

        private async void btnResume_Click(object sender, RoutedEventArgs e) {
            ResumeWindow resume = new ResumeWindow(controller);
            ListBoxSessionItem selected = await resume.ShowDialog<ListBoxSessionItem>(this);
            if (selected != null) {
                controller.resumeSession(selected.id);
            }
        }

        private void onSessionResumed() {
            txtSessionName.Text = controller.currentSession.ToString();
            btnStart.IsVisible = controller.currentSession.eventType.RapidFire;
            markSelectedEvent();
            showShots();
            updateStatus();
        }

        //---------------------------------------------------------------- calibration

        private void openCalibration() {
            if (calibrationWindow != null) {
                calibrationWindow.Activate();
                return;
            }
            //not modal: the target behind it shows what the calibration does
            calibrationWindow = new CalibrationWindow(controller);
            calibrationWindow.Closed += (s, e) => calibrationWindow = null;
            calibrationWindow.Show(this);
        }

        //the menu shows that the shots are not displayed where the target measured them
        private void showCalibration() {
            string mark = controller.isCalibrated() ? " •" : "";
            mnuCalibration.Header = "Calibration" + mark;
            btnMenu.Content = "Menu" + mark;
            string values = "Calibration - X: " + controller.calibrationX.ToString(CultureInfo.InvariantCulture) + " Y: " + controller.calibrationY.ToString(CultureInfo.InvariantCulture)
                + " Angle: " + controller.calibrationAngle.ToString(CultureInfo.InvariantCulture);
            ToolTip.SetTip(mnuCalibration, values);
            ToolTip.SetTip(btnMenu, controller.isCalibrated() ? values : null);
        }

        //---------------------------------------------------------------- report

        private void exportPdf() {
            string file = controller.exportPdf();
            if (file == null) {
                return;
            }
            try {
                PDFGenerator.open(file); //in the PDF viewer of the system. printing is done from there
            } catch (Exception ex) {
                controller.showWarning("The report was saved as " + file + " but could not be opened: " + ex.Message, "PDF export");
            }
        }

        //---------------------------------------------------------------- settings

        private async void btnSettings_Click(object sender, RoutedEventArgs e) {
            SettingsWindow settings = new SettingsWindow(controller);
            bool saved = await settings.ShowDialog<bool>(this);
            //events are saved by the window itself, whether it is closed with OK or not
            if (saved) {
                controller.saveSettings();
                txtOutput.IsVisible = Settings.Default.displayDebugConsole;
                onMessage("Settings saved", false);
            }
            showShots(); //colors and scoring display may have changed
        }
    }
}
