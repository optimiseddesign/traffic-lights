# BBC micro:bit V2 Robot - CODAL Project

This project uses the official CODAL (Component Oriented Device Abstraction Layer) build system for BBC micro:bit V2 development.

## Features

This micro:bit robot firmware provides:

- **Bluetooth Low Energy (BLE) UART Service** - Uses Nordic UART Service (NUS) for wireless communication
- **Robot Motor Control** - Drive commands for controlling robot car movement via I2C PWM controller
- **RGB LED Control** - Control onboard RGB LEDs with hex color codes
- **Visual Feedback** - Custom LED display patterns for connection status and received commands
- **Audio Feedback** - Sound expressions that respond to received commands
- **Command Processing** - Receives structured text commands via BLE with automatic command routing and audio/visual feedback

### Current Functionality

- **Connection Status Display**:
  - ❤️ Heart pattern on startup
  - ✓ Tick pattern when BLE connects
  - ✗ Cross pattern when BLE disconnects

- **Robot Control Commands** (format: `DRV_<command>#`):
  - `DRV_W#` - Move forward at full speed
  - `DRV_S#` - Move backward at full speed
  - `DRV_A#` - Spin left (left motor backward, right motor forward)
  - `DRV_D#` - Spin right (left motor forward, right motor backward)
  - `DRV_WA#` - Move forward-left (differential steering)
  - `DRV_WD#` - Move forward-right (differential steering)
  - `DRV_SA#` - Move backward-left (differential steering)
  - `DRV_SD#` - Move backward-right (differential steering)
  - `DRV_stop#` - Stop all motors

- **LED Control Commands** (format: `LED_<RRGGBB>#`):
  - `LED_FF0000#` - Set LEDs to red
  - `LED_00FF00#` - Set LEDs to green
  - `LED_0000FF#` - Set LEDs to blue
  - `LED_FFFF00#` - Set LEDs to yellow (red + green)
  - `LED_FF00FF#` - Set LEDs to magenta (red + blue)
  - `LED_00FFFF#` - Set LEDs to cyan (green + blue)
  - `LED_FFFFFF#` - Set LEDs to white (all colors)
  - `LED_000000#` - Turn LEDs off (no color)

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
  - Commands use structured format with 3-character type identifier and underscore separator
  - All commands must end with `#` delimiter
  - Automatic command routing based on prefix (`DRV_`, `LED_`, or direct commands)
  - Real-time feedback via BLE UART with command confirmation
  - Scrolls received commands on micro:bit LED display
  - Error handling with audio feedback for invalid commands

- **Hardware Integration**:
  - I2C PWM controller communication for motor and LED control
  - Compatible with Keyes MiniCar robot platform
  - Automatic LED initialization (all LEDs off at startup)

- **BLE Services**:
  - Nordic UART Service (NUS) for bidirectional text communication
  - Standard micro:bit button service

## Testing with Web Bluetooth

### Web Device CLI Tool

You can test the BLE UART functionality using the **Web Device CLI** tool, which provides a web-based interface for communicating with Nordic UART Service (NUS) enabled devices:

**Live Web Interface:**: https://optimiseddesign.github.io/web-device-cli (fork with swapped TX/RX UUIDs)

**GitHub Repository**: https://github.com/optimiseddesign/web-device-cli (fork with swapped TX/RX UUIDs)

### Testing Steps

1. **Build and flash** your micro:bit with this firmware
2. **Open the Web Device CLI** in a Web Bluetooth enabled browser (Chrome 56+, Edge 79+, etc.)
3. **Click "Connect"** and select your micro:bit from the device list
4. **Wait for connection** - you should see the tick (✓) pattern on your micro:bit
5. **Send commands** through the web interface:
   - **Motor Commands**: `DRV_W#` (forward), `DRV_S#` (backward), `DRV_A#` (left), `DRV_D#` (right), `DRV_stop#` (stop)
   - **LED Commands**: `LED_FF0000#` (red), `LED_00FF00#` (green), `LED_0000FF#` (blue), `LED_000000#` (off)
  - **Audio Commands**: `SPK_HAPPY#`, `SPK_GIGGLE#`, etc.
   - Watch for audio feedback, LED control, motor movement, and command scrolling on the micro:bit
   - See command confirmations and error messages in the web interface

### Example Commands

