using freETarget.Properties;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using Xunit;

namespace freETarget.Tests {

    public class StorageTests : IDisposable {

        private readonly TempDir dir = new TempDir();
        private readonly FakeHost host = new FakeHost();
        private readonly StorageController storage;

        public StorageTests() {
            Make.resetSettings();
            storage = new StorageController(host, dir.path);
            host.eventManager = new EventManager(storage);
        }

        public void Dispose() {
            Make.resetSettings();
            dir.Dispose();
        }

        private void loadEvents() {
            host.eventManager.initializeEvents();
            host.eventManager.setEventsList(storage.loadEvents());
            host.eventManager.setActiveEventsList(storage.loadActiveEventsIDs());
        }

        [Fact]
        public void first_start_copies_the_template_database() {
            Assert.True(File.Exists(Path.Combine(dir.path, "Storage.db")));
            Assert.Null(storage.checkDB(4));
            Assert.Empty(host.warnings);
        }

        [Fact]
        public void default_events_are_created_and_loaded() {
            Assert.Equal(0, storage.checkEvents());

            loadEvents();

            List<Event> events = host.eventManager.getEventsList();
            Assert.Equal(new[] { "Pistol Practice", "Pistol Match", "Pistol Final", "Rifle Practice", "Rifle Match", "Rifle Final" }, events.Select(e => e.Name));
            Assert.Equal(6, host.eventManager.getActiveEventsList().Count);

            Event final = host.eventManager.findEventByName("Rifle Final");
            Assert.IsType<targets.AirRifle>(final.Target);
            Assert.Equal(Event.EventType.Final, final.Type);
            Assert.True(final.DecimalScoring);
            Assert.Equal(24, final.NumberOfShots);
            Assert.Equal(4.5m, final.ProjectileCaliber);
            Assert.Equal(5, final.Final_NumberOfShotPerSeries);
            Assert.Equal(Color.DodgerBlue, final.TabColor);
            Assert.False(final.RapidFire);
        }

        [Fact]
        public void event_with_decimal_rapid_fire_times_survives_storing() {
            using (new Culture("de-DE")) {
                storage.storeEvent(Make.rapidFireEvent(5, -1, 3.5m, 7.25m, 60));

                Event loaded = storage.loadEvents().Single();

                Assert.True(loaded.RapidFire);
                Assert.Equal(5, loaded.RF_NumberOfShots);
                Assert.Equal(3.5m, loaded.RF_TimePerShot);
                Assert.Equal(7.25m, loaded.RF_TimeBetweenShots);
                Assert.Equal(60, loaded.RF_LoadTime);
            }
        }

        [Theory]
        [InlineData("en-US")]
        [InlineData("de-DE")]
        public void session_survives_storing(string culture) {
            using (new Culture(culture)) {
                loadEvents();
                Settings.Default.name = "tester";
                Event ev = host.eventManager.findEventByName("Pistol Practice");

                Session session = Session.createNewSession(ev, "tester");
                session.startTime = new DateTime(2026, 10, 12, 10, 0, 0);
                session.addShot(Make.shot(0, 0, 1));
                session.addShot(Make.shot(8.01m, 0, 2));
                session.addShot(Make.shot(-3.3m, 16.2m, 3));
                session.diaryEntry = "windy";
                storage.storeSession(session, true);

                Assert.Empty(host.errors);
                Assert.Equal(new[] { "tester" }, storage.findAllUsers());
                ListBoxSessionItem item = storage.findSessionsForUser("tester", ev).Single();

                Session loaded = storage.findSession(item.id);

                Assert.Empty(host.errors); //no "MD5 check failed"
                Assert.NotNull(loaded);
                Assert.Equal(item.id, loaded.id);
                Assert.Equal(session.score, loaded.score);
                Assert.Equal(session.decimalScore, loaded.decimalScore);
                Assert.Equal(1, loaded.innerX);
                Assert.Equal(3, loaded.actualNumberOfShots);
                Assert.Equal("windy", loaded.diaryEntry);
                Assert.Equal(session.startTime, loaded.startTime);
                Assert.Equal(new[] { 10.9m, 9.9m, 8.9m }, loaded.Shots.Select(s => s.decimalScore));
                Assert.Equal(-3.3m, loaded.Shots[2].getX());
                Assert.Equal(TimeSpan.FromSeconds(30), loaded.Shots[1].shotDuration);
            }
        }

        private Session storedSession(params Shot[] shots) {
            loadEvents();
            Settings.Default.name = "tester";
            Event ev = host.eventManager.findEventByName("Rifle Practice"); //decimal scoring
            Session session = Session.createNewSession(ev, "tester");
            session.startTime = new DateTime(2026, 10, 12, 10, 0, 0);
            foreach (Shot shot in shots) {
                session.addShot(shot);
            }
            storage.storeSession(session, true);
            session.id = storage.findSessionsForUser("tester", ev).Single().id;
            return session;
        }

