using System.Drawing;

namespace freETarget.Properties {

    /**
     * Application settings. The values are persisted in the Settings table of the database:
     * every public writable property is mirrored there by name (see SettingsStore).
     * Property names and types must not change, existing databases refer to them.
     */
    public sealed class Settings {

        public static Settings Default { get; } = new Settings();

        public string name { get; set; } = "";
        public string portName { get; set; } = "";
        public int baudRate { get; set; } = 115200;
        public bool displayDebugConsole { get; set; } = false;
        public string defaultTarget { get; set; } = "Pistol Practice";
        public Color targetColor { get; set; } = Color.Moccasin;
        public bool drawMeanGroup { get; set; } = true;
        public decimal calibrationX { get; set; } = 0;
        public decimal calibrationY { get; set; } = 0;
        public bool OnlySeries { get; set; } = false;
        public bool voiceCommands { get; set; } = true;
        public string pdfPath { get; set; } = "";
        public decimal targetDistance { get; set; } = 100;
        public bool scoreVoice { get; set; } = false;
        public Color score10BackgroundColor { get; set; } = Color.Aqua;
        public Color score9BackgroundColor { get; set; } = Color.Aqua;
        public Color scoreDefaultBackgroundColor { get; set; } = Color.Aqua;
        public Color scoreOldBackgroundColor { get; set; } = Color.Blue;
        public Color score10PenColor { get; set; } = Color.Blue;
        public Color score9PenColor { get; set; } = Color.Blue;
        public Color scoreDefaultPenColor { get; set; } = Color.Blue;
        public Color scoreOldPenColor { get; set; } = Color.LightSkyBlue;
        public decimal calibrationAngle { get; set; } = 0;
        public bool fileLogging { get; set; } = true;
        public bool ignoreMiss { get; set; } = false;
        public string CommProtocol { get; set; } = "USB";
        public string TcpIP { get; set; } = "192.168.10.9";
        public int TcpPort { get; set; } = 1090;
        public bool showScoring { get; set; } = true;
        public string Board { get; set; } = "Arduino";
        public string uploaderLoc { get; set; } = "";
        /** appearance of the desktop application: System (follow the operating system), Light or Dark */
        public string theme { get; set; } = "System";

        /**
         * kept for the existing call sites. the settings live in the database only, see SettingsStore.save
         */
        public void Save() {
        }
    }
}
