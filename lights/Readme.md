# BBC micro:bit V2 Traffic Lights - CODAL Project

This project uses the official CODAL (Component Oriented Device Abstraction Layer) build system for BBC micro:bit V2 development.

It was adapted from an earlier [Keyes MiniCar robot project](https://github.com/lorempipsum/digital-democracy/tree/main/robot) and keeps that project's build system and Bluetooth command structure.

## Features

This micro:bit traffic light firmware provides:

- **UK Traffic Light Cycle** - Drives a 3-lamp (red, amber, green) traffic light through the standard UK sequence
- **Party Mode** - Random lamp combinations at random speeds, toggled by a push button or Bluetooth
- **Bluetooth Low Energy (BLE) UART Service** - Uses Nordic UART Service (NUS) for wireless communication
- **Lamp Brightness and Fades** - Set each lamp's brightness (PWM) with hex codes, instantly or with a timed fade
- **Visual Feedback** - Custom LED display patterns for startup, connection status and party mode
- **Audio Feedback** - Sound expressions on mode changes, errors, and on command
- **Command Processing** - Receives structured text commands via BLE with automatic command routing and audio/visual feedback

### Current Functionality

- **Startup**:
  - Plays the "hello" sound and shows the heart pattern
  - Lamp test: all lamps fade up and down 3 times (about 2.4 seconds)
  - Then starts the traffic cycle

- **Operating Modes** (only one runs at a time):
  - **Traffic** (default) - Runs the UK cycle, always starting from red:

    | Step | Lamps | Time |
    |------|-------|------|
    | 1 | Red | 20 s |
    | 2 | Red + Amber | 2 s |
    | 3 | Green | 15 s |
    | 4 | Amber | 3 s |

    The cycle is 40 s in total. The 2 s red+amber and 3 s amber times are fixed by UK regulations (TSRGD 2016). Red lasts 20 s to cover the other road's red+amber, green and amber.
  - **Party** - Shows a random lamp combination for 60-600 ms, never the same combination twice running. Starts with the "giggle" sound and the sparkle pattern; leaving party mode shows the heart again
  - **Bluetooth** - Holds whatever lamps the last `LED_` command set. Disconnecting Bluetooth hands control back to the traffic cycle

- **Party Button** (button A, or an external button on P5):
  - Each press toggles between party and traffic mode
  - From Bluetooth control, a press takes back local control and starts party mode
  - Acts on press, not click, so a press still works if the button is held for more than 1 second

- **Responsiveness**:
  - Mode changes take effect within about 20 ms, even partway through a 20 s red or a fade
  - `MOD_TRAFFIC#` while already in traffic mode restarts the cycle from red

- **Display Patterns**:
  - ❤️ Heart pattern on startup, and when leaving party mode
  - ✓ Tick pattern when BLE connects
  - ✗ Cross pattern when BLE disconnects
  - ✨ Sparkle pattern in party mode

- **LED Control Commands** (format: `LED_<RRAAGG>#` or `LED_<RRAAGG>_<TT>#`):
  - `RR`, `AA`, `GG` are the red, amber and green brightness in hex, `00` (off) to `FF` (full)
  - Optional `_TT` fades to the new levels over `TT` tenths of a second (`00`-`99`, so up to 9.9 s); without it the change is instant
  - Switches to Bluetooth mode and holds the lamps until the next command, button press or disconnect
  - `LED_FF0000#` - Red only
  - `LED_00FF00#` - Amber only
  - `LED_0000FF#` - Green only
  - `LED_FFFF00#` - Red + amber
  - `LED_FFFFFF#` - All lamps full
  - `LED_FF8000#` - Red full, amber about half
  - `LED_404040#` - All lamps dim
  - `LED_000000#` - All lamps off
  - `LED_00FF00_20#` - Fade to amber only over 2 seconds
  - `LED_000000_50#` - Fade all lamps off over 5 seconds
  - `LED_?#` - Query the lamps, e.g. reply `OK LED_FF8000`. During a fade this is the level being faded to

- **Mode Commands** (format: `MOD_<MODE>#`):
  - `MOD_TRAFFIC#` - Switch to traffic mode (restarts the cycle from red)
  - `MOD_PARTY#` - Switch to party mode
  - `MOD_?#` - Query the mode: reply `OK MOD_TRAFFIC`, `OK MOD_PARTY`, or `OK MOD_BLE` while holding lamps from an `LED_` command

- **Audio Commands** (format: `SPK_<SOUND>#`):
  - `SPK_GIGGLE#` - Play "giggle" sound expression
  - `SPK_HAPPY#` - Play "happy" sound expression
  - `SPK_HELLO#` - Play "hello" sound expression
  - `SPK_MYSTERIOUS#` - Play "mysterious" sound expression
  - `SPK_SAD#` - Play "sad" sound expression
  - `SPK_SLIDE#` - Play "slide" sound expression
  - `SPK_SOARING#` - Play "soaring" sound expression
  - `SPK_SPRING#` - Play "spring" sound expression
  - `SPK_TWINKLE#` - Play "twinkle" sound expression
  - `SPK_YAWN#` - Play "yawn" sound expression

- **Command Processing**:
  - Commands use structured format with 3-character type identifier and underscore separator (`LED_`, `MOD_`, `SPK_`)
  - All commands must end with `#` delimiter
  - Every command gets exactly one reply line (ending `\r\n`), and every message fits in one 20-byte Bluetooth packet:
    - `OK` on success, or `OK <answer>` for a query (`OK LED_FF8000`, `OK MOD_PARTY`)
    - `ERR <reason>` on failure: `ERR bad LED_`, `ERR bad MOD_`, `ERR bad SPK_` or `ERR unknown cmd`
  - Commands are handled one at a time in the order received, so a client can match replies to its commands in order
  - Invalid `LED_` or `MOD_` commands, and unknown command types, also play the "sad" sound
  - Invalid `SPK_` commands play no sound, since "sad" is itself a valid sound command

- **Status Messages** (`STA_`, sent without being asked):
  - `STA_MOD_<MODE>` (`TRAFFIC`, `PARTY` or `BLE`) and `STA_LED_<RRAAGG>`, using the same form as the query answers
  - Both are sent 1 s after a client connects
  - `STA_MOD_` is sent whenever the mode changes, by command, party button or disconnect
  - `STA_LED_` is sent whenever new lamp levels are set: each traffic cycle step, and each `LED_` command (once, with the levels being faded to). Not for each party mode flash (too frequent)
  - They can arrive before a command's reply, so read `STA_` lines separately from `OK`/`ERR`

- **Hardware Integration**:
  - Lamps driven from micro:bit pins through a FET LED driver board, with 1 kHz PWM for flicker-free dimming
  - Squared brightness curve, so dim levels and fades look even. Only level `00` is off: `01`-`08` all give the minimum PWM duty (1/1023)
  - Audio on the onboard speaker only (P0 audio output disabled)
  - All lamps off at startup until the lamp test runs

- **BLE Services**:
  - Nordic UART Service (NUS) for bidirectional text communication
  - Standard micro:bit button service

## Hardware

### Pin Connections

| Function | Pin | Notes |
|----------|-----|-------|
| Red lamp | P2 | HIGH = on, PWM for brightness |
| Amber lamp | P8 | HIGH = on, PWM for brightness |
| Green lamp | P9 | HIGH = on, PWM for brightness |
| Party button | P5 | Same pin as button A |

P3, P4, P7 and P10 are avoided because they drive the 5x5 LED display, so the display stays free for status patterns. P5, P8 and P9 are on the small edge connector strips, so an edge connector breakout board is needed to reach them.

### Wiring

- **Lamps**: Wire P2, P8 and P9 (and GND) to the red, amber and green inputs of the FET LED driver board
  - The driver board's optoisolators have their own input resistors, so no resistors are needed on the micro:bit side
- **External button** (optional): Wire a push button from P5 to GND, in parallel with button A
  - No external pull-up resistor is needed; the micro:bit board already has one on button A
- **Sound**: Uses the onboard speaker, so nothing needs to be connected to P0

## Testing with Web Bluetooth

### Web Device CLI Tool

You can test the BLE UART functionality using the **Web Device CLI** tool, which provides a web-based interface for communicating with Nordic UART Service (NUS) enabled devices:

**Live Web Interface:**: https://optimiseddesign.github.io/web-device-cli (fork with swapped TX/RX UUIDs)

**GitHub Repository**: https://github.com/optimiseddesign/web-device-cli (fork with swapped TX/RX UUIDs)

### Testing Steps

1. **Build and flash** your micro:bit with this firmware
2. **Watch the startup** - you should hear "hello", see the heart (❤️) pattern, and see all three lamps fade up and down 3 times before the traffic cycle starts
3. **Press button A** (or the external button) - party mode should start with a giggle and the sparkle pattern; press again to return to traffic mode
4. **Open the Web Device CLI** in a Web Bluetooth enabled browser (Chrome 56+, Edge 79+, etc.)
5. **Click "Connect"** and select your micro:bit from the device list
6. **Wait for connection** - you should see the tick (✓) pattern on your micro:bit, and the current state in the web interface, e.g. `STA_MOD_TRAFFIC` and `STA_LED_FF0000`. A new `STA_LED_` line follows each traffic cycle step
7. **Send commands** through the web interface:
   - **LED Commands**: `LED_FF0000#` (red), `LED_00FF00#` (amber), `LED_0000FF#` (green), `LED_404040#` (all dim), `LED_000000#` (off)
   - **Fade Commands**: `LED_FFFFFF_20#` (fade all on over 2 s), `LED_000000_50#` (fade all off over 5 s)
   - **Mode Commands**: `MOD_TRAFFIC#`, `MOD_PARTY#`
   - **Query Commands**: `LED_?#`, `MOD_?#`
   - **Audio Commands**: `SPK_HAPPY#`, `SPK_GIGGLE#`, etc.
   - Watch for lamp changes, mode changes and audio feedback on the micro:bit
   - See one `OK` or `ERR` reply per command in the web interface, and `STA_` lines when the button or traffic cycle changes things
8. **Disconnect** - you should see the cross (✗) pattern, and if Bluetooth was holding the lamps, the traffic cycle restarts from red

### Example Commands

```
LED_FF0000#     # Red only (switches to Bluetooth mode)
LED_FF8000#     # Red full, amber about half
LED_00FF00_20#  # Fade to amber only over 2 seconds
LED_404040#     # All lamps dim
LED_000000#     # All lamps off
MOD_PARTY#      # Start party mode
MOD_TRAFFIC#    # Back to the UK traffic cycle, from red
LED_?#          # Query the lamps, e.g. OK LED_FF0000
MOD_?#          # Query the mode, e.g. OK MOD_TRAFFIC
SPK_HAPPY#      # Play happy sound
```

### Compatible Browsers

- **Chrome 56+**
- **Edge 79+**
- **Opera 43+**
- **Android Browser** (with Web Bluetooth enabled)

**Note**: Web Bluetooth is not supported in Firefox or Safari as of 2025.

## Prerequisites
### VS Code Extensions
- `ms-vscode.cpptools` for C++ IntelliSense support

### Required Software
#### Windows
- **Python 3.x** - Required for the CODAL build system
- **CMake** - Install via:
  - Download: `cmake-4.1.1-windows-x86_64.msi` from https://cmake.org/download/
  - Or: `winget install Kitware.CMake`
- **ARM GNU Toolchain** (14.2 and 14.3 both work) - Install via:
  - Download: `arm-gnu-toolchain-14.3.rel1-mingw-w64-x86_64-arm-none-eabi.exe` from https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
  - Or: `winget install Arm.GnuArmEmbeddedToolchain`
- **Ninja Build System** - Install via:
  - `winget install Ninja-build.Ninja`
  - Or download from: https://github.com/ninja-build/ninja/releases
- **Git** - Ensure git is in your PATH. If using SmartGit or similar:
  ```powershell
  $env:PATH = "C:\Program Files\SmartGit\git\cmd;$env:PATH"
  ```
#### Linux (Ubuntu/Debian)
```bash
# Install dependencies
sudo apt update
sudo apt install python3 python3-pip cmake gcc-arm-none-eabi binutils-arm-none-eabi ninja-build git
```

## Building the Project

### Basic Python Command Line

The project uses a Python build script that works on all platforms. Run it from the `lights/` folder:

```bash
# Build the project
python build.py

# Clean build (removes previous build artifacts)
python build.py --clean
```

**Note:** On Windows you can use `py` instead of `python` (the VS Code tasks do this). On some Linux/macOS systems, you may need to use `python3` instead of `python`.

### Flashing

Copy `release/MICROBIT.hex` onto the `MICROBIT` USB drive that appears when the micro:bit is plugged in. The micro:bit restarts with the new firmware once the copy finishes.

### VS Code Integration

The project includes pre-configured VS Code tasks for easy building within the editor.

#### Accessing Build Tasks

**Method 1: Quick Build (Recommended)**
- Press **`Ctrl+Shift+B`** - Shows build tasks directly since they're configured as build tasks

**Method 2: Task Command Palette**
- Press **`Ctrl+Shift+P`**
- Type **"Tasks: Run Task"**
- Select from available tasks:
  - **"CODAL build"** - Standard build
  - **"CODAL clean build"** - Clean build (removes previous artifacts)

**Method 3: Terminal Menu**
- Go to **Terminal** → **Run Task...**
- Select the desired build task

#### Available Tasks

- **CODAL build** - Standard incremental build (default build task)
- **CODAL clean build** - Clean build that removes all previous build artifacts

#### Task Troubleshooting

If VS Code tasks are not visible:
1. Ensure the `lights/.vscode/tasks.json` file exists, and open the `lights/` folder (or `robot.code-workspace`) in VS Code
2. Reload VS Code window: **`Ctrl+Shift+P`** → **"Developer: Reload Window"**
3. Try accessing tasks via **`Ctrl+Shift+B`** for build tasks specifically

## Project Structure

- `source/` - Your application source code (main.cpp)
- `libraries/` - CODAL libraries (auto-managed as git submodules)
- `build/` - Build output directory (intermediate files)
- `release/` - Final output directory (MICROBIT.hex and other artifacts)
- `utils/` - CODAL build utilities
- `codal.json` - Target configuration
- `build.py` - CODAL build script
- `robot.code-workspace` - VS Code workspace file (name kept from the robot car project)

### Code Overview

Almost all of the logic is in `source/main.cpp`:

- **Definitions** - Lamp pins, `LightLevels` (red, amber and green brightness, 0-255 each), the `LIGHT_*` constants, `SequenceStep` (lamps plus how long to hold them), `UK_TRAFFIC_SEQUENCE` and `PARTY_LIGHTS`
- **State** - `mode`, `bleLights`, `bleFadeMs`, `currentLights` and `modeChangeCount`, plus `modeStatus` and `lightsStatus`: the mode and lamps as text (e.g. `MOD_TRAFFIC`, `LED_FF0000`) for `STA_` messages and queries
- **Mode and Timing**:
  - `setMode()` changes the mode, increments `modeChangeCount`, updates `modeStatus` and sends `STA_MOD_` if the mode changed; `onModeChanged()` plays the mode sounds and shows the icons
  - `waitMs()` sleeps in 20 ms slices and returns `false` early if the mode changed during the wait
  - Only the main loop drives the lamps; button and command handlers just change the mode
- **Lamps**:
  - `levelToDuty()` converts a 0-255 level to a 0-1023 PWM duty using the brightness curve
  - `writeLights()` is the only code that writes to the pins
  - `setLights(target, fadeMs = 0)` is what everything else calls, for both instant changes and fades. A fade starts from the lamps showing now, so a new command partway through a fade continues smoothly. It updates `lightsStatus` with the target and sends `STA_LED_` (except in party mode)
  - `configureLightPWM()` raises the shared PWM rate from CODAL's 50 Hz default (which flickers) to 1 kHz. This only works once the pins are already in PWM mode
- **Commands** - `onDelim()` splits a command into its 4-character type and payload, then calls `handleLEDCommand()`, `handleModeCommand()` or `handleSoundCommand()`. `hexCharToValue()` and `parseHexColor()` parse the hex levels, and `parseFadeTime()` reads the optional `_TT` suffix

## Output

After a successful build, you'll find:
- `release/MICROBIT.hex` - Firmware file ready to flash to your micro:bit
- `release/MICROBIT.bin` - Binary firmware file (if generated)
- Build artifacts in the `build/` directory (intermediate files)

## Configuration

The project includes several configuration files:

- **`codal.json`** - Application configuration (Bluetooth enabled for NUS communication)
- **`codal.ble.json`** - Default Bluetooth configuration
- **`codal.dev.json`** - Development configuration (debugging enabled, dev mode, Bluetooth disabled)
- **`codal.min.json`** - Minimal configuration (Bluetooth disabled for smaller firmware)

To switch configurations, copy the desired file to `codal.json`:
```powershell
# Windows - Use minimal config (Bluetooth disabled)
Copy-Item codal.min.json codal.json

# Windows - Enable development mode
Copy-Item codal.dev.json codal.json
```

```bash
# Linux/macOS - Use minimal config (Bluetooth disabled)
cp codal.min.json codal.json

# Linux/macOS - Enable development mode
cp codal.dev.json codal.json
```

**Note**: The traffic light firmware needs Bluetooth. All of `main.cpp` is inside `#if CONFIG_ENABLED(DEVICE_BLE)`, so with `codal.min.json` or `codal.dev.json` none of the application code is built. Keep the default `codal.json` unless you are changing the code to work without Bluetooth.

## Troubleshooting

### Build Issues
- Ensure all prerequisites are installed and in your PATH
- If build fails, try a clean build: `python build.py --clean`
- On Windows, ensure you're using PowerShell or Command Prompt, not Git Bash
- Verify ARM toolchain installation: `arm-none-eabi-gcc --version`
- If the build fails to link, or the micro:bit does nothing after flashing, check that `codal.json` has Bluetooth enabled (see [Configuration](#configuration))

### Lamp Issues
- **No lamps at startup**: Check the wiring from P2, P8 and P9 to the FET LED driver board, the shared GND, and the driver board's lamp supply. The lamp test should fade all three up and down 3 times
- **Dim levels not visible**: `LED_` levels `01`-`08` all give the minimum PWM duty (about 1 µs on in every 1 ms), which the driver board may barely show or not respond to; try `20` or higher
- **Flicker**: The PWM rate is set to 1 kHz at startup. If lamps flicker at 50 Hz, check that `configureLightPWM()` still runs after the pins are put into PWM mode
- **Shimmer during slow fades**: CODAL restarts the PWM output on every update. If this shows, `writeLights()` could be changed to update only the lamps whose level changed

### Button Issues
- The party button is button A on P5. An external button must connect P5 to GND when pressed
- Each press toggles party mode; holding the button still counts as one press

### BLE Connection Issues
- Ensure you're using a Web Bluetooth compatible browser (Chrome 56+, Edge 79+)
- Make sure Bluetooth is enabled on your computer/device
- Try refreshing the web page and reconnecting
- Check that the micro:bit shows the heart pattern (❤️) on startup
- If connection fails, try power cycling the micro:bit

### Command Not Working
- Ensure commands follow the correct format:
  - LED commands: `LED_<RRAAGG>#` or `LED_<RRAAGG>_<TT>#` (e.g., `LED_FF0000#`, `LED_00FF00_20#`)
  - Mode commands: `MOD_TRAFFIC#` or `MOD_PARTY#` (upper case)
  - Audio commands: `SPK_<SOUND>#` (e.g., `SPK_HAPPY#`, upper case)
- Check that the micro:bit shows tick (✓) pattern when connected
- Check each command gets an `OK` or `ERR` reply; no reply at all usually means the `#` was missing
- For LED commands, ensure hex codes are exactly 6 characters (RRAAGG format), and any fade suffix is `_` plus exactly 2 digits (`_1`, `_AB` and `_200` are rejected)
- Invalid `LED_` and `MOD_` commands trigger "sad" audio feedback and an `ERR` reply; invalid `SPK_` commands only give the `ERR` reply
- After an `LED_` command the lamps stay as set until another command, a button press or a disconnect; send `MOD_TRAFFIC#` to resume the traffic cycle
