using freETarget.Properties;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using Xunit;

namespace freETarget.Tests {

    public class LogTests : IDisposable {

        private readonly TempDir dir = new TempDir();
        private readonly Logger logger;

        public LogTests() {
            Make.resetSettings();
            logger = new Logger(dir.path);
        }

        public void Dispose() {
            Make.resetSettings();
            dir.Dispose();
        }

        private void startApplication() {
            logger.log(SessionRecovery.StartupMarker + "\n\nStarting freETarget - v4.12.2 ... ");
            logger.log("Events loaded: Pistol Practice | Pistol Match ");
        }

        [Fact]
        public void log_files_are_created() {
            Assert.True(File.Exists(logger.logFile));
            Assert.True(File.Exists(logger.shotFile));
            Assert.EndsWith("Shot.log", logger.shotFile);
        }

        [Fact]
        public void lines_are_appended_with_a_timestamp() {
            logger.log("first");
            logger.log("  second  ");
            logger.log("");

            string[] lines = File.ReadAllLines(logger.logFile);
            Assert.Equal(2, lines.Length);
            Assert.EndsWith(" | second", lines[1]);
            DateTime.ParseExact(lines[0].Substring(0, lines[0].IndexOf('|') - 1), Logger.TimestampFormat, CultureInfo.InvariantCulture);
        }

        [Fact]
        public void shot_file_keeps_only_the_last_shot() {
            logger.log("shot 1", logger.shotFile, true);
            logger.log("shot 2", logger.shotFile, true);

            Assert.EndsWith("shot 2", File.ReadAllLines(logger.shotFile).Single());
        }

        [Fact]
        public void nothing_is_written_when_logging_is_switched_off() {
            Settings.Default.fileLogging = false;

            logger.log("secret");

            Assert.Empty(File.ReadAllLines(logger.logFile));
        }

        [Fact]
        public void empty_log_and_normal_shutdown_are_clean() {
            Assert.True(SessionRecovery.wasCleanShutdown(logger.logFile));

            startApplication();
            Assert.False(SessionRecovery.wasCleanShutdown(logger.logFile));

            logger.log(SessionRecovery.NormalShutdownMarker);
            Assert.True(SessionRecovery.wasCleanShutdown(logger.logFile));
        }

        [Fact]
        public void shots_of_the_last_session_are_recovered_after_a_crash() {
            //an earlier, finished run of the application
            startApplication();
            logger.log("### New Session 'Rifle Match' started ###");
            logger.log("{\"shot\":1, \"x\":1.00, \"y\":1.00}");
            logger.log(SessionRecovery.NormalShutdownMarker);

            //the run that crashed, with two sessions
            startApplication();
            logger.log("### New Session 'Pistol Practice' started ###");
            logger.log("{\"shot\":1, \"x\":2.00, \"y\":2.00}");
            logger.log("### New Session 'Pistol Match' started ###");
            logger.log("Sending: {\"ATHLETE\":\"tester\", \"EVENT\":\"Pistol Match\"}");
            logger.log("{\"shot\":1, \"x\":8.00, \"y\":0.00, \"r\":8.00, \"a\":0.00}");
            logger.log("{\"shot\":2, \"x\":0.00, \"y\":16.00, ");
            logger.log("\"r\":16.00, \"a\":90.00}");

            Assert.False(SessionRecovery.wasCleanShutdown(logger.logFile));
            RecoveredSession recovered = SessionRecovery.read(logger.logFile);

            Assert.Equal("Pistol Match", recovered.sessionName);
            DateTime.ParseExact(recovered.sessionTime, Logger.TimestampFormat, CultureInfo.InvariantCulture);

            //replaying the lines gives the shots of the last session only, in the order they were fired
            MessageReader reader = new MessageReader();
            List<Shot> shots = new List<Shot>();
            foreach (string line in recovered.lines) {
                reader.append(line);
                string message;
                while ((message = reader.next()) != null) {
                    if (TargetMessage.classify(message) == TargetMessage.Kind.Shot) {
                        shots.Add(TargetMessage.parseShot(message, 0, 0, 0, new targets.AirPistol(4.5m)));
                    }
                }
            }
            Assert.Equal(new[] { 1, 2 }, shots.Select(s => s.count));
            Assert.Equal(new[] { 10.0m, 9.0m }, shots.Select(s => s.decimalScore));
        }

        [Fact]
        public void crash_before_any_session_recovers_nothing() {
            startApplication();

            RecoveredSession recovered = SessionRecovery.read(logger.logFile);

            Assert.Equal("", recovered.sessionName);
        }

        [Fact]
        public void reverse_reader_returns_the_lines_from_last_to_first() {
            string file = Path.Combine(dir.path, "lines.txt");
            File.WriteAllText(file, "one\r\ntwo\r\nthree");

            List<string> lines = new List<string>();
            using (FileStream stream = new FileStream(file, FileMode.Open, FileAccess.Read)) {
                ReverseTextReader reader = new ReverseTextReader(stream, Encoding.UTF8);
                while (!reader.EndOfStream) {
                    lines.Add(reader.ReadLine().Trim());
                }
            }

            Assert.Equal(new[] { "three", "two", "one" }, lines);
        }
    }
}
