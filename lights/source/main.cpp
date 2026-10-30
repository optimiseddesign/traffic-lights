#include "MicroBit.h"

#if CONFIG_ENABLED(DEVICE_BLE)

MicroBit uBit;
MicroBitUARTService *uart;

// Traffic light GPIO pins (HIGH = on, PWM for brightness)
// P3/P4/P7/P10 avoided as they drive the 5x5 LED matrix
// P5 is button A (party button, external button wired in parallel)
#define RED_PIN     uBit.io.P2
#define AMBER_PIN   uBit.io.P8
#define GREEN_PIN   uBit.io.P9

// Brightness of each light, like an RGB colour: 0 = off, 255 = full (PWM)
struct LightLevels {
    uint8_t red;
    uint8_t amber;
    uint8_t green;
};

//                                              R    A    G
static const LightLevels LIGHT_OFF         = {  0,   0,   0};
static const LightLevels LIGHT_RED         = {255,   0,   0};
static const LightLevels LIGHT_AMBER       = {  0, 255,   0};
static const LightLevels LIGHT_GREEN       = {  0,   0, 255};
static const LightLevels LIGHT_RED_AMBER   = {255, 255,   0};
static const LightLevels LIGHT_RED_GREEN   = {255,   0, 255};
static const LightLevels LIGHT_AMBER_GREEN = {  0, 255, 255};
static const LightLevels LIGHT_ALL         = {255, 255, 255};

// One step of a light sequence: which lights are on and for how long
struct SequenceStep {
    LightLevels lights;
    int durationMs;
};

// UK traffic light cycle (40s). Red+amber 2s and amber 3s are fixed by law (TSRGD 2016);
// red lasts while the other road gets its red+amber, green and amber (2 + 15 + 3 = 20s)
static const SequenceStep UK_TRAFFIC_SEQUENCE[] = {
    {LIGHT_RED,       20000},
    {LIGHT_RED_AMBER,  2000},
    {LIGHT_GREEN,     15000},
    {LIGHT_AMBER,      3000},
};

// Every combination with at least one light on, picked at random in party mode
static const LightLevels PARTY_LIGHTS[] = {
    LIGHT_RED, LIGHT_AMBER, LIGHT_GREEN, LIGHT_RED_AMBER,
    LIGHT_RED_GREEN, LIGHT_AMBER_GREEN, LIGHT_ALL,
};
static const int PARTY_LIGHTS_COUNT = sizeof(PARTY_LIGHTS) / sizeof(PARTY_LIGHTS[0]);

// Operating modes: local UK traffic cycle or party, or lights held by Bluetooth LED_ commands
// Only the main loop drives the lights - handlers just set the mode and what to show
enum Mode { MODE_TRAFFIC, MODE_PARTY, MODE_BLE };
static Mode mode = MODE_TRAFFIC;
static LightLevels bleLights = LIGHT_OFF;       // lights held while in MODE_BLE
static int bleFadeMs = 0;                       // fade time to bleLights (0 = instant)
static LightLevels currentLights = LIGHT_OFF;   // lights showing now, so fades know where to start

// Counts every setMode() call, even to the same mode, so a new command interrupts the current wait
static int modeChangeCount = 0;

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

const char * const party_emoji =
"    255,000,255,000,255\n"
"    000,255,255,255,000\n"
"    255,255,000,255,255\n"
"    000,255,255,255,000\n"
"    255,000,255,000,255\n";

// Create global image objects from the patterns
MicroBitImage heartImage(heart_emoji);
MicroBitImage tickImage(tick_emoji);
MicroBitImage crossImage(cross_emoji);
MicroBitImage partyImage(party_emoji);

// Sound and icon feedback when the mode changes
static void onModeChanged(Mode oldMode, Mode newMode)
{
    // Party starts with a giggle and the party icon; leaving it shows the heart again
    if (newMode == MODE_PARTY && oldMode != MODE_PARTY) {
        uBit.audio.soundExpressions.playAsync("giggle");
        uBit.display.print(partyImage);
    } else if (newMode != MODE_PARTY && oldMode == MODE_PARTY) {
        uBit.display.print(heartImage);
    }
}

static void setMode(Mode newMode)
{
    onModeChanged(mode, newMode);
    mode = newMode;
    modeChangeCount++;
}

// Sleep for durationMs in short slices, returning false early if the mode changes
static bool waitMs(int durationMs)
{
    int startCount = modeChangeCount;
    for (int elapsedMs = 0; elapsedMs < durationMs; elapsedMs += 20) {
        uBit.sleep(20);
        if (modeChangeCount != startCount) return false;
    }
    return true;
}

