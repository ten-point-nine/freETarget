using freETarget.Properties;
using System;
using System.Globalization;
using System.IO;

namespace freETarget {

    /**
     * the application log. it is also read back: after an abnormal shutdown the shots of the last session are recovered from it (see SessionRecovery),
     * so the format of the lines matters
     */
    public class Logger {

        public const string TimestampFormat = "yyyy.MM.dd HH:mm:ss:fff";

        /** today's log file. null if it could not be created */
        public string logFile { get; private set; }

        /** the last shot is logged in this file, with X,Y coordinates and calculated score plus decimalscore. null if it could not be created */
        public string shotFile { get; private set; }

        public Logger() : this(null) {
        }

        public Logger(string logDirectory) {
            if (logDirectory == null) {
                logDirectory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "freETarget", "log");
                if (!Directory.Exists(logDirectory)) {
                    try {
                        Directory.CreateDirectory(logDirectory);
                    } catch (Exception ex) {
                        Console.WriteLine(ex.Message);

                        //if there is no write permission in the documents folder, write the log in the application data folder of the user
                        logDirectory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "freETarget", "log");
                        Directory.CreateDirectory(logDirectory);
                    }
                }
            } else {
                Directory.CreateDirectory(logDirectory);
            }

            logFile = createIfMissing(Path.Combine(logDirectory, "freETarget." + DateTime.Now.ToString("yyyy.MM.dd", CultureInfo.InvariantCulture) + ".log"));
            shotFile = createIfMissing(Path.Combine(logDirectory, "Shot.log"));
        }

        private static string createIfMissing(string file) {
            if (!File.Exists(file)) {
                try {
                    File.Create(file).Close();
                } catch (Exception ex) {
                    Console.WriteLine(ex.Message);
                    return null;
                }
            }
            return file;
        }

        public void log(string s) {
            log(s, this.logFile);
        }

        public void log(string s, string logFile) {
            log(s, logFile, false);
        }

        public void log(string s, string logFile, bool truncate) {
            if (s == null || s == "" || logFile == null) {
                return;
            }

            FileMode mode = FileMode.Append;
            if (truncate) {
                mode = FileMode.Truncate;
            }

            if (Settings.Default.fileLogging) { //log enabled from settings

                try {
                    //Opens a new file stream which allows asynchronous reading and writing
                    using (StreamWriter sw = new StreamWriter(new FileStream(logFile, mode, FileAccess.Write, FileShare.ReadWrite))) {
                        sw.WriteLine(DateTime.Now.ToString(TimestampFormat, CultureInfo.InvariantCulture) + " | " + s.Trim());
                    }
                } catch (Exception ex) {
                    //oh well...
                    Console.WriteLine("Error logging (" + logFile + "): " + ex.Message);
                }
            }
        }
    }
}
