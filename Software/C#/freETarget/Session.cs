using freETarget.Properties;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace freETarget {
    public class Session {

/*        public enum SessionType {
            Practice,
            Match,
            Final
        }*/


        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session")]
        [DisplayName("Session ID")]
        [Description("Unique ID given to the session when it was saved")]
        public long id { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Event")]
        [DisplayName("Event Name")]
        [Description("Competition event type")]
        public Event eventType { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Event")]
        [DisplayName("Target Type")]
        [Description("Type of target used by this event")]
        public string targetType { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Event")]
        [DisplayName("Events shot count")]
        [Description("Maximum number of shots in this event")]
        public int numberOfShots { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Event")]
        [DisplayName("Decimal scoring")]
        [Description("Flag if this event uses decimal scoring")]
        public bool decimalScoring { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Event")]
        [DisplayName("Session Type")]
        [Description("Type of this session")]
        public Event.EventType sessionType { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Event")]
        [DisplayName("Event duration")]
        [Description("Duration in minutes for this event. -1 if the duration is unlimited")]
        public int minutes { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Description("Number of shots in this session")]
        [Category("Session")]
        [DisplayName("Session Shots")]
        public int actualNumberOfShots { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session")]
        [DisplayName("Score")]
        [Description("The integer score for this session")]
        public int score { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session")]
        [DisplayName("Decimal score")]
        [Description("The decimal score for this session")]
        public decimal decimalScore { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session")]
        [DisplayName("Average score")]
        [Description("Average shot score for this session")]
        public decimal averageScore { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session")]
        [DisplayName("Inner tens")]
        [Description("Number of inner tens (Xs) in this session")]
        public int innerX { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session")]
        [DisplayName("Windage")]
        [Description("Median point of impact horizontal deviation. Negative to the left, positive to the right")]
        public decimal xbar { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session")]
        [DisplayName("Elevation")]
        [Description("Median point of impact vertical deviation. Negative down, positive up")]
        public decimal ybar { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session")]
        [DisplayName("Mean radius")]
        [Description("Radius of the mean group")]
        public decimal rbar { get; set; }


        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session")]
        [DisplayName("Group size")]
        [Description("Dimension of the group - the maximum distance between 2 shots, calculated from the center of the holes")]
        public decimal groupSize { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session")]
        [DisplayName("Start time")]
        [Description("Time when the session started")]
        public DateTime startTime { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session")]
        [DisplayName("End time")]
        [Description("Time when the session ended (was saved)")]
        public DateTime endTime { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session")]
        [DisplayName("Average time")]
        [Description("Average time between 2 shots")]
        public TimeSpan averageTimePerShot { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session")]
        [DisplayName("Shortest shot")]
        [Description("Shortest time interval between 2 shots")]
        public TimeSpan shortestShot { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session")]
        [DisplayName("Longest shot")]
        [Description("Longest time interval between 2 shots")]
        public TimeSpan longestShot { get; set; }

        [Browsable(false)]
        public string diaryEntry = "";

        [Browsable(false)]
        public string user { get; set; }


        [Browsable(false)]
        private targets.aTarget target;



        private VirtualRO vRO = null;

        internal List<Shot> Shots { get => shots; set => shots = value; }
        internal List<Shot> CurrentSeries { get => currentSeries; set => currentSeries = value; }
        internal List<List<Shot>> AllSeries { get => allSeries; set => allSeries = value; }
        internal List<Shot> LoadedShots { get => loadedShots; set => loadedShots = value; }

        private List<Shot> shots = new List<Shot>();

        private List<Shot> loadedShots = new List<Shot>();

        private List<Shot> currentSeries = new List<Shot>();

        private List<List<Shot>> allSeries = new List<List<Shot>>();

        protected Session() {

        }

        public targets.aTarget getTarget() {
            return this.target;
        }

        public void setTarget(targets.aTarget trgt) {
            this.target = trgt;
        }

        public void clear() {
            this.Shots.Clear();
            this.score = 0;
            this.decimalScore = 0;
            this.innerX = 0;
            this.xbar = 0;
            this.ybar = 0;
            this.rbar = 0;

            this.CurrentSeries.Clear();
            this.AllSeries.Clear();
        }
        public static Session createNewSession(Event eventType, string user) {
            Session newSession = new Session();
            newSession.user = user;

            newSession.eventType = eventType;
            newSession.decimalScoring = eventType.DecimalScoring;
            newSession.sessionType = eventType.Type;
            newSession.setTarget(eventType.Target);
            newSession.targetType = eventType.Target.getName();

            if (newSession.sessionType.Equals(Event.EventType.Practice)) {
                newSession.numberOfShots = -1;
                newSession.minutes = -1;
            } else if (newSession.sessionType.Equals(Event.EventType.Match)) {
                newSession.numberOfShots = eventType.NumberOfShots;
                newSession.minutes = eventType.Minutes;
            } else if (newSession.sessionType.Equals(Event.EventType.Final)) {
                newSession.numberOfShots = eventType.NumberOfShots;
                newSession.minutes = -1;
                newSession.vRO = new VirtualRO(eventType);
            } else {
                Console.WriteLine("Could not identify event type " + eventType + " and session type " + newSession.sessionType);
                return null;
            }

            newSession.AllSeries.Add(newSession.currentSeries);
            return newSession;
        }

        public void addShot(Shot s) {
            this.Shots.Add(s);
            this.score += s.score;
            this.decimalScore += s.decimalScore;

            if (s.innerTen) {
                this.innerX++;
            }
            s.index = this.Shots.Count - 1;

            if (this.allSeries.Count > 0) {
                this.currentSeries = this.allSeries[this.allSeries.Count - 1];
            }
            addToSeries(s);

        }


        public void addLoadedShot(Shot s) {
            this.LoadedShots.Add(s);

            addToSeries(s);
        }

        public void addToSeries(Shot s) {
            bool newSeries;
            if (sessionType != Event.EventType.Final) {
                newSeries = (s.index % 10 == 0);
            } else {
                if (s.index < eventType.Final_NumberOfShotsBeforeSingleShotSeries) {
                    newSeries = (s.index % eventType.Final_NumberOfShotPerSeries == 0);
                } else {
                    newSeries = (s.index % eventType.Final_NumberOfShotsInSingleShotSeries == 0);
                }
            }

            if (newSeries) {
                currentSeries = new List<Shot>();
                allSeries.Add(currentSeries);
                currentSeries.Add(s);
            } else {
                currentSeries.Add(s);
            }
        }

        public void start() {
            this.startTime = DateTime.Now;
            if (this.minutes > 0) {
                this.endTime = DateTime.Now.AddMinutes(this.minutes);
            } else {
                this.endTime = DateTime.MinValue;
            }
        }

        public string getTime(out Color c) { //it is called every 500 miliseconds by a timer
            DateTime now = DateTime.Now;
            TimeSpan ts;
            string command = "";
            if (sessionType != Event.EventType.Final) {
                if (endTime == DateTime.MinValue) {
                    ts = now - startTime;
                } else {
                    ts = endTime - now;
                    if(ts < TimeSpan.Zero) {
                        command = "End";
                    }
            
                }
            } else { //final - do per series timers
                if (vRO != null) {
                    ts = vRO.getTime(out command);
                } else {
                    ts = TimeSpan.FromSeconds(-1);
                    Console.WriteLine("VirtualRO not initialized");
                }
            }

            if (sessionType == Event.EventType.Practice) {
                c = Color.White;
            }else if (sessionType == Event.EventType.Match) {
                if (TimeSpan.Compare(ts, TimeSpan.FromSeconds(60)) <= 0) { //last 60 seconds displayed in red
                    c = Color.Red;
                } else if (TimeSpan.Compare(ts, TimeSpan.FromMinutes(10)) <= 0) { //last 10 minutes displayed in orange
                    c = Color.Orange;
                } else {
                    c = Color.White;
                }
            } else { //final
                if (TimeSpan.Compare(ts, TimeSpan.FromSeconds(5)) <= 0) { //last 5 seconds displayed in red
                    c = Color.Red;
                } else if (TimeSpan.Compare(ts, TimeSpan.FromSeconds(15)) <= 0) { //last 15 seconds displayed in orange
                    c = Color.Orange;
                } else {
                    c = Color.White;
                }
            }

            if (ts < TimeSpan.Zero) {
                c = Color.DeepSkyBlue;
                return "@"+command;
            } else {
                return ts.ToString(@"hhmmss"); 
            }
        }

        public void prepareForSaving() {
            this.user = Settings.Default.name;
            this.endTime = DateTime.Now;
            this.actualNumberOfShots = this.shots.Count;

            decimal sum = 0;
            TimeSpan shortest = TimeSpan.MaxValue;
            TimeSpan longest = TimeSpan.MinValue;

            for(int i = 0; i<shots.Count;i++) {
                Shot s = shots[i];
                if (i == 0) {
                    s.shotDuration = s.timestamp - this.startTime;
                } else {
                    s.shotDuration = s.timestamp - shots[i - 1].timestamp;
                }

                if (s.shotDuration > longest) {
                    longest = s.shotDuration;
                }

                if(s.shotDuration < shortest) {
                    shortest = s.shotDuration;
                }

                if (this.decimalScoring) {
                    sum += s.decimalScore;
                } else {
                    sum += s.score;
                }

            }

            this.averageTimePerShot = TimeSpan.FromTicks( (shots[shots.Count - 1].timestamp - shots[0].timestamp).Ticks /  shots.Count);
            this.longestShot = longest;
            this.shortestShot = shortest;
            this.averageScore = sum / actualNumberOfShots;
        }

        public override string ToString() {
            return this.eventType.ToString();
        }
    }
}
