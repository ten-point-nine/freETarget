using System;
using System.Collections.Generic;
using System.Drawing;
using System.Globalization;
using System.Reflection;

namespace freETarget {

    /** the colors that have a name. settings and events store colors by name */
    public static class NamedColors {

        private static List<string> names;

        /** all names in alphabetical order, without the transparent one */
        public static IReadOnlyList<string> All {
            get {
                if (names == null) {
                    List<string> list = new List<string>();
                    foreach (PropertyInfo p in typeof(Color).GetProperties(BindingFlags.Public | BindingFlags.Static)) {
                        if (p.PropertyType == typeof(Color) && p.Name != "Transparent") {
                            list.Add(p.Name);
                        }
                    }
                    list.Sort(StringComparer.Ordinal);
                    names = list;
                }
                return names;
            }
        }
    }

    /**
     * an event while it is edited: the fields as the user typed them.
     * validate() says what is wrong, create() and applyTo() turn it into an event
     */
    public class EventDraft {

        public string name = "";
        public Event.EventType? type = null;
        public bool decimalScoring = false;
        public string numberOfShots = "";
        /** full class name of the target */
        public string target = null;
        public string caliber = "";
        public string minutes = "";
        public string color = null;

        //finals
        public string shotsPerSeries = "";
        public string seriesSeconds = "";
        public string shotsInSeries = "";
        public string singleShotSeconds = "";
        public string shotsInSingleSeries = "";

        public bool rapidFire = false;
        public string rfNumberOfShots = "";
        public string rfTimePerSerie = "";
        public string rfTimePerShot = "";
        public string rfPauseTime = "";
        public string rfLoadTime = "";

        private static string text(int value) {
            return value.ToString(CultureInfo.InvariantCulture);
        }

        private static string text(decimal value) {
            return value.ToString(CultureInfo.InvariantCulture);
        }

        public static EventDraft from(Event ev) {
            EventDraft d = new EventDraft();
            d.name = ev.Name;
            d.type = ev.Type;
            d.decimalScoring = ev.DecimalScoring;
            d.numberOfShots = text(ev.NumberOfShots);
            d.target = ev.Target.getName();
            d.caliber = text(ev.ProjectileCaliber);
            d.minutes = text(ev.Minutes);
            d.color = ev.TabColor.Name;
            d.shotsPerSeries = text(ev.Final_NumberOfShotPerSeries);
            d.seriesSeconds = text(ev.Final_SeriesSeconds);
            d.shotsInSeries = text(ev.Final_NumberOfShotsBeforeSingleShotSeries);
            d.singleShotSeconds = text(ev.Final_SingleShotSeconds);
            d.shotsInSingleSeries = text(ev.Final_NumberOfShotsInSingleShotSeries);
            d.rapidFire = ev.RapidFire;
            d.rfNumberOfShots = text(ev.RF_NumberOfShots);
            d.rfTimePerSerie = text(ev.RF_TimePerSerie);
            d.rfTimePerShot = text(ev.RF_TimePerShot);
            d.rfPauseTime = text(ev.RF_TimeBetweenShots);
            d.rfLoadTime = text(ev.RF_LoadTime);
            return d;
        }

        /** which of the fields an event of this type uses. the others are stored as -1 */
        public static bool usesShotsAndMinutes(Event.EventType? type) {
            return type == Event.EventType.Match || type == Event.EventType.Final;
        }

        public static bool usesFinalFields(Event.EventType? type) {
            return type == Event.EventType.Final;
        }

        //fields that do not apply to the type of the event get the value that means "not used"
        private void fillUnused() {
            if (type != null) {
                if (!usesShotsAndMinutes(type)) {
                    numberOfShots = "-1";
                    minutes = "-1";
                }
                if (!usesFinalFields(type)) {
                    shotsPerSeries = "-1";
                    seriesSeconds = "-1";
                    shotsInSeries = "-1";
                    singleShotSeconds = "-1";
                    shotsInSingleSeries = "-1";
                }
            }

            if (!rapidFire) {
                rfNumberOfShots = "-1";
                rfTimePerSerie = "-1";
                rfTimePerShot = "-1";
                rfPauseTime = "-1";
                rfLoadTime = "-1";
            }
        }

        private static bool validNumber(string s) {
            int x;
            return int.TryParse(s, NumberStyles.AllowLeadingSign, CultureInfo.InvariantCulture, out x);
        }

