using freETarget.Properties;
using System.Collections.Generic;
using System.Reflection;

namespace freETarget {

    /**
     * mirrors the Settings object into the Settings table of the database, one row per property
     */
    public static class SettingsStore {

        private static IEnumerable<PropertyInfo> properties() {
            foreach (PropertyInfo m in typeof(Settings).GetProperties(BindingFlags.Public | BindingFlags.Instance)) {
                if (m.CanWrite && m.CanRead) {
                    yield return m;
                }
            }
        }

        /** creates the rows of settings that are not in the database yet, using their default values */
        public static void init(StorageController storage, Settings settings) {
            foreach (PropertyInfo m in properties()) {
                if (storage.getSetting(m.Name) == null) {
                    storage.storeSetting(m.Name, m.PropertyType, m.GetValue(settings));
                }
            }
        }

        /** reads all settings from the database. a setting whose row cannot be read keeps its current value */
        public static void load(StorageController storage, Settings settings) {
            foreach (PropertyInfo m in properties()) {
                object obj = storage.getSetting(m.Name);
                if (obj != null && m.PropertyType.IsInstanceOfType(obj)) {
                    m.SetValue(settings, obj);
                }
            }
        }

        public static void save(StorageController storage, Settings settings) {
            foreach (PropertyInfo m in properties()) {
                storage.updateSetting(m.Name, m.GetValue(settings));
            }
        }
    }
}
