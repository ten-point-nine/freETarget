# Testing freETarget

All commands are run from the root of the repository. The only requirement is the .NET 10 SDK; the section about the downloaded packages needs nothing at all.

## Automated tests

```
dotnet test freETarget.Tests            # logic: scoring, sessions, storage, messages, controller
dotnet test freETarget.Desktop.Tests    # the Avalonia window, driven without a display
```

Both run on Windows, macOS and Linux and need no hardware. They use temporary folders, not your own data.

Run a single test class or a single test:

```
dotnet test freETarget.Tests --filter "FullyQualifiedName~StorageTests"
dotnet test freETarget.Tests --filter "DisplayName~session_survives_storing"
```

### Screenshots from the tests

Set `FET_RENDER_DIR` to an existing folder and the tests write PNG images there:

```
mkdir -p /tmp/fet
FET_RENDER_DIR=/tmp/fet dotnet test freETarget.Desktop.Tests
FET_RENDER_DIR=/tmp/fet dotnet test freETarget.Tests
```

- `freETarget.Desktop.Tests` writes pictures of the main window (empty, with shots, with one series selected, with a rifle session, in dark appearance) and of the other windows (settings tabs, calibration, target settings, resume).
- `freETarget.Tests` writes sample target images from the renderer and a sample PDF report (`Session 17.pdf`).

This is the quickest way to see what a change to the window or to the target drawing looks like.

## Trying the application without a target

The target simulator is a console program that pretends to be a WiFi target. It runs on every platform, on the same computer as the application.

### With the downloaded packages

No development tools are needed for this. Download two packages for your computer from the same place (the *Artifacts* of a run of the workflow *Build* on GitHub, or a release): the application `freETarget-<version>-<system>` and the simulator `freETarget-simulator-<version>-<system>`. `README.md` says which `<system>` fits which computer and how to open the application.

1. Unpack the simulator. It is a folder `freETarget-simulator` with one program in it.

2. Open a terminal (on Windows: *Terminal* or *Command Prompt*) in that folder and start the program:

   | System | Command |
   |---|---|
   | Windows | `TargetSimulator.Tcp.exe` |
   | Linux | `./TargetSimulator.Tcp` |
   | macOS | `xattr -d com.apple.quarantine TargetSimulator.Tcp` once, then `./TargetSimulator.Tcp` |

   - *Windows* may show "Windows protected your PC" the first time: choose *More info*, then *Run anyway*. If the firewall asks, allowing access on private networks is enough; for a test on the same computer you can also decline.
   - *macOS* blocks programs downloaded from the internet that are not signed by an Apple developer. The `xattr` command above removes that block for this one program. Without it macOS says the program cannot be opened or is damaged.
   - *Linux*: if the program is "not executable", run `chmod +x TargetSimulator.Tcp` once.

   The simulator prints that it is listening on port 1090.

3. Start the application and continue with step 3 of the next section.

### From the source code

1. Start the simulator in one terminal:

   ```
   dotnet run --project TargetSimulator.Tcp
   ```

2. Start the application in a second terminal:

   ```
   dotnet run --project freETarget.Desktop
   ```

3. In the application open **Menu > Settings**. On the *General* tab enter a shooter name; on the *Connection* tab enter:

   | Setting | Value |
   |---|---|
   | Communication protocol | `TCP` |
   | Target IP | `127.0.0.1` |
   | Target Port | `1090` |

4. Press **Connect**. The simulator prints `PC software connected` and the light in the status bar of the application turns green.

5. Fire shots with the keys of the simulator terminal:

   | Key | Action |
   |---|---|
   | Enter or `s` | one shot |
   | `b` | burst of 5 shots |
   | `m` | a miss |
   | `a` | automatic fire on / off |
   | `d` | drop the connection |
   | `q` | quit the simulator |

The simulator shows every line it sends (`->`) and everything the application sends (`<-`).

To use the simulator from another computer or a tablet in the same network, enter the IP address of the computer that runs the simulator instead of `127.0.0.1`.

### Simulator options

```
dotnet run --project TargetSimulator.Tcp -- --auto 3 --spread 5 --port 1090
./TargetSimulator.Tcp --auto 3 --spread 5 --port 1090        # the downloaded program
```

