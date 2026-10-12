using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Data.Sqlite;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Security.Cryptography;
using System.IO;

namespace freETarget {
    public class StorageController {

        private const string dbFileName = "Storage.db";

        private string connString = null;
        private string templateConnString = null;
        private IAppHost mainWindow;

        private bool initiated = false;

        private readonly string dbPath;

        //the empty template database is installed next to the executable
        private static string getTemplateFile() {
            return Path.Combine(AppContext.BaseDirectory, dbFileName);
        }

        public static string getDefaultDBPath() {
            return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "freETarget", "database");
        }

        private static string getConnectionString(string file) {
            //no pooling: checkDB may overwrite the file right after closing a connection to it
            return new SqliteConnectionStringBuilder { DataSource = file, Pooling = false }.ToString();
        }

        public StorageController(IAppHost mainWindow) : this(mainWindow, getDefaultDBPath()) {
        }

        public StorageController(IAppHost mainWindow, string dbPath) {
            this.mainWindow = mainWindow;
            this.dbPath = dbPath;
            this.templateConnString = getConnectionString(getTemplateFile());
            try {
                string dbFile = Path.Combine(dbPath, dbFileName);
                if (Directory.Exists(dbPath) == false) {
                    Directory.CreateDirectory(dbPath);
                    File.Copy(getTemplateFile(), dbFile);
                    mainWindow.log("Database directory created at: " + dbPath);
                    mainWindow.displayMessage("Database directory created at: " + dbPath, false);
                } else if (File.Exists(dbFile) == false) {
                    //the directory exists, but someone deleted the DB file :(
                    File.Copy(getTemplateFile(), dbFile);
                    mainWindow.log("Database file copied at: " + dbPath);
                    mainWindow.displayMessage("Database file copied at: " + dbPath, false);
                }

                connString = getConnectionString(dbFile);

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

                SqliteConnection tempCon = new SqliteConnection(templateConnString);
                tempCon.Open();
                SqliteCommand cmdCon = new SqliteCommand("select version from Version", tempCon);
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

                SqliteConnection con = new SqliteConnection(connString);
                con.Open();
                SqliteCommand cmd = new SqliteCommand("select version from Version", con);
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

            string dbFile = Path.Combine(dbPath, dbFileName);
            if (userOK) {
                
                if (objCon != obj) {
                    bool overwrite = mainWindow.confirm("User database at '" + dbFile + "' is a different version than this installation requires." + Environment.NewLine + Environment.NewLine
                        + Environment.NewLine + "Press 'OK' to overwrite user database with the template from this version (empty database)" + Environment.NewLine + Environment.NewLine +
                        "Press 'Cancel' to exit the application without overwritting", "Database version problem");
                    if (!overwrite) {
                        mainWindow.log("User selected to cancel the database overwrite. Exiting app...");
                        Environment.Exit(0);
                    }

                    File.Copy(getTemplateFile(), dbFile, true);
                    mainWindow.log("Database version mismatch.  '" + dbFile + "' overwritten with local template");
                }

                mainWindow.displayMessage("User database at: " + dbPath, false);
                mainWindow.log("User database at: " + dbPath);
            } else {
                //user database not available
                if (templateOK && objCon==required_version) {
                    //template database is available. use that
                    connString = templateConnString;
                    mainWindow.showWarning("User database at '" + dbFile + "' is not accesible, maybe due to write access privileges needed by SQLite. " + Environment.NewLine
                        + "The template database in the installation folder will be used: " + Environment.NewLine + connString + Environment.NewLine + Environment.NewLine
                        + "This database will be overwritten when a new version of the program will be installed and all sessions will be lost. Be carefull.",
                         "Database problem");

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
            SqliteConnection con = new SqliteConnection(connString);
            con.Open();
            SqliteCommand cmd = new SqliteCommand("select distinct user from Sessions", con);
            SqliteDataReader rdr = cmd.ExecuteReader();
            List<string> ret = new List<string>();
            while (rdr.Read()) {
                ret.Add(rdr.GetString(0));
            }

            rdr.Close();
            con.Close();
            return ret;
        }

        public long checkEvents() {
            SqliteConnection con = new SqliteConnection(connString);
            con.Open();
            SqliteCommand cmd = new SqliteCommand("select count(*) " +
                "  from Events" +
                "  order by id asc", con);
            SqliteDataReader rdr = cmd.ExecuteReader();
            rdr.Read();
            long count = rdr.GetInt64(0);
            rdr.Close();
            con.Close();

            return count;
        }

        public List<Event> loadEvents() {
            List<Event> ret = new List<Event>();
            SqliteConnection con = new SqliteConnection(connString);
            con.Open();
            SqliteCommand cmd = new SqliteCommand("select * from Events", con);
            SqliteDataReader rdr = cmd.ExecuteReader();
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
                decimal rf_timePerShot = rdr.GetDecimal(17);
                decimal rf_pauseTime = rdr.GetDecimal(18);
                int rf_loadTime = rdr.GetInt32(19);

                targets.aTarget target_class = targets.aTarget.create(target, caliber);
                if (target_class == null) {
                    Console.WriteLine("Could not find target with name " + target);
                    continue;
                }

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
            SqliteConnection con = new SqliteConnection(connString);
            con.Open();
            SqliteCommand cmd = new SqliteCommand("select EventID from EventTabs order by rowid", con);
            SqliteDataReader rdr = cmd.ExecuteReader();
            while (rdr.Read()) {
                long id = rdr.GetInt64(0);
                ret.Add(id);
            }
            rdr.Close();
            con.Close();
            return ret;
        }

        public List<ListBoxSessionItem> findSessionsForUser(string user, Event cof) {
            SqliteConnection con = new SqliteConnection(connString);
            con.Open();
            SqliteCommand cmd = new SqliteCommand("select id, decimalScoring, score, decimalScore,innerX, startTime " +
                "  from Sessions where user = @user and courseOfFire = @cof " +
                "  order by id desc", con);
            addParam(cmd, "@cof", cof.ID);
            addParam(cmd, "@user", user);
            SqliteDataReader rdr = cmd.ExecuteReader();
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
            SqliteConnection con = new SqliteConnection(connString);
            con.Open();
            SqliteCommand cmd = new SqliteCommand("select averageScore " +
                "  from Sessions where user = @user and courseOfFire = @cof  order by id desc", con);
            addParam(cmd, "@cof", eventType.ID);
            addParam(cmd, "@user", user);
            SqliteDataReader rdr = cmd.ExecuteReader();

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
            SqliteConnection con = new SqliteConnection(connString);
            con.Open();
            SqliteCommand cmd = new SqliteCommand("select rbar " +
                "  from Sessions where user = @user and courseOfFire = @cof  order by id desc", con);
            addParam(cmd, "@cof", eventType.ID);
            addParam(cmd, "@user", user);
            SqliteDataReader rdr = cmd.ExecuteReader();

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
            SqliteConnection con = new SqliteConnection(connString);
            con.Open();
            SqliteCommand cmd = new SqliteCommand("select xbar " +
                "  from Sessions where user = @user and courseOfFire = @cof  order by id desc", con);
            addParam(cmd, "@cof", eventType.ID);
            addParam(cmd, "@user", user);
            SqliteDataReader rdr = cmd.ExecuteReader();

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
            SqliteConnection con = new SqliteConnection(connString);
            con.Open();
            SqliteCommand cmd = new SqliteCommand("select ybar " +
                "  from Sessions where user = @user and courseOfFire = @cof  order by id desc", con);
            addParam(cmd, "@cof", eventType.ID);
            addParam(cmd, "@user", user);
            SqliteDataReader rdr = cmd.ExecuteReader();

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
            SqliteConnection con = new SqliteConnection(connString);
            con.Open();
            SqliteCommand cmd = new SqliteCommand("select courseOfFire, numberOfShots, user, score, decimalScore,innerX, xBar, yBar, rBar," +
                " shots, startTime, endTime, averageScore, actualNumberOfShots, diary, averageShotDuration, longestShot, shortestShot, groupSize, hash " +
                "  from Sessions where id = @id", con);
            addParam(cmd, "@id", id);
            SqliteDataReader rdr = cmd.ExecuteReader();
            if (!rdr.Read()) {
                //no session with this id (deleted in the meantime)
                rdr.Close();
                con.Close();
                mainWindow.log("Could not find session with id " + id);
                return null;
            }

            Event cof = mainWindow.findEventByID(rdr.GetInt64(0));
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
            //older versions computed the group size wrongly, so the stored value is not used
            session.groupSize = Session.computeGroupSize(session.Shots);
            session.id = id;

            string hash = rdr.GetString(19);
            if(verifySessionHash(session, hash) == false) {
                mainWindow.showError("MD5 check failed. Session data corrupted/modified.", "Error loading session");
                rdr.Close();
                con.Close();
                return null;
            }

            rdr.Close();
            con.Close();
            return session;
        }

        public void updateDiary(long id, string text) {
            SqliteConnection con = new SqliteConnection(connString);
            con.Open();
            SqliteCommand cmd = con.CreateCommand();
            cmd.CommandText = "UPDATE Sessions set diary = @diary WHERE id = @id";
            addParam(cmd, "@id", id);
            addParam(cmd, "@diary", text);
            cmd.Prepare();

            cmd.ExecuteNonQuery();
            con.Close();
        }

        public void updateActiveEvents(List<Event> events) {
            SqliteConnection con = new SqliteConnection(connString);
            con.Open();
            SqliteCommand cmd = con.CreateCommand();
            cmd.CommandText = "DELETE FROM EventTabs";
            cmd.Prepare();
            cmd.ExecuteNonQuery();

            foreach (Event ev in events) {
                cmd.CommandText = "INSERT INTO EventTabs( EventID) VALUES (@ID)";
                cmd.Parameters.Clear(); //the command is reused for every event
                addParam(cmd, "@ID", ev.ID);
                cmd.Prepare();
                cmd.ExecuteNonQuery();

                Console.WriteLine("Event (" + ev.ID + ") " + ev.Name + " added in the EventTabs table");
            }
            con.Close();
        }

        public void storeSession(Session session, bool prepare) {
            SqliteConnection con = new SqliteConnection(connString);
            con.Open();
            SqliteCommand cmd = con.CreateCommand();
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
            addParam(cmd, "@courseOfFire", session.eventType.ID);
            addParam(cmd, "@targetType", session.targetType);
            addParam(cmd, "@numberOfShots", session.numberOfShots);
            addParam(cmd, "@decimalScoring", convertBoolToInt(session.decimalScoring));
            addParam(cmd, "@sessionType", session.sessionType);
            addParam(cmd, "@minutes", session.minutes);
            addParam(cmd, "@score", session.score);
            addParam(cmd, "@decimalScore", session.decimalScore);
            addParam(cmd, "@innerX", session.innerX);
            addParam(cmd, "@xBar", session.xbar);
            addParam(cmd, "@ybar", session.ybar);
            addParam(cmd, "@rbar", session.rbar);
            addParam(cmd, "@groupSize", session.groupSize);
            addParam(cmd, "@shots", convertListOfShotsToString(session.Shots));
            addParam(cmd, "@startTime", convertDatetimeToString(session.startTime));
            addParam(cmd, "@endTime", convertDatetimeToString(session.endTime));
            addParam(cmd, "@user", session.user);
            addParam(cmd, "@averageScore", session.averageScore.ToString("F2", CultureInfo.InvariantCulture));
            addParam(cmd, "@actualNumberOfShots", session.actualNumberOfShots);
            addParam(cmd, "@diary", session.diaryEntry);
            addParam(cmd, "@averageShotDuration", convertTimespanToDecimal(session.averageTimePerShot));
            addParam(cmd, "@longestShot", convertTimespanToDecimal(session.longestShot));
            addParam(cmd, "@shortestShot", convertTimespanToDecimal(session.shortestShot));
            addParam(cmd, "@hash", getSessionHash(session));

            try {
                cmd.Prepare();

                cmd.ExecuteNonQuery();

                mainWindow.log("~~~ Session saved ~~~");
            } catch(Exception ex){
                string s = "Error saving session to the database. Make sure you have write access to the folder." + Environment.NewLine + ex.Message;
                mainWindow.log(s);
                mainWindow.showError(s, "Error writing to DB");
            } finally {
                con.Close();
            }
        }

        public void updateSession(Session session) {
            SqliteConnection con = new SqliteConnection(connString);
            con.Open();
            SqliteCommand cmd = con.CreateCommand();
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
            
            addParam(cmd, "@courseOfFire", session.eventType.ID);
            addParam(cmd, "@targetType", session.targetType);
            addParam(cmd, "@numberOfShots", session.numberOfShots);
            addParam(cmd, "@decimalScoring", convertBoolToInt(session.decimalScoring));
            addParam(cmd, "@sessionType", session.sessionType);
            addParam(cmd, "@minutes", session.minutes);
            addParam(cmd, "@score", session.score);
            addParam(cmd, "@decimalScore", session.decimalScore);
            addParam(cmd, "@innerX", session.innerX);
            addParam(cmd, "@xBar", session.xbar);
            addParam(cmd, "@ybar", session.ybar);
            addParam(cmd, "@rbar", session.rbar);
            addParam(cmd, "@groupSize", session.groupSize);
            addParam(cmd, "@shots", convertListOfShotsToString(session.Shots));
            addParam(cmd, "@startTime", convertDatetimeToString(session.startTime));
            addParam(cmd, "@endTime", convertDatetimeToString(session.endTime));
            addParam(cmd, "@user", session.user);
            addParam(cmd, "@averageScore", session.averageScore.ToString("F2", CultureInfo.InvariantCulture));
            addParam(cmd, "@actualNumberOfShots", session.actualNumberOfShots);
            addParam(cmd, "@diary", session.diaryEntry);
            addParam(cmd, "@averageShotDuration", convertTimespanToDecimal(session.averageTimePerShot));
            addParam(cmd, "@longestShot", convertTimespanToDecimal(session.longestShot));
            addParam(cmd, "@shortestShot", convertTimespanToDecimal(session.shortestShot));
            addParam(cmd, "@hash", getSessionHash(session));
            addParam(cmd, "@id", session.id);

            try {
                cmd.Prepare();

                cmd.ExecuteNonQuery();

                Console.WriteLine("Session " + session.id + " updated");
            } catch (Exception ex) {
                string s = "Error updating session " + session.id + " to the database. Make sure you have write access to the folder." + Environment.NewLine + ex.Message;
                mainWindow.log(s);
                mainWindow.showError(s, "Error writing to DB");
            } finally {
                con.Close();
            }
        }


        public void storeEvent(Event ev){
            SqliteConnection con = new SqliteConnection(connString);
            con.Open();
            SqliteCommand cmd = con.CreateCommand();
            cmd.CommandText = "INSERT INTO Events(" +
                "Type, Name, DecimalScoring, Target, NumberOfShots, Minutes, Caliber, Final_NumberOfShotPerSeries, " +
                "Final_SeriesSeconds, Final_NumberOfShotsBeforeSingleShotSeries, Final_NumberOfShotsInSingleShotSeries, Final_SingleShotSeconds, Color, " +
                "RapidFire, RF_NumberOfShots, RF_TimePerSerie, RF_TimePerShot, RF_PauseTime, RF_LoadTime " +
                ") VALUES(@Type, @Name, @DecimalScoring, @Target, @NumberOfShots, @Minutes, @Caliber, @Final_NumberOfShotPerSeries," +
                "@Final_SeriesSeconds, @Final_NumberOfShotsBeforeSingleShotSeries, @Final_NumberOfShotsInSingleShotSeries, @Final_SingleShotSeconds, @Color, " +
                "@rapidFire, @rf_numberOfShots, @rf_timePerSerie, @rf_timePerShot, @rf_pauseTime, @rf_loadTime)";

            addParam(cmd, "@Type", ev.Type);
            addParam(cmd, "@Name", ev.Name);
            addParam(cmd, "@DecimalScoring", convertBoolToInt(ev.DecimalScoring));
            addParam(cmd, "@Target", ev.Target.getName());
            addParam(cmd, "@NumberOfShots", ev.NumberOfShots);
            addParam(cmd, "@Minutes", ev.Minutes);
            addParam(cmd, "@Caliber", ev.ProjectileCaliber);
            addParam(cmd, "@Final_NumberOfShotPerSeries", ev.Final_NumberOfShotPerSeries);
            addParam(cmd, "@Final_SeriesSeconds", ev.Final_SeriesSeconds);
            addParam(cmd, "@Final_NumberOfShotsBeforeSingleShotSeries", ev.Final_NumberOfShotsBeforeSingleShotSeries);
            addParam(cmd, "@Final_NumberOfShotsInSingleShotSeries", ev.Final_NumberOfShotsInSingleShotSeries);
            addParam(cmd, "@Final_SingleShotSeconds", ev.Final_SingleShotSeconds);
            addParam(cmd, "@Color", ev.TabColor.Name);
            addParam(cmd, "@rapidFire", convertBoolToInt(ev.RapidFire));
            addParam(cmd, "@rf_numberOfShots", ev.RF_NumberOfShots);
            addParam(cmd, "@rf_timePerSerie", ev.RF_TimePerSerie);
            addParam(cmd, "@rf_timePerShot", ev.RF_TimePerShot);
            addParam(cmd, "@rf_pauseTime", ev.RF_TimeBetweenShots);
            addParam(cmd, "@rf_loadTime", ev.RF_LoadTime);



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
            SqliteConnection con = new SqliteConnection(connString);
            con.Open();
            SqliteCommand cmd = con.CreateCommand();

            cmd.CommandText = "UPDATE Events SET " +
                "Type = @Type, Name = @Name, DecimalScoring = @DecimalScoring, Target = @Target, NumberOfShots = @NumberOfShots, Minutes = @Minutes, Caliber = @Caliber, " +
                "Final_NumberOfShotPerSeries = @Final_NumberOfShotPerSeries, Final_SeriesSeconds = @Final_SeriesSeconds, " +
                "Final_NumberOfShotsBeforeSingleShotSeries = @Final_NumberOfShotsBeforeSingleShotSeries, " +
                "Final_NumberOfShotsInSingleShotSeries = @Final_NumberOfShotsInSingleShotSeries, Final_SingleShotSeconds = @Final_SingleShotSeconds, Color = @Color, " +
                "RapidFire = @rapidFire, RF_NumberOfShots = @rf_numberOfShots, RF_TimePerSerie = @rf_timePerSerie, RF_TimePerShot = @rf_timePerShot, RF_PauseTime = @rf_pauseTime, RF_LoadTime = @rf_loadTime " +
                "WHERE id = @id ";

            addParam(cmd, "@Type", ev.Type);
            addParam(cmd, "@Name", ev.Name);
            addParam(cmd, "@DecimalScoring", convertBoolToInt(ev.DecimalScoring));
            addParam(cmd, "@Target", ev.Target.getName());
            addParam(cmd, "@NumberOfShots", ev.NumberOfShots);
            addParam(cmd, "@Minutes", ev.Minutes);
            addParam(cmd, "@Caliber", ev.ProjectileCaliber);
            addParam(cmd, "@Final_NumberOfShotPerSeries", ev.Final_NumberOfShotPerSeries);
            addParam(cmd, "@Final_SeriesSeconds", ev.Final_SeriesSeconds);
            addParam(cmd, "@Final_NumberOfShotsBeforeSingleShotSeries", ev.Final_NumberOfShotsBeforeSingleShotSeries);
            addParam(cmd, "@Final_NumberOfShotsInSingleShotSeries", ev.Final_NumberOfShotsInSingleShotSeries);
            addParam(cmd, "@Final_SingleShotSeconds", ev.Final_SingleShotSeconds);
            addParam(cmd, "@Color", ev.TabColor.Name);
            addParam(cmd, "@rapidFire", convertBoolToInt(ev.RapidFire));
            addParam(cmd, "@rf_numberOfShots", ev.RF_NumberOfShots);
            addParam(cmd, "@rf_timePerSerie", ev.RF_TimePerSerie);
            addParam(cmd, "@rf_timePerShot", ev.RF_TimePerShot);
            addParam(cmd, "@rf_pauseTime", ev.RF_TimeBetweenShots);
            addParam(cmd, "@rf_loadTime", ev.RF_LoadTime);

            addParam(cmd, "@id", ev.ID);


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
            SqliteConnection con = new SqliteConnection(connString);
            con.Open();
            SqliteCommand cmd = new SqliteCommand("select type, value " +
                "  from Settings where Name = @name ", con);
            addParam(cmd, "@name", name);
            SqliteDataReader rdr = cmd.ExecuteReader();

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
                        string knownName = sValue.Substring(sValue.IndexOf('[') + 1, sValue.IndexOf(']') - sValue.IndexOf('[') - 1);
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
            SqliteConnection con = new SqliteConnection(connString);
            con.Open();
            SqliteCommand cmd = con.CreateCommand();
            cmd.CommandText = "INSERT INTO Settings(" +
                "Name, Type, Value) VALUES(@Name, @Type, @Value)";

            addParam(cmd, "@Type", typ);
            addParam(cmd, "@Name", name);
            addParam(cmd, "@Value", value);

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
            SqliteConnection con = new SqliteConnection(connString);
            con.Open();
            SqliteCommand cmd = con.CreateCommand();
            cmd.CommandText = "UPDATE Settings SET Value=@Value WHERE Name=@Name";

            addParam(cmd, "@Name", name);
            addParam(cmd, "@Value", value);

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

        //the text whose hash is stored with a session to detect changes made outside of the application.
        //it must not depend on the language settings of the computer, or the session cannot be loaded on another one
        public static string getControlString(Session session) {
            return getControlString(session, session.decimalScore.ToString("F1", CultureInfo.InvariantCulture));
        }

        private static string getControlString(Session session, string decimalScore) {
            return session.score.ToString(CultureInfo.InvariantCulture) + "~" + decimalScore + "~" + session.innerX.ToString(CultureInfo.InvariantCulture) + "~"
                + convertListOfShotsToString(session.Shots) + "~" + session.user + "~" + session.actualNumberOfShots.ToString(CultureInfo.InvariantCulture);
        }

        //older versions wrote the decimal score the way the computer was set up when the session was saved: with a point, a comma or
        //another decimal separator, and with or without a trailing zero
        private static IEnumerable<string> getLegacyControlStrings(Session session) {
            HashSet<string> separators = new HashSet<string> { ".", ",", CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator };
            foreach (CultureInfo culture in CultureInfo.GetCultures(CultureTypes.AllCultures)) {
                separators.Add(culture.NumberFormat.NumberDecimalSeparator);
            }

            HashSet<string> scores = new HashSet<string>();
            foreach (string format in new string[] { "G", "F1", "0.#" }) {
                string invariant = session.decimalScore.ToString(format, CultureInfo.InvariantCulture);
                foreach (string separator in separators) {
                    scores.Add(invariant.Replace(".", separator));
                }
            }

            foreach (string score in scores) {
                yield return getControlString(session, score);
            }
        }

        public static string getSessionHash(Session session) {
            return GetMd5Hash(getControlString(session));
        }

        public static bool verifySessionHash(Session session, string hash) {
            if (VerifyMd5Hash(getControlString(session), hash)) {
                return true;
            }

            foreach (string legacy in getLegacyControlStrings(session)) {
                if (VerifyMd5Hash(legacy, hash)) {
                    return true;
                }
            }
            return false;
        }

        public void deleteSession(long id) {
            SqliteConnection con = new SqliteConnection(connString);
            con.Open();
        
            SqliteCommand cmd = con.CreateCommand();
            cmd.CommandText = "DELETE FROM Sessions WHERE id = @id";
            addParam(cmd, "@id", id);
            cmd.Prepare();
            cmd.ExecuteNonQuery();
            Console.WriteLine("Session " + id + "deleted ");
            con.Close();
        }

        //Microsoft.Data.Sqlite rejects null values and types it does not know.
        //System.Data.SQLite stored those as NULL and as their ToString() text, and the Settings table relies on that text format
        private static void addParam(SqliteCommand cmd, string name, object value) {
            if (value == null) {
                value = DBNull.Value;
            } else if (value is Type || value is System.Drawing.Color) {
                value = value.ToString();
            }
            cmd.Parameters.AddWithValue(name, value);
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
