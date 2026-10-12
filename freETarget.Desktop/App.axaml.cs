using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Markup.Xaml;
using Avalonia.Threading;
using System;
using System.IO;
using System.Reflection;

namespace freETarget.Desktop {

    public partial class App : Application {

        public override void Initialize() {
            AvaloniaXamlLoader.Load(this);
        }

        public override void OnFrameworkInitializationCompleted() {
            if (ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop) {
                //questions can come up before the main window exists (recovery after a crash, database problems).
                //such a message box is the only window then, and closing the last window would end the application
                desktop.ShutdownMode = ShutdownMode.OnExplicitShutdown;

                Speech.Current = PlatformSpeech.create();

                Dialogs dialogs = new Dialogs();
                MainController controller = new MainController(dialogs, runOnUIThread);

                Version version = Assembly.GetExecutingAssembly().GetName().Version;
                string versionText = "v" + version.Major + "." + version.Minor + "." + version.Build;

                //"--data <folder>" keeps database and log in that folder instead of Documents/freETarget
                string dataFolder = getOption(desktop.Args, "--data");
                string databaseFolder = dataFolder == null ? null : Path.Combine(dataFolder, "database");
                string logFolder = dataFolder == null ? null : Path.Combine(dataFolder, "log");

                string error = controller.init(version.Major, versionText, databaseFolder, logFolder);
                if (error != null) {
                    dialogs.showError(error + Environment.NewLine + Environment.NewLine + "The application will now exit!", "Database problem");
                    Environment.Exit(1);
                }

                ThemeSetting.apply(); //the settings are loaded now

                MainWindow window = new MainWindow(controller, versionText);
                dialogs.owner = window;
                desktop.MainWindow = window;
                desktop.ShutdownMode = ShutdownMode.OnMainWindowClose;
            }

            base.OnFrameworkInitializationCompleted();
        }

        private static string getOption(string[] args, string name) {
            if (args != null) {
                for (int i = 0; i < args.Length - 1; i++) {
                    if (args[i] == name) {
                        return Path.GetFullPath(args[i + 1]);
                    }
                }
            }
            return null;
        }

        //the controller raises its events on the UI thread. what comes from other threads is queued, in the order it arrived
        internal static void runOnUIThread(Action action) {
            if (Dispatcher.UIThread.CheckAccess()) {
                action();
            } else {
                Dispatcher.UIThread.Post(action);
            }
        }
    }
}
