using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;

namespace freETarget {

    /**
     * a session as a file, to move it to another computer. the .exp file holds everything and can be imported again,
     * the .csv file next to it is the list of shots for spreadsheets
     */
    public static class SessionFile {

        public static string getExportFolder() {
            return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "freETarget", "sessionExport");
        }

        private static string safeName(string name) {
            StringBuilder ret = new StringBuilder();
            foreach (char c in name) {
                ret.Append(Array.IndexOf(Path.GetInvalidFileNameChars(), c) >= 0 || c == '/' || c == '\\' ? '_' : c);
            }
            return ret.ToString();
        }

        /** the content of the .exp file: the fields of the session separated by ~ and its hash at the end */
        public static string toText(Session session) {
            StringBuilder sb = new StringBuilder();
            sb.Append(session.eventType.Name + "~");
            sb.Append(session.user + "~");
            sb.Append(session.score.ToString(CultureInfo.InvariantCulture) + "~");
            sb.Append(session.decimalScore.ToString(CultureInfo.InvariantCulture) + "~");
            sb.Append(session.innerX.ToString(CultureInfo.InvariantCulture) + "~");
            sb.Append(session.xbar.ToString(CultureInfo.InvariantCulture) + "~");
            sb.Append(session.ybar.ToString(CultureInfo.InvariantCulture) + "~");
            sb.Append(session.rbar.ToString(CultureInfo.InvariantCulture) + "~");
            sb.Append(session.groupSize.ToString(CultureInfo.InvariantCulture) + "~");
            sb.Append(StorageController.convertListOfShotsToString(session.Shots) + "~");
            sb.Append(StorageController.convertDatetimeToString(session.startTime) + "~");
            sb.Append(StorageController.convertDatetimeToString(session.endTime) + "~");
            sb.Append(session.averageScore.ToString("F2", CultureInfo.InvariantCulture) + "~");
            sb.Append(session.actualNumberOfShots.ToString(CultureInfo.InvariantCulture) + "~");
            sb.Append(session.diaryEntry + "~");
            sb.Append(StorageController.convertTimespanToDecimal(session.averageTimePerShot).ToString("F3", CultureInfo.InvariantCulture) + "~");
            sb.Append(StorageController.convertTimespanToDecimal(session.longestShot).ToString("F3", CultureInfo.InvariantCulture) + "~");
            sb.Append(StorageController.convertTimespanToDecimal(session.shortestShot).ToString("F3", CultureInfo.InvariantCulture) + "~");
            sb.Append(StorageController.getSessionHash(session));
            return sb.ToString();
        }

        /** one line per shot: number, time, x, y, decimal score */
        public static string toCsv(Session session) {
            StringBuilder sb = new StringBuilder();
            foreach (Shot shot in session.Shots) {
                sb.AppendLine(shot.count + "," + shot.timestamp.ToString("yyyy-MM-dd HH:mm:ss", CultureInfo.InvariantCulture) + "," + shot.getX().ToString(CultureInfo.InvariantCulture)
                    + "," + shot.getY().ToString(CultureInfo.InvariantCulture) + "," + shot.decimalScore.ToString(CultureInfo.InvariantCulture));
            }
            return sb.ToString();
        }

        /** writes the .exp and the .csv file of the session to the folder and returns the name of the .exp file */
        public static string export(Session session, string folder) {
            Directory.CreateDirectory(folder);
            string name = "freETargetSession_" + safeName(session.user) + "-" + safeName(session.eventType.Name) + "_" + session.id;
            string file = Path.Combine(folder, name + ".exp");
            File.WriteAllText(file, toText(session) + Environment.NewLine);
            File.WriteAllText(Path.Combine(folder, name + ".csv"), toCsv(session));
            return file;
        }

        /**
         * reads a session from the content of an .exp file. the event is looked up by its name.
         * returns null and the reason if the text is not a valid session
         */
        public static Session fromText(string text, EventManager events, out string error) {
            error = null;
            string[] items = text.Split('~');
            if (items.Length < 19) {
                error = "This is not a session file of freETarget.";
                return null;
            }

            string eventName = items[0];
            Event cof = events.findEventByName(eventName);
            if (cof == null) {
                error = "Could not find event '" + eventName + "'. Please create it before importing.";
                return null;
            }

            Session session;
            string hash;
            try {
                session = Session.createNewSession(cof, items[1]);
                session.score = int.Parse(items[2], CultureInfo.InvariantCulture);
                session.decimalScore = decimal.Parse(items[3], CultureInfo.InvariantCulture);
                session.innerX = int.Parse(items[4], CultureInfo.InvariantCulture);
                session.xbar = decimal.Parse(items[5], CultureInfo.InvariantCulture);
                session.ybar = decimal.Parse(items[6], CultureInfo.InvariantCulture);
                session.rbar = decimal.Parse(items[7], CultureInfo.InvariantCulture);
                session.Shots = StorageController.convertStringToListOfShots(items[9]);
                //items[8] is the group size. older versions computed it wrongly, so it is computed again from the shots
                session.groupSize = Session.computeGroupSize(session.Shots);
                session.startTime = StorageController.convertStringToDate(items[10]);
                session.endTime = StorageController.convertStringToDate(items[11]);
                session.averageScore = decimal.Parse(items[12], CultureInfo.InvariantCulture);
                session.actualNumberOfShots = int.Parse(items[13], CultureInfo.InvariantCulture);
                //the diary can contain the separator itself: everything between the fixed fields before and after it is diary
                session.diaryEntry = string.Join("~", items, 14, items.Length - 18);
                session.averageTimePerShot = StorageController.convertDecimalToTimespan(decimal.Parse(items[items.Length - 4], CultureInfo.InvariantCulture));
                session.longestShot = StorageController.convertDecimalToTimespan(decimal.Parse(items[items.Length - 3], CultureInfo.InvariantCulture));
                session.shortestShot = StorageController.convertDecimalToTimespan(decimal.Parse(items[items.Length - 2], CultureInfo.InvariantCulture));
                hash = items[items.Length - 1].Trim();
            } catch (Exception ex) when (ex is FormatException || ex is IndexOutOfRangeException || ex is ArgumentException || ex is OverflowException) {
                error = "The session file cannot be read: " + ex.Message;
                return null;
            }

            if (StorageController.verifySessionHash(session, hash) == false) {
                error = "MD5 check failed. Session data corrupted/modified.";
                return null;
            }
            return session;
        }
    }

    /** the values of one of the statistics charts of the journal */
    public class StatisticsSeries {

        /** one value per session, oldest first */
        public List<double> values = new List<double>();
        /** average of the ten newest sessions */
        public double lastTenAverage;
        public double min;
        public double max;

        /** from the values as the database returns them: newest session first */
        public static StatisticsSeries from(List<decimal> newestFirst) {
            StatisticsSeries ret = new StatisticsSeries();
            if (newestFirst.Count == 0) {
                return ret;
            }

            decimal sum = 0;
            int count = 0;
            decimal min = decimal.MaxValue;
            decimal max = decimal.MinValue;
            for (int i = newestFirst.Count - 1; i >= 0; i--) {
                ret.values.Add((double)Math.Round(newestFirst[i], 2));
                if (i < 10) {
                    sum += newestFirst[i];
                    count++;
                }
                min = Math.Min(min, newestFirst[i]);
                max = Math.Max(max, newestFirst[i]);
            }
            ret.lastTenAverage = (double)(sum / count);
            ret.min = (double)min;
            ret.max = (double)max;
            return ret;
        }
    }
}
