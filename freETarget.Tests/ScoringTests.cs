using System;
using System.Linq;
using Xunit;

namespace freETarget.Tests {

    // air pistol target with 4.5mm pellets: every ring is 8mm of radius, the 10 ring ends at 8mm, the inner ten at 4.75mm, the target at 80mm
    public class ScoringTests {

        [Theory]
        [InlineData(0, 0, 10, 10.9)]     //dead center is capped at 10.9
        [InlineData(8, 0, 10, 10.0)]     //exactly on the edge of the 10 ring
        [InlineData(8.01, 0, 9, 9.9)]    //just outside of it. decimals are truncated, not rounded
        [InlineData(0, 16, 9, 9.0)]
        [InlineData(-40, 0, 6, 6.0)]
        [InlineData(0, -80, 1, 1.0)]     //edge of the target
        [InlineData(80.1, 0, 0, 0.0)]    //outside the target
        public void score_follows_the_distance_from_the_center(double x, double y, int score, double decimalScore) {
            Shot shot = Make.shot((decimal)x, (decimal)y);

            Assert.Equal(score, shot.score);
            Assert.Equal((decimal)decimalScore, shot.decimalScore);
        }

        [Fact]
        public void inner_ten_ends_at_4_75_mm() {
            Assert.True(Make.shot(4.75m, 0).innerTen);
            Assert.False(Make.shot(4.8m, 0).innerTen);
        }

        [Fact]
        public void miss_scores_zero() {
            Shot shot = new Shot(0, 0, 0);
            shot.miss = true;
            shot.computeScore(new targets.AirPistol(4.5m));

            Assert.Equal(0, shot.score);
            Assert.Equal(0m, shot.decimalScore);
            Assert.False(shot.innerTen);
        }

        [Theory]
        [InlineData(8, 0, 0)]
        [InlineData(0, 8, 90)]
        [InlineData(-8, 0, 180)]
        public void radius_and_angle_are_computed_from_x_and_y(double x, double y, double angle) {
            Shot shot = Make.shot((decimal)x, (decimal)y);

            Assert.Equal(8m, shot.radius);
            Assert.Equal((decimal)angle, shot.angle);
        }

        [Fact]
        public void calibration_offset_moves_the_shot() {
            Shot shot = new Shot(8, 0, 0); //calibration of 8mm to the right
            shot.setX(0);
            shot.setY(0);
            shot.computeScore(new targets.AirPistol(4.5m));

            Assert.Equal(10.0m, shot.decimalScore);
            Assert.Equal(8m, shot.getX());
        }

        [Fact]
        public void larger_caliber_scores_higher_for_the_same_hole_center() {
            Shot shot = new Shot(0, 0, 0);
            shot.setX(9);
            shot.setY(0);

            shot.computeScore(new targets.AirPistol(4.5m));
            Assert.Equal(9, shot.score);

            shot.computeScore(new targets.AirPistol(7m)); //10 ring ends at 5.75 + 3.5 = 9.25mm
            Assert.Equal(10, shot.score);
        }
    }

    public class TargetTests {

        [Fact]
        public void all_targets_can_be_created_from_the_name_they_are_stored_with() {
            var names = targets.aTarget.getTargetNames();

            Assert.Equal(17, names.Count);
            foreach (string name in names) {
                targets.aTarget target = targets.aTarget.create(name, 4.5m);

                Assert.NotNull(target);
                Assert.Equal(name, target.getName());
                Assert.StartsWith("freETarget.targets.", name);
                Assert.NotEmpty(target.getRings());
                Assert.True(target.getScore(0) >= 10, name + " does not score a 10 in the center");
                Assert.True(target.getSize() > 0);
            }
        }

        [Fact]
        public void names_stored_in_existing_databases_still_resolve() {
            Assert.IsType<targets.AirPistol>(targets.aTarget.create("freETarget.targets.AirPistol", 4.5m));
            Assert.IsType<targets.AirRifle>(targets.aTarget.create("freETarget.targets.AirRifle", 4.5m));
        }

        [Theory]
        [InlineData("freETarget.targets.DoesNotExist")]
        [InlineData("freETarget.targets.aTarget")]
        [InlineData("System.String")]
        public void unknown_names_give_no_target(string name) {
            Assert.Null(targets.aTarget.create(name, 4.5m));
        }

        [Fact]
        public void shot_coordinates_map_onto_the_image() {
            targets.aTarget target = new targets.AirPistol(4.5m);
            decimal zoom = target.getZoomFactor(target.getTrkZoomValue());
            float halfRange = (float)(target.getSize() * zoom) / 2f;

            var center = target.transform(0, 0, 500, zoom);
            var topRight = target.transform(halfRange, halfRange, 500, zoom);

            Assert.Equal(250, center.X, 3);
            Assert.Equal(250, center.Y, 3);
            Assert.Equal(500, topRight.X, 3); //x grows to the right
            Assert.Equal(0, topRight.Y, 3);   //y grows upwards on the target, downwards in the image
        }
    }
}
