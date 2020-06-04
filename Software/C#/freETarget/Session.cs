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

namespace freETarget {
    public class Session {
        public enum TargetType {
            Pistol,
            Rifle
        }

        public enum SessionType {
            Practice,
            Match,
            Final
        }


        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session data")]
        [DisplayName("Session ID")]
        [Description("Unique ID given to the session when it was saved")]
        public long id { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session configuration")]
        [DisplayName("Event")]
        [Description("Competition event type")]
        public EventType eventType { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session configuration")]
        [DisplayName("Target Type")]
        [Description("Type of target used by this event")]
        public TargetType targetType { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session configuration")]
        [DisplayName("Events shot count")]
        [Description("Maximum number of shots in this event")]
        public int numberOfShots { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session configuration")]
        [DisplayName("Decimal scoring")]
        [Description("Flag if this event uses decimal scoring")]
        public bool decimalScoring { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session configuration")]
        [DisplayName("Session Type")]
        [Description("Type of this session")]
        public SessionType sessionType { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session configuration")]
        [DisplayName("Event duration")]
        [Description("Duration in minutes for this event. -1 is the duration is unlimited")]
        public int minutes { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Description("Number of shots in this session")]
        [Category("Session data")]
        [DisplayName("Session Shots")]
        public int actualNumberOfShots { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session data")]
        [DisplayName("Score")]
        [Description("The integer score for this session")]
        public int score { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session data")]
        [DisplayName("Decimal score")]
        [Description("The decimal score for this session")]
        public decimal decimalScore { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session data")]
        [DisplayName("Average score")]
        [Description("Average shot score for this session")]
        public decimal averageScore { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session data")]
        [DisplayName("Inner tens")]
        [Description("Number of inner tens (Xs) in this session")]
        public int innerX { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session data")]
        [DisplayName("Windage")]
        [Description("Median point of impact horizontal deviation. Negative to the left, positive to the right")]
        public decimal xbar { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session data")]
        [DisplayName("Elevation")]
        [Description("Median point of impact vertical deviation. Negative down, positive up")]
        public decimal ybar { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session data")]
        [DisplayName("Mean radius")]
        [Description("Radius of the mean group")]
        public decimal rbar { get; set; }


        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session data")]
        [DisplayName("Group size")]
        [Description("Dimension of the group - the maximum distance between 2 shots, calculated from the center of the holes")]
        public decimal groupSize { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session data")]
        [DisplayName("Start time")]
        [Description("Time when the session started")]
        public DateTime startTime { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session data")]
        [DisplayName("End time")]
        [Description("Time when the session ended (was saved)")]
        public DateTime endTime { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session data")]
        [DisplayName("Average time")]
        [Description("Average time between 2 shots")]
        public TimeSpan averageTimePerShot { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session data")]
        [DisplayName("Shortest shot")]
        [Description("Shortest time interval between 2 shots")]
        public TimeSpan shortestShot { get; set; }

        [Browsable(true)]
        [ReadOnly(true)]
        [Category("Session data")]
        [DisplayName("Longest shot")]
        [Description("Longest time interval between 2 shots")]
        public TimeSpan longestShot { get; set; }



        [Browsable(false)]
        public string diaryEntry = "";



    private VirtualRO currentFinal = null;

        [Browsable(false)]
        public string user { get; set; }

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

        public static Session createNewSession(string name, string user) {
            return createNewSession(EventType.GetEvent(name), Settings.Default.name, Settings.Default.MatchShots);
        }
        public static Session createNewSession(EventType sessionType, string user, int noOfShots) {
            Session newSession = new Session();
            newSession.user = user;

            if (sessionType.Equals(EventType.AirPistolPractice)) {
                newSession.decimalScoring = false;
                newSession.sessionType = SessionType.Practice;
                newSession.eventType = EventType.AirPistolPractice;
                newSession.numberOfShots = -1;
                newSession.targetType = TargetType.Pistol;
                newSession.minutes = -1;
            } else if (sessionType.Equals(EventType.AirPistolMatch)) {
                newSession.decimalScoring = false;
                newSession.sessionType = SessionType.Match;
                newSession.eventType = EventType.AirPistolMatch;
                newSession.numberOfShots = noOfShots;
                if(noOfShots == 60) {
                    newSession.numberOfShots = ISSF.match60NoOfShots;
                    newSession.minutes = ISSF.match60Time;
                } else if (noOfShots == 40)  {
                    newSession.numberOfShots = ISSF.match40NoOfShots;
                    newSession.minutes = ISSF.match40Time;
                } else {
                    newSession.numberOfShots = -1;
                    newSession.minutes = -1;
                }
                newSession.targetType = TargetType.Pistol;
            } else if (sessionType.Equals(EventType.AirPistolFinal)) {
                newSession.decimalScoring = true;
                newSession.sessionType = SessionType.Final;
                newSession.eventType = EventType.AirPistolFinal;
                newSession.numberOfShots = ISSF.finalNoOfShots;
                newSession.targetType = TargetType.Pistol;
                newSession.minutes = -1;
                newSession.currentFinal = new VirtualRO();
            } else if (sessionType.Equals(EventType.AirRiflePractice)) {
                newSession.decimalScoring = true;
                newSession.sessionType = SessionType.Practice;
                newSession.eventType = EventType.AirRiflePractice;
                newSession.numberOfShots = -1;
                newSession.targetType = TargetType.Rifle;
                newSession.minutes = -1;
            } else if (sessionType.Equals(EventType.AirRifleMatch)) {
                newSession.decimalScoring = true;
                newSession.sessionType = SessionType.Match;
                newSession.eventType = EventType.AirRifleMatch;
                newSession.numberOfShots = noOfShots;
                newSession.targetType = TargetType.Rifle;
                if (noOfShots == 60) {
                    newSession.numberOfShots = ISSF.match60NoOfShots;
                    newSession.minutes = ISSF.match60Time;
                } else if (noOfShots == 40) {
                    newSession.numberOfShots = ISSF.match40NoOfShots;
                    newSession.minutes = ISSF.match40Time;
                } else {
                    newSession.numberOfShots = -1;
                    newSession.minutes = -1;
                }
            } else if (sessionType.Equals(EventType.AirRifleFinal)) {
                newSession.decimalScoring = true;
                newSession.sessionType = SessionType.Final;
                newSession.eventType = EventType.AirRifleFinal;
                newSession.numberOfShots = ISSF.finalNoOfShots;
                newSession.targetType = TargetType.Rifle;
                newSession.minutes = -1;
                newSession.currentFinal = new VirtualRO();
            } else {
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
            if (sessionType != SessionType.Final) {
                newSeries = (s.index % 10 == 0);
            } else {
                if (s.index < 10) {
                    newSeries = (s.index % 5 == 0);
                } else {
                    newSeries = (s.index % 2 == 0);
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
            if (sessionType != SessionType.Final) {
                if (endTime == DateTime.MinValue) {
                    ts = now - startTime;
                } else {
                    ts = endTime - now;
                    if(ts < TimeSpan.Zero) {
                        command = "End";
                    }
            
                }
            } else { //final - do per series timers
                if (currentFinal != null) {
                    ts = currentFinal.getTime(out command);
                } else {
                    ts = TimeSpan.FromSeconds(-1);
                    Console.WriteLine("VirtualRO not initialized");
                }
            }

            if (sessionType == SessionType.Practice) {
                c = Color.White;
            }else if (sessionType == SessionType.Match) {
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
    }
}