| Option | Meaning | Default |
|---|---|---|
| `--auto <seconds>` | fire a shot every so many seconds without key presses | off (the `a` key uses 3 seconds) |
| `--spread <mm>` | how far the shots scatter around the centre | 12 |
| `--port <number>` | TCP port to listen on | 1090 |

### Limits of the simulator

- It keeps the target settings in memory: `ECHO` is answered with all of them and settings sent by the application are stored, until the simulator is closed. `VERSION` is only printed.
- Offsets stored in the simulated target are reported but not applied to the shots it fires.
- It does not act on the rapid-fire commands; shots are fired by key or by `--auto` only.

## Keeping test data away from your own

```
dotnet run --project freETarget.Desktop -- --data /tmp/fet-data
```

With `--data <folder>` the application keeps its database and log in that folder instead of `Documents/freETarget`. Use it to try things without touching your real sessions and settings. PDF reports still go to the folder from the settings.

## What the application does to your computer

- It keeps its database and log in `Documents/freETarget` and creates that folder on the first start. Sessions you shoot against the simulator are saved there like real ones.
- Close the application with its window. If it is killed, the log has no shutdown marker and the next start asks whether to recover the last session. Answer **No** unless you want to test the recovery.

## Things worth checking by hand

With the simulator connected:

- Shots appear on the target, in the shot list and in the series list; total, mean radius, windage, elevation and group size update.
- Switching to another event saves the running session without asking and starts an empty one.
- **Disconnect** with shots in the session asks whether to save. *Cancel* keeps the session and the connection.
- Closing the window with shots in the session asks the same question. *Cancel* keeps the window open.
- Selecting a line in the series list shows only that series on the target; **Show all** goes back.
- Zoom with the slider or the mouse wheel over the target.
- **Resume**: shoot a few shots, switch to another event and back, choose **Menu > Resume** and pick the session: its shots are back, new shots are added to it, and it is saved as the same session, not as a second one. The button is only available while connected and before the first shot of a session.
- **Menu > Calibration**: the arrows move all shots on the target behind the window and the scores change with them; **Reset** puts them back. The values survive a restart of the application.
- **Menu > Target settings**: the window fills with the values of the simulator. Change the name or the LED brightness, press **Apply settings to target** and watch the simulator terminal receive them; open the window again and the new values are there.
- **Menu > Journal**: with a few saved sessions (shoot, switch event, switch back), the journal lists them per shooter and event. Selecting one shows its details; **Load** shows it in the main window; **Diary** edits its notes; **Print** opens its PDF; **Chart** shows its shots over time; **Export** writes a file to `Documents/freETarget/sessionExport` that **Import** reads back; the *Statistics* tab has one point per session. Opening the journal while connected asks about the running session and disconnects. A loaded session stays in the main window after the journal is closed, until you connect or choose an event.
- **Menu > Terminal** (while connected): shows what the simulator sends and, marked with `>`, what the application sends. **ECHO** makes the simulator answer with its settings; a parametrized command such as `LED_BRIGHT` 80 shows up in the simulator terminal and in the next ECHO answer. The window closes when the target is disconnected.
- **Menu > Appearance**: *Dark* and *Light* switch at once, also while connected; *Same as system* follows the operating system. The choice is kept for the next start.
- **Menu > Settings > Events**: add an event, save it, then make it active on the *Active Events* tab and move it up. After **OK** its button is in the toolbar at that position, without a restart.
- **Menu > Settings > Colors**: another target color and shot colors show on the target after **OK**.
- **Export** with shots in the session writes the report to `Documents/freETarget/pdf` (or the folder from the settings) and opens it in the PDF viewer. The session continues afterwards: further shots arrive and a match clock keeps counting down.
- Press `d` in the simulator: the status light turns red and goes back to green within a few seconds; shots fired afterwards arrive.
- Kill the application while a session has shots, start it again and answer **Yes** or **No** to the recovery question: the main window opens either way. With **Yes**, the shots are back after connecting.
- A **Final** event: the clock shows the commands of the range officer and, with voice commands switched on, they are spoken.

## Not tested yet

All development so far happened on macOS, against the simulator. Still to be confirmed by someone who has the means:

- The application on Windows and on Linux, including the PDF report (fonts) and the spoken commands.
- A real target, over WiFi and over USB. The USB connection has only been run through tests with a simulated serial module.
- A database from the original Windows application of the freETarget project: copy its `Documents\freETarget\database\Storage.db` to the same place on the new computer and check that sessions, settings and events are there.
