using Avalonia;
using Avalonia.Controls;
using Avalonia.Headless;
using Avalonia.Headless.XUnit;
using Avalonia.Media.Imaging;
using Avalonia.Threading;
using freETarget.Properties;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using Xunit;

[assembly: AvaloniaTestApplication(typeof(freETarget.Desktop.Tests.TestAppBuilder))]
[assembly: CollectionBehavior(DisableTestParallelization = true)]

namespace freETarget.Desktop.Tests {

    public class TestAppBuilder {
        //the real application with its styles and colors, drawn by Skia into memory instead of a window.
        //without a desktop lifetime it opens no window by itself, the tests create the windows
        public static AppBuilder BuildAvaloniaApp() => AppBuilder.Configure<App>()
            .UseSkia()
            .WithInterFont()
            .UseHeadless(new AvaloniaHeadlessPlatformOptions { UseHeadlessDrawing = false });
    }

    class FakeDialogs : IUserDialogs {
        public List<string> shown = new List<string>();
        public bool? yesNoCancelAnswer = false;
        public void showError(string text, string title) { shown.Add(text); }
        public void showWarning(string text, string title) { shown.Add(text); }
        public bool confirm(string text, string title) { shown.Add(text); return true; }
        public bool askYesNo(string text, string title) { shown.Add(text); return false; }
        public bool? askYesNoCancel(string text, string title) { shown.Add(text); return yesNoCancelAnswer; }
    }

    class FakeComm : comms.aCommModule {
        public override event CommEventHandler CommDataReceivedEvent;
#pragma warning disable CS0067
        public override event CommEventHandler CommDisconnectedEvent;
#pragma warning restore CS0067
        public List<string> sent = new List<string>();
        public override void open(comms.OpenParams value) { }
        public override void close() { }
        public override void sendData(string text) { sent.Add(text); }
        public override string getCommInfo() { return "USB = COM3"; }
        public void receive(string text) { CommDataReceivedEvent?.Invoke(this, new comms.CommEventArgs(text)); }
    }

    public class MainWindowTests : IDisposable {

        private readonly string dir = Path.Combine(Path.GetTempPath(), "freETarget.Desktop.Tests." + Guid.NewGuid().ToString("N"));
        private readonly FakeDialogs dialogs = new FakeDialogs();
        private FakeComm comm;
        private MainController controller;

        public void Dispose() {
            controller?.Dispose();
            foreach (var p in typeof(Settings).GetProperties().Where(p => p.CanWrite && !p.GetMethod.IsStatic)) {
                p.SetValue(Settings.Default, p.GetValue(new Settings()));
            }
            try {
                Directory.Delete(dir, true);
            } catch (IOException) {
            }
        }

        private MainWindow open() {
            Application.Current.RequestedThemeVariant = Avalonia.Styling.ThemeVariant.Light;
            controller = new MainController(dialogs, App.runOnUIThread);
            controller.connectDelay = 0;
            controller.versionDelay = 0;
            controller.commFactory = host => comm = new FakeComm();
            Assert.Null(controller.init(4, "v4.12.2", Path.Combine(dir, "database"), Path.Combine(dir, "log")));
            Settings.Default.name = "Marvin";
            Settings.Default.portName = "COM3";
            Settings.Default.displayDebugConsole = true;

            MainWindow window = new MainWindow(controller, "v4.12.2");
            window.Show();
            Dispatcher.UIThread.RunJobs();
            return window;
        }

        private async Task connect() {
            await controller.connect();
            comm.receive("freETarget 5.2.1\r\n");
            Dispatcher.UIThread.RunJobs();
        }

        private void shoot(int count, double x, double y) {
            comm.receive(FormattableString.Invariant($"{{\"shot\":{count}, \"miss\":0, \"x\":{x:F2}, \"y\":{y:F2}, \"r\":0.00, \"a\":0.00}}\r\n"));
            Dispatcher.UIThread.RunJobs();
        }

