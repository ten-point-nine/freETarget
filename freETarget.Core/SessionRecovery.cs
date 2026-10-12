using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace freETarget {

    /** the unsaved session found in the log after an abnormal shutdown */
    public class RecoveredSession {
        /** name of the event of the session */
        public string sessionName = "";
        /** start time of the session, in Logger.TimestampFormat */
        public string sessionTime = "";
        /** what the target sent during the session, oldest first. feed these to the message handling again */
        public List<string> lines = new List<string>();
    }

    public static class SessionRecovery {

        public const string NormalShutdownMarker = "@@@@@ normal shutdown @@@@@";
        public const string StartupMarker = "------------------------------------------------------------------------";
        public const string NewSessionMarker = "### New Session";

        /** the application writes the shutdown marker as last line of the log when it closes normally */
        public static bool wasCleanShutdown(string logFile) {
            IEnumerable<String> lines = File.ReadLines(logFile);
            int count = lines.Count();
            if (count > 0) {
                String lastLine = lines.Last();
                if (lastLine.Contains(NormalShutdownMarker)) {
                    return true;
                } else {
                    return false;
                }
            } else {
                return true;
            }
        }

        /** parses the log from the end to find the shots of the last session */
        public static RecoveredSession read(string logFile) {
            List<String> tempLines = new List<string>();
            using (FileStream stream = new FileStream(logFile, FileMode.Open, FileAccess.Read)) {
                var reader = new ReverseTextReader(stream, Encoding.UTF8);
                while (!reader.EndOfStream) {
                    String line = reader.ReadLine().Trim();
                    tempLines.Add(line);
                    if (line.Contains(NewSessionMarker)) {
                        //we reached the start of the last session
                        break;
                    }
                }
            }

            //select only the actual shots
            List<String> sessionLines = new List<string>();
            String sessionTitle = "";
            for (int i = tempLines.Count - 1; i >= 0; i--) {
                if (tempLines[i].Contains(StartupMarker)) {
                    //stop at the last start
                    break;
                }

                //isolate the session line
                if (tempLines[i].Contains(NewSessionMarker)) {
                    sessionTitle = tempLines[i];
                }

                sessionLines.Add(tempLines[i]);
            }

            RecoveredSession ret = new RecoveredSession();

            int firstQuote = sessionTitle.IndexOf('\'');
            int secondQuote = sessionTitle.LastIndexOf('\'');
            if (firstQuote > 0) {
                ret.sessionTime = sessionTitle.Substring(0, sessionTitle.IndexOf('|') - 1);
                sessionTitle = sessionTitle.Substring(firstQuote + 1, secondQuote - firstQuote - 1);
            }

            //isolate just the shot jsons
            for (int i = 0; i < sessionLines.Count; i++) {
                if (sessionLines[i].IndexOf('|') == -1) {
                    continue;
                }
                sessionLines[i] = sessionLines[i].Substring(sessionLines[i].IndexOf('|') + 2);
            }

            ret.sessionName = sessionTitle;
            ret.lines = sessionLines;
            return ret;
        }
    }
}
