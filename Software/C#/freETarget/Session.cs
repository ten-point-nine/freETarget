using freETarget.Properties;
using System;
using System.Collections.Generic;
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


        public SessionType sessionType { get; set; }
        public TargetType targetType { get; set; }
        public int numberOfShots { get; set; }
        public bool decimalScoring { get; set; }
        public bool final { get; set; }
        public bool practice { get; set; }
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

        public DateTime startTime { get; private set; }
        public DateTime endTime { get; private set; }

        public Session() {

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

        public static Session createNewSession(string name) {
            return createNewSession(SessionType.GetSessionType(name));
        }
        public static Session createNewSession(SessionType sessionType) {
            Session newSession = new Session();

            if (sessionType.Equals(SessionType.AirPistolPractice)) {
                newSession.decimalScoring = false;
                newSession.final = false;
                newSession.sessionType = SessionType.AirPistolPractice;
                newSession.numberOfShots = -1;
                newSession.targetType = TargetType.Pistol;
                newSession.practice = true;
                newSession.minutes = -1;
            } else if (sessionType.Equals(SessionType.AirPistolMatch)) {
                newSession.decimalScoring = false;
                newSession.final = false;
                newSession.sessionType = SessionType.AirPistolMatch;
                newSession.numberOfShots = Settings.Default.MatchShots;
                newSession.targetType = TargetType.Pistol;
                newSession.practice = false;
                if (newSession.numberOfShots == 60) {
                    newSession.minutes = 75;
                } else if (newSession.numberOfShots == 40) {
                    newSession.minutes = 50;
                } else {
                    newSession.minutes = -1;
                }
            } else if (sessionType.Equals(SessionType.AirPistolFinal)) {
                newSession.decimalScoring = true;
                newSession.final = true;
                newSession.sessionType = SessionType.AirPistolFinal;
                newSession.numberOfShots = 24;
                newSession.targetType = TargetType.Pistol;
                newSession.practice = false;
                newSession.minutes = -1;
            } else if (sessionType.Equals(SessionType.AirRiflePractice)) {
                newSession.decimalScoring = true;
                newSession.final = false;
                newSession.sessionType = SessionType.AirRiflePractice;
                newSession.numberOfShots = -1;
                newSession.targetType = TargetType.Rifle;
                newSession.practice = true;
                newSession.minutes = -1;
            } else if (sessionType.Equals(SessionType.AirRifleMatch)) {
                newSession.decimalScoring = true;
                newSession.final = false;
                newSession.sessionType = SessionType.AirRifleMatch;
                newSession.numberOfShots = Settings.Default.MatchShots;
                newSession.targetType = TargetType.Rifle;
                newSession.practice = false;
                if (newSession.numberOfShots == 60) {
                    newSession.minutes = 75;
                } else if (newSession.numberOfShots == 40) {
                    newSession.minutes = 50;
                } else {
                    newSession.minutes = -1;
                }
            } else if (sessionType.Equals(SessionType.AirRifleFinal)) {
                newSession.decimalScoring = true;
                newSession.final = true;
                newSession.sessionType = SessionType.AirRifleFinal;
                newSession.numberOfShots = 24;
                newSession.targetType = TargetType.Rifle;
                newSession.practice = false;
                newSession.minutes = -1;
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
            if (this.final == false) {
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

    }
}
