using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace freETarget {
    class CourseOfFire {

        private CourseOfFire(string name) { Name = name; }
        public string Name { get; set; }

        public static CourseOfFire AirPistolPractice { get { return new CourseOfFire("Air Pistol Practice"); } }
        public static CourseOfFire AirPistolMatch { get { return new CourseOfFire("Air Pistol Match"); } }
        public static CourseOfFire AirPistolFinal { get { return new CourseOfFire("Air Pistol Final"); } }
        public static CourseOfFire AirRiflePractice { get { return new CourseOfFire("Air Rifle Practice"); } }
        public static CourseOfFire AirRifleMatch { get { return new CourseOfFire("Air Rifle Match"); } }
        public static CourseOfFire AirRifleFinal { get { return new CourseOfFire("Air Rifle Final"); } }

 
        public static CourseOfFire GetCourseOfFire(string name) {
            if (CourseOfFire.AirPistolPractice.Name.Contains(name)) {
                return AirPistolPractice;
            } else if (CourseOfFire.AirPistolMatch.Name.Contains(name)) {
                return AirPistolMatch;
            } else if (CourseOfFire.AirPistolFinal.Name.Contains(name)) {
                return AirPistolFinal;
            } else if (CourseOfFire.AirRiflePractice.Name.Contains(name)) {
                return AirRiflePractice;
            } else if (CourseOfFire.AirRifleMatch.Name.Contains(name)) {
                return AirRifleMatch;
            } else if (CourseOfFire.AirRifleFinal.Name.Contains(name)) {
                return AirRifleFinal;
            } else {
                return null;
            }
        }

        public override bool Equals(object obj) {
            return obj is CourseOfFire type &&  Name == type.Name;
        }


        public override string ToString() {
            return this.Name;
        }

        public override int GetHashCode() {
            return base.GetHashCode();
        }
    }
}