        //saves a picture of the window when the environment variable FET_RENDER_DIR is set, for a visual check
        private static void snapshot(Window window, string name) {
            string target = Environment.GetEnvironmentVariable("FET_RENDER_DIR");
            if (string.IsNullOrEmpty(target)) {
                return;
            }
            Dispatcher.UIThread.RunJobs();
            using (WriteableBitmap frame = window.CaptureRenderedFrame()) {
                if (frame != null) {
                    using (FileStream file = File.Create(Path.Combine(target, name + ".png"))) {
#pragma warning disable CS0618 //the simple PNG export is all that is needed here
                        frame.Save(file);
#pragma warning restore CS0618
                    }
                }
            }
        }

        [AvaloniaFact]
        public void window_opens_with_the_default_session() {
            MainWindow window = open();

            Assert.Equal("Pistol Practice", window.txtSessionName.Text);
            Assert.Equal("Connect", window.btnConnect.Content);
            Assert.Equal("Menu", window.btnMenu.Content);
            Assert.Equal("Export", window.btnExport.Content);
            Assert.True(window.mnuSettings.IsEnabled);
            Assert.False(window.mnuTargetSettings.IsEnabled);
            Assert.Equal(6, window.eventTabs.Children.Count);
            Assert.Equal(0, window.shotsList.ItemCount);
            Assert.Empty(dialogs.shown);
            snapshot(window, "window-start");
            window.Close();
        }

        [AvaloniaFact]
        public async Task shots_appear_in_the_window() {
            MainWindow window = open();
            await connect();

            Assert.Equal("Disconnect", window.btnConnect.Content);
            Assert.False(window.mnuSettings.IsEnabled); //not while connected
            Assert.True(window.mnuTargetSettings.IsEnabled);

            //the menu opens with its four entries
            window.btnMenu.Flyout.ShowAt(window.btnMenu);
            Dispatcher.UIThread.RunJobs();
            snapshot(window, "window-menu");
            window.btnMenu.Flyout.Hide();
            Dispatcher.UIThread.RunJobs();
            Assert.Contains("Connected to freETarget 5.2.1", window.statusText.Text);

            double[,] points = { { 0.5, -1 }, { 8.5, 3 }, { -12, 9 }, { 4, -17 }, { -22, -6 }, { 3, 2 }, { -6, 5 }, { 10, 10 }, { 1, -4 }, { -3, -3 }, { 15, 2 }, { -2, 7 } };
            for (int i = 0; i < points.GetLength(0); i++) {
                shoot(i + 1, points[i, 0], points[i, 1]);
            }

            Assert.Equal(12, window.shotsList.ItemCount);
            Assert.StartsWith("12: ", window.txtTotal.Text);
            Assert.Equal(2, window.seriesList.ItemCount);
            Assert.EndsWith(" mm", window.txtMaxSpread.Text);
            Assert.Equal("10.0", window.txtLastShot.Text);
            snapshot(window, "window-shots");

            //one series alone
            window.seriesList.SelectedIndex = 0;
            Dispatcher.UIThread.RunJobs();
            Assert.Equal(10, controller.getShotList().Count);
            Assert.True(window.btnAllShots.IsEnabled);
            snapshot(window, "window-series");

            //another event: the session is saved and the window starts empty
            Event rifle = controller.eventManager.findEventByName("Rifle Match");
            controller.selectEvent(rifle);
            Dispatcher.UIThread.RunJobs();
            Assert.Equal("Rifle Match", window.txtSessionName.Text);
            Assert.Equal(0, window.shotsList.ItemCount);
            Assert.Equal("", window.txtTotal.Text);
            Assert.Single(controller.storage.findSessionsForUser("Marvin", controller.eventManager.findEventByName("Pistol Practice")));

            shoot(1, 0.3, 0.2);
            snapshot(window, "window-rifle");

            window.Close();
            Assert.Equal(MainController.Status.NOT_CONNECTED, controller.currentStatus);
        }

        [AvaloniaFact]
        public async Task window_stays_open_when_the_user_keeps_the_session() {
            MainWindow window = open();
            await connect();
            shoot(1, 0, 0);
            dialogs.yesNoCancelAnswer = null; //cancel

            window.Close();
            Dispatcher.UIThread.RunJobs();

            Assert.True(window.IsVisible);
            Assert.Equal(MainController.Status.CONNECTED, controller.currentStatus);

            dialogs.yesNoCancelAnswer = false;
            window.Close();
            Assert.False(window.IsVisible);
        }

