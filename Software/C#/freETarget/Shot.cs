using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms.DataVisualization.Charting;

namespace freETarget {
    public class Shot {
        public int index;
        public int count;
        public decimal x;
        public decimal y;
        public decimal radius;
        public decimal angle;
        public int score;
        public decimal decimalScore;
        public bool innerTen;
        public DateTime timestamp;
        public TimeSpan shotDuration;

        public void computeScore(Session.TargetType type) {
            //using liner interpolation with the "official" values found here: http://targettalk.org/viewtopic.php?p=100591#p100591


            double coef = 0d;
            if (type == Session.TargetType.Pistol) {
                coef = 9.9d / (((float)ISSF.outterRingPistol / 2d) + ((float)ISSF.pelletCaliber / 2d));
            } else if (type == Session.TargetType.Rifle) {
                coef = 9.9d / (((float)ISSF.outterRingRifle / 2d) + ((float)ISSF.pelletCaliber / 2d));
            }

            double score = 10.9999d - (coef * (float)this.radius); //10.9999 is needed to get the incline just right. center should be almost 11

            this.decimalScore = (decimal)(Math.Truncate(score * 10)) / 10m;
            this.decimalScore += 0.0m; //add a decimal if the result is an integer


            if (this.decimalScore < 1) { //shot outside the target
                this.decimalScore = 0;
            }

            this.score = (int)Math.Floor(this.decimalScore);

            if (type == Session.TargetType.Pistol) {
                //double score = linearInterpolation(ISSF.pistol1X, ISSF.pistol1Y, ISSF.pistol2X, ISSF.pistol2Y, (float)this.radius);

                //determine if inner ten (X)
                if (this.radius <= ISSF.innerTenRadiusPistol) {
                    this.innerTen = true;
                } else {
                    this.innerTen = false;
                }
            } else if (type == Session.TargetType.Rifle) {
                //double score = linearInterpolation(ISSF.rifle1X, ISSF.rifle1Y, ISSF.rifle2X, ISSF.rifle2Y, (float)this.radius);

                //determine if inner ten (X)
                if (this.radius <= ISSF.innerTenRadiusRifle) {
                    this.innerTen = true;
                } else {
                    this.innerTen = false;
                }
            } else {
                Console.WriteLine("Unknown current target " + type);
            }

        }

/*        private double linearInterpolation(float x1, float y1, float x2, float y2, float x) {
            double y = ((x2 - x) * y1 + (x - x1) * y2) / (x2 - x1);
            return y;
        }*/

        public override string ToString() {
            string ret = "";
            ret +=  index + ",";
            ret += count + ",";
            ret += x.ToString("F2", CultureInfo.InvariantCulture) + ",";
            ret += y.ToString("F2", CultureInfo.InvariantCulture) + ",";
            ret += radius.ToString("F2", CultureInfo.InvariantCulture) + ",";
            ret += angle.ToString("F2", CultureInfo.InvariantCulture) + ",";
            ret += score + ",";
            ret += decimalScore.ToString("F1", CultureInfo.InvariantCulture) + ",";
            ret += innerTen + ",";
            ret += timestamp.ToString("yyyy-MM-dd hh:mm:ss") + ",";
            ret += shotDuration.TotalSeconds.ToString("F2", CultureInfo.InvariantCulture);

            return ret;
        }

        public static Shot parse(string input) {
            Shot shot = new Shot();
            string[] s = input.Split(',');
            shot.index = int.Parse(s[0]);
            shot.count = int.Parse(s[1]);
            shot.x = decimal.Parse(s[2], CultureInfo.InvariantCulture);
            shot.y = decimal.Parse(s[3], CultureInfo.InvariantCulture);
            shot.radius = decimal.Parse(s[4], CultureInfo.InvariantCulture);
            shot.angle = decimal.Parse(s[5], CultureInfo.InvariantCulture);
            shot.score = int.Parse(s[6]);
            shot.decimalScore = decimal.Parse(s[7], CultureInfo.InvariantCulture);
            shot.innerTen = bool.Parse(s[8]);
            shot.timestamp = DateTime.Parse(s[9]);
            double d = double.Parse(s[10], CultureInfo.InvariantCulture);
            shot.shotDuration = TimeSpan.FromSeconds(d);

            return shot;
        }

    }



}