        [Fact]
        public void session_saved_with_a_decimal_comma_loads_with_a_decimal_point_and_the_other_way_round() {
            long id;
            using (new Culture("de-DE")) {
                id = storedSession(Make.shot(0, 0, 1), Make.shot(8.01m, 0, 2)).id;
            }

            using (new Culture("en-US")) {
                Assert.NotNull(storage.findSession(id));
            }
            using (new Culture("fr-FR")) {
                Assert.NotNull(storage.findSession(id));
            }
            Assert.Empty(host.errors);
        }

        [Theory]
        [InlineData("20,8")]   //saved by an older version on a computer with a decimal comma
        [InlineData("20.8")]   //... with a decimal point
        public void sessions_saved_by_older_versions_still_pass_the_check(string decimalScore) {
            Session session = storedSession(Make.shot(0, 0, 1), Make.shot(8.01m, 0, 2));
            string oldControlString = "19~" + decimalScore + "~1~" + StorageController.convertListOfShotsToString(session.Shots) + "~tester~2";
            string oldHash = StorageController.GetMd5Hash(oldControlString);

            Assert.True(StorageController.verifySessionHash(session, oldHash));
        }

        [Fact]
        public void old_session_with_a_whole_number_as_decimal_score_passes_the_check() {
            Session session = storedSession(Make.shot(8, 0, 1), Make.shot(8, 0, 2)); //10.0 + 10.0
            string shots = StorageController.convertListOfShotsToString(session.Shots);

            Assert.True(StorageController.verifySessionHash(session, StorageController.GetMd5Hash("20~20,0~0~" + shots + "~tester~2")));
            Assert.True(StorageController.verifySessionHash(session, StorageController.GetMd5Hash("20~20~0~" + shots + "~tester~2")));
        }

        [Fact]
        public void changed_session_fails_the_check() {
            Session session = storedSession(Make.shot(0, 0, 1), Make.shot(8.01m, 0, 2));
            string hash = StorageController.getSessionHash(session);

            session.score += 1;
            Assert.False(StorageController.verifySessionHash(session, hash));

            session.score -= 1;
            session.decimalScore += 0.1m;
            Assert.False(StorageController.verifySessionHash(session, hash));
        }

        [Fact]
        public void wrong_group_size_stored_by_older_versions_is_corrected_when_loading() {
            Session session = storedSession(Make.shot(0, 0, 1), Make.shot(7, 7, 2));
            using (var con = new Microsoft.Data.Sqlite.SqliteConnection(new Microsoft.Data.Sqlite.SqliteConnectionStringBuilder { DataSource = Path.Combine(dir.path, "Storage.db"), Pooling = false }.ToString())) {
                con.Open();
                var cmd = con.CreateCommand();
                cmd.CommandText = "update Sessions set groupSize = 0"; //what the old formula computed for these two shots
                cmd.ExecuteNonQuery();
            }

            Assert.Equal(9.9m, storage.findSession(session.id).groupSize);
        }

        [Fact]
        public void stored_session_can_be_updated_and_deleted() {
            loadEvents();
            Settings.Default.name = "tester";
            Event ev = host.eventManager.findEventByName("Pistol Practice");
            Session session = Session.createNewSession(ev, "tester");
            session.startTime = new DateTime(2026, 10, 12, 10, 0, 0);
            session.addShot(Make.shot(0, 0, 1));
            storage.storeSession(session, true);
            long id = storage.findSessionsForUser("tester", ev).Single().id;

            Session loaded = storage.findSession(id);
            loaded.addShot(Make.shot(8, 0, 2));
            storage.updateSession(loaded);
            storage.updateDiary(id, "second thoughts");

            Session again = storage.findSession(id);
            Assert.Empty(host.errors);
            Assert.Equal(2, again.Shots.Count);
            Assert.Equal(20, again.score);
            Assert.Equal("second thoughts", again.diaryEntry);

            storage.deleteSession(id);
            Assert.Empty(storage.findSessionsForUser("tester", ev));
        }

        [Fact]
        public void settings_survive_storing() {
            using (new Culture("de-DE")) {
                Settings settings = new Settings();
                SettingsStore.init(storage, settings); //first start: rows with the defaults

                settings.name = "Marvin";
                settings.baudRate = 9600;
                settings.voiceCommands = false;
                settings.scoreVoice = true;
                settings.targetDistance = 50.5m;
                settings.calibrationX = -1.25m;
                settings.targetColor = Color.Red;
                settings.Board = "ESP32";
                SettingsStore.save(storage, settings);

                Settings loaded = new Settings();
                SettingsStore.load(storage, loaded);

                foreach (var p in typeof(Settings).GetProperties().Where(p => p.CanWrite && !p.GetMethod.IsStatic)) {
                    Assert.True(Equals(p.GetValue(settings), p.GetValue(loaded)), p.Name);
                }
                Assert.DoesNotContain(host.logs, l => l.StartsWith("Error"));
            }
        }

