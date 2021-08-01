using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Drawing;

namespace freETarget {
    public class Event {

        public enum EventType {
            Practice,
            Match,
            Final
        }

        public long ID { get; }

        public string Name { get; set; }

        public bool DecimalScoring { get; set; }

        public EventType Type { get; set; }

        public int NumberOfShots { get; set; }

        public targets.aTarget Target { get; set; }

        public int Minutes { get; set; }

        public decimal ProjectileCaliber { get; set; }

        public int Final_NumberOfShotPerSeries { get; set; } //5 

        public int Final_SeriesSeconds { get; set; } //250

        public int Final_NumberOfShotsBeforeSingleShotSeries { get; set; } //10 for air pistol/rifle

        public int Final_NumberOfShotsInSingleShotSeries { get; set; } //2 for air pistol/rifle

        public int Final_SingleShotSeconds { get; set; } //50

        public Color TabColor { get; set; }

        public Event(long id, string name, bool decimalScoring, EventType type, int numberOfShots, targets.aTarget target, int minutes, decimal caliber, 
            int final_seriesShots, int seriesSeconds, int shotsInSeries, int shotsInSingle, int singleSeconds, Color tabColor )  {
            this.ID = id;
            this.Name = name;
            this.DecimalScoring = decimalScoring;
            this.Type = type;
            this.NumberOfShots = numberOfShots;
            this.Target = target;
            this.Minutes = minutes;
            this.ProjectileCaliber = caliber;
            this.Final_NumberOfShotPerSeries = final_seriesShots;
            this.Final_SeriesSeconds = seriesSeconds;
            this.Final_NumberOfShotsBeforeSingleShotSeries = shotsInSeries;
            this.Final_NumberOfShotsInSingleShotSeries = shotsInSingle;
            this.Final_SingleShotSeconds = singleSeconds;
            this.TabColor = tabColor;
        }

        public override string ToString() {
            return this.Name;
        }
    }
}
