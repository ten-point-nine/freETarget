using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Data.SQLite;
using System.Security.Cryptography.X509Certificates;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Security.Cryptography;
using System.Windows.Forms;
using System.IO;

namespace freETarget {
    public class StorageController {

        private string connString = null;
        private string templateConnString = "Data Source=.\\Storage.db;";
        private frmMainWindow mainWindow;

        private bool initiated = false;

        private string getDBPath() {
            return Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments) + "\\freETarget\\database";
        }


        public StorageController(frmMainWindow mainWindow) {
            this.mainWindow = mainWindow;
            try {
                string dbPath = getDBPath();
                if (Directory.Exists(dbPath) == false) {
                    Directory.CreateDirectory(dbPath);
                    File.Copy(".\\Storage.db", dbPath + "\\Storage.db");
                    mainWindow.log("Database directory created at: " + dbPath);
                    mainWindow.displayMessage("Database directory created at: " + dbPath, false);
                } else if (File.Exists(dbPath + "\\Storage.db") == false) {
                    //the directory exists, but someone deleted the DB file :(
                    File.Copy(".\\Storage.db", dbPath + "\\Storage.db");
                    mainWindow.log("Database file copied at: " + dbPath);
                    mainWindow.displayMessage("Database file copied at: " + dbPath, false);
                }

                connString = "Data Source=" + dbPath + "\\Storage.db;";

                initiated = true;
            }catch(Exception ex) {
                mainWindow.log("Exception in StorageController constructor: " + ex.Message);
            }
        }

