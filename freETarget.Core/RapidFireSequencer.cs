using System.Globalization;

namespace freETarget {

    /** what to do on a tick of the rapid fire timer */
    public class RapidFireStep {
        /** command to send to the target. null if there is nothing to send */
        public string command;
        /** new interval of the timer in miliseconds. 0 keeps the current interval */
        public int nextInterval;
        /** the sequence is over, stop the timer */
        public bool finished;
        /** the event has neither a time per shot nor a time per series */
        public bool misconfigured;
    }

    /**
     * sequence of commands that runs one rapid fire series on the target:
     * load time, then either one command for the whole series or one command per shot (duel), then a cooldown, then the command that ends the series.
     * the caller owns the timer: start() gives the first interval, every tick of the timer calls tick()
     */
    public class RapidFireSequencer {

        public const int Cooldown = 3; //seconds
        public const string DisableCommand = "{\"RAPID_ENABLE\":0 }";

        private const int notStarted = -1;
        private const int coolingDown = 1000;

        private readonly Event ev;
        private int duelCounter = notStarted;

        public RapidFireSequencer(Event ev) {
            this.ev = ev;
        }

        /** starts the load time. returns the interval in miliseconds until the first tick */
        public int start() {
            duelCounter = notStarted;
            return ev.RF_LoadTime * 1000;
        }

        private string enableCommand(int count, decimal time) {
            return "{"
                + "\"RAPID_COUNT\":" + count.ToString(CultureInfo.InvariantCulture) + ", "
                + "\"RAPID_WAIT\":" + ev.RF_TimeBetweenShots.ToString(CultureInfo.InvariantCulture) + ", "
                + "\"RAPID_TIME\":" + time.ToString(CultureInfo.InvariantCulture) + ", "
                + "\"RAPID_ENABLE\":1"
                + " }";
        }

        public RapidFireStep tick() {
            RapidFireStep step = new RapidFireStep();

            if (duelCounter == notStarted) {
                //first tick - load time over
                duelCounter = 0;

                //send first commands to target
                if (ev.RF_TimePerShot > 0) {
                    //duel
                    step.command = enableCommand(1, ev.RF_TimePerShot);
                    duelCounter++;
                    step.nextInterval = (int)((ev.RF_TimeBetweenShots + ev.RF_TimePerShot) * 1000);
                } else if (ev.RF_TimePerSerie > 0) {
                    //normal rapid fire
                    step.command = enableCommand(ev.RF_NumberOfShots, ev.RF_TimePerSerie);
                    step.nextInterval = (int)((ev.RF_TimeBetweenShots + ev.RF_TimePerSerie) * 1000);
                } else {
                    step.misconfigured = true;
                    step.finished = true;
                }
            } else if (duelCounter >= 0 && duelCounter < coolingDown) {
                //second tick onward
                if (ev.RF_TimePerShot > 0) {
                    //duel
                    if (duelCounter < ev.RF_NumberOfShots) {
                        step.command = enableCommand(1, ev.RF_TimePerShot);
                        duelCounter++;
                    } else {
                        step.nextInterval = Cooldown * 1000;
                        duelCounter = coolingDown;
                    }
                } else if (ev.RF_TimePerSerie > 0) {
                    //normal rapid fire
                    step.nextInterval = Cooldown * 1000;
                    duelCounter = coolingDown;
                } else {
                    step.misconfigured = true;
                    step.finished = true;
                }
            } else {
                //last tick
                step.command = DisableCommand;
                step.finished = true;
            }

            return step;
        }
    }
}
