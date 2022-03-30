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
    [Serializable]
    class Rifle50m : aTarget {

        private decimal pelletCaliber;// = 5.6m; //.22LR
        private const decimal targetSize = 250; //mm
        private const int rifleBlackRings = 4;
        private const bool solidInnerTenRing = false;     

        private const int trkZoomMin = 0;
        private const int trkZoomMax = 5;
        private const int trkZoomVal = 0;
        private const decimal pdfZoomFactor = 0.29m;

        private const decimal outterRing = 154.4m; //mm
        private const decimal ring2 = 138.4m; //mm
        private const decimal ring3 = 122.4m; //mm
        private const decimal ring4 = 106.4m; //mm
        private const decimal ring5 = 90.4m; //mm
        private const decimal ring6 = 74.4m; //mm
        private const decimal ring7 = 58.4m; //mm
        private const decimal ring8 = 42.4m; //mm
        private const decimal ring9 = 26.4m; //mm
        private const decimal ring10 = 10.4m; //mm
        private const decimal innerRing = 5m; //mm

        private const decimal blackCircle = 112.4m; //mm

        private decimal innerTenRadiusRifle;// = innerRing / 2m + pelletCaliber / 2m; 

        private static readonly decimal[] ringsRifle = new decimal[] { outterRing, ring2, ring3, ring4, ring5, ring6, ring7, ring8, ring9, ring10, innerRing };


        public Rifle50m(decimal caliber) : base(caliber) {
            this.pelletCaliber = caliber;
            innerTenRadiusRifle = innerRing / 2m + pelletCaliber / 2m; //4.75m;
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

        public override decimal get10Radius() {
            return ring10 / 2m + pelletCaliber / 2m;
        }

        public override string getName() {
            return typeof(Rifle50m).FullName;
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
            return diff / 6f; 
        }

        public override decimal getBlackDiameter() {
            return blackCircle;
        }

        public override int getRingTextCutoff() {
            return 8;
        }

        public override float getTextOffset(float diff, int ring) {
            if (ring != 3) {
                //return diff / 4;
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
