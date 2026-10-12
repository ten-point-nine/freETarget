using System;
using System.Collections.Generic;
using System.Linq;
using Xunit;

namespace freETarget.Tests {

    public class SessionTests {

        [Fact]
        public void shots_add_up() {
            Session session = Session.createNewSession(Make.practiceEvent(), "tester");

            session.addShot(Make.shot(0, 0, 1));     //10.9, inner ten
            session.addShot(Make.shot(8, 0, 2));     //10.0
            session.addShot(Make.shot(0, 16, 3));    //9.0

            Assert.Equal(29, session.score);
            Assert.Equal(29.9m, session.decimalScore);
            Assert.Equal(1, session.innerX);
            Assert.Equal(new[] { 0, 1, 2 }, session.Shots.Select(s => s.index));
        }

        [Fact]
        public void practice_and_match_series_have_ten_shots() {
            Session session = Session.createNewSession(Make.practiceEvent(), "tester");

            for (int i = 1; i <= 23; i++) {
                session.addShot(Make.shot(0, 0, i));
            }

            Assert.Equal(new[] { 10, 10, 3 }, session.AllSeries.Select(s => s.Count));
            Assert.Equal(3, session.CurrentSeries.Count);
        }

        [Fact]
        public void final_has_two_series_of_five_followed_by_series_of_two() {
            Session session = Session.createNewSession(Make.finalEvent(), "tester");

            for (int i = 1; i <= 14; i++) {
                session.addShot(Make.shot(0, 0, i));
            }

            Assert.Equal(new[] { 5, 5, 2, 2 }, session.AllSeries.Select(s => s.Count));
        }

        [Fact]
        public void mean_point_of_impact_and_mean_radius() {
            Session session = Session.createNewSession(Make.practiceEvent(), "tester");
            List<Shot> shots = new List<Shot> { Make.shot(10, 4), Make.shot(-10, 4) };

            session.calculateMeanValuesAndGroupSize(shots);

            Assert.Equal(0m, session.xbar);
            Assert.Equal(4m, session.ybar);
            Assert.Equal(10m, session.rbar);
        }

        [Fact]
        public void misses_do_not_move_the_mean_point_of_impact() {
            Session session = Session.createNewSession(Make.practiceEvent(), "tester");
            Shot miss = new Shot(0, 0, 0);
            miss.miss = true;
            List<Shot> shots = new List<Shot> { Make.shot(6, 0), miss, Make.shot(10, 0) };

            session.calculateMeanValuesAndGroupSize(shots);

            Assert.Equal(8m, session.xbar);
        }

        [Theory]
        [InlineData(3, 4, 5.0)]      //diagonal: 3-4-5 triangle
        [InlineData(10, 0, 10.0)]
        [InlineData(0, 10, 10.0)]
        [InlineData(7, 7, 9.9)]      //the old formula gave 0 here
        public void group_size_is_the_distance_between_the_two_shots_furthest_apart(double x, double y, double expected) {
            Session session = Session.createNewSession(Make.practiceEvent(), "tester");
            List<Shot> shots = new List<Shot> { Make.shot(0, 0), Make.shot((decimal)x, (decimal)y), Make.shot((decimal)x / 2, (decimal)y / 2) };

            session.calculateMeanValuesAndGroupSize(shots);

            Assert.Equal((decimal)expected, session.groupSize);
        }

        [Fact]
        public void group_size_ignores_misses_and_needs_two_shots() {
            Shot miss = new Shot(0, 0, 0);
            miss.miss = true;

            Assert.Equal(0m, Session.computeGroupSize(new List<Shot> { Make.shot(30, 0) }));
            Assert.Equal(0m, Session.computeGroupSize(new List<Shot> { Make.shot(30, 0), miss }));
            Assert.Equal(5m, Session.computeGroupSize(new List<Shot> { Make.shot(30, 0), miss, Make.shot(30, 5) }));
        }

        [Fact]
        public void session_with_nothing_but_misses_has_no_statistics() {
            Session session = Session.createNewSession(Make.practiceEvent(), "tester");
            Shot miss = new Shot(0, 0, 0);
            miss.miss = true;

            session.calculateMeanValuesAndGroupSize(new List<Shot> { miss });

            Assert.Equal(0m, session.xbar);
            Assert.Equal(0m, session.rbar);
            Assert.Equal(0m, session.groupSize);
        }

        [Fact]
        public void copy_does_not_share_its_shot_lists() {
            Session session = Session.createNewSession(Make.practiceEvent(), "tester");
            session.addShot(Make.shot(0, 0, 1));

            Session copy = session.copy();
            copy.addShot(Make.shot(8, 0, 2));
            copy.user = "someone else";

            Assert.Single(session.Shots);
            Assert.Single(session.AllSeries[0]);
            Assert.Equal("tester", session.user);
            Assert.Equal(2, copy.Shots.Count);
        }

        [Fact]
        public void match_runs_for_the_minutes_of_its_event() {
            Event match = new Event(2, "Pistol Match", false, Event.EventType.Match, 60, new targets.AirPistol(4.5m), 75, 4.5m, -1, -1, -1, -1, -1, System.Drawing.Color.Orange, false, -1, -1, -1, -1, -1);
            Session session = Session.createNewSession(match, "tester");

            session.start();

            Assert.Equal(60, session.numberOfShots);
            Assert.Equal(TimeSpan.FromMinutes(75), session.endTime - session.startTime);
        }
    }

    public class RapidFireSequencerTests {

