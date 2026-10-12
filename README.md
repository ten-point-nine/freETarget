# freETarget PC software

**What is this?**

freETarget is an open-source electronic target for air rifles and air pistols: microphones around the target card hear the pellet go through the paper, and the position of the shot is calculated from the differences in time. The hardware and its firmware are developed in the [freETarget project](https://github.com/ten-point-nine/freETarget); see also https://free-e-target.com/.

This repository contains the PC software for such a target, for Windows, Linux and macOS:

* **freETarget** shows the target with every shot as it is fired, keeps the score, speaks the commands of a final, saves the sessions and prints reports.
* **The target simulator** pretends to be a target, so that the software can be tried and tested without any hardware.

It is a new edition of the Windows application of the freETarget project. It reads the same database, so sessions and settings can be taken along.

**What it can do**

- Practice, match and final events for air pistol and air rifle, and a number of other targets; own events can be added
- Integer and decimal scoring, series, inner tens
- Mean point of impact, mean radius and group size, drawn on the target
- A journal of all saved sessions per shooter, with a diary page for each, statistics over time, and export and import of sessions
- A PDF report of every session
- A virtual range officer with spoken commands for finals and rapid fire
- Calibration of the shot positions, settings of the target, and a terminal to talk to the target directly
- Targets connected by USB or over WiFi
- Light and dark appearance

Not included:

- *Uploading firmware to the target.* Use the Windows application of the [freETarget project](https://github.com/ten-point-nine/freETarget) for that (folder [`Software/C#`](https://github.com/ten-point-nine/freETarget/tree/master/Software/C%23) there). The firmware itself is in its folder [`Software`](https://github.com/ten-point-nine/freETarget/tree/master/Software) and on https://www.free-e-target.com/downloads.
- *USB drivers for the boards.* They are in the freETarget project as well: [`Software/C#/freETarget Setup/drivers`](https://github.com/ten-point-nine/freETarget/tree/master/Software/C%23/freETarget%20Setup/drivers).

**How do I get it?**

Packages are built automatically: see the *Actions* page of the repository on GitHub, workflow *Build*, section *Artifacts* of a run, or the *Releases* page. They contain everything needed, nothing has to be installed first. Pick the one for your computer:

| Computer | Package |
|---|---|
| Windows, Intel or AMD processor | `freETarget-<version>-win-x64.zip` |
| Windows on ARM | `freETarget-<version>-win-arm64.zip` |
| Linux, Intel or AMD processor | `freETarget-<version>-linux-x64.tar.gz` |
| Linux on ARM, for example a Raspberry Pi 4 or 5 with a 64 bit system | `freETarget-<version>-linux-arm64.tar.gz` |
| Mac with Apple Silicon (M1 and later) | `freETarget-<version>-osx-arm64.zip` |
| Mac with Intel processor | `freETarget-<version>-osx-x64.zip` |

*Windows:* unpack the zip file and start `freETarget.Desktop.exe` in the folder `freETarget`. Windows may warn about an unknown publisher the first time; choose *More info* and *Run anyway*. The USB drivers for the boards are not part of the package; if Windows does not recognize the target, install the driver of its USB chip from the [drivers folder of the freETarget project](https://github.com/ten-point-nine/freETarget/tree/master/Software/C%23/freETarget%20Setup/drivers).

*Linux:* unpack with `tar -xzf freETarget-<version>-linux-x64.tar.gz` and start `freETarget/freETarget.Desktop`. To use a target over USB your user has to be allowed to use serial ports; on most distributions that means being a member of the group `dialout` (`sudo usermod -a -G dialout $USER`, then log out and in again). For spoken commands install `speech-dispatcher` or `espeak-ng`.

*macOS:* the application is not signed with an Apple developer certificate, so macOS does not open it right away. Do this once:

1. Unpack the zip file and move `freETarget` to the *Applications* folder.
2. Double-click it. macOS says that it cannot be opened because the developer cannot be verified (or that Apple could not check it for malicious software). Close that message with *Done* (or *OK*), do not move the application to the bin.
3. Open *System Settings*, go to *Privacy & Security* and scroll down to *Security*. There is a line saying that "freETarget" was blocked. Click *Open Anyway* and confirm with your password or Touch ID.
4. From now on it opens like any other application.

On macOS 14 and older you can also right-click the application, choose *Open* and confirm with *Open*.

If you are comfortable with the Terminal, this does the same in one step:

```
xattr -dr com.apple.quarantine /Applications/freETarget.app
```

If macOS says the application "is damaged and can't be opened", it is the same block under another name: use the Terminal command above.

On all three systems the application keeps its data (sessions, settings and log) in the folder `freETarget` inside your documents folder.

The application has so far been tried on macOS only. If something does not work on Windows or Linux, please report it.

**Can I try it without a target?**

Yes. Next to each package of the application there is a small package `freETarget-simulator-<version>-<system>`. It contains one program that pretends to be a target with a network connection, on the same computer. [TEST.md](TEST.md) explains how to start it and what to try.

**I want to work on the source code**

Great! The software is written in C#. All you need is the .NET 10 SDK, which is free and available for Windows, Linux and macOS; any editor works.

```
dotnet run --project freETarget.Desktop     # the application
dotnet run --project TargetSimulator.Tcp    # the simulator
dotnet test                                 # the tests
packaging/package.sh osx-arm64              # packages like the downloadable ones, in dist/
```

| Folder | Content |
|---|---|
| `freETarget.Core` | everything that is not user interface: scoring, sessions, storage, connection to the target, reports |
| `freETarget.Desktop` | the application |
| `TargetSimulator.Tcp` | the simulator |
| `freETarget.Tests`, `freETarget.Desktop.Tests` | the tests |
| `packaging` | how the packages are built |

[TEST.md](TEST.md) describes how to test, [CLAUDE.md](CLAUDE.md) how the code is organized.

**License**

MIT, see [LICENSE.md](LICENSE.md). This software is derived from the PC software of the [freETarget project](https://github.com/ten-point-nine/freETarget) (ten-point-nine), which is published under the same license; its copyright notice is kept in the license file.