        [AvaloniaFact]
        public async Task calibration_window_moves_the_shots() {
            MainWindow window = open();
            Assert.False(window.mnuCalibration.IsEnabled); //only with a target
            await connect();
            Assert.True(window.mnuCalibration.IsEnabled);
            shoot(1, 0, 0);
            shoot(2, -8, 0);

            CalibrationWindow calibration = new CalibrationWindow(controller);
            calibration.Show(window);
            Dispatcher.UIThread.RunJobs();

            click(calibration.btnRight); //0.5mm per click
            click(calibration.btnRight);
            click(calibration.btnUp);
            Assert.Equal("1.0", calibration.txtXoffset.Text);
            Assert.Equal("0.5", calibration.txtYoffset.Text);
            Assert.Equal(1.0m, controller.calibrationX);
            Assert.Equal(2, window.shotsList.ItemCount);
            Assert.Contains("\u2022", (string)window.mnuCalibration.Header); //marked as calibrated
            Assert.Contains("\u2022", (string)window.btnMenu.Content);       //visible without opening the menu
            snapshot(calibration, "calibration");

            click(calibration.btnReset);
            Assert.Equal("0", calibration.txtXoffset.Text);
            Assert.Equal("10.0", window.txtLastShot.Text);

            click(calibration.btnAntiClockwise);
            calibration.Close();
            Assert.Equal(1m, Settings.Default.calibrationAngle); //saved when the window closes

            window.Close();
        }

        [AvaloniaFact]
        public async Task target_settings_window_reads_and_writes_the_target() {
            MainWindow window = open();
            await connect();

            TargetSettingsWindow settings = new TargetSettingsWindow(controller);
            settings.Show(window);
            Dispatcher.UIThread.RunJobs();
            Assert.Equal("{\"ECHO\":0}", comm.sent.Last());
            Assert.False(settings.form.IsEnabled); //nothing to edit before the target answered

            comm.receive("{\"NAME\":\"DOC\", \"NAME_ID\":11, \"LED_BRIGHT\":50, \"PAPER_TIME\":0, \"STEP_TIME\":30, \"STEP_COUNT\":200, \"SENSOR\":230.5, "
                + "\"X_OFFSET\":0, \"Y_OFFSET\":0, \"Z_OFFSET\":13, \"NORTH_X\":1, \"NORTH_Y\":-2, \"EAST_X\":3, \"EAST_Y\":4, \"SOUTH_X\":5, \"SOUTH_Y\":6, \"WEST_X\":7, \"WEST_Y\":8}");
            Dispatcher.UIThread.RunJobs();

            Assert.True(settings.form.IsEnabled);
            Assert.Equal(11, settings.cmbName.SelectedIndex);
            Assert.Equal(50, settings.trkLEDbright.Value);
            Assert.True(settings.rbStepper.IsChecked);
            Assert.False(settings.txtPaperTime.IsEnabled);
            Assert.Equal("200", settings.txtSteps.Text);
            Assert.Equal("230.5", settings.txtSensorDiameter.Text);
            Assert.Equal("-2", settings.txtNorthY.Text);
            snapshot(settings, "target-settings");

            //wrong input is refused
            settings.txtZOffset.Text = "abc";
            int sent = comm.sent.Count;
            click(settings.btnApply);
            Assert.Equal("Z offset is not a number", settings.txtError.Text);
            Assert.Equal(sent, comm.sent.Count);

            settings.txtZOffset.Text = "15";
            settings.cmbName.SelectedIndex = 12;
            settings.rbDC.IsChecked = true;
            Dispatcher.UIThread.RunJobs();
            settings.txtPaperTime.Text = "500";
            click(settings.btnApply);

            Assert.Equal("", settings.txtError.Text);
            Assert.Contains("\"Z_OFFSET\":15, \"PAPER_TIME\":500, \"STEP_TIME\":0, \"STEP_COUNT\":0", comm.sent.Last());
            Assert.Contains("\"NAME_ID\":12, \"ECHO\":9 }", comm.sent.Last());

            settings.Close();
            window.Close();
        }

