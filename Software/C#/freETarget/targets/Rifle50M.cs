/*
 * Fifty Meter Rifle Target
 */

using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

/*
 * 50 meter 22 rifle target.  ISSF Sectoin 6.3.4.2
 */
namespace freETarget.targets {
    class rifle50M : aTarget {

        private const decimal pelletCaliber = 5.56m;
        private const decimal targetSize = 170; //mm
        private const int rifleBlackRings = 4;
        private const bool solidInnerTenRing = false;         // The inner ring is a circle

        private const int trkZoomMin = 0;
        private const int trkZoomMax = 5;
        private const int trkZoomVal = 0;
        private const decimal pdfZoomFactor = 0.29m;

        private const decimal outterRingRifle = 154.4m; //mm
        private const decimal ring2Rifle = 138.4m; //mm
        private const decimal ring3Rifle = 122.4m; //mm
        private const decimal ring4Rifle = 106.4m; //mm
        private const decimal ring5Rifle = 90.4m; //mm
        private const decimal ring6Rifle = 74.4m; //mm
        private const decimal ring7Rifle = 58.4m; //mm
        private const decimal ring8Rifle = 42.4m; //mm
        private const decimal ring9Rifle = 26.4m; //mm
        private const decimal ring10Rifle = 10.4m; //mm

        private const decimal innerTenRadiusRifle = pelletCaliber / 2m - ring10Rifle / 2m; //2.0m; ISSF rules states: Inner Ten = When the 10 ring (dot) has been shot out completely

        private static readonly decimal[] ringsRifle = new decimal[] { outterRingRifle, ring2Rifle, ring3Rifle, ring4Rifle, ring5Rifle, ring6Rifle, ring7Rifle, ring8Rifle, ring9Rifle, ring10Rifle };


        public override int getBlackRings() {
            return rifleBlackRings;
        }

        public override decimal getInnerTenRadius() {
            return innerTenRadiusRifle;
        }

        public override string getName() {
            return "Air Rifle";
        }

        public override decimal getOutterRing() {
            return outterRingRifle;
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

        public override decimal getPDFZoomFactor(List<Shot> shotList) {
            if (shotList == null) {
                return pdfZoomFactor;
            } else {
                bool zoomed = true;
                bool zoomed2 = true;
                foreach (Shot s in shotList) {
                    if (s.score < 6) {
                        zoomed = false;
                    }
                    if (s.score < 1) {
                        zoomed2 = false;
                    }

                }
                if (zoomed) {
                    return 0.12m;
                } else {
                    if (zoomed2) {
                        return 0.29m;
                    } else {
                        return 1m;
                    }

                }
            }
        }
    }
}
