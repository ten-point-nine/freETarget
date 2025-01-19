# FreeTarget App - Electronic Target System

## Features
- Supports both Air Pistol and Air Rifle disciplines
- Match and Practice modes available
- Adjustable timer functionality
- Real-time shot detection and scoring
- Group analysis and session review
- Customizable zoom levels
- Hardware settings configuration

## Setup Instructions

### Connection Setup (Important: Follow these steps exactly)
1. Power on the ETS first and wait for it to initialize
2. Open the FreeTarget app
3. Turn OFF your phone's WiFi
4. Turn ON WiFi and look for network name starting with "FET-" (e.g., "FET-Target")
5. Connect to the FET- network
6. In the app, the IP (192.168.10.9) and port should already be set
7. Press the connect button in the app

Note: 
- The ETS network will always start with "FET-" 
- The app is currently not fully responsive to different screen sizes
- This specific connection sequence has been found to be most reliable for establishing connection
- Wait for each step to complete before moving to the next one

### Connection Troubleshooting
If you experience connection issues:
1. Turn off your phone's WiFi
2. Turn it back on and reconnect to the network
3. Try connecting through the app again
4. If connection drops during use, follow the same steps

## Current Limitations
- Hardware controls (LED and motors) are not yet tested/implemented
- PDF export functionality is not yet implemented
- Player information storage is not yet available
- Session review can only be saved via screenshot
- Occasional shot detection misses (1-2 shots per 50-100 shots) may occur when keepalive command is sent from Arduino
- PDF export functionality is not yet implemented
- Player information storage is not yet available
- Session review can only be saved via screenshot
- Occasional shot detection misses (1-2 shots per 50-100 shots) may occur when keepalive command is sent from Arduino

## Using the App

### Basic Controls
- Use the toggle switch to select between Air Pistol and Air Rifle
- Adjust zoom levels using the slider in the top bar
- Timer can be customized for different match requirements

### Session Review
1. Complete your shooting session
2. Review your shots and scores
3. Take screenshots to save your session results
4. Use the group analysis toggle to view shot grouping statistics

### Hardware Settings (⚠️ Under Development)
Note: The following features are present in the interface but have not been fully tested:
- LED brightness adjustment
- Paper feed settings
- Motor type selection (DC/Stepper)

Please be aware that these hardware control features are still under development and their functionality cannot be guaranteed in the current version.

## Known Issues
1. Connection may fail if app is opened before ETS is fully initialized
2. Occasional connection drops may require WiFi reconnection
3. Some shots might be missed during keepalive commands
4. Initial connection might require multiple attempts
5. App is not yet responsive to different screen sizes - may appear differently on various devices
6. UI elements might be misaligned or cut off on some screen sizes
7. Some interface elements might be difficult to access on smaller screens

## Future Implementations
- PDF export functionality
- Player information storage
- Enhanced session review system
- Improved connection stability

## Troubleshooting Decision Tree
```
Cannot Connect to ETS?
├── Is ETS powered on?
│   ├── NO → Power on ETS and wait for initialization
│   └── YES → Continue
│
├── Are both devices on same WiFi?
│   ├── NO → Connect both devices to same network
│   └── YES → Continue
│
├── Is the IP address correct (192.168.10.9)?
│   ├── NO → Enter correct IP
│   └── YES → Continue
│
├── Is the port number correct?
│   ├── NO → Enter correct port
│   └── YES → Continue
│
├── Still not connecting?
│   ├── Try these steps in order:
│   │   1. Close app completely
│   │   2. Turn off WiFi on phone
│   │   3. Wait 10 seconds
│   │   4. Turn on WiFi
│   │   5. Reconnect to network
│   │   6. Reopen app
│   │   7. Try connecting
│   │
│   └── Still not working?
│       ├── Check if ETS is responding to ping
│       └── Verify no firewall blocking

Connection Drops During Use?
├── Immediate steps:
│   1. Leave app open
│   2. Toggle WiFi off/on
│   3. Wait for reconnection
│
└── If not reconnecting:
    1. Close app
    2. Follow "Cannot Connect" steps above

Missing Shots?
├── Is it happening when status shows "Keep alive"?
│   ├── YES → Normal behavior, continue shooting
│   └── NO → Check ETS connection stability
│
└── How frequent?
    ├── 1-2 per 50 shots → Normal behavior
    └── More frequent → Check ETS settings
```

## Best Practices
1. Always power on ETS before attempting to connect
2. Ensure stable WiFi connection
3. Monitor the status messages at the bottom of the screen
4. Save screenshots of important sessions
5. Check connection status before starting a match

## Technical Information
- Target IP: 192.168.10.9
- Uses local WiFi network for communication
- Real-time shot detection and scoring
- Compatible with standard ISSF scoring rules

For technical support or to report issues, please contact the development team.

---
Note: This is a beta version of the application. Features and functionality may be updated in future releases.
