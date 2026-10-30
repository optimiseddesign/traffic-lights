#include "MicroBit.h"

#if CONFIG_ENABLED(DEVICE_BLE)

MicroBit uBit;
MicroBitUARTService *uart;

// Traffic light GPIO pins (HIGH = on)
// P3/P4/P7/P10 avoided as they drive the 5x5 LED matrix
// P5 is button A (party button, external button wired in parallel)
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
static LightLevels bleLights = LIGHT_OFF;   // lights held while in MODE_BLE

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

// Switch each light on or off to match the given brightness (PWM to be added later)
static void setLights(LightLevels lights)
{
    RED_PIN.setDigitalValue(lights.red > 0 ? 1 : 0);
    AMBER_PIN.setDigitalValue(lights.amber > 0 ? 1 : 0);
    GREEN_PIN.setDigitalValue(lights.green > 0 ? 1 : 0);
}

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

// Show a random light combination for 60-600ms, never the same one twice running
static void runPartyStep()
{
    static int partyIndex = 0;

    // Move 1 to (count - 1) places round the list, so it always lands on a different combination
    partyIndex = (partyIndex + 1 + uBit.random(PARTY_LIGHTS_COUNT - 1)) % PARTY_LIGHTS_COUNT;
    setLights(PARTY_LIGHTS[partyIndex]);
    waitMs(60 + uBit.random(541));
}

// Function to handle LED commands starting with LED_
// Placeholder for future Bluetooth light control - fixed test commands only for now
static void handleLEDCommand(ManagedString ledCmd) {
    if (ledCmd == "R") {
        bleLights = LIGHT_RED;
    } else if (ledCmd == "A") {
        bleLights = LIGHT_AMBER;
    } else if (ledCmd == "G") {
        bleLights = LIGHT_GREEN;
    } else if (ledCmd == "OFF") {
        bleLights = LIGHT_OFF;
    } else {
        uBit.audio.soundExpressions.playAsync("sad");
        uart->send(ManagedString("ERROR: unrecognised LED_ command\r\n"));
        return;
    }

    setMode(MODE_BLE);
    uart->send(ManagedString("LED lights set\r\n"));
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

    // Lamp test: flash all lights quickly for 2 seconds
    for (int elapsedMs = 0; elapsedMs < 2000; elapsedMs += 200) {
        setLights(LIGHT_ALL);
        uBit.sleep(100);
        setLights(LIGHT_OFF);
        uBit.sleep(100);
    }
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
                // Hold the lights from the last LED_ command until something changes
                setLights(bleLights);
                waitMs(1000);
                break;
        }
    }
}

#endif // CONFIG_ENABLED(DEVICE_BLE)
