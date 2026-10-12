using System.Collections.Generic;

namespace freETarget {

    /** a setting of the target that can be changed with a command, for the terminal window */
    public class FirmwareCommand {

        public readonly string command;
        public readonly string defaultValue;

        public FirmwareCommand(string command, string defaultValue) {
            this.command = command;
            this.defaultValue = defaultValue;
        }

        public override string ToString() {
            return command;
        }

        /** the commands that take one value. the names are defined by the firmware */
        public static readonly IReadOnlyList<FirmwareCommand> All = new List<FirmwareCommand> {
            new FirmwareCommand("CALIBREx10", "45"),
            new FirmwareCommand("DIP", "0"),
            new FirmwareCommand("LED_BRIGHT", "50"),
            new FirmwareCommand("MFS", "0"),
            new FirmwareCommand("NAME_ID", "1"),
            new FirmwareCommand("PAPER_ECO", "0"),
            new FirmwareCommand("PAPER_TIME", "0"),
            new FirmwareCommand("POWER_SAVE", "30"),
            new FirmwareCommand("SEND_MISS", "0"),
            new FirmwareCommand("SENSOR", "230.00"),
            new FirmwareCommand("STEP_COUNT", "0"),
            new FirmwareCommand("STEP_TIME", "0"),
            new FirmwareCommand("TABATA_ENABLE", "0"),
            new FirmwareCommand("TABATA_CYCLES", "0"),
            new FirmwareCommand("TABATA_ON", "0"),
            new FirmwareCommand("TABATA_REST", "0"),
            new FirmwareCommand("TARGET_TYPE", "0"),
            new FirmwareCommand("RAPID_ENABLE", "0"),
            new FirmwareCommand("RAPID_CYCLES", "0"),
            new FirmwareCommand("RAPID_ON", "0"),
            new FirmwareCommand("RAPID_REST", "0"),
            new FirmwareCommand("RAPID_TYPE", "0"),
            new FirmwareCommand("TEST", "0"),
            new FirmwareCommand("TRACE", "0"),
            new FirmwareCommand("Z_OFFSET", "0"),
            new FirmwareCommand("NORTH_X", "0"),
            new FirmwareCommand("NORTH_Y", "0"),
            new FirmwareCommand("EAST_X", "0"),
            new FirmwareCommand("EAST_Y", "0"),
            new FirmwareCommand("SOUTH_X", "0"),
            new FirmwareCommand("SOUTH_Y", "0"),
            new FirmwareCommand("WEST_X", "0"),
            new FirmwareCommand("WEST_Y", "0"),
            new FirmwareCommand("ANGLE", "45")
        };
    }
}
