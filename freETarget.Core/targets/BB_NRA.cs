using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace freETarget.targets {
    [Serializable]
    internal class BB_NRA : aTarget {
        private decimal pelletCaliber;
        private const decimal targetSize = 80; //mm
        private const int pistolBlackRings = 8;
        private const int pistolFirstRing = 1;
        private const bool solidInnerTenRing = false;

        private const int trkZoomMin = 1;
        private const int trkZoomMax = 5;
        private const int trkZoomVal = 1;
        private const decimal pdfZoomFactor = 1;

        private const decimal outterRing = 72m; //mm
        private const decimal ring2 = 64.26m; //mm
        private const decimal ring3 = 56.7m; //mm
        private const decimal ring4 = 49.4m; //mm
        private const decimal ring5 = 41.58m; //mm
        private const decimal ring6 = 34.02m; //mm
        private const decimal ring7 = 26.46m; //mm
        private const decimal ring8 = 18.90m; //mm
        private const decimal ring9 = 11.34m; //mm
        private const decimal ring10 = 3.78m; //mm
        private const decimal innerRing = 0.1m; //mm

        private decimal innerTenRadiusPistol;

        private static readonly decimal[] ringsPistol = new decimal[] { outterRing, ring2, ring3, ring4, ring5, ring6, ring7, ring8, ring9, ring10, innerRing };

        public BB_NRA(decimal caliber) : base(caliber) {
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
            return ring10 / 2m + pelletCaliber / 2m;
        }

        public override decimal getOutterRadius() {
            return getOutterRing() / 2m + pelletCaliber / 2m;
        }

        public override string getName() {
            return typeof(BB_NRA).FullName;
        }

        public override decimal getOutterRing() {
            return outterRing;
        }

        public override float getFontSize(float diff) {
            return diff / 4f; //8 is empirically determinted for best look
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
            return ring8;
        }

        public override int getRingTextCutoff() {
            return 8;
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

        //
        // Function to compute the score based on the where the bullet lands
        // Corrects for bullet diameter
        // 
        // Note this only computes integral (non-decimal) scoring
        //
        public override decimal getScore(decimal radius) {
            if (radius >= 0 && radius <= ring10 / 2 + pelletCaliber / 2m) {
                return 10;
            } else if (radius > ring10 / 2m + pelletCaliber / 2m && radius <= ring9 / 2m + pelletCaliber / 2m) {
                return 9;
            } else if (radius > ring9 / 2m + pelletCaliber / 2m && radius <= ring8 / 2m + pelletCaliber / 2m) {
                return 8;
            } else if (radius > ring8 / 2m + pelletCaliber / 2m && radius <= ring7 / 2m + pelletCaliber / 2m) {
                return 7;
            } else if (radius > ring7 / 2m + pelletCaliber / 2m && radius <= ring6 / 2m + pelletCaliber / 2m) {
                return 6;
            } else if (radius > ring6 / 2m + pelletCaliber / 2m && radius <= ring5 / 2m + pelletCaliber / 2m) {
                return 5;
            } else if (radius > ring5/ 2m + pelletCaliber / 2m && radius <= ring4 / 2m + pelletCaliber / 2m) {
                return 4;
            } else if (radius > ring4 / 2m + pelletCaliber / 2m && radius <= ring3 / 2m + pelletCaliber / 2m) {
                return 3;
            } else if (radius > ring3 / 2m + pelletCaliber / 2m && radius <= ring2 / 2m + pelletCaliber / 2m) {
                return 2;
            } else if (radius > ring2 / 2m + pelletCaliber / 2m && radius <= outterRing / 2m + pelletCaliber / 2m) {
                return 1;
            } else {
                return 0;
            }
        }

    }
}
