using freETarget.Properties;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Globalization;
using System.IO;
using Xunit;

// Settings.Default and the current culture are process-wide state, so the tests must not run in parallel
[assembly: CollectionBehavior(DisableTestParallelization = true)]

namespace freETarget.Tests {

    /** stand-in for the main window */
    class FakeHost : IAppHost {
        public List<string> logs = new List<string>();
        public List<string> errors = new List<string>();
        public List<string> warnings = new List<string>();
        public bool confirmAnswer = true;
        public EventManager eventManager;

        public void log(string s) { logs.Add(s); }
        public void displayMessage(string text, bool showToolTip) { }
        public void showError(string text, string title) { errors.Add(text); }
        public void showWarning(string text, string title) { warnings.Add(text); }
        public bool confirm(string text, string title) { return confirmAnswer; }
        public Event findEventByID(long id) { return eventManager.findEventByID(id); }
    }

    /** a directory that is deleted after the test */
    class TempDir : IDisposable {
        public readonly string path = Path.Combine(Path.GetTempPath(), "freETarget.Tests." + Guid.NewGuid().ToString("N"));

        public TempDir() {
            Directory.CreateDirectory(path);
        }

        public void Dispose() {
            try {
                Directory.Delete(path, true);
            } catch (IOException) {
            }
        }
    }

    /** runs a test under another culture. German uses a decimal comma, like many users of the application */
    class Culture : IDisposable {
        private readonly CultureInfo before = CultureInfo.CurrentCulture;

        public Culture(string name) {
            CultureInfo.CurrentCulture = new CultureInfo(name);
        }

        public void Dispose() {
            CultureInfo.CurrentCulture = before;
        }
    }

    static class Make {

        public static void resetSettings() {
            Settings defaults = new Settings();
            foreach (var p in typeof(Settings).GetProperties()) {
                if (p.CanWrite && p.GetIndexParameters().Length == 0 && !p.GetMethod.IsStatic) {
                    p.SetValue(Settings.Default, p.GetValue(defaults));
                }
            }
        }

        public static Event practiceEvent(long id = 1) {
            return new Event(id, "Pistol Practice", false, Event.EventType.Practice, -1, new targets.AirPistol(4.5m), -1, 4.5m, -1, -1, -1, -1, -1, Color.Gold, false, -1, -1, -1, -1, -1);
        }

        public static Event finalEvent() {
            return new Event(3, "Pistol Final", true, Event.EventType.Final, 24, new targets.AirPistol(4.5m), -1, 4.5m, 5, 250, 10, 2, 50, Color.Red, false, -1, -1, -1, -1, -1);
        }

        public static Event rapidFireEvent(int shots, int timePerSerie, decimal timePerShot, decimal timeBetweenShots, int loadTime) {
            return new Event(9, "Rapid Fire", false, Event.EventType.Practice, -1, new targets.AirPistol(4.5m), -1, 4.5m, -1, -1, -1, -1, -1, Color.Gold, true, shots, timePerSerie, timePerShot, timeBetweenShots, loadTime);
        }

        /** a shot at x,y milimeters from the center of an air pistol target */
        public static Shot shot(decimal x, decimal y, int count = 1) {
            Shot s = new Shot(0, 0, 0);
            s.count = count;
            s.setX(x);
            s.setY(y);
            s.computeScore(new targets.AirPistol(4.5m));
            s.timestamp = new DateTime(2026, 10, 12, 10, 0, 0).AddSeconds(30 * count);
            return s;
        }
    }
}
