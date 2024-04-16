using freETarget.Properties;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Speech.Synthesis;
using System.Text;
using System.Threading.Tasks;

namespace freETarget {
    [Serializable]
    class VirtualRO {
        private bool firstSeries = false;
        private bool firstSeriesLoadCommand = false;
        private bool firstSeriesStartCommand = false;
        private bool firstSeriesStopCommand = false;

        private bool secondSeries = false;
        private bool secondSeriesLoadCommand = false;
        private bool secondSeriesStartCommand = false;
        private bool secondSeriesStopCommand = false;

        private bool singleShot = false;
        private bool singleShotLoadCommand = false;
        private bool singleShotStartCommand = false;
        private bool singleShotStopCommand = false;
        
        private bool finished = false;


        private bool RFseriesLoadCommand = false;
        private bool RFseriesStartCommand = false;
        private bool RFseriesStopCommand = false;


        private int shotsCount = 0;

        private DateTime nextCommand = DateTime.MinValue;

        private readonly SpeechSynthesizer synth;

        private const int readyDelay = 3; //seconds
        private const int loadDelay = 7;
        private const int betweenSeries = 15;
        private const int betweenShots = 3;

        private const int RFcooldown = 1; //seconds

        private Event ev;
        private Session session;

        public VirtualRO(Session session) {
            synth = new SpeechSynthesizer();
            synth.SetOutputToDefaultAudioDevice();

            this.session = session;
            this.ev = session.eventType;
        }

        public void reset() {
            //put all flags to false

            nextCommand = DateTime.MinValue;

            firstSeries = false;
            firstSeriesLoadCommand = false;
            firstSeriesStartCommand = false;
            firstSeriesStopCommand = false;

            secondSeries = false;
            secondSeriesLoadCommand = false;
            secondSeriesStartCommand = false;
            secondSeriesStopCommand = false;

            singleShot = false;
            singleShotLoadCommand = false;
            singleShotStartCommand = false;
            singleShotStopCommand = false;

            finished = false;


            RFseriesLoadCommand = false;
            RFseriesStartCommand = false;
            RFseriesStopCommand = false;


            shotsCount = 0;
        }

        public TimeSpan getTime(out string command) {
            command = "";
            TimeSpan ts = nextCommand - DateTime.Now;

            //first series

            if (ts <= TimeSpan.Zero && firstSeries == false) { 
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
                nextCommand = DateTime.Now + TimeSpan.FromSeconds(ev.Final_SeriesSeconds);
                command = "Start";
                firstSeriesStartCommand = true;
            } else if (ts <= TimeSpan.Zero && firstSeriesStartCommand == true && firstSeriesStopCommand == false) {
                string speach = "STOP!";
                speakCommand(speach);
                nextCommand = DateTime.Now + TimeSpan.FromSeconds(betweenSeries);
                firstSeriesStopCommand = true;
                shotsCount += ev.Final_NumberOfShotPerSeries;
                command = "Stop";

            //second series onward

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
                nextCommand = DateTime.Now + TimeSpan.FromSeconds(ev.Final_SeriesSeconds);
                command = "Start";
                secondSeriesStartCommand = true;
            } else if (ts <= TimeSpan.Zero && secondSeriesStartCommand == true && secondSeriesStopCommand == false) {
                string speach = "STOP!";
                speakCommand(speach);
                nextCommand = DateTime.Now + TimeSpan.FromSeconds(betweenSeries);
                command = "Stop";
                shotsCount += ev.Final_NumberOfShotPerSeries;
                if (shotsCount < ev.Final_NumberOfShotsBeforeSingleShotSeries) {
                    secondSeries = false;
                    secondSeriesLoadCommand = false;
                    secondSeriesStartCommand = false;
                    secondSeriesStopCommand = false;
                } else {
                    secondSeriesStopCommand = true;
                }

             //single shots

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
                nextCommand = DateTime.Now + TimeSpan.FromSeconds(ev.Final_SingleShotSeconds);
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
                
                if (shotsCount < ev.NumberOfShots) {
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

        public TimeSpan getRFTime(out string command) {
            command = "";
            TimeSpan ts = nextCommand - DateTime.Now;

            if (session.RFseriesActive) {
                if (ts <= TimeSpan.Zero && RFseriesLoadCommand == false) {
                    string speach = "LOAD!";
                    speakCommand(speach);
                    nextCommand = DateTime.Now + TimeSpan.FromSeconds(ev.RF_LoadTime);
                    RFseriesLoadCommand = true;
                    command = "Load";
                } else if (ts <= TimeSpan.Zero && RFseriesLoadCommand == true && RFseriesStartCommand == false) {
                    string speach = "ATTENTION!";
                    speakCommand(speach);
                    if (this.session.eventType.RF_TimePerShot > 0) {
                        //duel
                        nextCommand = DateTime.Now + TimeSpan.FromSeconds((ev.RF_TimeBetweenShots + ev.RF_TimePerShot) * ev.RF_NumberOfShots);
                    } else {
                        //rapid fire
                        nextCommand = DateTime.Now + TimeSpan.FromSeconds(ev.RF_TimeBetweenShots + ev.RF_TimePerSerie);
                    }
                    RFseriesStartCommand = true;
                    command = "Attent";
                } else if (ts <= TimeSpan.Zero && RFseriesStartCommand == true && RFseriesStopCommand == false) {
                    nextCommand = DateTime.Now + TimeSpan.FromSeconds(RFcooldown);
                    RFseriesStopCommand = true;
                    command = "";
                } else if (ts <= TimeSpan.Zero && RFseriesStopCommand == true && finished == false) {
                    string speach = "UNLOAD!";
                    speakCommand(speach);
                    finished = true;
                    command = "Unload";
                } else if (finished == true) {
                    command = "End";
                }          
            } else {
                return TimeSpan.Zero;
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
