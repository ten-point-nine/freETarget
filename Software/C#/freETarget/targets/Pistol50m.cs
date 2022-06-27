using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace freETarget.targets {
    [Serializable]
    class Pistol50m : aTarget {
        private decimal pelletCaliber;
        private const decimal targetSize = 550; //mm
        private const int pistolBlackRings = 7;
        private const bool solidInnerTenRing = false;

        private const int trkZoomMin = 1;
        private const int trkZoomMax = 5;
        private const int trkZoomVal = 1;
        private const decimal pdfZoomFactor = 1;

        private const decimal outterRing = 500m; //mm
        private const decimal ring2 = 450m; //mm
        private const decimal ring3 = 400m; //mm
        private const decimal ring4 = 350m; //mm
        private const decimal ring5 = 300m; //mm
        private const decimal ring6 = 250m; //mm
        private const decimal ring7 = 200m; //mm
        private const decimal ring8 = 150m; //mm
        private const decimal ring9 = 100m; //mm
        private const decimal ring10 = 50m; //mm
        private const decimal innerRing = 25m; //mm

        private decimal innerTenRadiusPistol;

        private static readonly decimal[] ringsPistol = new decimal[] { outterRing, ring2, ring3, ring4, ring5, ring6, ring7, ring8, ring9, ring10, innerRing };

        public Pistol50m(decimal caliber) : base(caliber) {
            this.pelletCaliber = caliber;
            innerTenRadiusPistol = innerRing / 2m + pelletCaliber / 2m;
        }

        public override int getBlackRings() {
            return pistolBlackRings;
        }

        public override decimal getInnerTenRadius() {
            return innerTenRadiusPistol;
        }

        public override decimal getOutterRadius() {
            return getOutterRing() / 2m + pelletCaliber / 2m;
        }

        public override decimal get10Radius() {
            return ring10 / 2m + pelletCaliber / 2m;
        }
        public override string getName() {
            return typeof(Pistol50m).FullName;
        }

        public override decimal getOutterRing() {
            return outterRing;
        }

        public override float getFontSize(float diff) {
            return diff / 8f; //8 is empirically determinted for best look
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
            return 9;
        }
        public override float getTextOffset(float diff, int ring) {
            //return diff / 4;
            return 0;
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
        public override decimal getScore(decimal radius) {
            //if (radius > get10Radius()) {
                return 10 - (radius - get10Radius()) / 25;
            //} else {
            //    return 11 - (radius / get10Radius());
           // }
        } 
    }
}
