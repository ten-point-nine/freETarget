using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace freETarget {
    class Shot {
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

        public void computeScore(Session.TargetType type) {
            //using liner interpolation with the "official" values found here: http://targettalk.org/viewtopic.php?p=100591#p100591


            if (type == Session.TargetType.Pistol) {
                double score = linearInterpolation(ISSF.pistol1X, ISSF.pistol1Y, ISSF.pistol2X, ISSF.pistol2Y, (float)this.radius);

                this.decimalScore = (decimal)(Math.Truncate(score * 10)) / 10m;
                this.decimalScore += 0.0m; //add a decimal is the result is an integer

                if (this.decimalScore < 1) { //shot outside the target
                    this.decimalScore = 0;
                }

                this.score = (int)Math.Floor(this.decimalScore);

                //determine if inner ten (X)
                if (this.radius <= ISSF.innerTenRadiusPistol) {
                    this.innerTen = true;
                } else {
                    this.innerTen = false;
                }
            } else if (type == Session.TargetType.Rifle) {
                double score = linearInterpolation(ISSF.rifle1X, ISSF.rifle1Y, ISSF.rifle2X, ISSF.rifle2Y, (float)this.radius);

                this.decimalScore = (decimal)(Math.Truncate(score * 10)) / 10m;
                this.decimalScore += 0.0m; //add a decimal is the result is an integer

                if (this.decimalScore >= 11m) { //the linear interpolation returns 11.000003814 for 0 (dead centre)
                    this.decimalScore = 10.9m;
                }

                if (this.decimalScore < 1) {//shot outside the target
                    this.decimalScore = 0;
                }

                this.score = (int)Math.Floor(this.decimalScore);

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

        private double linearInterpolation(float x1, float y1, float x2, float y2, float x) {
            double y = ((x2 - x) * y1 + (x - x1) * y2) / (x2 - x1);
            return y;
        }

    }



}
