using System;
using System.Collections.Generic;
using System.Drawing;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms.DataVisualization.Charting;

namespace freETarget {
    public class Shot {
        public int index;
        public int count;
        private decimal x;
        private decimal y;
        public decimal radius; //not used
        public decimal angle; //not used
        public int score;
        public decimal decimalScore;
        public bool innerTen;
        public DateTime timestamp;
        public TimeSpan shotDuration;
        public Boolean miss;

        public decimal calibrationX;
        public decimal calibrationY;
        public decimal calibrationAngle;

        private Shot() {

        }
        
        public Shot(decimal calibrateX, decimal calibrateY, decimal calibrateAngle) {
            this.calibrationX = calibrateX;
            this.calibrationY = calibrateY;
            this.calibrationAngle = calibrateAngle;
            miss = false;
        }


        private PointF RotatePoint(PointF pointToRotate, PointF centerPoint, float angleInDegrees) {
            double angleInRadians = angleInDegrees * (Math.PI / 180);
            double cosTheta = Math.Cos(angleInRadians);
            double sinTheta = Math.Sin(angleInRadians);
            return new PointF {
                X =
                    (float)
                    (cosTheta * (pointToRotate.X - centerPoint.X) -
                    sinTheta * (pointToRotate.Y - centerPoint.Y) + centerPoint.X),
                Y =
                    (float)
                    (sinTheta * (pointToRotate.X - centerPoint.X) +
                    cosTheta * (pointToRotate.Y - centerPoint.Y) + centerPoint.Y)
            };

        }

        public decimal getX() {
            PointF p = new PointF((float)(this.x + calibrationX), (float)this.y);
            PointF rotP = RotatePoint(p, new PointF(0, 0), (float)this.calibrationAngle);
            return (decimal)rotP.X;

        }

        public void setX(decimal nr) {
            this.x = nr;
        }

        public decimal getY() {
            PointF p = new PointF((float)(this.x), (float)(this.y + this.calibrationY));
            PointF rotP = RotatePoint(p, new PointF(0, 0), (float)this.calibrationAngle);
            return (decimal)rotP.Y;
        }

        public void setY(decimal nr) {
            this.y = nr;
        }

        public void computeScore(targets.aTarget target) {

            if(this.miss == true) {
                //target reported a miss. set score to 0
                this.decimalScore = 0;
                this.score = 0;
                this.innerTen = false;

                return;
            }

            //using liner interpolation with the "official" values found here: http://targettalk.org/viewtopic.php?p=100591#p100591


            double coef = 0d;

            coef = 9.9d / (((float)target.getOutterRing() / 2d) + ((float)target.getProjectileCaliber() / 2d));

            float newRadius = recomputeRadiusFromXY(); //use the calculated radius from x,y instead of the received one. x and y are adjusted with calibration
            this.radius = (decimal)newRadius;

            double score = 10.9999d - (coef * newRadius); //10.9999 is needed to get the incline just right. center should be almost 11
          

            this.decimalScore = (decimal)(Math.Truncate(score * 10)) / 10m;
            this.decimalScore += 0.0m; //add a decimal if the result is an integer


            if (this.decimalScore < 1) { //shot outside the target
                this.decimalScore = 0;
            }

            this.score = (int)Math.Floor(this.decimalScore);

            if (this.radius <= target.getInnerTenRadius()) {
                this.innerTen = true;
            } else {
                this.innerTen = false;
            }
        }

        private float recomputeRadiusFromXY() {
            return (float)Math.Sqrt(Math.Pow((double)(this.x + this.calibrationX) , 2) + Math.Pow((double)(this.y+this.calibrationY), 2));
        }

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
            ret += timestamp.ToString("yyyy-MM-dd HH:mm:ss") + ",";
            ret += shotDuration.TotalSeconds.ToString("F2", CultureInfo.InvariantCulture);
            if (calibrationX != 0 || calibrationY != 0 || calibrationAngle != 0) {
                ret += "," + calibrationX.ToString("F2", CultureInfo.InvariantCulture) + ",";
                ret += calibrationY.ToString("F2", CultureInfo.InvariantCulture) + ",";
                ret += calibrationAngle.ToString("F2", CultureInfo.InvariantCulture);
            }

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
            if (s.Length == 11) {
                //shots saved without calibration data
                shot.calibrationX = 0;
                shot.calibrationY = 0;
                shot.calibrationAngle = 0;
            } else {
                shot.calibrationX = decimal.Parse(s[11], CultureInfo.InvariantCulture);
                shot.calibrationY = decimal.Parse(s[12], CultureInfo.InvariantCulture);
                shot.calibrationAngle = decimal.Parse(s[13], CultureInfo.InvariantCulture);
            }

            shot.shotDuration = TimeSpan.FromSeconds(d);

            return shot;
        }

    }



}
