using freETarget.Properties;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace freETarget.targets {
    class AirPistol : aTarget {

        private const decimal pelletCaliber = 4.5m;
        private const decimal targetSize = 170; //mm
        private const int pistolBlackRings = 7;
        private const bool solidInnerTenRing = false;

        private const int trkZoomMin = 1;
        private const int trkZoomMax = 5;
        private const int trkZoomVal = 1;
        private const decimal pdfZoomFactor = 1;

        private const decimal outterRingPistol = 155.5m; //mm
        private const decimal ring2Pistol = 139.5m; //mm
        private const decimal ring3Pistol = 123.5m; //mm
        private const decimal ring4Pistol = 107.5m; //mm
        private const decimal ring5Pistol = 91.5m; //mm
        private const decimal ring6Pistol = 75.5m; //mm
        private const decimal ring7Pistol = 59.5m; //mm
        private const decimal ring8Pistol = 43.5m; //mm
        private const decimal ring9Pistol = 27.5m; //mm
        private const decimal ring10Pistol = 11.5m; //mm
        private const decimal innerRingPistol = 5m; //mm

        private const decimal innerTenRadiusPistol = innerRingPistol / 2m + pelletCaliber / 2m; //4.75m;

        private static readonly decimal[] ringsPistol = new decimal[] { outterRingPistol, ring2Pistol, ring3Pistol, ring4Pistol, ring5Pistol, ring6Pistol, ring7Pistol, ring8Pistol, ring9Pistol, ring10Pistol, innerRingPistol };

        public override int getBlackRings() {
            return pistolBlackRings;
        }

        public override decimal getInnerTenRadius() {
            return innerTenRadiusPistol;
        }

        public override string getName() {
            return "Air Pistol";
        }

        public override decimal getOutterRing() {
            return outterRingPistol;
        }

        public override decimal getPDFZoomFactor(List<Shot> shotList) {
            if (shotList == null) {
                return pdfZoomFactor;
            }
            else{
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
    }
}
