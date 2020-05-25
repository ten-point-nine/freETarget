using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace freETarget {
    class SessionType {

        private SessionType(string name) { Name = name; }
        public string Name { get; set; }

        public static SessionType AirPistolPractice { get { return new SessionType("Air Pistol Practice"); } }
        public static SessionType AirPistolMatch { get { return new SessionType("Air Pistol Match"); } }
        public static SessionType AirPistolFinal { get { return new SessionType("Air Pistol Final"); } }
        public static SessionType AirRiflePractice { get { return new SessionType("Air Rifle Practice"); } }
        public static SessionType AirRifleMatch { get { return new SessionType("Air Rifle Match"); } }
        public static SessionType AirRifleFinal { get { return new SessionType("Air Rifle Final"); } }

 
        public static SessionType GetSessionType(string name) {
            if (SessionType.AirPistolPractice.Name.Contains(name)) {
                return AirPistolPractice;
            } else if (SessionType.AirPistolMatch.Name.Contains(name)) {
                return AirPistolMatch;
            } else if (SessionType.AirPistolFinal.Name.Contains(name)) {
                return AirPistolFinal;
            } else if (SessionType.AirRiflePractice.Name.Contains(name)) {
                return AirRiflePractice;
            } else if (SessionType.AirRifleMatch.Name.Contains(name)) {
                return AirRifleMatch;
            } else if (SessionType.AirRifleFinal.Name.Contains(name)) {
                return AirRifleFinal;
            } else {
                return null;
            }
        }

        public override bool Equals(object obj) {
            return obj is SessionType type &&  Name == type.Name;
        }


        public override string ToString() {
            return this.Name;
        }

        public override int GetHashCode() {
            return base.GetHashCode();
        }
    }
}
