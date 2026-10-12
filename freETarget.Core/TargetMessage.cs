using freETarget.Properties;
using System;
using System.Globalization;

namespace freETarget {

    /**
     * collects the text received from the target and cuts it into messages.
     * a message is the text between an opening and the next closing accolade, the target sends one for every shot and as answer to commands
     */
    public class MessageReader {

        private string buffer = "";

        public void append(string data) {
            buffer += data;
        }

        /**
         * returns the next complete message without its accolades, or null if there is none yet.
         * text before the opening accolade is dropped
         */
        public string next() {
            int indexOpenBracket = buffer.IndexOf('{');
            if (indexOpenBracket > -1) {
                int indexClosedBracket = buffer.IndexOf('}', indexOpenBracket);

                if (indexClosedBracket > -1) {
                    string message = buffer.Substring(indexOpenBracket + 1, indexClosedBracket - indexOpenBracket - 1);
                    buffer = buffer.Substring(indexClosedBracket + 1);
                    return message;
                }
            }
            return null;
        }
    }

    public static class TargetMessage {

        public enum Kind {
            Shot,       //a shot (or a miss)
            BadShot,    //the target detected a shot but could not locate it. it sends the raw timer values instead
            Other       //anything else, usually the answer to an ECHO command
        }

        /** incoming message contains all the data without the accolades */
        public static Kind classify(string message) {
            string first = message.Split(',')[0];
            if (first.Contains("shot")) {
                return Kind.Shot;
            } else if (first.Contains("timer")) {
                return Kind.BadShot;
            } else {
                return Kind.Other;
            }
        }

        /** coordinates are scaled so that a target at a reduced distance scores like one at the full distance */
        public static decimal getScaledDimension(decimal input) {
            decimal ret = 100 * input / Settings.Default.targetDistance;
            ret = decimal.Round(ret, 9, MidpointRounding.AwayFromZero);
            return ret;
        }

        /**
         * parses a shot message and computes its score on the given target.
         * if the message cannot be parsed, the returned shot has a count of -1
         */
        public static Shot parseShot(string message, decimal calibrationX, decimal calibrationY, decimal calibrationAngle, targets.aTarget target) {
            string[] t2 = message.Split(',');

            Shot ret = new Shot(calibrationX, calibrationY, calibrationAngle);
            try {
                foreach (string t3 in t2) {
                    string[] t4 = t3.Split(':');
                    if (t4[0].Trim() == "\"shot\"") {
                        ret.count = int.Parse(t4[1], CultureInfo.InvariantCulture);
                    } else if (t4[0].Trim() == "\"x\"") {
                        ret.setX(getScaledDimension(decimal.Parse(t4[1], CultureInfo.InvariantCulture)));
                    } else if (t4[0].Trim() == "\"y\"") {
                        ret.setY(getScaledDimension(decimal.Parse(t4[1], CultureInfo.InvariantCulture)));
                    } else if (t4[0].Trim() == "\"r\"") {
                        ret.radius = getScaledDimension(decimal.Parse(t4[1], CultureInfo.InvariantCulture));
                    } else if (t4[0].Trim() == "\"a\"") {
                        ret.angle = decimal.Parse(t4[1], CultureInfo.InvariantCulture);
                    } else if (t4[0].Trim() == "\"miss\"") {
                        int mix = int.Parse(t4[1].Trim(), CultureInfo.InvariantCulture);
                        if (mix == 1) {
                            //miss reported by the target
                            ret.miss = true;
                        }
                    }
                }
            } catch (FormatException ex) {
                Console.WriteLine("Could not parse: " + message + " Error: " + ex.Message);
                Shot err = new Shot(calibrationX, calibrationY, calibrationAngle);
                err.count = -1;
                return err;
            }

            ret.computeScore(target);

            ret.timestamp = DateTime.Now;

            return ret;
        }
    }
}
