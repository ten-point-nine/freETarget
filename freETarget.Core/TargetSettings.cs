using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;

namespace freETarget {

    /**
     * the settings that are stored in the target itself: its name, the paper drive, LED brightness and the sensor geometry.
     * the values are kept as the user typed them, they go into the command for the target as they are after validate() accepted them
     */
    public class TargetSettings {

        /** the names a target can have. the target stores the index */
        public static readonly IReadOnlyList<string> TargetNames = new List<string>() { "TARGET",                           //  0
                        "1",      "2",        "3",     "4",      "5",       "6",       "7",     "8",     "9",      "10",    //  1
                        "DOC",    "DOPEY",  "HAPPY",   "GRUMPY", "BASHFUL", "SNEEZEY", "SLEEPY",                            // 11
                        "RUDOLF", "DONNER", "BLITXEM", "DASHER", "PRANCER", "VIXEN",   "COMET", "CUPID", "DUNDER",          // 18
                        "ODIN",   "WODEN",   "THOR",   "BALDAR"                                                             // 27
                        };

        public const int MaxLedBrightness = 99;

        public int nameId = 0;
        public int ledBrightness = 0;

        //paper drive: a direct current motor that runs for a time, or a stepper motor that makes a number of steps
        public string paperTime = "0";
        public string stepTime = "0";
        public string stepCount = "0";

        public string sensorDiameter = "230";
        public string xOffset = "0";
        public string yOffset = "0";
        public string zOffset = "0";

        public string northX = "0";
        public string northY = "0";
        public string eastX = "0";
        public string eastY = "0";
        public string southX = "0";
        public string southY = "0";
        public string westX = "0";
        public string westY = "0";

        /** true if the paper is moved by a direct current motor, false for a stepper motor */
        public bool isDirectCurrent() {
            int time;
            return int.TryParse(paperTime, NumberStyles.Integer, CultureInfo.InvariantCulture, out time) && time > 0;
        }

        public static TargetSettings fromEcho(Echo echo) {
            TargetSettings ret = new TargetSettings();
            ret.nameId = echo.NAME_ID >= 0 && echo.NAME_ID < TargetNames.Count ? echo.NAME_ID : 0;
            ret.ledBrightness = Math.Max(0, Math.Min(MaxLedBrightness, echo.LED_BRIGHT));
            ret.paperTime = echo.PAPER_TIME.ToString(CultureInfo.InvariantCulture);
            ret.stepTime = echo.STEP_TIME.ToString(CultureInfo.InvariantCulture);
            ret.stepCount = echo.STEP_COUNT.ToString(CultureInfo.InvariantCulture);
            ret.sensorDiameter = echo.SENSOR.ToString(CultureInfo.InvariantCulture);
            ret.xOffset = echo.X_OFFSET.ToString(CultureInfo.InvariantCulture);
            ret.yOffset = echo.Y_OFFSET.ToString(CultureInfo.InvariantCulture);
            ret.zOffset = echo.Z_OFFSET.ToString(CultureInfo.InvariantCulture);
            ret.northX = echo.NORTH_X.ToString(CultureInfo.InvariantCulture);
            ret.northY = echo.NORTH_Y.ToString(CultureInfo.InvariantCulture);
            ret.eastX = echo.EAST_X.ToString(CultureInfo.InvariantCulture);
            ret.eastY = echo.EAST_Y.ToString(CultureInfo.InvariantCulture);
            ret.southX = echo.SOUTH_X.ToString(CultureInfo.InvariantCulture);
            ret.southY = echo.SOUTH_Y.ToString(CultureInfo.InvariantCulture);
            ret.westX = echo.WEST_X.ToString(CultureInfo.InvariantCulture);
            ret.westY = echo.WEST_Y.ToString(CultureInfo.InvariantCulture);
            return ret;
        }

        private static bool validNumber(string text) {
            int x;
            return int.TryParse(text, NumberStyles.AllowLeadingSign, CultureInfo.InvariantCulture, out x);
        }

