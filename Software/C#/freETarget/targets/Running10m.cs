﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace freETarget.targets {
    [Serializable]
    class Running10m : aTarget {

        private decimal pelletCaliber; //4.5
        private const decimal targetSize = 60; //mm
        private const int rifleBlackRings = 5;
        private const bool solidInnerTenRing = true;

        private const int trkZoomMin = 1;
        private const int trkZoomMax = 5;
        private const int trkZoomVal = 1;
        private const decimal pdfZoomFactor = 0.29m;

        private const decimal outterRing = 50.5m; //mm
        private const decimal ring2 = 45.5m; //mm
        private const decimal ring3 = 40.5m; //mm
        private const decimal ring4 = 35.5m; //mm
        private const decimal ring5 = 30.5m; //mm
        private const decimal ring6 = 25.5m; //mm
        private const decimal ring7 = 20.5m; //mm
        private const decimal ring8 = 15.5m; //mm
        private const decimal ring9 = 10.5m; //mm
        private const decimal ring10 = 5.5m; //mm
        private const decimal innerRing = 0.5m; //mm

        private const decimal blackCircle = 30.5m; //mm

        private decimal innerTenRadiusRifle;

        private static readonly decimal[] ringsRifle = new decimal[] { outterRing, ring2, ring3, ring4, ring5, ring6, ring7, ring8, ring9, ring10, innerRing };

        public Running10m(decimal caliber) : base(caliber) {
            this.pelletCaliber = caliber;
            innerTenRadiusRifle = innerRing / 2m + pelletCaliber / 2m; ;
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
            return diff / 3f;
        }

        public override decimal getInnerTenRadius() {
            return innerTenRadiusRifle;
        }

        public override string getName() {
            return typeof(Running10m).FullName;
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

        public override decimal getScore(decimal radius) {
            if (radius > get10Radius()) {
                return 10 - ((radius - get10Radius()) / 2.5m);
            } else {
                return 11 - (radius / get10Radius());
            }
        }
    }

}

