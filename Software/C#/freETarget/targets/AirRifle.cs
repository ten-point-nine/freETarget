using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace freETarget.targets {
    class AirRifle : aTarget {

        private const decimal pelletCaliber = 4.5m;
        private const decimal targetSize = 170; //mm
        private const int rifleBlackRings = 4;
        private const bool solidInnerTenRing = true;

        private const int trkZoomMin = 0;
        private const int trkZoomMax = 5;
        private const int trkZoomVal = 0;
        private const decimal pdfZoomFactor = 0.29m;

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

        private const decimal innerTenRadiusRifle = pelletCaliber / 2m - ring10 / 2m; //2.0m; ISSF rules states: Inner Ten = When the 10 ring (dot) has been shot out completely

        private static readonly decimal[] ringsRifle = new decimal[] { outterRing, ring2, ring3, ring4, ring5, ring6, ring7, ring8, ring9, ring10 };


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
            return diff / 4;
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