        private static bool positiveNumber(string text) {
            int x;
            return int.TryParse(text, NumberStyles.None, CultureInfo.InvariantCulture, out x);
        }

        //after parsing and converting to string again, the text should be identical. otherwise, the parsing was not correct.
        //this catches a decimal comma, which would not be an error but give a wrong number
        private static bool validDecimal(string text) {
            decimal r;
            if (!decimal.TryParse(text, NumberStyles.Number, CultureInfo.InvariantCulture, out r)) {
                return false;
            }
            return r.ToString(CultureInfo.InvariantCulture) == text;
        }

        /** returns null if everything can be sent to the target, otherwise what is wrong */
        public string validate() {
            if (nameId < 0 || nameId >= TargetNames.Count) {
                return "Please select a target name";
            }
            if (ledBrightness < 0 || ledBrightness > MaxLedBrightness) {
                return "LED brightness must be between 0 and " + MaxLedBrightness;
            }
            if (!validDecimal(sensorDiameter)) {
                return "Sensor diameter is not a decimal";
            }
            if (!validNumber(zOffset)) {
                return "Z offset is not a number";
            }
            if (!validDecimal(xOffset)) {
                return "X offset is not a number";
            }
            if (!validDecimal(yOffset)) {
                return "Y offset is not a number";
            }

            string[][] sensors = {
                new[] { northX, "North X" }, new[] { northY, "North Y" }, new[] { westX, "West X" }, new[] { westY, "West Y" },
                new[] { southX, "South X" }, new[] { southY, "South Y" }, new[] { eastX, "East X" }, new[] { eastY, "East Y" } };
            foreach (string[] sensor in sensors) {
                if (!validNumber(sensor[0])) {
                    return "Offset for " + sensor[1] + " sensor is not a number";
                }
            }

            if (!positiveNumber(stepCount)) {
                return "Stepper steps must be a positive number";
            }
            if (!positiveNumber(stepTime)) {
                return "Stepper duration must be a positive number";
            }
            if (!positiveNumber(paperTime)) {
                return "Direct Current time must be a positive number";
            }
            return null;
        }

        /** the command that stores these settings in the target and makes it report its settings back */
        public string toCommand() {
            StringBuilder sb = new StringBuilder("{");
            sb.Append("\"SENSOR\":" + sensorDiameter + ", ");
            sb.Append("\"X_OFFSET\":" + xOffset + ", ");
            sb.Append("\"Y_OFFSET\":" + yOffset + ", ");
            sb.Append("\"Z_OFFSET\":" + zOffset + ", ");
            if (isDirectCurrent()) {
                sb.Append("\"PAPER_TIME\":" + paperTime + ", ");
                sb.Append("\"STEP_TIME\":0, ");
                sb.Append("\"STEP_COUNT\":0, ");
            } else {
                sb.Append("\"PAPER_TIME\":0, ");
                sb.Append("\"STEP_TIME\":" + stepTime + ", ");
                sb.Append("\"STEP_COUNT\":" + stepCount + ", ");
            }
            sb.Append("\"NORTH_X\":" + northX + ", ");
            sb.Append("\"NORTH_Y\":" + northY + ", ");
            sb.Append("\"EAST_X\":" + eastX + ", ");
            sb.Append("\"EAST_Y\":" + eastY + ", ");
            sb.Append("\"SOUTH_X\":" + southX + ", ");
            sb.Append("\"SOUTH_Y\":" + southY + ", ");
            sb.Append("\"WEST_X\":" + westX + ", ");
            sb.Append("\"WEST_Y\":" + westY + ", ");
            sb.Append("\"LED_BRIGHT\":" + ledBrightness.ToString(CultureInfo.InvariantCulture) + ", ");
            sb.Append("\"NAME_ID\":" + nameId.ToString(CultureInfo.InvariantCulture) + ", ");
            sb.Append("\"ECHO\":9 }");
            return sb.ToString();
        }
    }
}
