using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace freETarget.targets {
    [Serializable]
    internal class NRA_SR200:aTarget {


        //
        // Define the target geometry.  
        // Distance from edge to edge (diameter) of each ring
        //
        private const decimal targetSize = 1066.8m; //mm  - 42"
        private const decimal outterRing = 939.8m; //mm - 37" - 5 ring
        private const decimal ring6 = 787.4m; //mm  - 31"
        private const decimal ring7 = 635m; //mm - 25"
        private const decimal ring8 = 482.6m; //mm - 19"
        private const decimal ring9 = 330.2m; //mm - 13"
        private const decimal ring10 = 177.8m; //mm - 7"
        private const decimal innerRing = 76.2m; //mm - 3"

        // Define how the target is going to be painted on the display
        private const int pistolBlackRings = 9;               // Largest Black Ring
        private const int pistolFirstRing = 5;                // Largest ring present on target
        private const bool solidInnerTenRing = false;          // TRUE if the inner ring painted solid white
        private const bool pistolRapidFire = false;           // TRUE if this is a rapid fire target

        // Rings as they appear on the display screen.  List the rings that are used in outer to inner order
        private static readonly decimal[] ringspistol = new decimal[] { outterRing, ring6, ring7, ring8, ring9, ring10, innerRing };

        // Working variables
        private decimal pelletCaliber;
        private const int trkZoomMin = 0;
        private const int trkZoomMax = 3;
        private const int trkZoomVal = 0;
        private const decimal pdfZoomFactor = 1m;

        public NRA_SR200(decimal caliber) : base(caliber) {
            this.pelletCaliber = caliber;
        }



        public override int getBlackRings() {
            return pistolBlackRings;
        }

        public override decimal getInnerTenRadius() {
            return innerRing;
        }

        public override decimal getOutterRadius() {
            return getOutterRing() / 2m + pelletCaliber / 2m;
        }

        public override decimal get10Radius() {
            return ring10 / 2m + pelletCaliber / 2m;
        }
        public override string getName() {
            return typeof(NRA_SR200).FullName;
        }

        public override decimal getOutterRing() {
            return outterRing;
        }

        public override decimal getProjectileCaliber() {
            return pelletCaliber;
        }

        public override decimal[] getRings() {
            return ringspistol;
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
            if (diff == 0) {
                return 15; //hardcoded since for X there is no diff
            } else {
                return 15; //hardcoded because the distance between rings (diff) increases but the text should be the same
            }

        }

        public override decimal getBlackDiameter() {
            return ring9;
        }

        public override int getRingTextCutoff() {
            return 11;
        }

        public override float getTextOffset(float diff, int ring) {
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
            } else if (radius > ring7 / 2m + pelletCaliber / 2m && radius <= outterRing / 2m + pelletCaliber / 2m) {
                return 5;
            } else {
                return 0;
            }
        }
    }
}