        [AvaloniaFact]
        public async Task saved_session_is_resumed_from_the_list() {
            MainWindow window = open();
            Assert.False(window.mnuResume.IsEnabled);
            await connect();
            Assert.True(window.mnuResume.IsEnabled);

            shoot(1, 0, 0);
            shoot(2, 8, 0);
            Assert.False(window.mnuResume.IsEnabled); //not with shots in the session
            Event practice = controller.selectedEvent;
            controller.selectEvent(controller.eventManager.findEventByName("Rifle Match")); //saves the session
            controller.selectEvent(practice);
            Dispatcher.UIThread.RunJobs();
            Assert.Equal(0, window.shotsList.ItemCount);
            Assert.True(window.mnuResume.IsEnabled);

            ResumeWindow resume = new ResumeWindow(controller);
            resume.Show(window);
            Dispatcher.UIThread.RunJobs();
            Assert.Equal(1, resume.lstbSessions.ItemCount);
            Assert.False(resume.btnOK.IsEnabled);
            resume.lstbSessions.SelectedIndex = 0;
            Dispatcher.UIThread.RunJobs();
            Assert.True(resume.btnOK.IsEnabled);
            snapshot(resume, "resume");
            ListBoxSessionItem selected = (ListBoxSessionItem)resume.lstbSessions.SelectedItem;
            resume.Close();

            controller.resumeSession(selected.id);
            Dispatcher.UIThread.RunJobs();

            Assert.Equal(2, window.shotsList.ItemCount);
            Assert.StartsWith("2: 20 ", window.txtTotal.Text);
            Assert.Equal("Pistol Practice", window.txtSessionName.Text);
            Assert.False(window.mnuResume.IsEnabled);

            shoot(3, 0, 16);
            Assert.Equal(3, window.shotsList.ItemCount);
            Assert.StartsWith("3: 29 ", window.txtTotal.Text);
            snapshot(window, "window-resumed");

            dialogs.yesNoCancelAnswer = false;
            window.Close();
        }

