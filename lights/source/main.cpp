#include "MicroBit.h"
#include "MiniCar.h"

#if CONFIG_ENABLED(DEVICE_BLE)

MicroBit uBit;
MicroBitUARTService *uart;
MiniCar *car;

// Define custom 5x5 LED patterns using CODAL string format
const char * const heart_emoji =
"    000,255,000,255,000\n"
"    255,255,255,255,255\n"
"    255,255,255,255,255\n"
"    000,255,255,255,000\n"
"    000,000,255,000,000\n";

const char * const tick_emoji =
"    000,000,000,000,000\n"
"    000,000,000,000,255\n"
"    000,000,000,255,000\n"
"    255,000,255,000,000\n"
"    000,255,000,000,000\n";

const char * const cross_emoji =
"    255,000,000,000,255\n"
"    000,255,000,255,000\n"
"    000,000,255,000,000\n"
"    000,255,000,255,000\n"
"    255,000,000,000,255\n";

// Create global image objects from the patterns
MicroBitImage heartImage(heart_emoji);
MicroBitImage tickImage(tick_emoji);
MicroBitImage crossImage(cross_emoji);

// Helper function to convert hex character to value
static int hexCharToValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1; // Invalid hex character
}

// Helper function to parse 6-character hex color code (RRGGBB) and extract RGB values
static bool parseHexColor(ManagedString hexColor, uint8_t &red, uint8_t &green, uint8_t &blue) {
    if (hexColor.length() != 6) return false;
    
    // Parse red component (first 2 chars)
    int r1 = hexCharToValue(hexColor.charAt(0));
    int r2 = hexCharToValue(hexColor.charAt(1));
    if (r1 < 0 || r2 < 0) return false;
    
    // Parse green component (chars 2-3)
    int g1 = hexCharToValue(hexColor.charAt(2));
    int g2 = hexCharToValue(hexColor.charAt(3));
    if (g1 < 0 || g2 < 0) return false;
    
    // Parse blue component (chars 4-5)
    int b1 = hexCharToValue(hexColor.charAt(4));
    int b2 = hexCharToValue(hexColor.charAt(5));
    if (b1 < 0 || b2 < 0) return false;
    
    // Convert to RGB values (0-255) and invert for LED control (0=brightest, 255=off)
    red = 255 - (r1 * 16 + r2);
    green = 255 - (g1 * 16 + g2);
    blue = 255 - (b1 * 16 + b2);
    
    return true;
}

// Function to handle motor commands starting with DRV_
static void handleMotorCommand(ManagedString motorCmd) {
    if (motorCmd == "W") {
        car->Motor_L(1, 60);
        car->Motor_R(1, 60);
        uart->send(ManagedString("forward\r\n"));
    } else if (motorCmd == "S") {
        car->Motor_L(0, 60);
        car->Motor_R(0, 60);
        uart->send(ManagedString("backward\r\n"));
    } else if (motorCmd == "A") {
        car->Motor_L(0, 37);
        car->Motor_R(1, 37);
        uart->send(ManagedString("spin-left\r\n"));
    } else if (motorCmd == "D") {
        car->Motor_L(1, 37);
        car->Motor_R(0, 37);
        uart->send(ManagedString("spin-right\r\n"));
    } else if (motorCmd == "WA") {
        car->Motor_L(1, 40);
        car->Motor_R(1, 65);
        uart->send(ManagedString("forward-left\r\n"));
    } else if (motorCmd == "WD") {
        car->Motor_L(1, 65);
        car->Motor_R(1, 40);
        uart->send(ManagedString("forward-right\r\n"));
    } else if (motorCmd == "SA") {
        car->Motor_L(0, 40);
        car->Motor_R(0, 65);
        uart->send(ManagedString("backward-left\r\n"));
    } else if (motorCmd == "SD") {
        car->Motor_L(0, 65);
        car->Motor_R(0, 40);
        uart->send(ManagedString("backward-right\r\n"));
    } else if (motorCmd == "stop") {
        car->Motor_stop();
        uart->send(ManagedString("stopped\r\n"));
    } else {
        uBit.audio.soundExpressions.playAsync("sad");
        uart->send(ManagedString("ERROR: unrecognised DRV_ command\r\n"));
    }
}

// Function to handle LED commands starting with LED_
static void handleLEDCommand(ManagedString ledCmd) {
    // Expected format: 6-character hex color code (RRGGBB)
    // Example: LED_FF0000# for red, LED_00FF00# for green, LED_0000FF# for blue
    
    uint8_t red, green, blue;
    if (parseHexColor(ledCmd, red, green, blue)) {
        // Set both left and right LEDs to the same color
        car->setLeftRGB(red, green, blue);
        car->setRightRGB(red, green, blue);
        uart->send(ManagedString("LED color set\r\n"));
    } else {
        uBit.audio.soundExpressions.playAsync("sad");
        uart->send(ManagedString("ERROR: invalid LED_ hex color format\r\n"));
    }
}

