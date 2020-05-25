using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace freETarget {
    class Session {
        public enum TargetType {
            Pistol,
            Rifle
        }


        public string name { get; set; }
        public TargetType type { get; set; }
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

        public DateTime startTime { get; set; }
        public DateTime endTime { get; set; }

        public Session() {

        }

        public Session(string name, TargetType type, int numberOfShots, bool decimalScoring, bool final, bool practice, int minutes) {
            this.name = name;
            this.type = type;
            this.numberOfShots = numberOfShots;
            this.decimalScoring = decimalScoring;
            this.final = final;
            this.practice = practice;
            this.minutes = minutes;
        }

        public void Clear() {
            this.Shots.Clear();
            this.score = 0;
            this.decimalScore = 0;
            this.innerX = 0;
            this.xbar = 0;
            this.ybar = 0;
            this.rbar = 0;
        }

    }
}
