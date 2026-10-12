using System;
using System.IO;
using System.Text;
using Xunit;

namespace freETarget.Tests {

    public class PdfTests : IDisposable {

        private readonly TempDir dir = new TempDir();

        public PdfTests() {
            Make.resetSettings();
        }

        public void Dispose() {
            dir.Dispose();
        }

        private static Session session(Event ev, int shots) {
            Session session = Session.createNewSession(ev, "Marvin");
            session.startTime = new DateTime(2026, 10, 12, 18, 0, 0);
            Random random = new Random(7);
            for (int i = 1; i <= shots; i++) {
                Shot shot = new Shot(0, 0, 0);
                shot.count = i;
                shot.setX((decimal)Math.Round((random.NextDouble() - 0.5) * 30, 2));
                shot.setY((decimal)Math.Round((random.NextDouble() - 0.5) * 30, 2));
                shot.computeScore(ev.Target);
                shot.timestamp = session.startTime.AddSeconds(40 * i + random.Next(20));
                session.addShot(shot);
            }
            Properties.Settings.Default.name = "Marvin";
            session.prepareForSaving();
            session.endTime = session.startTime.AddSeconds(40 * shots + 60);
            return session;
        }

        private static string export(Session session, string folder, string diary) {
            return PDFGenerator.generateAndSavePDF(session, folder, null, diary);
        }

        [Fact]
        public void report_of_a_session_is_written() {
            Session s = session(Make.practiceEvent(), 23);
            s.id = 17;

            string file = export(s, dir.path, "Good day.\nWind from the left.");

            Assert.Equal(Path.Combine(dir.path, "Session 17.pdf"), file);
            byte[] pdf = File.ReadAllBytes(file);
            Assert.True(pdf.Length > 5000);
            Assert.Equal("%PDF-", Encoding.ASCII.GetString(pdf, 0, 5));

            //kept for a look when the environment variable FET_RENDER_DIR is set
            string keep = Environment.GetEnvironmentVariable("FET_RENDER_DIR");
            if (!string.IsNullOrEmpty(keep)) {
                File.Copy(file, Path.Combine(keep, "Session 17.pdf"), true);
            }
        }

        [Fact]
        public void session_that_was_never_saved_is_named_after_its_start() {
            Session s = session(Make.finalEvent(), 3);

            string file = export(s, dir.path, null);

            Assert.Equal("Session 2026-10-12 180000.pdf", Path.GetFileName(file));
            Assert.True(File.Exists(file));
        }

        [Fact]
        public void every_target_can_be_reported() {
            using (new Culture("de-DE")) {
                foreach (string name in targets.aTarget.getTargetNames()) {
                    targets.aTarget target = targets.aTarget.create(name, 4.5m);
                    Event ev = new Event(1, name, true, Event.EventType.Practice, -1, target, -1, 4.5m, -1, -1, -1, -1, -1, System.Drawing.Color.Gold, false, -1, -1, -1, -1, -1);
                    Session s = session(ev, 12);
                    s.id = 1;

                    Assert.True(new FileInfo(export(s, dir.path, "")).Length > 5000, name);
                }
            }
        }

        [Fact]
        public void exporting_does_not_touch_the_session_on_the_screen() {
            Session live = session(Make.practiceEvent(), 12);
            int series = live.AllSeries.Count;

            export(live.copy(), dir.path, null);

            Assert.Equal(series, live.AllSeries.Count);
            Assert.Equal(12, live.Shots.Count);
            Assert.Empty(live.LoadedShots);
        }
    }

    public class RtfTextTests {

        [Fact]
        public void diary_written_by_the_windows_application() {
            string rtf = @"{\rtf1\ansi\ansicpg1252\deff0\nouicompat\deflang1031{\fonttbl{\f0\fnil\fcharset0 Segoe Print;}{\f1\fnil Arial;}}
{\colortbl ;\red0\green0\blue255;}
{\*\generator Riched20 10.0.19041}\viewkind4\uc1 
\pard\f0\fs24 Good day.\par
Wind from the \b left\b0 , 3 \{clicks\}.\par
Gr\'fc\'dfe \u8364?5\par
}";

            Assert.Equal("Good day." + Environment.NewLine + "Wind from the left, 3 {clicks}." + Environment.NewLine + "Grüße €5", RtfText.toPlainText(rtf));
        }

        [Theory]
        [InlineData("Good day.")]
        [InlineData("Gun: LP10\nPellets: R10 4.50\n\nWind {left} \\ 3 clicks\tok")]
        [InlineData("Gr\u00fc\u00dfe aus K\u00f6ln \u20ac5 \u2192 fine")]
        [InlineData("")]
        public void diary_text_survives_being_stored_as_rtf(string text) {
            string expected = text.Replace("\n", Environment.NewLine);

            string rtf = RtfText.fromPlainText(text);

            Assert.StartsWith("{\\rtf1", rtf);
            Assert.All(rtf, c => Assert.True(c < 128, "RTF has to be plain ASCII"));
            Assert.Equal(expected.Trim(), RtfText.toPlainText(rtf));
        }

        [Fact]
        public void new_diary_page_has_the_headings() {
            Assert.StartsWith("Gun:" + Environment.NewLine + "Gun adjustments:", RtfText.diaryTemplate());
        }

        [Theory]
        [InlineData(null, "")]
        [InlineData("", "")]
        [InlineData("just text, no RTF", "just text, no RTF")]
        public void text_that_is_not_rtf_stays_as_it_is(string input, string expected) {
            Assert.Equal(expected, RtfText.toPlainText(input));
        }
    }
}
