using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace freETarget.targets {
    [Serializable]
    class AirRifle : aTarget {

        private decimal pelletCaliber;
        private const decimal targetSize = 80; //mm
        private const int rifleBlackRings = 4;
        private const bool solidInnerTenRing = true;

        private const int trkZoomMin = 0;
        private const int trkZoomMax = 3;
        private const int trkZoomVal = 0;
        private const decimal pdfZoomFactor = 1m;

        private const decimal outterRing = 45.5m; //mm
        private const decimal ring2 = 40.5m; //mm
        private const decimal ring3 = 35.5m; //mm
        private const decimal ring4 = 30.5m; //mm
        private const decimal ring5 = 25.5m; //mm
        private const decimal ring6 = 20.5m; //mm
        private const decimal ring7 = 15.5m; //mm
        private const decimal ring8 = 10.5m; //mm
        private const decimal ring9 = 5.5m; //mm
        private const decimal ring10 = 0.5m; //mm

        private decimal innerTenRadiusRifle;

        private static readonly decimal[] ringsRifle = new decimal[] { outterRing, ring2, ring3, ring4, ring5, ring6, ring7, ring8, ring9, ring10 };


        public AirRifle(decimal caliber) : base(caliber) {
            this.pelletCaliber = caliber;
            innerTenRadiusRifle = pelletCaliber / 2m - ring10 / 2m; //2.0m; ISSF rules states: Inner Ten = When the 10 ring (dot) has been shot out completely
        }

        public override int getBlackRings() {
            return rifleBlackRings;
        }

        public override decimal getInnerTenRadius() {
            return innerTenRadiusRifle;
        }

        public override decimal getOutterRadius() {
            return getOutterRing() / 2m + pelletCaliber / 2m;
        }

        public override decimal get9Radius() {
            return ring9 / 2m + pelletCaliber / 2m;
        }
        public override string getName() {
            return typeof(AirRifle).FullName;
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
            return diff / 8f; //8 is empirically determinted for best look
        }

        public override decimal getBlackDiameter() {
            return ring4;
        }

        public override int getRingTextCutoff() {
            return 8;
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
            return 1;
        }

        public override bool isRapidFire() {
            return false;
        }
    }
}