        private static List<RapidFireStep> run(RapidFireSequencer sequencer) {
            List<RapidFireStep> steps = new List<RapidFireStep>();
            do {
                steps.Add(sequencer.tick());
            } while (!steps.Last().finished && steps.Count < 50);
            return steps;
        }

        [Fact]
        public void series_of_five_shots_in_eight_seconds() {
            RapidFireSequencer sequencer = new RapidFireSequencer(Make.rapidFireEvent(5, 8, -1, 7, 60));

            Assert.Equal(60000, sequencer.start()); //load time
            List<RapidFireStep> steps = run(sequencer);

            Assert.Equal(3, steps.Count);
            Assert.Equal("{\"RAPID_COUNT\":5, \"RAPID_WAIT\":7, \"RAPID_TIME\":8, \"RAPID_ENABLE\":1 }", steps[0].command);
            Assert.Equal(15000, steps[0].nextInterval); //wait + series
            Assert.Null(steps[1].command);
            Assert.Equal(3000, steps[1].nextInterval);  //cooldown
            Assert.Equal("{\"RAPID_ENABLE\":0 }", steps[2].command);
            Assert.True(steps[2].finished);
            Assert.DoesNotContain(steps, s => s.misconfigured);
        }

        [Fact]
        public void duel_sends_one_command_per_shot() {
            using (new Culture("de-DE")) {
                RapidFireSequencer sequencer = new RapidFireSequencer(Make.rapidFireEvent(5, -1, 3, 7.5m, 60));

                sequencer.start();
                List<RapidFireStep> steps = run(sequencer);

                Assert.Equal(7, steps.Count);
                for (int i = 0; i < 5; i++) {
                    Assert.Equal("{\"RAPID_COUNT\":1, \"RAPID_WAIT\":7.5, \"RAPID_TIME\":3, \"RAPID_ENABLE\":1 }", steps[i].command);
                }
                Assert.Equal(10500, steps[0].nextInterval);
                Assert.Equal(0, steps[1].nextInterval);     //the following shots keep the interval
                Assert.Null(steps[5].command);
                Assert.Equal(3000, steps[5].nextInterval);
                Assert.Equal("{\"RAPID_ENABLE\":0 }", steps[6].command);
            }
        }

        [Fact]
        public void event_without_times_is_reported_and_stops() {
            RapidFireSequencer sequencer = new RapidFireSequencer(Make.rapidFireEvent(5, -1, -1, 7, 60));

            sequencer.start();
            RapidFireStep step = sequencer.tick();

            Assert.True(step.misconfigured);
            Assert.True(step.finished);
            Assert.Null(step.command);
        }

        [Fact]
        public void sequence_can_be_started_again() {
            RapidFireSequencer sequencer = new RapidFireSequencer(Make.rapidFireEvent(5, 8, -1, 7, 60));
            sequencer.start();
            run(sequencer);

            sequencer.start();

            Assert.Equal(3, run(sequencer).Count);
        }
    }
}

namespace freETarget.Tests {
    public class TargetRendererTests {

        [Fact]
        public void every_target_can_be_painted() {
            foreach (string name in targets.aTarget.getTargetNames()) {
                targets.aTarget target = targets.aTarget.create(name, 4.5m);
                Event ev = new Event(1, name, true, Event.EventType.Practice, -1, target, -1, 4.5m, -1, -1, -1, -1, -1, System.Drawing.Color.Gold, false, -1, -1, -1, -1, -1);
                Session session = Session.createNewSession(ev, "tester");
                Shot shot = new Shot(0, 0, 0);
                shot.setX(3);
                shot.setY(-2);
                shot.computeScore(target);
                session.addShot(shot);

                byte[] png = TargetRenderer.renderPng(target, 300, target.getTrkZoomValue(), false, session, session.Shots);

                Assert.True(png.Length > 1000, name);
            }
        }
    }
}

namespace freETarget.Tests {
    public class RenderSamples {
        //writes sample images for a visual check when the environment variable FET_RENDER_DIR is set
        [Fact]
        public void write_sample_images() {
            string dir = System.Environment.GetEnvironmentVariable("FET_RENDER_DIR");
            if (string.IsNullOrEmpty(dir)) {
                return;
            }
            foreach (string name in new[] { "AirPistol", "AirRifle", "RapidFire10m", "NRA_B19" }) {
                targets.aTarget target = targets.aTarget.create("freETarget.targets." + name, 4.5m);
                Event ev = new Event(1, name, true, name == "AirRifle" ? Event.EventType.Match : Event.EventType.Practice, -1, target, -1, 4.5m, -1, -1, -1, -1, -1, System.Drawing.Color.Gold, false, -1, -1, -1, -1, -1);
                Session session = Session.createNewSession(ev, "tester");
                decimal scale = target.getSize() / 170m;
                decimal[,] points = { { 2, 3 }, { -9, 6 }, { 14, -11 }, { -20, -4 }, { 5, 22 } };
                for (int i = 0; i < 5; i++) {
                    Shot shot = new Shot(0, 0, 0);
                    shot.setX(points[i, 0] * scale);
                    shot.setY(points[i, 1] * scale);
                    shot.computeScore(target);
                    session.addShot(shot);
                }
                session.calculateMeanValuesAndGroupSize(session.Shots);
                System.IO.File.WriteAllBytes(System.IO.Path.Combine(dir, name + ".png"), TargetRenderer.renderPng(target, 500, target.getTrkZoomValue(), name == "NRA_B19", session, session.Shots));
            }
        }
    }
}