        //after parsing and converting to string again, the text should be identical. this refuses a decimal comma
        private static bool validDecimal(string s) {
            decimal r;
            return decimal.TryParse(s, NumberStyles.Number, CultureInfo.InvariantCulture, out r) && r.ToString(CultureInfo.InvariantCulture) == s;
        }

        private static void check(List<string> errors, string value, string label, bool isDecimal, string what) {
            if (string.IsNullOrEmpty(value)) {
                errors.Add(" * '" + label + "' is required.");
            } else if (isDecimal ? !validDecimal(value) : !validNumber(value)) {
                errors.Add(" * '" + label + "' field is not a " + what + ".");
            }
        }

        /** returns the list of problems, one per line. empty if the event can be saved */
        public string validate() {
            fillUnused();

            List<string> errors = new List<string>();
            if (string.IsNullOrWhiteSpace(name)) {
                errors.Add(" * 'Name' is required.");
            }
            if (type == null) {
                errors.Add(" * Please select an event 'Type'.");
            }
            check(errors, numberOfShots, "Number of Shots", false, "number");
            check(errors, caliber, "Caliber", true, "decimal");
            if (target == null || targets.aTarget.create(target, 4.5m) == null) {
                errors.Add(" * Please select a 'Target'.");
            }
            check(errors, minutes, "Duration", false, "number");
            if (string.IsNullOrEmpty(color)) {
                errors.Add(" * Please select a 'Color'.");
            }
            check(errors, shotsPerSeries, "Shots per Serie", false, "number");
            check(errors, seriesSeconds, "Series Duration", false, "number");
            check(errors, shotsInSeries, "Shots in All Series", false, "number");
            check(errors, singleShotSeconds, "Single Shot Duration", false, "number");
            check(errors, shotsInSingleSeries, "Shots in Single Series", false, "number");
            check(errors, rfNumberOfShots, "Rapid Fire: Nr Shots", false, "number");
            check(errors, rfTimePerSerie, "Rapid Fire: Time per serie", false, "number");
            check(errors, rfTimePerShot, "Rapid Fire: Time per shot", true, "number");
            check(errors, rfPauseTime, "Rapid Fire: Pause time", true, "number");
            check(errors, rfLoadTime, "Rapid Fire: Load time", false, "number");

            return string.Join(Environment.NewLine, errors);
        }

        private static int number(string s) {
            return int.Parse(s, NumberStyles.AllowLeadingSign, CultureInfo.InvariantCulture);
        }

        private static decimal dec(string s) {
            return decimal.Parse(s, NumberStyles.Number, CultureInfo.InvariantCulture);
        }

        /** a new event from the fields. call only after validate() returned no problems */
        public Event create() {
            Event ev = new Event(-1, "", false, Event.EventType.Practice, -1, null, -1, 0, -1, -1, -1, -1, -1, Color.Empty, false, -1, -1, -1, -1, -1);
            applyTo(ev);
            return ev;
        }

        /** writes the fields to an existing event. call only after validate() returned no problems */
        public void applyTo(Event ev) {
            ev.Name = name.Trim();
            ev.Type = type.Value;
            ev.DecimalScoring = decimalScoring;
            ev.NumberOfShots = number(numberOfShots);
            ev.ProjectileCaliber = dec(caliber);
            ev.Target = targets.aTarget.create(target, ev.ProjectileCaliber);
            ev.Minutes = number(minutes);
            ev.TabColor = Color.FromName(color);
            ev.Final_NumberOfShotPerSeries = number(shotsPerSeries);
            ev.Final_SeriesSeconds = number(seriesSeconds);
            ev.Final_NumberOfShotsBeforeSingleShotSeries = number(shotsInSeries);
            ev.Final_SingleShotSeconds = number(singleShotSeconds);
            ev.Final_NumberOfShotsInSingleShotSeries = number(shotsInSingleSeries);
            ev.RapidFire = rapidFire;
            ev.RF_NumberOfShots = number(rfNumberOfShots);
            ev.RF_TimePerSerie = number(rfTimePerSerie);
            ev.RF_TimePerShot = dec(rfTimePerShot);
            ev.RF_TimeBetweenShots = dec(rfPauseTime);
            ev.RF_LoadTime = number(rfLoadTime);
        }
    }
}