        [Fact]
        public void settings_added_by_a_new_version_get_their_default() {
            Settings settings = new Settings();

            SettingsStore.init(storage, settings);
            SettingsStore.init(storage, settings); //second start must not add the rows again

            Settings loaded = new Settings { baudRate = 1 };
            SettingsStore.load(storage, loaded);
            Assert.Equal(115200, loaded.baudRate);
            Assert.Equal(Color.Moccasin, loaded.targetColor);
        }

        [Fact]
        public void setting_names_are_the_ones_existing_databases_use() {
            string[] expected = {
                "Board", "CommProtocol", "OnlySeries", "TcpIP", "TcpPort", "baudRate", "calibrationAngle", "calibrationX", "calibrationY", "defaultTarget",
                "displayDebugConsole", "drawMeanGroup", "fileLogging", "ignoreMiss", "name", "pdfPath", "portName", "score10BackgroundColor", "score10PenColor",
                "score9BackgroundColor", "score9PenColor", "scoreDefaultBackgroundColor", "scoreDefaultPenColor", "scoreOldBackgroundColor", "scoreOldPenColor",
                "scoreVoice", "showScoring", "targetColor", "targetDistance", "theme", "uploaderLoc", "voiceCommands" };

            var actual = typeof(Settings).GetProperties().Where(p => p.CanWrite && !p.GetMethod.IsStatic).Select(p => p.Name).OrderBy(n => n, StringComparer.Ordinal);

            Assert.Equal(expected.OrderBy(n => n, StringComparer.Ordinal), actual);
        }

        [Fact]
        public void database_of_another_version_is_replaced_by_the_empty_template_when_the_user_agrees() {
            loadEvents();
            Assert.Null(storage.checkDB(4)); //same version: nothing is asked, nothing is lost
            Assert.Equal(6, storage.checkEvents());

            using (var con = new Microsoft.Data.Sqlite.SqliteConnection(new Microsoft.Data.Sqlite.SqliteConnectionStringBuilder { DataSource = Path.Combine(dir.path, "Storage.db"), Pooling = false }.ToString())) {
                con.Open();
                var cmd = con.CreateCommand();
                cmd.CommandText = "update Version set version = 3";
                cmd.ExecuteNonQuery();
            }

            host.confirmAnswer = true;
            Assert.Null(storage.checkDB(4));

            Assert.Equal(0, storage.checkEvents());
            Assert.Contains(host.logs, l => l.StartsWith("Database version mismatch"));
        }

        [Fact]
        public void unusable_database_folder_is_reported() {
            string notAFolder = Path.Combine(dir.path, "Storage.db", "sub"); //below a file

            Assert.NotNull(new StorageController(host, notAFolder).checkDB(4));
        }

        [Fact]
        public void active_events_keep_the_order_the_user_gave_them() {
            loadEvents();
            List<Event> events = host.eventManager.getEventsList();

            storage.updateActiveEvents(new List<Event> { events[4], events[0], events[2] });
            host.eventManager.setActiveEventsList(storage.loadActiveEventsIDs());

            Assert.Equal(new[] { "Rifle Match", "Pistol Practice", "Pistol Final" }, host.eventManager.getActiveEventsList().Select(e => e.Name));
        }

        [Fact]
        public void event_can_be_changed() {
            loadEvents();
            Event ev = host.eventManager.findEventByName("Pistol Match");
            ev.Name = "Club Match";
            ev.NumberOfShots = 40;
            ev.Target = new targets.AirRifle(4.5m);
            ev.TabColor = Color.Purple;

            storage.updateEvent(ev);

            Event loaded = storage.loadEvents().Single(e => e.ID == ev.ID);
            Assert.Equal("Club Match", loaded.Name);
            Assert.Equal(40, loaded.NumberOfShots);
            Assert.IsType<targets.AirRifle>(loaded.Target);
            Assert.Equal(Color.Purple, loaded.TabColor);
        }

        [Fact]
        public void statistics_of_a_user_come_from_the_stored_sessions() {
            using (new Culture("de-DE")) {
                loadEvents();
                Settings.Default.name = "tester";
                Event ev = host.eventManager.findEventByName("Pistol Practice");
                Session session = Session.createNewSession(ev, "tester");
                session.startTime = new DateTime(2026, 10, 12, 10, 0, 0);
                session.addShot(Make.shot(10, 4, 1));
                session.addShot(Make.shot(-10, 4, 2));
                storage.storeSession(session, true);

                Assert.Equal(new[] { 9.0m }, storage.findScoresForUser("tester", ev)); //average of two 9s, integer scoring
                Assert.Equal(new[] { 10m }, storage.findRBarForUser("tester", ev));
                Assert.Equal(new[] { 0m }, storage.findXBarForUser("tester", ev));
                Assert.Equal(new[] { 4m }, storage.findYBarForUser("tester", ev));
                Assert.Empty(storage.findScoresForUser("someone else", ev));
            }
        }
    }
}
