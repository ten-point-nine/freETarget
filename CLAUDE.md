# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Scope

This repository holds the cross-platform PC software for freETarget, an open-source electronic target that locates shots acoustically, and a simulator of such a target. It started as the `Software/C#` folder of the freETarget project (https://github.com/ten-point-nine/freETarget); the hardware, the firmware and the original Windows application live there, not here.

The application has to stay compatible with that project in three places: the commands and messages of the target firmware, the database file (users bring their `Storage.db` from the original Windows application), and the session export files.

Projects:

- `freETarget.Core/` — class library with everything that is not UI: shots, sessions, events, targets, storage, comm modules, logging, the main controller, target drawing, PDF report. Plain `net10.0`. Namespaces are `freETarget.*`.
- `freETarget.Tests/` — xUnit tests for the core library.
- `freETarget.Desktop/` — the application (Avalonia 12), for Windows, Linux and macOS.
- `freETarget.Desktop.Tests/` — headless UI tests that drive the real Avalonia windows against a simulated target (xUnit v3, required by `Avalonia.Headless.XUnit`).
- `TargetSimulator.Tcp/` — console tool that impersonates a WiFi target over TCP. The way to try the application without hardware.
- `packaging/` — the script and files that build the downloadable packages.

## Build and run

The only requirement is the .NET 10 SDK.

```
dotnet build freETarget.sln                            # everything
dotnet run --project freETarget.Desktop                # the application
dotnet run --project freETarget.Desktop -- --data /tmp/fet-data    # with its own database and log
dotnet run --project TargetSimulator.Tcp               # the simulator
dotnet test freETarget.Tests                           # core tests
dotnet test freETarget.Desktop.Tests                   # UI tests, no display needed
dotnet test freETarget.Tests --filter "FullyQualifiedName~StorageTests"
dotnet test freETarget.Tests --filter "DisplayName~session_survives_storing"
```

- A clean build has no warnings.
- Tests share process-wide state (`Settings.Default`, the current culture), so parallel execution is switched off in `TestSupport.cs`. Tests that touch numbers or the database also run under `de-DE`, because many users have a decimal comma.
- To see what the windows look like without starting the application, set `FET_RENDER_DIR` to a directory when running the tests: `freETarget.Desktop.Tests` writes PNG screenshots of the windows there, `freETarget.Tests` sample target images and a sample PDF report.
- Running the application for real uses the user's data in `Documents/freETarget` (database and log) unless `--data <folder>` is given. On macOS that location does not follow `HOME`. Killing the application leaves the log without its shutdown marker, so the next start offers a crash recovery.
- There is no linter configuration. CI is the GitHub workflow `.github/workflows/build.yml`: tests on Windows, Linux and macOS, then the packages. It has not run yet at the time of writing, so its first run may show problems on Windows and Linux that never showed on macOS, where all development happened so far.

`TEST.md` describes manual testing with the simulator and lists what to check by hand.

### Packaging

```
packaging/package.sh osx-arm64 [app|simulator]     # result in dist/, ignored by git
```

- Targets: `win-x64`, `win-arm64`, `linux-x64`, `linux-arm64`, `osx-x64`, `osx-arm64`. Each gets two self-contained packages: the application and the simulator (one trimmed single-file program).
- Every target can be cross-built from any system. The macOS packages are only complete when built on macOS: the icon (`sips`, `iconutil`) and the signature (`codesign`) need tools that exist only there.
- macOS packages are signed ad hoc only, by decision: no Apple developer certificate, no notarization. The ad-hoc signature is not optional, macOS refuses to start unsigned Apple Silicon programs at all. Users have to allow the application once; `README.md` explains how. Do not add certificate signing without being asked.
- The package version is the `<Version>` of `freETarget.Desktop/freETarget.Desktop.csproj`, unless the environment variable `FET_VERSION` gives another one. Its major number is the database version the application demands (see Storage), so `package.sh` refuses a `FET_VERSION` whose major number differs from the project file: a new database version is only made by editing the project file.
- A release is made by pushing a tag `pc-v<version>` (three numbers, for example `pc-v4.13.0`); the workflow sets `FET_VERSION` from the tag and attaches the twelve packages to the release. The `<Version>` of the project file is what local builds and builds without a tag show, it does not have to be raised for a release with the same major number.

### Not there yet

Compared with the original Windows application these are missing: firmware upload (it shelled out to `avrdude` for Arduino boards and `esptool` for ESP32 boards), the score breakdown chart in the main window, the Wi-Fi name check when connecting, keeping the computer awake while connected. The packages do not contain the USB drivers for the boards. `README.md` points to the upstream repository for both firmware upload and drivers.

## Architecture

### Core and UI

`freETarget.Core` must not reference Avalonia, `System.Drawing.Common` or anything that exists on one operating system only (`System.Drawing.Color` and `PointF` are fine, they are part of the base library).

`MainController` holds what the main window does apart from drawing: startup, connection, the running session, incoming messages, rapid fire, crash recovery, calibration, events, journal, terminal text. The windows call its methods and listen to its events; everything arriving from comm threads is moved to the UI thread before state changes, so the events are raised there. New logic goes into the core with a test, not into a window.

Questions to the user go through `IUserDialogs`, which blocks until answered (`Dialogs` in the desktop project uses a nested dispatcher frame for that). Such a question can come up during startup, before the main window exists; `App` therefore starts with `ShutdownMode.OnExplicitShutdown` and switches to `OnMainWindowClose` once the main window is set. Without that, closing the message box counts as closing the last window and the application dies with "Dispatcher shut down".

Two small seams connect older core code to the application:

- `IAppHost` — log, status messages, error/warning/confirm dialogs, event lookup. `MainController` implements it and hands itself to `StorageController` and the comm modules.
- `Speech.Current` (`ISpeech`) — text to speech for `VirtualRO`. Silent by default; `PlatformSpeech` installs the engine of the platform at startup (Windows speech, `say` on macOS, `spd-say` / `espeak-ng` on Linux).

Settings are the plain class `Properties.Settings` in the core, reached everywhere as `Settings.Default`.

### Windows of the application

The main window is XAML (`MainWindow.axaml`); the other windows are built in code. Most are reached through the **Menu** drop-down of the toolbar.

Appearance: the `theme` setting is `System` (default, follows the operating system), `Light` or `Dark`; `ThemeSetting.apply` maps it to Avalonia's theme variant. Colors that are not part of the Fluent theme are defined once per appearance in `App.axaml` and used with `{DynamicResource ...}` in XAML or `withBrush(...)` in code-built windows. Do not hard-code colors in windows, they would be wrong in one of the two appearances. The target itself is not themed: it is a picture of the paper target in the colors of the settings.

Charts are drawn by the small `SimpleChart` control; there is no chart library.

### Data flow

1. A `comms.aCommModule` implementation (`USB` for serial, `TCP` for WiFi targets) raises `CommDataReceivedEvent` with raw text, from a worker thread.
2. `MainController` brings it to the UI thread and feeds it to a `MessageReader`, which cuts it into `{...}` messages. The firmware's output is JSON-like but is parsed by hand (split on `,` and `:`), not with a JSON library.
3. `TargetMessage.classify` decides what a message is: one whose first key contains `shot` becomes a `Shot` (`TargetMessage.parseShot`); anything else is tried as an `Echo`, the target's settings/status dump.
4. The shot is scored and added to the `Session`; the controller raises `ShotAdded` and the window draws it.

Commands to the target are sent as `{"KEY":value}` strings via `MainController.send` (for example `{"VERSION":7}` right after connecting, `{"ECHO":0}` to request the settings dump). The command names are defined by the firmware. The settings stored in the target are modelled by `TargetSettings` (built from an `Echo`, validated, turned into the command that writes them back). `RapidFireSequencer` produces the command sequence of a rapid-fire series; the controller owns the timer.

### Scoring and coordinates

- Shot coordinates arrive in millimetres from the target centre. On receipt they are scaled by `100 / Settings.targetDistance`, which lets a reduced-distance range be scored as if at full distance.
- `Shot.computeScore` ignores the radius and angle sent by the target and recomputes them from x/y plus the calibration offsets, then asks the target for the score. Local calibration is reset to zero when the target's `Echo` reports its own non-zero offsets.
- The default score is `11 - radius / get10Radius()`, truncated to one decimal; target classes override `getScore` when their rings are not evenly spaced.
- Shots are drawn at their true size relative to the rings, which is also what the scoring uses (a shot scores the higher ring when its edge touches the line).

### Targets

Each target face is a subclass of `targets.aTarget`, which defines ring geometry, zoom limits and label placement through abstract getters. `TargetRenderer` (SkiaSharp) draws any of them from those getters; `TargetView` in the desktop project puts the result on the screen.

- Target classes are discovered by reflection (`aTarget.getTargetNames`), so adding a target only needs a new class with a `(decimal caliber)` constructor in `freETarget.Core/targets/`. `TargetTests` asserts the number of targets.
- Events store the target as its full type name in the database (`freETarget.targets.AirPistol`) and instantiate it with `aTarget.create`. Renaming or moving a target class, or changing the namespace, breaks every stored event that uses it.

### Events and sessions

An `Event` is a user-configurable course of fire (practice / match / final, shot count, time limit, target, caliber, final-series and rapid-fire timings). `EventManager` seeds six default air pistol/rifle events when the table is empty. Events are edited through `EventDraft` (the fields as typed, validation, conversion) and `MainController.saveEvent` / `setActiveEvents`; the order of the active events is the order of their buttons in the main window.

`Session` holds the shots of one run of an event, splits them into series, computes the statistics (mean point of impact, mean radius, group size) and drives the session clock. `VirtualRO` is the spoken range-officer state machine used for finals and rapid-fire timing.

### Storage

`StorageController` uses hand-written SQL through `Microsoft.Data.Sqlite`.

- Parameters are bound through `StorageController.addParam`, not `Parameters.AddWithValue` directly: it maps `null` to `DBNull` and writes `Type` and `Color` values as their `ToString()` text, which is the format the `Settings` table has always used (`Color [Moccasin]`, booleans as `1`/`0`).
- Connection strings set `Pooling=False` because the startup check may overwrite the database file right after closing a connection to it.
- `freETarget.Core/Storage.db` is a checked-in **template** database that is copied to the build output of every application referencing the core. On first run it is copied to `Documents/freETarget/database/Storage.db`, which is the database actually used.
- On startup the `Version` table of the user database is compared with the template's, and the template version must equal the major version of the application. On a mismatch the user is asked to overwrite their database with the empty template, losing all saved sessions. Any schema change therefore means editing the binary template, and bumping its version is destructive for existing users.
- Rows are read by column ordinal (`rdr.GetInt32(5)` and so on), so column order in the template matters.
- Shots are stored as text inside the session row, using `Shot.ToString()` / `Shot.parse` (comma-separated, invariant culture, with optional trailing calibration fields). Changing that format breaks loading of existing sessions.
- Each session row carries a hash of its content (`getSessionHash` / `verifySessionHash`). New hashes are independent of the language settings; the check also accepts the variants that older versions wrote.
- Application settings are persisted in the database `Settings` table by `SettingsStore`: on startup every writable property of `Properties.Settings` is mirrored into the table by reflection and loaded back. A new property is picked up automatically; renaming one orphans the stored value (`StorageTests` pins the names).

### Journal

`JournalWindow` shows saved sessions. While it is open the main window has no target connection (`MainController.prepareForJournal` closes the session and disconnects, the connect button is disabled); a session picked there is shown in the main window with status `LOADED`, which is view-only: its shots are in `LoadedShots`, it is never saved or asked about. It stays on the screen after the journal is closed; connecting to a target or choosing an event puts it away and starts an empty session. Session export/import files are handled by `SessionFile`, the statistics by `StatisticsSeries`.

### PDF report

`PDFGenerator` writes the session report with the platform independent `PDFsharp` package. Things that are easy to get wrong:

- That build of PDFsharp finds no fonts by itself. `PdfFonts` supplies them: it locates Arial, or the nearest installed sans-serif font, through SkiaSharp and hands the file to PDFsharp. Font collections (`.ttc`) are skipped because they cannot be embedded.
- The generator rebuilds the series of the session it is given. Pass a copy (`Session.copy()`), never the session on the screen; `MainController.exportPdf` does this and also computes the statistics on the copy, so the running session and its clock are untouched.
- The report draws its own simplified target, separate from `TargetRenderer`.
- The diary is stored as RTF, because the original Windows application can read nothing else. This application edits it as plain text: `RtfText.toPlainText` reads it, `RtfText.fromPlainText` writes it. Formatting (bold, sizes) made in the Windows application is lost when the diary is saved here.
- "Print" and "Export" mean: the PDF is opened in the viewer of the system and printed from there.

### Logs and crash recovery

Logs go to `Documents/freETarget/log/` (a daily `freETarget.<date>.log` plus `Shot.log`). `Logger` writes them. The log is also a data source: after an unclean shutdown `SessionRecovery` reads it backwards (`ReverseTextReader`) to find the last `### New Session` marker and the controller replays the lines logged after it once a target is connected. Changing the text of the session-start line, the startup separator line or the shot lines breaks recovery.

## Conventions

- Number parsing and formatting for anything sent to the target, logged or stored uses `CultureInfo.InvariantCulture`; the app is used in locales with a decimal comma.
- String searches use the `char` overloads or `StringComparison.Ordinal`. Culture-sensitive `IndexOf("...")` behaves differently between operating systems.
- Existing code uses camelCase method names, K&R braces and `a`-prefixed abstract base classes (`aTarget`, `aCommModule`).
