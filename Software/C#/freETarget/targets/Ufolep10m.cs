using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace freETarget.targets {
    [Serializable]
    class Ufolep10m : aTarget {

        private decimal pelletCaliber;
        private const decimal targetSize = 100; //mm
        private const int rifleBlackRings = 7;
        private const bool solidInnerTenRing = true;

        private const int trkZoomMin = 0;
        private const int trkZoomMax = 3;
        private const int trkZoomVal = 0;
        private const decimal pdfZoomFactor = 1m;

        private const decimal outterRing = 56m; //mm
        private const decimal ring5 = 47m; //mm
        private const decimal ring6 = 38m; //mm
        private const decimal ring7 = 29m; //mm
        private const decimal ring8 = 20m; //mm
        private const decimal ring9 = 11m; //mm
        private const decimal ring10 = 2m; //mm
        private const decimal innerRing = 2m; //mm

        private static readonly decimal[] ringsRifle = new decimal[] { outterRing, ring5, ring6, ring7, ring8, ring9, ring10, innerRing };


        public Ufolep10m(decimal caliber) : base(caliber) {
            this.pelletCaliber = caliber;
         }

        public override int getBlackRings() {
            return rifleBlackRings;
        }

        public override decimal getInnerTenRadius() {
            return -1;
        }

        public override decimal getOutterRadius() {
            return getOutterRing() / 2m + pelletCaliber / 2m;
        }

        public override decimal get10Radius() {
            return ring10 / 2m + pelletCaliber / 2m;
        }
        public override string getName() {
            return typeof(Ufolep10m).FullName;
        }

        public override decimal getOutterRing() {
            return outterRing;
        }

        public override decimal getProjectileCaliber() {
            return pelletCaliber;
        }

        public override decimal[] getRings() {
            return ringsRifle;
        }

        public override decimal getSize() {
            return targetSize;
        }

        public override decimal getZoomFactor(int value) {
            return (decimal)(1 / Math.Pow(2, value));
        }

        public override bool isSolidInner() {
            return solidInnerTenRing;
        }

        public override int getTrkZoomMaximum() {
            return trkZoomMax;
        }

        public override int getTrkZoomMinimum() {
            return trkZoomMin;
        }

        public override int getTrkZoomValue() {
            return trkZoomVal;
        }

        public override float getFontSize(float diff) {
            return diff / 6f; //8 is empirically determinted for best look
        }

        public override decimal getBlackDiameter() {
            return ring7;
        }

        public override int getRingTextCutoff() {
            return 9;
        }

        public override float getTextOffset(float diff, int ring) {
            //return diff / 4;
            return 0;
        }

        public override decimal getPDFZoomFactor(List<Shot> shotList) {
            if (shotList == null) {
                return pdfZoomFactor;
            } else {
                bool zoomed = true;
                bool zoomedLess = true;
                foreach (Shot s in shotList) {
                    if (s.decimalScore <= 9.4m) {
                        zoomed = false;
                    }
                    if (s.decimalScore <= 7.2m) {
                        zoomedLess = false;
                    }

                }
                if (zoomed) {
                    return 0.15m;
                } else {
                    if (zoomedLess) {
                        return 0.29m;
                    } else {
                        return pdfZoomFactor;
                    }

                }
            }
        }

        public override int getTextRotation() {
            return 0;
        }

        public override int getFirstRing() {
            return 4;
        }

        public override (decimal, decimal) rapidFireBarDimensions() {
            return (-1, -1);
        }

        public override decimal getScore(decimal radius) {
            if (radius >= 0 && radius <= ring10/2  + pelletCaliber / 2m) {
                return 10;
            } else if (radius > ring10/2m  + pelletCaliber / 2m && radius <= ring9/2m + pelletCaliber / 2m) {
                return 9;
            } else if (radius > ring9/2m + pelletCaliber / 2m && radius <= ring8/2m + pelletCaliber / 2m) {
                return 8;
            } else if (radius > ring8/2m + pelletCaliber / 2m && radius <= ring7/2m + pelletCaliber / 2m) {
                return 7;
            } else if (radius > ring7/2m + pelletCaliber / 2m && radius <= ring6/2m + pelletCaliber / 2m) {
                return 6;
            } else if (radius > ring6/2m + pelletCaliber / 2m && radius <= ring5/2m + pelletCaliber / 2m) {
                return 5;
            } else if (radius > ring5/2m + pelletCaliber / 2m && radius <= outterRing/2m + pelletCaliber / 2m) {
                return 4;
            } else {
                return 0;
            }
        }

        public override bool drawNorthText() {
            return true;
        }

        public override bool drawSouthText() {
            return true;
        }

        public override bool drawWestText() {
            return true;
        }

        public override bool drawEastText() {
            return true;
        }
    }
}
