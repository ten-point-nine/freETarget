using freETarget.Properties;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
//
// Data file for UK_25Yard Target
//
namespace freETarget.targets {
    [Serializable]
    class NSRA_25Y : aTarget {

        //
        // Define the target geometry.  
        // Distance from edge to edge (diameter) of each ring
        //
        private const decimal targetSize = 100; //mm
        private const decimal outterRing = 78.75m; //mm
        private const decimal ring2 = 71.44m;
        private const decimal ring3 = 64.12m;
        private const decimal ring4 = 56.81m;
        private const decimal ring5 = 49.49m;
        private const decimal ring6 = 42.18m;
        private const decimal ring7 = 34.86m; //mm
        private const decimal ring8 = 27.55m; //mm
        private const decimal ring9 = 20.23m; //mm
        private const decimal ring10 = 12.92m; //mm
        private const decimal innerRing = 1.00m; //mm

        private const decimal blackCircle = 51.39m; //mm

        // Define how the target is going to be painted on the display
        private const int pistolBlackRings = 5;               // Largest Black Ring
        private const int pistolFirstRing = 1;                // Largest ring present on target
        private const bool solidInnerTenRing = true;          // TRUE if the inner ring painted solid white
        private const bool pistolRapidFire = false;           // TRUE if this is a rapid fire target

        // Rings as they appear on the display screen.  List the rings that are used in outer to inner order
        private static readonly decimal[] ringspistol = new decimal[] { outterRing, ring2, ring3, ring4, ring5, ring6, ring7, ring8, ring9, ring10, innerRing };

        // Working variables
        private decimal pelletCaliber;
        private const int trkZoomMin = 0;
        private const int trkZoomMax = 3;
        private const int trkZoomVal = 0;
        private const decimal pdfZoomFactor = 1m;

        //
        // Methods provided by the target function
        // Do not modify this section
        //
        public NSRA_25Y(decimal caliber) : base(caliber) {
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
            return typeof(NSRA_25Y).FullName;
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
            return diff / 6f;

        }

        public override decimal getBlackDiameter() {
            return blackCircle;
        }

        public override int getRingTextCutoff() {
            return 8;
        }

        public override float getTextOffset(float diff, int ring) {
            if (ring != 4) {
                return 0;
            } else {
                return diff / 12;

            }
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
            } else if (radius > ring5 / 2m + pelletCaliber / 2m && radius <= ring4 / 2m + pelletCaliber / 2m) {
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
