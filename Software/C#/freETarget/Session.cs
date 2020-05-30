using freETarget.Properties;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace freETarget {
    class Session {
        public enum TargetType {
            Pistol,
            Rifle
        }

        public enum SessionType {
            Practice,
            Match,
            Final
        }

        public CourseOfFire courseOfFire { get; set; }
        public TargetType targetType { get; set; }
        public int numberOfShots { get; set; }
        public bool decimalScoring { get; set; }
        public SessionType sessionType;
        public int minutes { get; set; }
        public int score { get; set; }
        public decimal decimalScore { get; set; }
        public int innerX { get; set; }
        public decimal xbar { get; set; }
        public decimal ybar { get; set; }
        public decimal rbar { get; set; }


        internal List<Shot> Shots { get => shots; set => shots = value; }
        internal List<Shot> CurrentSeries { get => currentSeries; set => currentSeries = value; }
        internal List<List<Shot>> AllSeries { get => allSeries; set => allSeries = value; }

        private List<Shot> shots = new List<Shot>();

        private List<Shot> currentSeries = new List<Shot>();

        private List<List<Shot>> allSeries = new List<List<Shot>>();

        public DateTime startTime;
        public DateTime endTime;

        private VirtualRO currentFinal = null;

        public string user;
        public decimal averageScore;
        public TimeSpan averageTimePerShot;
        public TimeSpan shortestShot;
        public TimeSpan longestShot;
        public int actualNumberOfShots;
        public string diaryEntry = "";
        public long id= -1;

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
            return createNewSession(CourseOfFire.GetCourseOfFire(name), Settings.Default.name, Settings.Default.MatchShots);
        }
        public static Session createNewSession(CourseOfFire sessionType, string user, int noOfShots) {
            Session newSession = new Session();

            if (sessionType.Equals(CourseOfFire.AirPistolPractice)) {
                newSession.decimalScoring = false;
                newSession.sessionType = SessionType.Practice;
                newSession.courseOfFire = CourseOfFire.AirPistolPractice;
                newSession.numberOfShots = -1;
                newSession.targetType = TargetType.Pistol;
                newSession.minutes = -1;
            } else if (sessionType.Equals(CourseOfFire.AirPistolMatch)) {
                newSession.decimalScoring = false;
                newSession.sessionType = SessionType.Match;
                newSession.courseOfFire = CourseOfFire.AirPistolMatch;
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
            } else if (sessionType.Equals(CourseOfFire.AirPistolFinal)) {
                newSession.decimalScoring = true;
                newSession.sessionType = SessionType.Final;
                newSession.courseOfFire = CourseOfFire.AirPistolFinal;
                newSession.numberOfShots = ISSF.finalNoOfShots;
                newSession.targetType = TargetType.Pistol;
                newSession.minutes = -1;
                newSession.currentFinal = new VirtualRO();
            } else if (sessionType.Equals(CourseOfFire.AirRiflePractice)) {
                newSession.decimalScoring = true;
                newSession.sessionType = SessionType.Practice;
                newSession.courseOfFire = CourseOfFire.AirRiflePractice;
                newSession.numberOfShots = -1;
                newSession.targetType = TargetType.Rifle;
                newSession.minutes = -1;
            } else if (sessionType.Equals(CourseOfFire.AirRifleMatch)) {
                newSession.decimalScoring = true;
                newSession.sessionType = SessionType.Match;
                newSession.courseOfFire = CourseOfFire.AirRifleMatch;
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
            } else if (sessionType.Equals(CourseOfFire.AirRifleFinal)) {
                newSession.decimalScoring = true;
                newSession.sessionType = SessionType.Final;
                newSession.courseOfFire = CourseOfFire.AirRifleFinal;
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

        public bool addShot(Shot s) {
            this.Shots.Add(s);
            this.score += s.score;
            this.decimalScore += s.decimalScore;

            if (s.innerTen) {
                this.innerX++;
            }
            s.index = this.Shots.Count - 1;

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

            return newSeries;
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
                    Console.WriteLine(shortest.ToString());
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