        [AvaloniaFact]
        public async Task journal_shows_saved_sessions_and_loads_one_into_the_main_window() {
            MainWindow window = open();
            await connect();
            Event practice = controller.selectedEvent;
            Event rifle = controller.eventManager.findEventByName("Rifle Match");

            //three practice sessions and one rifle session
            double[][] groups = { new[] { 2.0, 9, 14 }, new[] { 1.0, 5, 8 }, new[] { 0.5, 3, 6, 4 } };
            foreach (double[] group in groups) {
                for (int i = 0; i < group.Length; i++) {
                    shoot(i + 1, group[i], -group[i] / 2);
                }
                controller.selectEvent(rifle);   //saves the practice session
                shoot(1, 0.4, 0.1);
                controller.selectEvent(practice); //saves the rifle session
            }
            Dispatcher.UIThread.RunJobs();

            window.openJournal();
            Dispatcher.UIThread.RunJobs();
            JournalWindow journal = window.journalWindow;
            Assert.NotNull(journal);
            Assert.Equal(MainController.Status.NOT_CONNECTED, controller.currentStatus); //the journal disconnects
            Assert.False(window.btnConnect.IsEnabled);

            Assert.Equal("Marvin", journal.cmbUsers.SelectedItem);
            Assert.Equal(practice.Name, ((Event)journal.cmbEvents.SelectedItem).Name);
            Assert.Equal(3, journal.lstbSessions.ItemCount);
            Assert.False(journal.btnLoadSession.IsEnabled);
            Assert.Equal(3, journal.chartScore.PointCount);
            Assert.Contains("last 10 sessions average score", journal.chartScore.Title);

            //newest first: the session with four shots
            journal.lstbSessions.SelectedIndex = 0;
            Dispatcher.UIThread.RunJobs();
            Assert.True(journal.btnLoadSession.IsEnabled);
            Assert.Equal(4, journal.currentSession.Shots.Count);
            Assert.Equal("(empty)", journal.txtDiary.Text);

            journal.saveDiary("Gun: LP10\nGood day \u2192 more of this");
            Assert.StartsWith("Gun: LP10", journal.txtDiary.Text);
            Assert.StartsWith("{\\rtf1", controller.storage.findSession(journal.currentSession.id).diaryEntry); //readable by the original Windows application
            snapshot(journal, "journal-sessions");

            click(journal.btnLoadSession);
            Assert.Equal(MainController.Status.LOADED, controller.currentStatus);
            Assert.Equal(4, window.shotsList.ItemCount);
            Assert.StartsWith("4: ", window.txtTotal.Text);
            Assert.StartsWith("Pistol Practice  (", window.txtSessionName.Text);
            Assert.Contains("loaded from the journal", window.statusText.Text);
            snapshot(window, "window-journal-session");

            //the other event has its own sessions
            journal.cmbEvents.SelectedItem = journal.cmbEvents.Items.Cast<Event>().First(e => e.Name == "Rifle Match");
            Dispatcher.UIThread.RunJobs();
            Assert.Equal(3, journal.lstbSessions.ItemCount);
            Assert.Null(journal.currentSession);
            journal.cmbEvents.SelectedItem = journal.cmbEvents.Items.Cast<Event>().First(e => e.Name == practice.Name);
            Dispatcher.UIThread.RunJobs();

            //delete the oldest one
            journal.lstbSessions.SelectedIndex = 2;
            Dispatcher.UIThread.RunJobs();
            click(journal.btnDelete);
            Assert.Equal(2, journal.lstbSessions.ItemCount);
            Assert.Equal(2, journal.chartMeanRadius.PointCount);
            Assert.Contains(dialogs.shown, text => text.Contains("permanently delete"));

            SessionChartWindow chart = new SessionChartWindow(controller.currentSession);
            chart.Show(journal);
            Dispatcher.UIThread.RunJobs();
            Assert.Equal(4, chart.chart.PointCount);
            snapshot(chart, "journal-chart");
            chart.Close();

            //closing the journal keeps the loaded session in the main window
            journal.Close();
            Dispatcher.UIThread.RunJobs();
            Assert.Null(window.journalWindow);
            Assert.Equal(MainController.Status.LOADED, controller.currentStatus);
            Assert.Equal(4, window.shotsList.ItemCount);
            Assert.StartsWith("4: ", window.txtTotal.Text);
            Assert.True(window.btnConnect.IsEnabled);
            Assert.Equal("Connect", window.btnConnect.Content);
            Assert.True(window.mnuSettings.IsEnabled);

            //connecting starts a clean session
            await connect();
            Assert.Equal(MainController.Status.CONNECTED, controller.currentStatus);
            Assert.Equal("Disconnect", window.btnConnect.Content);
            Assert.Equal(0, window.shotsList.ItemCount);
            Assert.Equal("Pistol Practice", window.txtSessionName.Text);
            Assert.Equal(2, controller.storage.findSessionsForUser("Marvin", practice).Count); //viewing saved nothing
            window.Close();
        }

