using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace freETarget.targets {
    [Serializable]
    public abstract class aTarget {


        // -----------  abstract functions to be implemented in child classes ----------

        public abstract string getName();

        public abstract decimal getProjectileCaliber();

        public abstract decimal getSize();

        public abstract decimal[] getRings();

        public abstract decimal getOutterRing();

        public abstract decimal getOutterRadius();

        public abstract decimal get10Radius();

        public abstract decimal getInnerTenRadius();

        public abstract int getBlackRings();

        public abstract decimal getBlackDiameter();

        public abstract decimal getZoomFactor(int zoomValue);

        public abstract bool isSolidInner();

        public abstract int getTrkZoomMinimum();

        public abstract int getTrkZoomMaximum();

        public abstract int getTrkZoomValue();

        public abstract float getFontSize(float diff);

        public abstract int getRingTextCutoff();

        public abstract float getTextOffset(float diff, int ring);

        public abstract decimal getPDFZoomFactor(List<Shot> shotList);

        public abstract int getTextRotation();

        public abstract int getFirstRing();

        public abstract (decimal, decimal) rapidFireBarDimensions();

        public abstract bool drawNorthText();

        public abstract bool drawSouthText();

        public abstract bool drawWestText();

        public abstract bool drawEastText();




        //------------ common implementations ----------------------------

        /**
         * creates a target from the full name of its class (freETarget.targets.AirPistol), which is how events store their target.
         * returns null if there is no such target
         */
        public static aTarget create(string typeName, decimal caliber) {
            Type type = typeof(aTarget).Assembly.GetType(typeName);
            if (type == null || type.IsAbstract || !type.IsSubclassOf(typeof(aTarget))) {
                return null;
            }
            return (aTarget)Activator.CreateInstance(type, new object[] { caliber });
        }

        /** full class names of all available targets */
        public static List<string> getTargetNames() {
            List<string> ret = new List<string>();
            foreach (Type type in typeof(aTarget).Assembly.GetTypes()) {
                if (type.IsClass && !type.IsAbstract && type.IsSubclassOf(typeof(aTarget))) {
                    ret.Add(type.ToString());
                }
            }
            return ret;
        }

        protected aTarget(decimal caliber) {

        }

        public float getDimension(decimal currentTargetSize, decimal milimiters, decimal zoomFactor) {
            return (float)((currentTargetSize * milimiters) / (getSize() * zoomFactor));
        }

        public PointF transform(float xp, float yp, float size, decimal zoomFactor) {
            //matrix magic from: https://docs.microsoft.com/en-us/previous-versions/windows/internet-explorer/ie-developer/samples/jj635757(v=vs.85)

            System.Numerics.Matrix4x4 M = new System.Numerics.Matrix4x4(0, 0, 1, 0,
                                                                        0, 0, 0, 1,
                                                                        size, size, 1, 0,
                                                                       -size, size, 0, 1);

            System.Numerics.Matrix4x4 Minverted;
            System.Numerics.Matrix4x4.Invert(M, out Minverted);

            float shotRange = (float)(getSize() * zoomFactor) / 2f;
            System.Numerics.Matrix4x4 xyPrime = new System.Numerics.Matrix4x4(-shotRange, 0, 0, 0,
                                                                                shotRange, 0, 0, 0,
                                                                                shotRange, 0, 0, 0,
                                                                               -shotRange, 0, 0, 0);

            System.Numerics.Matrix4x4 abcd = System.Numerics.Matrix4x4.Multiply(Minverted, xyPrime);

            float a = abcd.M11;
            float b = abcd.M21;
            float c = abcd.M31;
            float d = abcd.M41;

            float x = (a * xp + b * yp - b * d - a * c) / (a * a + b * b);
            float y = (b * xp - a * yp - b * c + a * d) / (a * a + b * b);

            PointF ret = new PointF(x, y);
            return ret;
        }


        //default score calculation. can be overriden at target level if the formula is not linear
        public virtual decimal getScore(decimal radius) {
            return 11 - (radius / get10Radius());
        }
    }
}
