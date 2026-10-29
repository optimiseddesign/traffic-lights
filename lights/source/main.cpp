#include "MicroBit.h"

#if CONFIG_ENABLED(DEVICE_BLE)

MicroBit uBit;
MicroBitUARTService *uart;

// Traffic light GPIO pins (HIGH = on)
// P3/P4/P7/P10 avoided as they drive the 5x5 LED matrix, P5 is button A
#define RED_PIN     uBit.io.P2
#define AMBER_PIN   uBit.io.P8
#define GREEN_PIN   uBit.io.P9

// Brightness of each light, like an RGB colour: 0 = off, 255 = full
// On/off only for now (any non-zero value is on) until PWM brightness is added
struct LightLevels {
    uint8_t red;
    uint8_t amber;
    uint8_t green;
};

//                                        R    A    G
static const LightLevels LIGHT_OFF   = {  0,   0,   0};
static const LightLevels LIGHT_RED   = {255,   0,   0};
static const LightLevels LIGHT_AMBER = {  0, 255,   0};
static const LightLevels LIGHT_GREEN = {  0,   0, 255};
static const LightLevels LIGHT_ALL   = {255, 255, 255};

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

// Switch each light on or off to match the given brightness (PWM to be added later)
static void setLights(LightLevels lights)
{
    RED_PIN.setDigitalValue(lights.red > 0 ? 1 : 0);
    AMBER_PIN.setDigitalValue(lights.amber > 0 ? 1 : 0);
    GREEN_PIN.setDigitalValue(lights.green > 0 ? 1 : 0);
}

// Function to handle LED commands starting with LED_
// Placeholder for future Bluetooth light control - fixed test commands only for now
static void handleLEDCommand(ManagedString ledCmd) {
    if (ledCmd == "R") {
        setLights(LIGHT_RED);
    } else if (ledCmd == "A") {
        setLights(LIGHT_AMBER);
    } else if (ledCmd == "G") {
        setLights(LIGHT_GREEN);
    } else if (ledCmd == "OFF") {
        setLights(LIGHT_OFF);
    } else {
        uBit.audio.soundExpressions.playAsync("sad");
        uart->send(ManagedString("ERROR: unrecognised LED_ command\r\n"));
        return;
    }

    uart->send(ManagedString("LED lights set\r\n"));
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
    if (r_clean.substring(0, 4) == "LED_") {
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

static void configureOnboardSpeaker()
{
    // Play audio on the onboard speaker only (CODAL also outputs on P0 by default)
    uBit.audio.setSpeakerEnabled(true);
    uBit.audio.setPinEnabled(false);
}

int main()
{
    // Initialise the micro:bit runtime, and do startup actions
    uBit.init();
    configureOnboardSpeaker();
    setLights(LIGHT_OFF);
    onStart();

    // Register event listeners for Bluetooth connection/disconnection
    uBit.messageBus.listen(MICROBIT_ID_BLE, MICROBIT_BLE_EVT_CONNECTED, onConnected);
    uBit.messageBus.listen(MICROBIT_ID_BLE, MICROBIT_BLE_EVT_DISCONNECTED, onDisconnected);
    uBit.messageBus.listen(MICROBIT_ID_BLE_UART, MICROBIT_UART_S_EVT_DELIM_MATCH, onDelim);

    // Create BLE services to enable advertising and discovery
    new MicroBitButtonService(*uBit.ble);           // Button service for standard micro:bit profile
    uart = new MicroBitUARTService(*uBit.ble, 32, 32);
    uart->eventOn("#");

    // Light test: step through each light, then all on, then all off
    // (LED_ commands work too, but only last until the next test step)
    static const LightLevels TEST_SEQUENCE[] = {LIGHT_RED, LIGHT_AMBER, LIGHT_GREEN, LIGHT_ALL, LIGHT_OFF};

    while (true) {
        for (const LightLevels &lights : TEST_SEQUENCE) {
            setLights(lights);
            uBit.sleep(1000);
        }
    }
}

#endif // CONFIG_ENABLED(DEVICE_BLE)
