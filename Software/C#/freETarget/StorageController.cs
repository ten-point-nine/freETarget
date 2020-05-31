using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Data.SQLite;
using System.Security.Cryptography.X509Certificates;
using System.Globalization;
using System.Runtime.CompilerServices;

namespace freETarget {
    class StorageController {

        private string connString = "Data Source=./Storage.db;";
        public StorageController() {
            
        }

        public List<string> findAllUsers() {
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand("select distinct user from Sessions", con);
            SQLiteDataReader rdr = cmd.ExecuteReader();
            List<string> ret = new List<string>();
            while (rdr.Read()) {
                ret.Add(rdr.GetString(0));
            }

            rdr.Close();
            con.Close();
            return ret;
        }

        public List<ListBoxSessionItem> findSessionsForUser(string user, EventType cof) {
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand("select id, decimalScoring, score, decimalScore,innerX, startTime " +
                "  from Sessions where user = @user and courseOfFire = @cof " +
                "  order by startTime desc", con);
            cmd.Parameters.AddWithValue("@cof", cof.Name);
            cmd.Parameters.AddWithValue("@user", user);
            SQLiteDataReader rdr = cmd.ExecuteReader();
            List<ListBoxSessionItem> ret = new List<ListBoxSessionItem>();
            while (rdr.Read()) {
                long id = rdr.GetInt64(0);
                int decimalScoring = rdr.GetInt32(1);
                int score = rdr.GetInt32(2);
                decimal decimalScore = rdr.GetDecimal(3);
                int innerX = rdr.GetInt32(4);
                DateTime date = convertStringToDate(rdr.GetString(5));

                if (decimalScoring == 0) {
                    ListBoxSessionItem item = new ListBoxSessionItem(date.ToString("yyyy-MM-dd"), score + "-" + innerX + "x", id);
                    ret.Add(item);
                } else {
                    ListBoxSessionItem item = new ListBoxSessionItem(date.ToString("yyyy-MM-dd"), decimalScore + "-" + innerX + "x", id);
                    ret.Add(item);
                }  
            }

            rdr.Close();
            con.Close();
            return ret;
        }

        public Session findSession(long id) {
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand("select courseOfFire, numberOfShots, user, score, decimalScore,innerX, xBar, yBar, rBar," +
                " shots, startTime, endTime, averageScore, actualNumberOfShots, diary, averageShotDuration, longestShot, shortestShot " +
                "  from Sessions where id = @id", con);
            cmd.Parameters.AddWithValue("@id", id);
            SQLiteDataReader rdr = cmd.ExecuteReader();
            rdr.Read();

            EventType cof = EventType.GetCourseOfFire(rdr.GetString(0));
            int numberOfShots = rdr.GetInt32(1);
            string user = rdr.GetString(2);

            Session session = Session.createNewSession(cof, user, numberOfShots);
            session.score = rdr.GetInt32(3);
            session.decimalScore = rdr.GetDecimal(4);
            session.innerX = rdr.GetInt32(5);
            session.xbar = rdr.GetDecimal(6);
            session.ybar = rdr.GetDecimal(7);
            session.rbar = rdr.GetDecimal(8);
            session.Shots = convertStringToListOfShots(rdr.GetString(9));
            session.startTime = convertStringToDate(rdr.GetString(10));
            session.endTime = convertStringToDate(rdr.GetString(11));
            session.averageScore = rdr.GetDecimal(12);
            session.actualNumberOfShots = rdr.GetInt32(13);
            session.diaryEntry = rdr.GetString(14);
            session.averageTimePerShot = convertDecimalToTimespan(rdr.GetDecimal(15));
            session.longestShot = convertDecimalToTimespan(rdr.GetDecimal(16));
            session.shortestShot = convertDecimalToTimespan(rdr.GetDecimal(17));
            session.id = id;

            rdr.Close();
            con.Close();
            return session;
        }

        public void updateDiary(long id, string text) {
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand(con);
            cmd.CommandText = "UPDATE Sessions set diary = @diary WHERE id = @id";
            cmd.Parameters.AddWithValue("@id", id);
            cmd.Parameters.AddWithValue("@diary", text);
            cmd.Prepare();

            cmd.ExecuteNonQuery();
            con.Close();
        }

