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
        public static EventType Rifle50MPractice { get { return new EventType("Rifle 50m Practice"); } }
        public static EventType Rifle50MMatch { get { return new EventType("Rifle 50m Match"); } }
        public static EventType Rifle50MFinal { get { return new EventType("Rifle 50m Final"); } }

    public static EventType GetEvent(string name) {
      if (EventType.AirPistolPractice.Name.Contains(name))
      {
        return AirPistolPractice;
      }
      else if (EventType.AirPistolMatch.Name.Contains(name))
      {
        return AirPistolMatch;
      }
      else if (EventType.AirPistolFinal.Name.Contains(name))
      {
        return AirPistolFinal;
      }
      else if (EventType.AirRiflePractice.Name.Contains(name))
      {
        return AirRiflePractice;
      }
      else if (EventType.AirRifleMatch.Name.Contains(name))
      {
        return AirRifleMatch;
      }
      else if (EventType.AirRifleFinal.Name.Contains(name))
      {
        return AirRifleFinal;
      }
      else if (EventType.Rifle50MPractice.Name.Contains(name))
      {
        return Rifle50MPractice;
      }
      else if (EventType.Rifle50MFinal.Name.Contains(name))
      {
        return Rifle50MFinal;
      }
      else if (EventType.Rifle50MMatch.Name.Contains(name))
      {
        return Rifle50MMatch;
      }

      else
      {
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

    }
}
