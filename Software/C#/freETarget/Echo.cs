using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Globalization;

namespace freETarget {
    public class Echo {

        public string NAME;
        public int ANGLE;
        public int CALIBREx10;
        public int DIP;
        public int LED_BRIGHT;
        public string MFS;
        public int NAME_ID;
        public int PAPER_ECO;
        public int PAPER_TIME;
        public int POWER_SAVE;
        public int SEND_MISS;
        public decimal SENSOR;
        public int SN;
        public int STEP_COUNT;
        public int STEP_TIME;
        public int TARGET_TYPE;
        public int TEST;
        public int TRGT_1_RINGx10;
        public int Z_OFFSET;
        public int NORTH_X;
        public int NORTH_Y;
        public int EAST_X;
        public int EAST_Y;
        public int SOUTH_X;
        public int SOUTH_Y;
        public int WEST_X;
        public int WEST_Y;
        public int IS_TRACE;
        public decimal TEMPERATURE;
        public decimal SPEED_SOUND;
        public decimal V_REF;
        public int TIMER_COUNT;
        public int WiFi;
        public string VERSION;
        public int BRD_REV;
        public int INIT;


        private Echo() {

        }
        public static Echo parseJson(string json) {

            string[] t2 = json.Split(',');
            if (t2[0].Contains("NAME")) {
                Echo ret = new Echo();
                foreach (string t3 in t2) {
                    string[] t4 = t3.Split(':');
                    switch (t4[0].Trim()) {
                        case "\"NAME\"":
                            ret.NAME = t4[1].ToString();
                            break;
                        case "\"ANGLE\"":
                            ret.ANGLE = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"CALIBREx10\"":
                            ret.CALIBREx10 = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"DIP\"":
                            ret.DIP = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"LED_BRIGHT\"":
                            ret.LED_BRIGHT = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"MFS\"":
                            ret.MFS = t4[1].ToString();
                            break;
                        case "\"NAME_ID\"":
                            ret.NAME_ID = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"PAPER_ECO\"":
                            ret.PAPER_ECO = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"PAPER_TIME\"":
                            ret.PAPER_TIME = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"POWER_SAVE\"":
                            ret.POWER_SAVE = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"SEND_MISS\"":
                            ret.SEND_MISS = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"SENSOR\"":
                            ret.SENSOR = decimal.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"SN\"":
                            ret.SN = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"STEP_COUNT\"":
                            ret.STEP_COUNT = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"STEP_TIME\"":
                            ret.STEP_TIME = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"TARGET_TYPE\"":
                            ret.TARGET_TYPE = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"TEST\"":
                            ret.TEST = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"TRGT_1_RINGx10\"":
                            ret.TRGT_1_RINGx10 = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"Z_OFFSET\"":
                            ret.Z_OFFSET = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"NORTH_X\"":
                            ret.NORTH_X = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"NORTH_Y\"":
                            ret.NORTH_Y = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"EAST_X\"":
                            ret.EAST_X = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"EAST_Y\"":
                            ret.EAST_Y = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"SOUTH_X\"":
                            ret.SOUTH_X = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"SOUTH_Y\"":
                            ret.SOUTH_Y = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"WEST_X\"":
                            ret.WEST_X = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"WEST_Y\"":
                            ret.WEST_Y = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"IS_TRACE\"":
                            ret.IS_TRACE = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"TEMPERATURE\"":
                            ret.TEMPERATURE = decimal.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"SPEED_SOUND\"":
                            ret.SPEED_SOUND = decimal.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"V_REF\"":
                            ret.V_REF = decimal.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"TIMER_COUNT\"":
                            ret.TIMER_COUNT = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"WiFi\"":
                            ret.WiFi = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"VERSION\"":
                            ret.VERSION = t4[1].ToString();
                            break;
                        case "\"BRD_REV\"":
                            ret.BRD_REV = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;
                        case "\"INIT\"":
                            ret.INIT = int.Parse(t4[1], CultureInfo.InvariantCulture);
                            break;

                    }
                }
                return ret;
            } else {
                return null;
            }
        }
    }



}
