using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace freETarget.targets {
    [Serializable]
    internal class B2at10m : aTarget {


        private decimal pelletCaliber;
        private const decimal targetSize = 170; //mm
        private const int pistolBlackRings = 7;
        private const int pistolFirstRing = 4;
        private const bool solidInnerTenRing = false;

        private const int trkZoomMin = 1;
        private const int trkZoomMax = 5;
        private const int trkZoomVal = 1;
        private const decimal pdfZoomFactor = 1;

        private const decimal outterRing = 122.17m;	// Ring diameter in mm
        private const decimal ring5 = 92.67m; //mm
        private const decimal ring6 = 69.33m; //mm
        private const decimal ring7 = 51.17m; //mm
        private const decimal ring8 = 37.17m; //mm
        private const decimal ring9 = 25.67m; //mm
        private const decimal innerRing = 15m; //mm

        private decimal innerTenRadiusPistol;

        private static readonly decimal[] ringsPistol = new decimal[] { outterRing, ring5, ring6, ring7, ring8, ring9, innerRing };

        public B2at10m(decimal caliber) : base(caliber) {
            this.pelletCaliber = caliber;
            innerTenRadiusPistol = innerRing / 2m + pelletCaliber / 2m; //4.75m;
        }

        public override int getBlackRings() {
            return pistolBlackRings;
        }

        public override decimal getInnerTenRadius() {
            return innerTenRadiusPistol;
        }

        public override decimal get10Radius() {
            return innerRing / 2m + pelletCaliber / 2m;
        }

        public override decimal getOutterRadius() {
            return getOutterRing() / 2m + pelletCaliber / 2m;
        }

        public override string getName() {
            return typeof(B2at10m).FullName;
        }

        public override decimal getOutterRing() {
            return outterRing;
        }

        public override float getFontSize(float diff) {
            if (diff == 0) {
                return 10; //hardcoded since for X there is no diff
            } else {
                return 10; //hardcoded because the distance between rings (diff) increases but the text should be the same
            }

        }

        public override decimal getPDFZoomFactor(List<Shot> shotList) {
            if (shotList == null) {
                return pdfZoomFactor;
            } else {
                bool zoomed = true;
                foreach (Shot s in shotList) {
                    if (s.score < 6) {
                        zoomed = false;
                    }
                }

                if (zoomed) {
                    return 0.5m;
                } else {
                    return 1;
                }
            }
        }

        public override decimal getProjectileCaliber() {
            return pelletCaliber;
        }

        public override decimal[] getRings() {
            return ringsPistol;
        }

        public override decimal getSize() {
            return targetSize;
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

        public override decimal getZoomFactor(int value) {
            return (decimal)(1 / (decimal)value);
        }

        public override bool isSolidInner() {
            return solidInnerTenRing;
        }

        public override decimal getBlackDiameter() {
            return ring7;
        }

        public override int getRingTextCutoff() {
            return 10;
        }
        public override float getTextOffset(float diff, int ring) {
            //return diff / 3;
            return 0;
        }

        public override int getTextRotation() {
            return 0;
        }

        public override int getFirstRing() {
            return pistolFirstRing;
        }

        public override (decimal, decimal) rapidFireBarDimensions() {
            return (-1, -1);
        }

        public override bool drawNorthText() {
            return false;
        }

        public override bool drawSouthText() {
            return false;
        }

        public override bool drawWestText() {
            return true;
        }

        public override bool drawEastText() {
            return false;
        }

        //
        // Function to compute the score based on the where the bullet lands
        // Corrects for bullet diameter
        // 
        // Note this only computes integral (non-decimal) scoring
        //
        public override decimal getScore(decimal radius) {
            if (radius >= 0 && radius <= innerRing / 2 + pelletCaliber / 2m) {
                return 10;
            } else if (radius > innerRing / 2m + pelletCaliber / 2m && radius <= ring9 / 2m + pelletCaliber / 2m) {
                return 9;
            } else if (radius > ring9 / 2m + pelletCaliber / 2m && radius <= ring8 / 2m + pelletCaliber / 2m) {
                return 8;
            } else if (radius > ring8 / 2m + pelletCaliber / 2m && radius <= ring7 / 2m + pelletCaliber / 2m) {
                return 7;
            } else if (radius > ring7 / 2m + pelletCaliber / 2m && radius <= ring6 / 2m + pelletCaliber / 2m) {
                return 6;
            } else if (radius > ring6 / 2m + pelletCaliber / 2m && radius <= ring5 / 2m + pelletCaliber / 2m) {
                return 5;
            } else if (radius > ring5 / 2m + pelletCaliber / 2m && radius <= outterRing / 2m + pelletCaliber / 2m) {
                return 4;
            } else {
                return 0;
            }
        }
    }
}