        [AvaloniaFact]
        public async Task terminal_shows_the_target_and_sends_commands() {
            MainWindow window = open();
            Assert.False(window.mnuTerminal.IsEnabled);
            await connect();
            Assert.True(window.mnuTerminal.IsEnabled);

            window.openTerminal();
            Dispatcher.UIThread.RunJobs();
            TerminalWindow terminal = window.terminalWindow;
            Assert.Equal("Terminal Arduino", terminal.Title);
            Assert.Contains("freETarget 5.2.1", terminal.txtOutput.Text); //what came before the window was opened
            comm.sent.Clear();

            comm.receive("{\"NAME\":\"DOC\", \"SN\":1234}\r\n");
            Dispatcher.UIThread.RunJobs();
            Assert.Contains("\"SN\":1234", terminal.txtOutput.Text);

            click(terminal.btnEcho);
            click(terminal.btnVersion);
            click(terminal.btnCalibration);
            Assert.Equal("STOP CAL", terminal.btnCalibration.Content);
            click(terminal.btnCalibration);
            Assert.Equal("CAL", terminal.btnCalibration.Content);
            terminal.sendInit("1234");
            Assert.Equal(new[] { "{\"ECHO\":0}", "{\"VERSION\":7}", "{\"CAL\":0}", "!", "{\"INIT\":1234}" }, comm.sent);
            Assert.Contains("> {\"ECHO\":0}", terminal.txtOutput.Text); //the sent commands are shown too

            //a command with a parameter: the usual value is offered
            terminal.cmbCommands.SelectedItem = FirmwareCommand.All.First(c => c.command == "LED_BRIGHT");
            Dispatcher.UIThread.RunJobs();
            Assert.Equal("50", terminal.txtParameter.Text);
            terminal.txtParameter.Text = "";
            click(terminal.btnSend);
            Assert.Equal("Cannot send empty value", terminal.txtError.Text);
            terminal.txtParameter.Text = "80";
            click(terminal.btnSend);
            Assert.Equal("", terminal.txtError.Text);
            Assert.Equal("{\"LED_BRIGHT\":80}", comm.sent.Last());

            terminal.txtGenericCommand.Text = "{\"TEST\":7}";
            click(terminal.btnSend2);
            Assert.Equal("{\"TEST\":7}", comm.sent.Last());
            snapshot(terminal, "terminal");

            //the terminal belongs to the connection
            dialogs.yesNoCancelAnswer = false;
            await controller.toggleConnection();
            Dispatcher.UIThread.RunJobs();
            Assert.Null(window.terminalWindow);
            Assert.False(window.mnuTerminal.IsEnabled);
            window.Close();
        }

        private static void click(Button button) {
            button.RaiseEvent(new Avalonia.Interactivity.RoutedEventArgs(Button.ClickEvent));
            Dispatcher.UIThread.RunJobs();
        }

        [AvaloniaFact]
        public void settings_window_writes_the_settings() {
            MainWindow window = open();
            SettingsWindow settings = new SettingsWindow(controller);
            settings.Show(window);
            Dispatcher.UIThread.RunJobs();
            snapshot(settings, "settings-general");

            settings.txtName.Text = "";
            click(settings.btnOK);
            Assert.Contains("Shooter name empty", settings.txtError.Text);
            Assert.True(settings.IsVisible);

            settings.txtName.Text = "Marvin E";
            settings.txtDistance.Text = "50";
            settings.tabs.SelectedIndex = 2;
            Dispatcher.UIThread.RunJobs();
            Assert.Equal("Moccasin", settings.cmbTargetColor.SelectedItem);
            settings.cmbTargetColor.SelectedItem = "LightGray";
            settings.cmb10Back.SelectedItem = "Red";
            Dispatcher.UIThread.RunJobs();
            snapshot(settings, "settings-colors");
            click(settings.btnOK);

            Assert.False(settings.IsVisible);
            Assert.Equal("Marvin E", Settings.Default.name);
            Assert.Equal(50m, Settings.Default.targetDistance);
            Assert.Equal(System.Drawing.Color.LightGray, Settings.Default.targetColor);
            Assert.Equal(System.Drawing.Color.Red, Settings.Default.score10BackgroundColor);
            Assert.Equal(System.Drawing.Color.Aqua, Settings.Default.score9BackgroundColor); //untouched
            window.Close();
        }

