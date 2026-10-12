using freETarget.Properties;
using System;
using System.Collections.Generic;
using Xunit;

namespace freETarget.Tests {

    public class MessageReaderTests {

        private static List<string> readAll(MessageReader reader) {
            List<string> ret = new List<string>();
            string message;
            while ((message = reader.next()) != null) {
                ret.Add(message);
            }
            return ret;
        }

        [Fact]
        public void message_split_over_several_packets() {
            MessageReader reader = new MessageReader();

            reader.append("{\"shot\":1, \"x\":");
            Assert.Null(reader.next());

            reader.append("8.0, \"y\":0.0}\r\n");
            Assert.Equal("\"shot\":1, \"x\":8.0, \"y\":0.0", reader.next());
            Assert.Null(reader.next());
        }

        [Fact]
        public void several_messages_in_one_packet() {
            MessageReader reader = new MessageReader();
            reader.append("{\"shot\":1}\r\n{\"shot\":2}\r\n{\"sho");

            Assert.Equal(new[] { "\"shot\":1", "\"shot\":2" }, readAll(reader));

            reader.append("t\":3}");
            Assert.Equal(new[] { "\"shot\":3" }, readAll(reader));
        }

        [Fact]
        public void text_outside_of_the_accolades_is_dropped() {
            MessageReader reader = new MessageReader();
            reader.append("freETarget 5.2 } noise {\"shot\":1} more noise");

            Assert.Equal(new[] { "\"shot\":1" }, readAll(reader));
        }
    }

    public class TargetMessageTests : IDisposable {

        private readonly targets.aTarget target = new targets.AirPistol(4.5m);

        public TargetMessageTests() {
            Make.resetSettings();
        }

        public void Dispose() {
            Make.resetSettings();
        }

        [Theory]
        [InlineData("\"shot\":1, \"x\":8.0", TargetMessage.Kind.Shot)]
        [InlineData("\"shot\":2, \"miss\":1", TargetMessage.Kind.Shot)]
        [InlineData("\"timer\":1, \"n\":100", TargetMessage.Kind.BadShot)]
        [InlineData("\"NAME\":\"TARGET\", \"ANGLE\":45", TargetMessage.Kind.Other)]
        public void message_kinds(string message, TargetMessage.Kind kind) {
            Assert.Equal(kind, TargetMessage.classify(message));
        }

        [Fact]
        public void shot_is_parsed_and_scored() {
            Shot shot = TargetMessage.parseShot("\"shot\":7, \"miss\":0, \"session_type\":0, \"x\":8.00, \"y\":0.00, \"r\":8.00, \"a\":0.00", 0, 0, 0, target);

            Assert.Equal(7, shot.count);
            Assert.False(shot.miss);
            Assert.Equal(8m, shot.getX());
            Assert.Equal(10, shot.score);
            Assert.Equal(10.0m, shot.decimalScore);
        }

        [Fact]
        public void numbers_use_a_decimal_point_whatever_the_culture() {
            using (new Culture("de-DE")) {
                Shot shot = TargetMessage.parseShot("\"shot\":1, \"x\":8.01, \"y\":0.00", 0, 0, 0, target);

                Assert.Equal(9.9m, shot.decimalScore);
            }
        }

        [Fact]
        public void miss() {
            Shot shot = TargetMessage.parseShot("\"shot\":3, \"miss\":1, \"x\":0, \"y\":0", 0, 0, 0, target);

            Assert.True(shot.miss);
            Assert.Equal(0, shot.score);
        }

        [Fact]
        public void reduced_target_distance_scales_the_coordinates() {
            Settings.Default.targetDistance = 50; //target at half the distance: the same hole counts as twice as far from the center

            Shot shot = TargetMessage.parseShot("\"shot\":1, \"x\":8.0, \"y\":0.0", 0, 0, 0, target);

            Assert.Equal(16m, shot.getX());
            Assert.Equal(9.0m, shot.decimalScore);
        }

        [Fact]
        public void unparseable_shot_has_count_minus_one() {
            Shot shot = TargetMessage.parseShot("\"shot\":1, \"x\":abc", 0, 0, 0, target);

            Assert.Equal(-1, shot.count);
        }
    }

    public class ShotTextFormatTests {

        [Theory]
        [InlineData("en-US")]
        [InlineData("de-DE")]
        public void shot_survives_being_stored_as_text(string culture) {
            using (new Culture(culture)) {
                Shot original = Make.shot(-12.34m, 5.67m, 4);
                original.index = 3;
                original.shotDuration = TimeSpan.FromSeconds(12.5);

                string text = original.ToString();
                Shot loaded = Shot.parse(text);

                Assert.DoesNotContain(";", text);
                Assert.Equal(11, text.Split(',').Length);
                Assert.Equal(3, loaded.index);
                Assert.Equal(4, loaded.count);
                Assert.Equal(-12.34m, loaded.getX());
                Assert.Equal(5.67m, loaded.getY());
                Assert.Equal(original.score, loaded.score);
                Assert.Equal(original.decimalScore, loaded.decimalScore);
                Assert.Equal(original.innerTen, loaded.innerTen);
                Assert.Equal(original.timestamp, loaded.timestamp);
                Assert.Equal(TimeSpan.FromSeconds(12.5), loaded.shotDuration);
            }
        }

        [Fact]
        public void calibration_is_appended_only_when_set() {
            Shot calibrated = new Shot(1.5m, -2m, 0);
            calibrated.setX(3);
            calibrated.setY(4);
            calibrated.computeScore(new targets.AirPistol(4.5m));
            calibrated.timestamp = new DateTime(2026, 10, 12, 10, 0, 0);

            string text = calibrated.ToString();
            Shot loaded = Shot.parse(text);

            Assert.Equal(14, text.Split(',').Length);
            Assert.Equal(1.5m, loaded.calibrationX);
            Assert.Equal(-2m, loaded.calibrationY);
        }

        [Fact]
        public void shots_stored_by_older_versions_can_be_read() {
            Shot loaded = Shot.parse("0,1,8.00,0.00,8.00,0.00,10,10.0,False,2024-03-01 18:30:05,42.50");

            Assert.Equal(10, loaded.score);
            Assert.Equal(new DateTime(2024, 3, 1, 18, 30, 5), loaded.timestamp);
            Assert.Equal(0m, loaded.calibrationX);
        }
    }
}