struct SoundCommandMapping {
    const char *command;
    const char *expression;
};

static bool handleSoundCommand(ManagedString soundCmd) {
    static const SoundCommandMapping SOUND_COMMANDS[] = {
        {"GIGGLE", "giggle"},
        {"HAPPY", "happy"},
        {"HELLO", "hello"},
        {"MYSTERIOUS", "mysterious"},
        {"SAD", "sad"},
        {"SLIDE", "slide"},
        {"SOARING", "soaring"},
        {"SPRING", "spring"},
        {"TWINKLE", "twinkle"},
        {"YAWN", "yawn"}
    };

    for (const auto &mapping : SOUND_COMMANDS) {
        if (soundCmd == mapping.command) {
            uBit.audio.soundExpressions.playAsync(mapping.expression);
            return true;
        }
    }

    return false;
}

static void onConnected(MicroBitEvent)
{
    // Display a tick when Bluetooth connects
    uBit.display.print(tickImage);

    // Additional small delay to ensure UART service is ready
    uBit.sleep(1000);

    uart->send(ManagedString("Ready to receive...\r\n"));
}

static void onDisconnected(MicroBitEvent)
{
    // Display a cross when Bluetooth disconnects
    uBit.display.print(crossImage);
}

static void onDelim(MicroBitEvent)
{
    ManagedString r = uart->readUntil("#");

    // Strip carriage returns and line feeds from received data
    ManagedString r_clean("");
    for (int i = 0; i < r.length(); i++) {
        char c = r.charAt(i);
        if (c != '\r' && c != '\n') {
            r_clean = r_clean + ManagedString(c);
        }
    }

    // Echo back the cleaned received string
    uart->send(ManagedString("Received: ") + r_clean + ManagedString("\r\n"));

    // Route commands to appropriate handlers
    if (r_clean.substring(0, 4) == "DRV_") {
        ManagedString motorCmd = r_clean.substring(4, r_clean.length() - 4);
        handleMotorCommand(motorCmd);
    } else if (r_clean.substring(0, 4) == "LED_") {
        ManagedString ledCmd = r_clean.substring(4, r_clean.length() - 4);
        handleLEDCommand(ledCmd);
    } else if (r_clean.substring(0, 4) == "SPK_") {
        if (r_clean.length() <= 4) {
            uBit.audio.soundExpressions.playAsync("sad");
            uart->send(ManagedString("ERROR: missing SPK_ command payload\r\n"));
            return;
        }

        ManagedString soundCmd = r_clean.substring(4, r_clean.length() - 4);
        if (!handleSoundCommand(soundCmd)) {
            uBit.audio.soundExpressions.playAsync("sad");
            uart->send(ManagedString("ERROR: unrecognised SPK_ command\r\n"));
        }
    } else {
        // Default behavior for unknown commands
        uBit.audio.soundExpressions.playAsync("sad");
        uart->send(ManagedString("ERROR: unrecognised command type\r\n"));
    }
}

static void onStart() {
    uBit.audio.soundExpressions.playAsync("hello");
    uBit.display.print(heartImage);
}

static void configureExternalSpeaker()
{
    // Route all audio to the external speaker connected on pin P2 and disable the onboard speaker.
    uBit.audio.setSpeakerEnabled(false);
    uBit.audio.setPin(uBit.io.P2);
    uBit.audio.setPinEnabled(true);
}

int main()
{
    // Initialise the micro:bit runtime, and do startup actions
    uBit.init();
    configureExternalSpeaker();
    onStart();         

    // Initialize the robot car
    car = new MiniCar(uBit.i2c);
    car->init();
    car->led_off(); // Turn off all LEDs initially

    // Register event listeners for Bluetooth connection/disconnection
    uBit.messageBus.listen(MICROBIT_ID_BLE, MICROBIT_BLE_EVT_CONNECTED, onConnected);
    uBit.messageBus.listen(MICROBIT_ID_BLE, MICROBIT_BLE_EVT_DISCONNECTED, onDisconnected);
    uBit.messageBus.listen(MICROBIT_ID_BLE_UART, MICROBIT_UART_S_EVT_DELIM_MATCH, onDelim);
    
    // Create BLE services to enable advertising and discovery
    new MicroBitButtonService(*uBit.ble);           // Button service for standard micro:bit profile
    uart = new MicroBitUARTService(*uBit.ble, 32, 32);
    uart->eventOn("#");

    while (true) {
        uBit.sleep(1000);
    }
}

#endif // CONFIG_ENABLED(DEVICE_BLE)