        public void storeSession(Session session) {
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand(con);
            cmd.CommandText = "INSERT INTO Sessions(" +
                "courseOfFire, targetType, numberOfShots, decimalScoring, sessionType, minutes, score, decimalScore, " +
                "innerX, xBar, ybar, rbar, shots, startTime, endTime, user, averageScore, " +
                "actualNumberOfShots, diary, averageShotDuration, longestShot, shortestShot " +
                ") VALUES(@courseOfFire, @targetType, @numberOfShots, @decimalScoring, @sessionType, @minutes, @score, @decimalScore," +
                "@innerX, @xBar, @ybar, @rbar, @shots, @startTime, @endTime, @user, @averageScore, @actualNumberOfShots, @diary," +
                " @averageShotDuration, @longestShot, @shortestShot)";

            session.prepareForSaving();
            cmd.Parameters.AddWithValue("@courseOfFire", session.eventType.Name);
            cmd.Parameters.AddWithValue("@targetType", session.targetType);
            cmd.Parameters.AddWithValue("@numberOfShots", session.numberOfShots);
            cmd.Parameters.AddWithValue("@decimalScoring", convertBoolToInt(session.decimalScoring));
            cmd.Parameters.AddWithValue("@sessionType", session.sessionType);
            cmd.Parameters.AddWithValue("@minutes", session.minutes);
            cmd.Parameters.AddWithValue("@score", session.score);
            cmd.Parameters.AddWithValue("@decimalScore", session.decimalScore.ToString("F1", CultureInfo.InvariantCulture));
            cmd.Parameters.AddWithValue("@innerX", session.innerX);
            cmd.Parameters.AddWithValue("@xBar", session.xbar.ToString("F2", CultureInfo.InvariantCulture));
            cmd.Parameters.AddWithValue("@ybar", session.ybar.ToString("F2", CultureInfo.InvariantCulture));
            cmd.Parameters.AddWithValue("@rbar", session.rbar.ToString("F2", CultureInfo.InvariantCulture));
            cmd.Parameters.AddWithValue("@shots", convertListOfShotsToString(session.Shots));
            cmd.Parameters.AddWithValue("@startTime", convertDatetimeToString(session.startTime));
            cmd.Parameters.AddWithValue("@endTime", convertDatetimeToString(session.endTime));
            cmd.Parameters.AddWithValue("@user", session.user);
            cmd.Parameters.AddWithValue("@averageScore", session.averageScore.ToString("F2", CultureInfo.InvariantCulture));
            cmd.Parameters.AddWithValue("@actualNumberOfShots", session.actualNumberOfShots);
            cmd.Parameters.AddWithValue("@diary", session.diaryEntry);
            cmd.Parameters.AddWithValue("@averageShotDuration", convertTimespanToDecimal(session.averageTimePerShot));
            cmd.Parameters.AddWithValue("@longestShot", convertTimespanToDecimal(session.longestShot));
            cmd.Parameters.AddWithValue("@shortestShot", convertTimespanToDecimal(session.shortestShot));

            cmd.Prepare();

            cmd.ExecuteNonQuery();
            con.Close();
            Console.WriteLine("Session saved");
        }

        public void deleteSession(long id) {
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
        
            SQLiteCommand cmd = new SQLiteCommand(con);
            cmd.CommandText = "DELETE FROM Sessions WHERE id = @id";
            cmd.Parameters.AddWithValue("@id", id);
            cmd.Prepare();
            cmd.ExecuteNonQuery();
            Console.WriteLine("Session " + id + "deleted ");
            con.Close();
        }

        private int convertBoolToInt(bool input) {
            if (input) {
                return 1;
            } else {
                return 0;
            }
        }

        private decimal convertTimespanToDecimal(TimeSpan input) {
            return (decimal)input.TotalSeconds;
        }

        private TimeSpan convertDecimalToTimespan(decimal input) {
            return TimeSpan.FromSeconds((double)input);
        }

        private string convertDatetimeToString(DateTime input) {
            return input.ToString("yyyy-MM-dd hh:mm:ss");
        }

        private string convertListOfShotsToString(List<Shot> input) {
            string ret = "";
            foreach(Shot s in input) {
                ret += s.ToString() + "|";
            }
            return ret.Substring(0,ret.Length-1);
        }

        private DateTime convertStringToDate(string input) {
            return DateTime.Parse(input);
        }

        private List<Shot> convertStringToListOfShots(string input) {
            List<Shot> list = new List<Shot>();

            string[] stringShots = input.Split('|');
            foreach (string s in stringShots) {
                Shot shot = Shot.Parse(s);
                list.Add(shot);
            }
            return list;
        }
    }

    public class ListBoxSessionItem {
        public string date;
        public string score;
        public long id;

        public ListBoxSessionItem(string date, string score, long id) {
            this.date = date;
            this.score = score;
            this.id = id;
        }
        public override string ToString() {
            return date + "\t" + score;
        }
    }
}
