using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace freETarget {
    public class EventType {

        private EventType(string name) { Name = name; }
        public string Name { get; set; }

        public static EventType AirPistolPractice { get { return new EventType("Air Pistol Practice"); } }
        public static EventType AirPistolMatch { get { return new EventType("Air Pistol Match"); } }
        public static EventType AirPistolFinal { get { return new EventType("Air Pistol Final"); } }
        public static EventType AirRiflePractice { get { return new EventType("Air Rifle Practice"); } }
        public static EventType AirRifleMatch { get { return new EventType("Air Rifle Match"); } }
        public static EventType AirRifleFinal { get { return new EventType("Air Rifle Final"); } }

 
        public static EventType GetEvent(string name) {
            if (EventType.AirPistolPractice.Name.Contains(name)) {
                return AirPistolPractice;
            } else if (EventType.AirPistolMatch.Name.Contains(name)) {
                return AirPistolMatch;
            } else if (EventType.AirPistolFinal.Name.Contains(name)) {
                return AirPistolFinal;
            } else if (EventType.AirRiflePractice.Name.Contains(name)) {
                return AirRiflePractice;
            } else if (EventType.AirRifleMatch.Name.Contains(name)) {
                return AirRifleMatch;
            } else if (EventType.AirRifleFinal.Name.Contains(name)) {
                return AirRifleFinal;
            } else {
                Console.WriteLine("Unknown event: " + name);
                return null;
            }
        }

        public override bool Equals(object obj) {
            return obj is EventType type &&  Name == type.Name;
        }


        public override string ToString() {
            return this.Name;
        }

        public override int GetHashCode() {
            return base.GetHashCode();
        }
    }
}