```
DRV_W#          # Move robot forward
LED_FF0000#     # Set LEDs to red
DRV_A#          # Spin robot left  
LED_00FF00#     # Set LEDs to green
DRV_stop#       # Stop robot
LED_000000#     # Turn off LEDs
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
- **ARM GNU Toolchain** - Install via:
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

The project uses a Python build script that works on all platforms:

```bash
# Build the project
python build.py

# Clean build (removes previous build artifacts)
python build.py --clean
```

**Note:** On some Linux/macOS systems, you may need to use `python3` instead of `python`.

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
1. Ensure the `.vscode/tasks.json` file exists in the project root
2. Reload VS Code window: **`Ctrl+Shift+P`** → **"Developer: Reload Window"**
3. Try accessing tasks via **`Ctrl+Shift+B`** for build tasks specifically

## Project Structure

- `source/` - Your application source code (main.cpp)
- `libraries/` - CODAL libraries (auto-managed as git submodules)
- `build/` - Build output directory (intermediate files)
- `release/` - Final output directory (MICROBIT.hex and other artifacts)
- `reference/` - Reference python code and examples for robot development
- `utils/` - CODAL build utilities
- `codal.json` - Target configuration
- `build.py` - CODAL build script

## Output

After a successful build, you'll find:
- `release/MICROBIT.hex` - Firmware file ready to flash to your micro:bit
- `release/MICROBIT.bin` - Binary firmware file (if generated)
- Build artifacts in the `build/` directory (intermediate files)

## Configuration

The project includes several configuration files:

- **`codal.json`** - Application configuration (Bluetooth enabled for NUS communication)
- **`codal.ble.json`** - Default Bluetooth configuration
- **`codal.dev.json`** - Development configuration (debugging enabled, dev mode)
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

**Note**: The current default `codal.json` has the evolved application functionality with Bluetooth enabled for BLE UART functionality. Use `codal.min.json` if you want a smaller firmware without Bluetooth.

## Reference Materials

The `reference/` directory contains example code and documentation for various micro:bit robot development scenarios:

### KS4036F Car Reference Code

- **`keyes_MiniCar.py`** - Complete Python class for controlling the KS4036F robot car platform
  - Motor control functions (left/right motor control, stop)
  - RGB LED control (individual left/right RGB LEDs)
  - Ultrasonic distance sensing
  - I2C communication for hardware control
  - Compatible with MicroPython on micro:bit

### Project Examples

The reference directory also includes 17 educational projects covering:

- **Basic Features**: Heartbeat, LED control, button programming
- **Sensors**: Temperature, compass, accelerometer, light detection, microphone
- **Communication**: Bluetooth wireless communication, touch-sensitive logo
- **Audio/Visual**: Speaker control, RGB modules
- **Advanced Robotics**: 
  - Motor driving and control
  - Light tracking smart car
  - Line tracking smart car  
  - Ultrasonic following and obstacle avoidance

These references provide implementation patterns that can be (and some have been) adapted from Python/MicroPython to C++ for use with the CODAL framework in this project.

## Troubleshooting

### Build Issues
- Ensure all prerequisites are installed and in your PATH
- If build fails, try a clean build: `python build.py --clean`
- On Windows, ensure you're using PowerShell or Command Prompt, not Git Bash
- Verify ARM toolchain installation: `arm-none-eabi-gcc --version`

### BLE Connection Issues
- Ensure you're using a Web Bluetooth compatible browser (Chrome 56+, Edge 79+)
- Make sure Bluetooth is enabled on your computer/device
- Try refreshing the web page and reconnecting
- Check that the micro:bit shows the heart pattern (❤️) on startup
- If connection fails, try power cycling the micro:bit

### Command Not Working
- Ensure commands follow the correct format:
  - Motor commands: `DRV_<command>#` (e.g., `DRV_W#`, `DRV_stop#`)
  - LED commands: `LED_<RRGGBB>#` (e.g., `LED_FF0000#`)
  - Audio commands: `SPK_<SOUND>#` (e.g., `SPK_HAPPY#`)
- Check that the micro:bit shows tick (✓) pattern when connected
- Verify the received command appears in the web interface echo
- Try different commands to test audio feedback, motor control, and LED control
- For LED commands, ensure hex color codes are exactly 6 characters (RRGGBB format)
- Invalid commands will trigger "sad" audio feedback and error messages