// Convert a 0-255 brightness level to a 0-1023 PWM duty
// Halfway between linear and squared: eyes see brightness non-linearly, so this makes
// dim levels and fades look more even than linear, while keeping low levels visible
static int levelToDuty(uint8_t level)
{
    return level * (level + 255) * 1023 / (2 * 255 * 255);
}

// Write brightness levels straight to the light pins using PWM - only setLights() calls this
static void writeLights(LightLevels lights)
{
    RED_PIN.setAnalogValue(levelToDuty(lights.red));
    AMBER_PIN.setAnalogValue(levelToDuty(lights.amber));
    GREEN_PIN.setAnalogValue(levelToDuty(lights.green));
    currentLights = lights;
}

// Change the lights to target, instantly or fading smoothly over fadeMs
// Returns false if a mode change cut the fade short
static bool setLights(LightLevels target, int fadeMs = 0)
{
    LightLevels start = currentLights;

    // Every 20ms, move each light part way from start to target
    for (int elapsedMs = 20; elapsedMs < fadeMs; elapsedMs += 20) {
        LightLevels step;
        step.red   = start.red   + (target.red   - start.red)   * elapsedMs / fadeMs;
        step.amber = start.amber + (target.amber - start.amber) * elapsedMs / fadeMs;
        step.green = start.green + (target.green - start.green) * elapsedMs / fadeMs;
        writeLights(step);

        if (!waitMs(20)) return false;
    }

    writeLights(target);
    return true;
}

// Show a random light combination for 60-600ms, never the same one twice running
static void runPartyStep()
{
    static int partyIndex = 0;

    // Move 1 to (count - 1) places round the list, so it always lands on a different combination
    partyIndex = (partyIndex + 1 + uBit.random(PARTY_LIGHTS_COUNT - 1)) % PARTY_LIGHTS_COUNT;
    setLights(PARTY_LIGHTS[partyIndex]);
    waitMs(60 + uBit.random(541));
}

// Helper function to convert hex character to value
static int hexCharToValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1; // Invalid hex character
}

// Helper function to parse 6-character hex color code (RRAAGG) and extract light levels
static bool parseHexColor(ManagedString hexColor, uint8_t &red, uint8_t &amber, uint8_t &green) {
    if (hexColor.length() != 6) return false;

    // Parse red component (first 2 chars)
    int r1 = hexCharToValue(hexColor.charAt(0));
    int r2 = hexCharToValue(hexColor.charAt(1));
    if (r1 < 0 || r2 < 0) return false;

    // Parse amber component (chars 2-3)
    int a1 = hexCharToValue(hexColor.charAt(2));
    int a2 = hexCharToValue(hexColor.charAt(3));
    if (a1 < 0 || a2 < 0) return false;

    // Parse green component (chars 4-5)
    int g1 = hexCharToValue(hexColor.charAt(4));
    int g2 = hexCharToValue(hexColor.charAt(5));
    if (g1 < 0 || g2 < 0) return false;

    // Convert to light levels (0-255, 0=off)
    red = r1 * 16 + r2;
    amber = a1 * 16 + a2;
    green = g1 * 16 + g2;

    return true;
}

// Helper function to parse an optional "_TT" fade time suffix (tenths of a second, 00-99) into ms
static bool parseFadeTime(ManagedString suffix, int &fadeMs) {
    if (suffix.length() == 0) {
        fadeMs = 0; // No suffix: instant
        return true;
    }
    if (suffix.length() != 3 || suffix.charAt(0) != '_') return false;

    char t1 = suffix.charAt(1);
    char t2 = suffix.charAt(2);
    if (t1 < '0' || t1 > '9' || t2 < '0' || t2 > '9') return false;

    fadeMs = ((t1 - '0') * 10 + (t2 - '0')) * 100;
    return true;
}

// Function to handle LED commands starting with LED_
static void handleLEDCommand(ManagedString ledCmd) {
    // Expected format: 6-character hex color code (RRAAGG), 00-FF brightness per light,
    // optionally followed by _TT fade time in tenths of a second (00-99), otherwise instant
    // Example: LED_FF0000# for red, LED_FF8000# for red + half amber, LED_00FF00_20# fade to amber over 2s

    uint8_t red, amber, green;
    int fadeMs;
    if (parseHexColor(ledCmd.substring(0, 6), red, amber, green) &&
        parseFadeTime(ledCmd.substring(6, ledCmd.length() - 6), fadeMs)) {
        // Fade to these lights in Bluetooth mode and hold them (the main loop sets them)
        bleLights = {red, amber, green};
        bleFadeMs = fadeMs;
        setMode(MODE_BLE);
        uart->send(ManagedString("LED lights set\r\n"));
    } else {
        uBit.audio.soundExpressions.playAsync("sad");
        uart->send(ManagedString("ERROR: invalid LED_ format, use LED_RRAAGG# or LED_RRAAGG_TT#\r\n"));
    }
}