        [AvaloniaFact]
        public void events_are_edited_and_made_active_in_the_settings() {
            MainWindow window = open();
            Assert.Equal(6, window.eventTabs.Children.Count);
            SettingsWindow settings = new SettingsWindow(controller);
            settings.Show(window);
            settings.tabs.SelectedIndex = 3;
            Dispatcher.UIThread.RunJobs();
            Assert.Equal(6, settings.lstEvents.ItemCount);
            Assert.False(settings.btnSaveEvent.IsEnabled);

            //an existing event shows its values. a practice has no number of shots
            settings.lstEvents.SelectedIndex = 0;
            Dispatcher.UIThread.RunJobs();
            Assert.Equal("Pistol Practice", settings.txtEventName.Text);
            Assert.False(settings.txtNoOfShots.IsEnabled);
            Assert.False(settings.txtRFshots.IsEnabled);

            //a new event
            click(settings.btnAddEvent);
            settings.txtEventName.Text = "Club Match";
            settings.cmbEventType.SelectedItem = Event.EventType.Match;
            Dispatcher.UIThread.RunJobs();
            Assert.True(settings.txtNoOfShots.IsEnabled);
            Assert.False(settings.txtShotsPerSeries.IsEnabled);
            settings.txtNoOfShots.Text = "forty";
            settings.txtMinutes.Text = "50";
            settings.cmbTarget.SelectedItem = "freETarget.targets.AirRifle";
            settings.cmbEventColor.SelectedItem = "Violet";
            click(settings.btnSaveEvent);
            Assert.Contains("'Number of Shots' field is not a number", settings.txtEventError.Text);
            Assert.Equal(6, settings.lstEvents.ItemCount);
            snapshot(settings, "settings-events");

            settings.txtNoOfShots.Text = "40";
            click(settings.btnSaveEvent);
            Assert.Equal("", settings.txtEventError.Text);
            Assert.Equal(7, settings.lstEvents.ItemCount);
            Event created = controller.eventManager.findEventByName("Club Match");
            Assert.Equal(40, created.NumberOfShots);
            Assert.IsType<targets.AirRifle>(created.Target);

            //make it active and first
            settings.tabs.SelectedIndex = 4;
            Dispatcher.UIThread.RunJobs();
            Assert.Equal(1, settings.lstAllEvents.ItemCount);
            Assert.Equal(6, settings.lstActiveEvents.ItemCount);
            settings.lstAllEvents.SelectedIndex = 0;
            Dispatcher.UIThread.RunJobs();
            click(settings.btnToActive);
            for (int i = 0; i < 6; i++) {
                click(settings.btnUp);
            }
            Assert.Equal("Club Match", ((Event)settings.lstActiveEvents.Items[0]).Name);
            Assert.False(settings.btnUp.IsEnabled);
            snapshot(settings, "settings-active");

            settings.cmbDefaultEvent.SelectedItem = "Club Match";
            Assert.Equal("Club Match", settings.cmbDefaultEvent.SelectedItem);
            Assert.Equal(7, settings.cmbDefaultEvent.ItemCount);
            click(settings.btnOK);
            Dispatcher.UIThread.RunJobs();

            Assert.Equal(7, window.eventTabs.Children.Count); //the main window shows the new event at once
            Assert.Equal("Club Match", ((Button)window.eventTabs.Children[0]).Content);
            Assert.Equal("Club Match", Settings.Default.defaultTarget);
            Assert.Equal(new[] { "Club Match", "Pistol Practice" }, controller.eventManager.getActiveEventsList().Take(2).Select(e => e.Name));
            window.Close();
        }

        [AvaloniaFact]
        public async Task appearance_can_be_switched_to_dark_and_back() {
            MainWindow window = open();
            await connect();
            shoot(1, 2, 3);
            shoot(2, -9, 6);
            Assert.True(window.mnuThemeSystem.IsChecked); //follows the operating system unless told otherwise

            window.setTheme(ThemeSetting.Dark);
            Dispatcher.UIThread.RunJobs();
            Assert.Equal(Avalonia.Styling.ThemeVariant.Dark, window.ActualThemeVariant);
            Assert.True(window.mnuThemeDark.IsChecked);
            Assert.Equal("Dark", Settings.Default.theme);
            snapshot(window, "window-dark");

            SettingsWindow settings = new SettingsWindow(controller);
            settings.Show(window);
            settings.tabs.SelectedIndex = 3;
            Dispatcher.UIThread.RunJobs();
            settings.lstEvents.SelectedIndex = 2;
            Dispatcher.UIThread.RunJobs();
            snapshot(settings, "settings-dark");
            settings.Close();

            window.setTheme(ThemeSetting.Light);
            Dispatcher.UIThread.RunJobs();
            Assert.Equal(Avalonia.Styling.ThemeVariant.Light, window.ActualThemeVariant);

            window.setTheme(ThemeSetting.System);
            Assert.Equal(Avalonia.Styling.ThemeVariant.Default, Application.Current.RequestedThemeVariant);

            dialogs.yesNoCancelAnswer = false;
            window.Close();
        }
    }
}