        public String checkDB(int required_version) {
            if (initiated == false) {
                return "StorageController constructor not initialized. Check the log.";
            }

            bool templateOK = false;
            bool userOK = false;

            int objCon=-1;
            int obj=-1;

            try {
                //step1: check if template database exists. if yes, get version

                SQLiteConnection tempCon = new SQLiteConnection(templateConnString);
                tempCon.Open();
                SQLiteCommand cmdCon = new SQLiteCommand("select version from Version", tempCon);
                objCon = Convert.ToInt32(cmdCon.ExecuteScalar());

                tempCon.Close();
                templateOK = true;
            } catch (Exception ex) {
                mainWindow.log("Template database exception: " + ex.Message +
                    Environment.NewLine + "Template connection string: " + templateConnString +
                    Environment.NewLine + "User connection string: " + connString +
                    Environment.NewLine + ex.ToString());
                //return ex.Message;
            }

            //step2: check if user database exists. if yes, get version from user database

            try {

                SQLiteConnection con = new SQLiteConnection(connString);
                con.Open();
                SQLiteCommand cmd = new SQLiteCommand("select version from Version", con);
                obj = Convert.ToInt32(cmd.ExecuteScalar());

                con.Close();
                userOK = true;
            } catch (Exception ex) {
                mainWindow.log("User database exception: " + ex.Message +
                    Environment.NewLine + "Template connection string: " + templateConnString +
                    Environment.NewLine + "User connection string: " + connString +
                    Environment.NewLine + ex.ToString());
                //return ex.Message;
            }



            mainWindow.log("Template database version = " + objCon + "   -   User database version = " + obj);

            //step3: compare versions. if different, copy template over user database

            string dbPath = getDBPath();
            if (userOK) {
                
                if (objCon != obj) {
                    DialogResult re =  MessageBox.Show("User database at '" + dbPath + "\\Storage.db' is a different version than this installation requires." + Environment.NewLine + Environment.NewLine
                        + Environment.NewLine + "Press 'OK' to overwrite user database with the template from this version (empty database)" + Environment.NewLine + Environment.NewLine +
                        "Press 'Cancel' to exit the application without overwritting", "Database version problem", MessageBoxButtons.OKCancel, MessageBoxIcon.Warning);
                    if (re != DialogResult.OK) {
                        mainWindow.log("User selected to cancel the database overwrite. Exiting app...");
                        Environment.Exit(0);
                    }

                    File.Copy(".\\Storage.db", dbPath + "\\Storage.db", true);
                    mainWindow.log("Database version mismatch.  '" + dbPath + "\\Storage.db' overwritten with local template");
                }

                mainWindow.displayMessage("User database at: " + dbPath, false);
                mainWindow.log("User database at: " + dbPath);
            } else {
                //user database not available
                if (templateOK && objCon==required_version) {
                    //template database is available. use that
                    connString = templateConnString;
                    MessageBox.Show("User database at '" + dbPath + "\\Storage.db' is not accesible, maybe due to write access privileges needed by SQLite. " + Environment.NewLine
                        + "The template database in the installation folder will be used: " + Environment.NewLine + connString + Environment.NewLine + Environment.NewLine
                        + "This database will be overwritten when a new version of the program will be installed and all sessions will be lost. Be carefull.",
                         "Database problem", MessageBoxButtons.OK, MessageBoxIcon.Warning
                        );

                    mainWindow.displayMessage("Using template database in the current/installation directory", false);
                    mainWindow.log("Using template database in the current/installation directory " + connString);
                } else {
                    // neither databases are available
                    mainWindow.displayMessage("Exception accessing both user and template databases. Check log for exact error", false);
                    mainWindow.log("Exception accessing both user and template databases. Check log for exact error" + Environment.NewLine + "Required version: " + required_version + " - template version: " + objCon);
                    return "Exception accessing both user and template databases. Check log for exact error";
                }
            }

            return null;
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

        public long checkEvents() {
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand("select count(*) " +
                "  from Events" +
                "  order by id asc", con);
            SQLiteDataReader rdr = cmd.ExecuteReader();
            rdr.Read();
            long count = rdr.GetInt64(0);
            rdr.Close();
            con.Close();

            return count;
        }

        public List<Event> loadEvents() {
            List<Event> ret = new List<Event>();
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand("select * from Events", con);
            SQLiteDataReader rdr = cmd.ExecuteReader();
            while (rdr.Read()) {
                long id = rdr.GetInt64(0);
                int type = rdr.GetInt32(1);
                string name = rdr.GetString(2);
                int decimalScoring = rdr.GetInt32(3);
                string target = rdr.GetString(4);
                int numberOfShots = rdr.GetInt32(5);
                int minutes = rdr.GetInt32(6);
                decimal caliber = rdr.GetDecimal(7);
                int final_seriesShots = rdr.GetInt32(8);
                int seriesSeconds = rdr.GetInt32(9);
                int shotsInSeries = rdr.GetInt32(10);
                int shotsInSingle = rdr.GetInt32(11);
                int singleSeconds = rdr.GetInt32(12);
                string colorText = rdr.GetString(13);
                int rapidFire = rdr.GetInt32(14);
                int rf_numberOfShots = rdr.GetInt32(15);
                int rf_timePerSerie = rdr.GetInt32(16);
                int rf_timePerShot = rdr.GetInt32(17);
                int rf_pauseTime = rdr.GetInt32(18);
                int rf_loadTime = rdr.GetInt32(19);

                Type target_type= Type.GetType(target);
                targets.aTarget target_class = (targets.aTarget)Activator.CreateInstance(target_type,new object[] { caliber });

                System.Drawing.Color color = System.Drawing.Color.FromName(colorText);
                Event e = new Event(id, name, convertIntToBool(decimalScoring), (Event.EventType)type, numberOfShots, target_class, minutes, caliber, final_seriesShots, seriesSeconds, shotsInSeries, shotsInSingle, singleSeconds, color,
                    convertIntToBool(rapidFire), rf_numberOfShots, rf_timePerSerie, rf_timePerShot, rf_pauseTime, rf_loadTime);
                ret.Add(e);
            }
            rdr.Close();
            con.Close();
            return ret;
        }

        public List<long> loadActiveEventsIDs() {
            List<long> ret = new List<long>();
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand("select EventID from EventTabs", con);
            SQLiteDataReader rdr = cmd.ExecuteReader();
            while (rdr.Read()) {
                long id = rdr.GetInt64(0);
                ret.Add(id);
            }
            rdr.Close();
            con.Close();
            return ret;
        }

        public List<ListBoxSessionItem> findSessionsForUser(string user, Event cof) {
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand("select id, decimalScoring, score, decimalScore,innerX, startTime " +
                "  from Sessions where user = @user and courseOfFire = @cof " +
                "  order by id desc", con);
            cmd.Parameters.AddWithValue("@cof", cof.ID);
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

        public List<decimal> findScoresForUser(string user, Event eventType) {
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand("select averageScore " +
                "  from Sessions where user = @user and courseOfFire = @cof  order by id desc", con);
            cmd.Parameters.AddWithValue("@cof", eventType.ID);
            cmd.Parameters.AddWithValue("@user", user);
            SQLiteDataReader rdr = cmd.ExecuteReader();

            List<decimal> ret = new List<decimal>();
            while (rdr.Read()) {
                decimal decimalScore = rdr.GetDecimal(0);
                ret.Add(decimalScore);
            }
            rdr.Close();
            con.Close();
            return ret;
        }

        public List<decimal> findRBarForUser(string user, Event eventType) {
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand("select rbar " +
                "  from Sessions where user = @user and courseOfFire = @cof  order by id desc", con);
            cmd.Parameters.AddWithValue("@cof", eventType.ID);
            cmd.Parameters.AddWithValue("@user", user);
            SQLiteDataReader rdr = cmd.ExecuteReader();

            List<decimal> ret = new List<decimal>();
            while (rdr.Read()) {
                decimal rBar = rdr.GetDecimal(0);
                ret.Add(rBar);
            }
            rdr.Close();
            con.Close();
            return ret;
        }

        public List<decimal> findXBarForUser(string user, Event eventType) {
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand("select xbar " +
                "  from Sessions where user = @user and courseOfFire = @cof  order by id desc", con);
            cmd.Parameters.AddWithValue("@cof", eventType.ID);
            cmd.Parameters.AddWithValue("@user", user);
            SQLiteDataReader rdr = cmd.ExecuteReader();

            List<decimal> ret = new List<decimal>();
            while (rdr.Read()) {
                decimal xBar = rdr.GetDecimal(0);
                ret.Add(xBar);
            }
            rdr.Close();
            con.Close();
            return ret;
        }

        public List<decimal> findYBarForUser(string user, Event eventType) {
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand("select ybar " +
                "  from Sessions where user = @user and courseOfFire = @cof  order by id desc", con);
            cmd.Parameters.AddWithValue("@cof", eventType.ID);
            cmd.Parameters.AddWithValue("@user", user);
            SQLiteDataReader rdr = cmd.ExecuteReader();

            List<decimal> ret = new List<decimal>();
            while (rdr.Read()) {
                decimal yBar = rdr.GetDecimal(0);
                ret.Add(yBar);
            }
            rdr.Close();
            con.Close();
            return ret;
        }

        public Session findSession(long id) {
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand("select courseOfFire, numberOfShots, user, score, decimalScore,innerX, xBar, yBar, rBar," +
                " shots, startTime, endTime, averageScore, actualNumberOfShots, diary, averageShotDuration, longestShot, shortestShot, groupSize, hash " +
                "  from Sessions where id = @id", con);
            cmd.Parameters.AddWithValue("@id", id);
            SQLiteDataReader rdr = cmd.ExecuteReader();
            rdr.Read();

            Event cof = mainWindow.eventManager.findEventByID(rdr.GetInt64(0));
            if (cof == null) {
                System.Diagnostics.StackTrace t = new System.Diagnostics.StackTrace();
                mainWindow.log("Could not find event with id " + rdr.GetInt64(0) + Environment.NewLine + t.ToString());
                return null;
            }

            int numberOfShots = rdr.GetInt32(1);
            string user = rdr.GetString(2);

            Session session = Session.createNewSession(cof, user);
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
            session.groupSize = rdr.GetDecimal(18);
            session.id = id;

            string hash = rdr.GetString(19);
            if(VerifyMd5Hash(getControlString(session), hash) == false) {
                MessageBox.Show("MD5 check failed. Session data corrupted/modified.","Error loading session",MessageBoxButtons.OK,MessageBoxIcon.Error);
                rdr.Close();
                con.Close();
                return null;
            }

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

        public void updateActiveEvents(List<Event> events) {
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand(con);
            cmd.CommandText = "DELETE FROM EventTabs";
            cmd.Prepare();
            cmd.ExecuteNonQuery();

            foreach (Event ev in events) {
                cmd.CommandText = "INSERT INTO EventTabs( EventID) VALUES (@ID)";
                cmd.Parameters.AddWithValue("@ID", ev.ID);
                cmd.Prepare();
                cmd.ExecuteNonQuery();

                Console.WriteLine("Event (" + ev.ID + ") " + ev.Name + " added in the EventTabs table");
            }
        }

        public void storeSession(Session session, bool prepare) {
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand(con);
            cmd.CommandText = "INSERT INTO Sessions(" +
                "courseOfFire, targetType, numberOfShots, decimalScoring, sessionType, minutes, score, decimalScore, " +
                "innerX, xBar, ybar, rbar, shots, startTime, endTime, user, averageScore, " +
                "actualNumberOfShots, diary, averageShotDuration, longestShot, shortestShot, groupSize, hash " +
                ") VALUES(@courseOfFire, @targetType, @numberOfShots, @decimalScoring, @sessionType, @minutes, @score, @decimalScore," +
                "@innerX, @xBar, @ybar, @rbar, @shots, @startTime, @endTime, @user, @averageScore, @actualNumberOfShots, @diary," +
                " @averageShotDuration, @longestShot, @shortestShot, @groupSize, @hash)";

            if (prepare) {
                session.prepareForSaving();
            }
            cmd.Parameters.AddWithValue("@courseOfFire", session.eventType.ID);
            cmd.Parameters.AddWithValue("@targetType", session.targetType);
            cmd.Parameters.AddWithValue("@numberOfShots", session.numberOfShots);
            cmd.Parameters.AddWithValue("@decimalScoring", convertBoolToInt(session.decimalScoring));
            cmd.Parameters.AddWithValue("@sessionType", session.sessionType);
            cmd.Parameters.AddWithValue("@minutes", session.minutes);
            cmd.Parameters.AddWithValue("@score", session.score);
            cmd.Parameters.AddWithValue("@decimalScore", session.decimalScore);
            cmd.Parameters.AddWithValue("@innerX", session.innerX);
            cmd.Parameters.AddWithValue("@xBar", session.xbar);
            cmd.Parameters.AddWithValue("@ybar", session.ybar);
            cmd.Parameters.AddWithValue("@rbar", session.rbar);
            cmd.Parameters.AddWithValue("@groupSize", session.groupSize);
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
            string controlString = getControlString(session);
            cmd.Parameters.AddWithValue("@hash", GetMd5Hash(controlString));

            try {
                cmd.Prepare();

                cmd.ExecuteNonQuery();

                Console.WriteLine("Session saved");
            } catch(Exception ex){
                string s = "Error saving session to the database. Make sure you have write access to the folder." + Environment.NewLine + ex.Message;
                mainWindow.log(s);
                MessageBox.Show(s, "Error writing to DB", MessageBoxButtons.OK, MessageBoxIcon.Error);
            } finally {
                con.Close();
            }
        }

        public void updateSession(Session session) {
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand(con);
            cmd.CommandText = "UPDATE Sessions SET " +
                "courseOfFire = @courseOfFire," +
                "targetType = @targetType, " +
                "numberOfShots = @numberOfShots, " +
                "decimalScoring = @decimalScoring, " +
                "sessionType = @sessionType, " +
                "minutes = @minutes, " +
                "score = @score, " +
                "decimalScore = @decimalScore, " +
                "innerX = @innerX, " +
                "xBar = @xBar, " +
                "ybar = @ybar, " +
                "rbar = @rbar, " +
                "shots = @shots, " +
                "startTime = @startTime, " +
                "endTime = @endTime, " +
                "user = @user, " +
                "averageScore = @averageScore, " +
                "actualNumberOfShots = @actualNumberOfShots, " +
                "diary = @diary, " +
                "averageShotDuration = @averageShotDuration, " +
                "longestShot = @longestShot, " +
                "shortestShot = @shortestShot, " +
                "groupSize = @groupSize, " +
                "hash = @hash " +
                "WHERE id = @id ";


            session.prepareForSaving();
            
            cmd.Parameters.AddWithValue("@courseOfFire", session.eventType.ID);
            cmd.Parameters.AddWithValue("@targetType", session.targetType);
            cmd.Parameters.AddWithValue("@numberOfShots", session.numberOfShots);
            cmd.Parameters.AddWithValue("@decimalScoring", convertBoolToInt(session.decimalScoring));
            cmd.Parameters.AddWithValue("@sessionType", session.sessionType);
            cmd.Parameters.AddWithValue("@minutes", session.minutes);
            cmd.Parameters.AddWithValue("@score", session.score);
            cmd.Parameters.AddWithValue("@decimalScore", session.decimalScore);
            cmd.Parameters.AddWithValue("@innerX", session.innerX);
            cmd.Parameters.AddWithValue("@xBar", session.xbar);
            cmd.Parameters.AddWithValue("@ybar", session.ybar);
            cmd.Parameters.AddWithValue("@rbar", session.rbar);
            cmd.Parameters.AddWithValue("@groupSize", session.groupSize);
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
            string controlString = getControlString(session);
            cmd.Parameters.AddWithValue("@hash", GetMd5Hash(controlString));
            cmd.Parameters.AddWithValue("@id", session.id);

            try {
                cmd.Prepare();

                cmd.ExecuteNonQuery();

                Console.WriteLine("Session " + session.id + " updated");
            } catch (Exception ex) {
                string s = "Error updating session " + session.id + " to the database. Make sure you have write access to the folder." + Environment.NewLine + ex.Message;
                mainWindow.log(s);
                MessageBox.Show(s, "Error writing to DB", MessageBoxButtons.OK, MessageBoxIcon.Error);
            } finally {
                con.Close();
            }
        }


        public void storeEvent(Event ev){
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand(con);
            cmd.CommandText = "INSERT INTO Events(" +
                "Type, Name, DecimalScoring, Target, NumberOfShots, Minutes, Caliber, Final_NumberOfShotPerSeries, " +
                "Final_SeriesSeconds, Final_NumberOfShotsBeforeSingleShotSeries, Final_NumberOfShotsInSingleShotSeries, Final_SingleShotSeconds, Color, " +
                "RapidFire, RF_NumberOfShots, RF_TimePerSerie, RF_TimePerShot, RF_PauseTime, RF_LoadTime " +
                ") VALUES(@Type, @Name, @DecimalScoring, @Target, @NumberOfShots, @Minutes, @Caliber, @Final_NumberOfShotPerSeries," +
                "@Final_SeriesSeconds, @Final_NumberOfShotsBeforeSingleShotSeries, @Final_NumberOfShotsInSingleShotSeries, @Final_SingleShotSeconds, @Color, " +
                "@rapidFire, @rf_numberOfShots, @rf_timePerSerie, @rf_timePerShot, @rf_pauseTime, @rf_loadTime)";

            cmd.Parameters.AddWithValue("@Type", ev.Type);
            cmd.Parameters.AddWithValue("@Name", ev.Name);
            cmd.Parameters.AddWithValue("@DecimalScoring", convertBoolToInt(ev.DecimalScoring));
            cmd.Parameters.AddWithValue("@Target", ev.Target.getName());
            cmd.Parameters.AddWithValue("@NumberOfShots", ev.NumberOfShots);
            cmd.Parameters.AddWithValue("@Minutes", ev.Minutes);
            cmd.Parameters.AddWithValue("@Caliber", ev.ProjectileCaliber);
            cmd.Parameters.AddWithValue("@Final_NumberOfShotPerSeries", ev.Final_NumberOfShotPerSeries);
            cmd.Parameters.AddWithValue("@Final_SeriesSeconds", ev.Final_SeriesSeconds);
            cmd.Parameters.AddWithValue("@Final_NumberOfShotsBeforeSingleShotSeries", ev.Final_NumberOfShotsBeforeSingleShotSeries);
            cmd.Parameters.AddWithValue("@Final_NumberOfShotsInSingleShotSeries", ev.Final_NumberOfShotsInSingleShotSeries);
            cmd.Parameters.AddWithValue("@Final_SingleShotSeconds", ev.Final_SingleShotSeconds);
            cmd.Parameters.AddWithValue("@Color", ev.TabColor.Name);
            cmd.Parameters.AddWithValue("@rapidFire", convertBoolToInt(ev.RapidFire));
            cmd.Parameters.AddWithValue("@rf_numberOfShots", ev.RF_NumberOfShots);
            cmd.Parameters.AddWithValue("@rf_timePerSerie", ev.RF_TimePerSerie);
            cmd.Parameters.AddWithValue("@rf_timePerShot", ev.RF_TimePerShot);
            cmd.Parameters.AddWithValue("@rf_pauseTime", ev.RF_TimeBetweenShots);
            cmd.Parameters.AddWithValue("@rf_loadTime", ev.RF_LoadTime);



            try {
                cmd.Prepare();

                cmd.ExecuteNonQuery();

                Console.WriteLine("Event saved");
            } catch (Exception ex) {
                mainWindow.log("Error storing event: " + ex.Message);
            } finally {
                con.Close();
            }

        }

        public void updateEvent(Event ev) {
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand(con);

            cmd.CommandText = "UPDATE Events SET " +
                "Type = @Type, Name = @Name, DecimalScoring = @DecimalScoring, Target = @Target, NumberOfShots = @NumberOfShots, Minutes = @Minutes, Caliber = @Caliber, " +
                "Final_NumberOfShotPerSeries = @Final_NumberOfShotPerSeries, Final_SeriesSeconds = @Final_SeriesSeconds, " +
                "Final_NumberOfShotsBeforeSingleShotSeries = @Final_NumberOfShotsBeforeSingleShotSeries, " +
                "Final_NumberOfShotsInSingleShotSeries = @Final_NumberOfShotsInSingleShotSeries, Final_SingleShotSeconds = @Final_SingleShotSeconds, Color = @Color, " +
                "RapidFire = @rapidFire, RF_NumberOfShots = @rf_numberOfShots, RF_TimePerSerie = @rf_timePerSerie, RF_TimePerShot = @rf_timePerShot, RF_PauseTime = @rf_pauseTime, RF_LoadTime = @rf_loadTime " +
                "WHERE id = @id ";

            cmd.Parameters.AddWithValue("@Type", ev.Type);
            cmd.Parameters.AddWithValue("@Name", ev.Name);
            cmd.Parameters.AddWithValue("@DecimalScoring", convertBoolToInt(ev.DecimalScoring));
            cmd.Parameters.AddWithValue("@Target", ev.Target.getName());
            cmd.Parameters.AddWithValue("@NumberOfShots", ev.NumberOfShots);
            cmd.Parameters.AddWithValue("@Minutes", ev.Minutes);
            cmd.Parameters.AddWithValue("@Caliber", ev.ProjectileCaliber);
            cmd.Parameters.AddWithValue("@Final_NumberOfShotPerSeries", ev.Final_NumberOfShotPerSeries);
            cmd.Parameters.AddWithValue("@Final_SeriesSeconds", ev.Final_SeriesSeconds);
            cmd.Parameters.AddWithValue("@Final_NumberOfShotsBeforeSingleShotSeries", ev.Final_NumberOfShotsBeforeSingleShotSeries);
            cmd.Parameters.AddWithValue("@Final_NumberOfShotsInSingleShotSeries", ev.Final_NumberOfShotsInSingleShotSeries);
            cmd.Parameters.AddWithValue("@Final_SingleShotSeconds", ev.Final_SingleShotSeconds);
            cmd.Parameters.AddWithValue("@Color", ev.TabColor.Name);
            cmd.Parameters.AddWithValue("@rapidFire", convertBoolToInt(ev.RapidFire));
            cmd.Parameters.AddWithValue("@rf_numberOfShots", ev.RF_NumberOfShots);
            cmd.Parameters.AddWithValue("@rf_timePerSerie", ev.RF_TimePerSerie);
            cmd.Parameters.AddWithValue("@rf_timePerShot", ev.RF_TimePerShot);
            cmd.Parameters.AddWithValue("@rf_pauseTime", ev.RF_TimeBetweenShots);
            cmd.Parameters.AddWithValue("@rf_loadTime", ev.RF_LoadTime);

            cmd.Parameters.AddWithValue("@id", ev.ID);


            try {
                cmd.Prepare();

                cmd.ExecuteNonQuery();

                Console.WriteLine("Event saved");
            } catch (Exception ex) {
                mainWindow.log("Error storing event: " + ex.Message);
            } finally {
                con.Close();
            }
        }

        public object getSetting(string name) {
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand("select type, value " +
                "  from Settings where Name = @name ", con);
            cmd.Parameters.AddWithValue("@name", name);
            SQLiteDataReader rdr = cmd.ExecuteReader();

            object ret = null;
            string sType=null;
            string sValue = null;
            while (rdr.Read()) {
                sType = rdr.GetString(0);
                sValue = rdr.GetString(1);
            }

            if (sType != null) {
                //a setting with this name was found
                switch (sType) {
                    case "System.String":
                        ret = sValue;
                        break;
                    case "System.Int32":
                        ret = Int32.Parse(sValue, CultureInfo.InvariantCulture);
                        break;
                    case "System.Boolean":
                        if(sValue == "0") {
                            ret = false;
                        } else {
                            ret = true;
                        }           
                        break;
                    case "System.Drawing.Color":
                        string knownName = sValue.Substring(sValue.IndexOf("[") + 1, sValue.IndexOf("]") - sValue.IndexOf("[") - 1);
                        ret = System.Drawing.Color.FromName(knownName);
                        break;
                    case "System.Decimal":
                        ret = Decimal.Parse(sValue, CultureInfo.InvariantCulture);
                        break;
                }
            }


            rdr.Close();
            con.Close();
            return ret;

        }

        public void storeSetting(string name, Type typ, object value) {
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand(con);
            cmd.CommandText = "INSERT INTO Settings(" +
                "Name, Type, Value) VALUES(@Name, @Type, @Value)";

            cmd.Parameters.AddWithValue("@Type", typ);
            cmd.Parameters.AddWithValue("@Name", name);
            cmd.Parameters.AddWithValue("@Value", value);

            try {
                cmd.Prepare();

                cmd.ExecuteNonQuery();

                Console.WriteLine("Setting " + name +" saved");
            } catch (Exception ex) {
                mainWindow.log("Error storing setting '" + name + "':" + ex.Message);
            } finally {
                con.Close();
            }

        }

        public void updateSetting(string name, object value) {
            SQLiteConnection con = new SQLiteConnection(connString);
            con.Open();
            SQLiteCommand cmd = new SQLiteCommand(con);
            cmd.CommandText = "UPDATE Settings SET Value=@Value WHERE Name=@Name";

            cmd.Parameters.AddWithValue("@Name", name);
            cmd.Parameters.AddWithValue("@Value", value);

            try {
                cmd.Prepare();

                cmd.ExecuteNonQuery();

                //Console.WriteLine("Setting " + name + " updated");
            } catch (Exception ex) {
                mainWindow.log("Error updating setting '" + name + "':" + ex.Message);
            } finally {
                con.Close();
            }

        }

        public static string getControlString(Session session) {
            return session.score + "~" + session.decimalScore + "~" + session.innerX + "~" + convertListOfShotsToString(session.Shots) + "~" + session.user + "~" + session.actualNumberOfShots;
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

        private bool convertIntToBool(int input) {
            if (input == 0) {
                return false;
            } else {
                return true;
            }
        }

        public static decimal convertTimespanToDecimal(TimeSpan input) {
            return (decimal)input.TotalSeconds;
        }

        public static TimeSpan convertDecimalToTimespan(decimal input) {
            return TimeSpan.FromSeconds((double)input);
        }

        public static string convertDatetimeToString(DateTime input) {
            return input.ToString("yyyy-MM-dd HH:mm:ss");
        }

        public static string convertListOfShotsToString(List<Shot> input) {
            string ret = "";
            foreach(Shot s in input) {
                ret += s.ToString() + "|";
            }
            return ret.Substring(0,ret.Length-1);
        }

        public static DateTime convertStringToDate(string input) {
            return DateTime.Parse(input);
        }

        public static List<Shot> convertStringToListOfShots(string input) {
            List<Shot> list = new List<Shot>();

            string[] stringShots = input.Split('|');
            foreach (string s in stringShots) {
                Shot shot = Shot.parse(s);
                list.Add(shot);
            }
            return list;
        }


        public static string GetMd5Hash(string input) {
            MD5 md5Hash = MD5.Create();

            // Convert the input string to a byte array and compute the hash.
            byte[] data = md5Hash.ComputeHash(Encoding.UTF8.GetBytes(input));

            // Create a new Stringbuilder to collect the bytes
            // and create a string.
            StringBuilder sBuilder = new StringBuilder();

            // Loop through each byte of the hashed data
            // and format each one as a hexadecimal string.
            for (int i = 0; i < data.Length; i++) {
                sBuilder.Append(data[i].ToString("x2"));
            }

            // Return the hexadecimal string.
            return sBuilder.ToString();
        }

        // Verify a hash against a string.
        public static bool VerifyMd5Hash(string input, string hash) {
            MD5 md5Hash = MD5.Create();

            // Hash the input.
            string hashOfInput = GetMd5Hash(input);

            // Create a StringComparer an compare the hashes.
            StringComparer comparer = StringComparer.OrdinalIgnoreCase;

            if (0 == comparer.Compare(hashOfInput, hash)) {
                return true;
            } else {
                return false;
            }
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
            return date + " (" + id + ")" + "\t" + score;
        }
    }
}