// Function to handle mode commands starting with MOD_
static void handleModeCommand(ManagedString modeCmd) {
    if (modeCmd == "TRAFFIC") {
        setMode(MODE_TRAFFIC);
        uart->send(ManagedString("Mode: traffic\r\n"));
    } else if (modeCmd == "PARTY") {
        setMode(MODE_PARTY);
        uart->send(ManagedString("Mode: party\r\n"));
    } else {
        uBit.audio.soundExpressions.playAsync("sad");
        uart->send(ManagedString("ERROR: unrecognised MOD_ command\r\n"));
    }
}

struct SoundCommandMapping {
    const char *command;
    const char *expression;
};

// Function to handle sound commands starting with SPK_
static void handleSoundCommand(ManagedString soundCmd) {
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
            return;
        }
    }

    // No error sound here, as "sad" is also a valid SPK_ command - silence shows the failure
    uart->send(ManagedString("ERROR: unrecognised SPK_ command\r\n"));
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

    // Hand control back to the local traffic cycle
    if (mode == MODE_BLE) setMode(MODE_TRAFFIC);
}

static void onPartyButton(MicroBitEvent)
{
    // Toggle party on/off; from Bluetooth control this takes back local control into party
    setMode(mode == MODE_PARTY ? MODE_TRAFFIC : MODE_PARTY);
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

    // Split into the 4-character command type (e.g. LED_) and its payload (empty if none)
    ManagedString cmdType = r_clean.substring(0, 4);
    ManagedString payload = r_clean.substring(4, r_clean.length() - 4);

    // Route commands to appropriate handlers
    if (cmdType == "LED_") {
        handleLEDCommand(payload);
    } else if (cmdType == "MOD_") {
        handleModeCommand(payload);
    } else if (cmdType == "SPK_") {
        handleSoundCommand(payload);
    } else {
        // Default behavior for unknown commands
        uBit.audio.soundExpressions.playAsync("sad");
        uart->send(ManagedString("ERROR: unrecognised command type\r\n"));
    }
}

static void onStart() {
    uBit.audio.soundExpressions.playAsync("hello");
    uBit.display.print(heartImage);

    // Lamp test: fade all lights up and down 3 times (checks PWM dimming)
    for (int i = 0; i < 3; i++) {
        setLights(LIGHT_ALL, 400);
        setLights(LIGHT_OFF, 400);
    }
}

static void configureOnboardSpeaker()
{
    // Play audio on the onboard speaker only (CODAL also outputs on P0 by default)
    uBit.audio.setSpeakerEnabled(true);
    uBit.audio.setPinEnabled(false);
}

static void configureLightPWM()
{
    // Put all lights into PWM mode (off), then raise the shared PWM rate from CODAL's 50Hz
    // default, which visibly flickers, to 1kHz. The rate can only be set once a pin uses PWM
    setLights(LIGHT_OFF);
    RED_PIN.setAnalogPeriodUs(1000);
}

int main()
{
    // Initialise the micro:bit runtime, and do startup actions
    uBit.init();
    configureOnboardSpeaker();
    configureLightPWM();
    onStart();

    // Register event listeners for Bluetooth connection/disconnection
    uBit.messageBus.listen(MICROBIT_ID_BLE, MICROBIT_BLE_EVT_CONNECTED, onConnected);
    uBit.messageBus.listen(MICROBIT_ID_BLE, MICROBIT_BLE_EVT_DISCONNECTED, onDisconnected);
    uBit.messageBus.listen(MICROBIT_ID_BLE_UART, MICROBIT_UART_S_EVT_DELIM_MATCH, onDelim);

    // Party button: act on press (not click, which CODAL drops if held over 1s)
    uBit.messageBus.listen(MICROBIT_ID_BUTTON_A, MICROBIT_BUTTON_EVT_DOWN, onPartyButton);

    // Create BLE services to enable advertising and discovery
    new MicroBitButtonService(*uBit.ble);           // Button service for standard micro:bit profile
    uart = new MicroBitUARTService(*uBit.ble, 32, 32);
    uart->eventOn("#");

    // Run the current mode forever; each pass ends early when the mode changes
    while (true) {
        switch (mode) {
            case MODE_TRAFFIC:
                // Run the UK cycle from red; stop early if the mode changes
                for (const SequenceStep &step : UK_TRAFFIC_SEQUENCE) {
                    setLights(step.lights);
                    if (!waitMs(step.durationMs)) break;
                }
                break;

            case MODE_PARTY:
                runPartyStep();
                break;

            case MODE_BLE:
                // Fade to the lights from the last LED_ command, then hold them until something changes
                if (setLights(bleLights, bleFadeMs)) {
                    while (waitMs(1000)) {}
                }
                break;
        }
    }
}

#endif // CONFIG_ENABLED(DEVICE_BLE)
