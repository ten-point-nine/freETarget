using freETarget.Properties;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Speech.Synthesis;
using System.Text;
using System.Threading.Tasks;

namespace freETarget {
    class VirtualRO {
        public bool firstSeries = false;
        public bool firstSeriesLoadCommand = false;
        public bool firstSeriesStartCommand = false;
        public bool firstSeriesStopCommand = false;

        public bool secondSeries = false;
        public bool secondSeriesLoadCommand = false;
        public bool secondSeriesStartCommand = false;
        public bool secondSeriesStopCommand = false;

        public bool singleShot = false;
        public bool singleShotLoadCommand = false;
        public bool singleShotStartCommand = false;
        public bool singleShotStopCommand = false;
        public bool finished = false;

        public int shotsCount = 0;

        public DateTime nextCommand = DateTime.MinValue;

        private readonly SpeechSynthesizer synth;

        private const int readyDelay = 3; //seconds
        private const int loadDelay = 5;
        private const int betweenSeries = 15;
        private const int betweenShots = 7;

        public VirtualRO() {
            synth = new SpeechSynthesizer();
            synth.SetOutputToDefaultAudioDevice();
        }

        public TimeSpan getTime(out string command) {
            command = "";
            TimeSpan ts = nextCommand - DateTime.Now;
            if (ts <= TimeSpan.Zero && firstSeries == false) { //first call
                nextCommand = DateTime.Now + TimeSpan.FromSeconds(readyDelay);
                firstSeries = true;
                command = "Ready";
            } else if (ts <= TimeSpan.Zero && firstSeries == true && firstSeriesLoadCommand == false) {
                string speach = "For the first competition series, LOAD!";
                speakCommand(speach);
                nextCommand = DateTime.Now + TimeSpan.FromSeconds(loadDelay);
                firstSeriesLoadCommand = true;
                command = "Load";
            } else if (ts <= TimeSpan.Zero && firstSeriesLoadCommand == true && firstSeriesStartCommand == false) {
                string speach = "START!";
                speakCommand(speach);
                nextCommand = DateTime.Now + TimeSpan.FromSeconds(ISSF.finalSeriesTime);
                command = "Start";
                firstSeriesStartCommand = true;
            } else if (ts <= TimeSpan.Zero && firstSeriesStartCommand == true && firstSeriesStopCommand == false) {
                string speach = "STOP!";
                speakCommand(speach);
                nextCommand = DateTime.Now + TimeSpan.FromSeconds(betweenSeries);
                firstSeriesStopCommand = true;
                shotsCount += 5;
                command = "Stop";
            } else if (ts <= TimeSpan.Zero && firstSeriesStopCommand == true && secondSeries == false) {
                nextCommand = DateTime.Now + TimeSpan.FromSeconds(readyDelay);
                secondSeries = true;
                command = "Ready";
            } else if (ts <= TimeSpan.Zero && secondSeries == true && secondSeriesLoadCommand == false) {
                string speach = "For the next competition series, LOAD!";
                speakCommand(speach);
                nextCommand = DateTime.Now + TimeSpan.FromSeconds(loadDelay);
                secondSeriesLoadCommand = true;
                command = "Load";
            } else if (ts <= TimeSpan.Zero && secondSeriesLoadCommand == true && secondSeriesStartCommand == false) {
                string speach = "START!";
                speakCommand(speach);
                nextCommand = DateTime.Now + TimeSpan.FromSeconds(ISSF.finalSeriesTime);
                command = "Start";
                secondSeriesStartCommand = true;
            } else if (ts <= TimeSpan.Zero && secondSeriesStartCommand == true && secondSeriesStopCommand == false) {
                string speach = "STOP!";
                speakCommand(speach);
                nextCommand = DateTime.Now + TimeSpan.FromSeconds(betweenSeries);
                secondSeriesStopCommand = true;
                command = "Stop";
                shotsCount += 5;
            } else if (ts <= TimeSpan.Zero && secondSeriesStopCommand == true && singleShot == false) {
                nextCommand = DateTime.Now + TimeSpan.FromSeconds(readyDelay);
                singleShot = true;
                command = "Ready";
            } else if (ts <= TimeSpan.Zero && singleShot == true && singleShotLoadCommand == false) {
                string speach = "For the next competition shot, LOAD!";
                speakCommand(speach);
                nextCommand = DateTime.Now + TimeSpan.FromSeconds(loadDelay);
                singleShotLoadCommand = true;
                command = "Load";
            } else if (ts <= TimeSpan.Zero && singleShotLoadCommand == true && singleShotStartCommand == false) {
                string speach = "START!";
                speakCommand(speach);
                nextCommand = DateTime.Now + TimeSpan.FromSeconds(ISSF.singleShotTime);
                command = "Start";
                singleShotStartCommand = true;
            } else if (ts <= TimeSpan.Zero && singleShotStartCommand == true && singleShotStopCommand == false) {
                string speach = "STOP!";
                speakCommand(speach);
                shotsCount += 1;
                if (shotsCount % 2 == 0) {
                    nextCommand = DateTime.Now + TimeSpan.FromSeconds(betweenSeries); //more time between odd shots - eliminations announcements
                } else {
                    nextCommand = DateTime.Now + TimeSpan.FromSeconds(betweenShots);
                }

                command = "Stop";
                
                if (shotsCount < ISSF.finalNoOfShots) {
                    singleShot = false;
                    singleShotStartCommand = false;
                    singleShotLoadCommand = false;
                    singleShotStopCommand = false;
                } else {
                    singleShotStopCommand = true;
                    nextCommand = DateTime.Now + TimeSpan.FromSeconds(1);
                }
            } else if(ts <= TimeSpan.Zero && singleShotStopCommand == true && finished == false) {
                command = "End";
                finished = true;
                string speach = "Unload! Results are final!";
                speakCommand(speach);
            } else if (finished == true) {
                command = "End";
            }

            return ts;
        }

        private void speakCommand(string speach) {
            if (Settings.Default.voiceCommands) {
                synth.SpeakAsync(speach);
            }
        }

        public void speakShot(Shot shot) {
            if (Settings.Default.scoreVoice) {
                if (shot.miss) {
                    synth.SpeakAsync("Miss");
                } else {
                    decimal x = 360 - shot.angle + 90;
                    string oclock = Math.Round(x % 360 / 30, 0, MidpointRounding.AwayFromZero).ToString();
                    if (oclock == "0") {
                        oclock = "12";
                    }

                    string decimals = "";
                    if (shot.score - shot.decimalScore == 0) {
                        decimals = "0";
                    } else if (shot.score - shot.decimalScore > 0) {
                        decimals = (shot.score - shot.decimalScore).ToString();
                        decimals = decimals.Substring(2);
                    } else {
                        decimals = (shot.score - shot.decimalScore).ToString();
                        decimals = decimals.Substring(3);
                    }
                    string speach = shot.score + " point " + decimals + " , at " + oclock + " o'clock.";
                    synth.SpeakAsync(speach);
                }
            }
        }
    }
}
