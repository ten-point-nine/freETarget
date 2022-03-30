using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace freETarget.targets {
    [Serializable]
    class Rifle300m : aTarget {

        private decimal pelletCaliber; //8mm BR
        private const decimal targetSize = 1300; //mm
        private const int rifleBlackRings = 5;
        private const bool solidInnerTenRing = false;

        private const int trkZoomMin = 1;
        private const int trkZoomMax = 5;
        private const int trkZoomVal = 1;
        private const decimal pdfZoomFactor = 0.29m;

        private const decimal outterRing = 1000m; //mm
        private const decimal ring2 = 900m; //mm
        private const decimal ring3 = 800m; //mm
        private const decimal ring4 = 700m; //mm
        private const decimal ring5 = 600m; //mm
        private const decimal ring6 = 500m; //mm
        private const decimal ring7 = 400m; //mm
        private const decimal ring8 = 300m; //mm
        private const decimal ring9 = 200m; //mm
        private const decimal ring10 = 100m; //mm
        private const decimal innerRing = 50m; //mm

        private const decimal blackCircle = 600m; //mm

        private decimal innerTenRadiusRifle;

        private static readonly decimal[] ringsRifle = new decimal[] { outterRing, ring2, ring3, ring4, ring5, ring6, ring7, ring8, ring9, ring10, innerRing };

        public Rifle300m(decimal caliber) : base(caliber) {
            this.pelletCaliber = caliber;
            innerTenRadiusRifle = innerRing / 2m + pelletCaliber / 2m;;
        }

        public override decimal get10Radius() {
            return ring10 / 2m + pelletCaliber / 2m;
        }

        public override decimal getBlackDiameter() {
            return blackCircle;
        }

        public override int getBlackRings() {
            return rifleBlackRings;
        }

        public override float getFontSize(float diff) {
            return diff / 6f;
        }

        public override decimal getInnerTenRadius() {
            return innerTenRadiusRifle;
        }

        public override string getName() {
            return typeof(Rifle300m).FullName;
        }

        public override decimal getOutterRadius() {
            return getOutterRing() / 2m + pelletCaliber / 2m;
        }

        public override decimal getOutterRing() {
            return outterRing;
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
            return ringsRifle;
        }

        public override int getRingTextCutoff() {
            return 9;
        }

        public override decimal getSize() {
            return targetSize;
        }

        public override float getTextOffset(float diff, int ring) {
            return 0;
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

        public override decimal getZoomFactor(int zoomValue) {
            return (decimal)(1 / (decimal)zoomValue);
        }

        public override bool isSolidInner() {
            return solidInnerTenRing;
        }

        public override int getTextRotation() {
            return 45;
        }

        public override int getFirstRing() {
            return 1;
        }

        public override bool isRapidFire() {
            return false;
        }
    }
}
