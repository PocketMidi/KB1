#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include <Preferences.h>
#include <MIDI.h>
#include <objects/Constants.h>

extern Adafruit_MCP23X17 mcp_U1;
extern Adafruit_MCP23X17 mcp_U2;

struct key {
    uint8_t midi;
    uint8_t pin;
    bool state;
    bool prevState;
    Adafruit_MCP23X17 *mcp;
    char name[12];

    // Constructor to allow initialization
    key(uint8_t m, uint8_t p, bool s, bool ps, Adafruit_MCP23X17 *_mcp, const char* n)
        : midi(m), pin(p), state(s), prevState(false), mcp(_mcp)
    {
        strncpy(name, n, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0'; // Ensure null-termination
        lastReading = s;
        debouncedState = s;
        lastDebounceTime = 0;
    }

    // Default constructor needed for array initialization
    key() : midi(0), pin(0), state(false), prevState(false), mcp(nullptr) {
        name[0] = '\0';
        lastReading = false;
        debouncedState = false;
        lastDebounceTime = 0;
    }
    
    // Debounce/state tracking
    bool lastReading;
    bool debouncedState;
    unsigned long lastDebounceTime;
};

// GPIO cache structure for bulk I2C reads (Performance Optimization)
// Reads all 32 GPIO pins (16 per MCP) in just 2 I2C transactions instead of 25+
// Performance: ~12× faster, 92% I2C bus reduction, -8-12mA power savings
struct GPIOCache {
    uint16_t u1_pins;        // All 16 pins from MCP U1
    uint16_t u2_pins;        // All 16 pins from MCP U2
    unsigned long timestamp; // When this snapshot was taken (microseconds)
    
    // Helper: Check if a pin is LOW (pressed/active) on U1
    inline bool isU1PinLow(uint8_t pin) const {
        return !(u1_pins & (1 << pin));
    }
    
    // Helper: Check if a pin is LOW (pressed/active) on U2
    inline bool isU2PinLow(uint8_t pin) const {
        return !(u2_pins & (1 << pin));
    }
};

enum class LeverFunctionMode {
    INTERPOLATED,
    PEAK_AND_DECAY,
    INCREMENTAL,
};

enum class LeverPushFunctionMode {
    INTERPOLATED,
    PEAK_AND_DECAY,
    STATIC,
    RESET,
};

enum class TouchFunctionMode {
    HOLD,
    TOGGLE,
    CONTINUOUS,
};

enum class InterpolationType {
    LINEAR,
    EXPONENTIAL,
    LOGARITHMIC
};

enum class ValueMode {
    UNIPOLAR,
    BIPOLAR
};

enum class ScaleType {
    CHROMATIC,
    MAJOR,
    MINOR,
    HARMONIC_MINOR,
    MELODIC_MINOR_ASC,
    PENTATONIC_MAJOR,
    PENTATONIC_MINOR,
    BLUES,
    DORIAN,
    PHRYGIAN,
    LYDIAN,
    MIXOLYDIAN,
    LOCRIAN,
    PHRYGIAN_DOMINANT,
    WHOLE_TONE,
    DIMINISHED,
    BLUES_MAJOR,
    HIRAJOSHI,
    IN_SEN,
    DOUBLE_HARMONIC,
    SUPER_LOCRIAN
};

enum class ChordType {
    MAJOR,
    MINOR,
    DIMINISHED,
    AUGMENTED,
    SUS2,
    SUS4,
    POWER,
    MAJOR7,
    MINOR7,
    DOM7,
    MAJOR_ADD9,
    MINOR_ADD9,
    MAJOR6,
    MINOR6,
    MAJOR9
};

enum class PlayMode {
    SCALE,
    CHORD
};

extern Preferences preferences;

#define MAX_KEYS 19


extern MIDI_NAMESPACE::MidiInterface<MIDI_NAMESPACE::SerialMIDI<HardwareSerial>> MIDI;

// Lever cooldown after BLE toggle (prevents MIDI output during lever release)
extern unsigned long leverCooldownUntil;

// Callbacks for syncing lever values when settings change via BLE
extern void (*syncLever1Callback)();
extern void (*syncLeverPush1Callback)();
extern void (*syncLever2Callback)();
extern void (*syncLeverPush2Callback)();

// Callback for notifying BLE when chord settings change from firmware
extern void (*notifyChordSettingsCallback)();

// Callback for notifying BLE when scale settings change from firmware
extern void (*notifyScaleSettingsCallback)();

// Callback for resetting pattern controls when shape mode is disabled
extern void (*resetPatternControlsCallback)();

// Runtime serial detection - only print when USB CDC terminal is connected
#ifdef SERIAL_PRINT_ENABLED
extern bool serialConnected;  // Global flag updated periodically in main loop
#define SERIAL_BEGIN() Serial.begin(115200); serialConnected = (bool)Serial;
#define SERIAL_PRINT(msg) if (serialConnected) Serial.print(msg);
#define SERIAL_PRINTLN(msg) if (serialConnected) Serial.println(msg);
#else
#define SERIAL_BEGIN()
#define SERIAL_PRINT(msg)
#define SERIAL_PRINTLN(msg)
#endif

// Battery state tracking
struct BatteryState {
    uint32_t accumulatedDischargeMs;  // Total time on battery since last full charge
    uint32_t lastUpdateMs;            // Last time battery was updated
    uint32_t accumulatedChargeMs;     // Total accumulated charge time (persists in NVS)
    uint32_t chargeSessionStartMs;    // When current charge session started (RAM only, 0 = not charging)
    uint32_t lastSaveMs;              // Last time battery state was saved to NVS
    uint32_t calibrationTimestamp;    // Unix timestamp when last calibrated (0 = never)
    bool lastUsbState;                // Previous USB connection state
    bool isFullyCharged;              // True if battery considered 100%
    bool isChargingMode;              // True only if valid sequence: boot on battery -> plug USB
    uint8_t estimatedPercentage;      // Current battery estimate (0-100, 254=uncalibrated, 255=usb/charging)
    
    // Power state tracking for accurate drain calculation (v1.6.x)
    uint32_t activeTimeMs;            // Time in active mode since last save (legacy/non-BLE active)
    uint32_t lightSleepTimeMs;        // Time in light sleep since last save
    uint32_t deepSleepTimeMs;         // Time in deep sleep since last save
    
    // BLE adaptive power mode tracking (v1.7.0)
    uint32_t bleLiveTimeMs;           // Time in LIVE_PERFORMANCE mode (95mA)
    uint32_t bleConfigTimeMs;         // Time in CONFIGURATION mode (60mA)
    uint32_t bleIdleTimeMs;           // Time in IDLE_CONNECTED mode (35mA)
    uint32_t lastModeChangeMs;        // When we last changed BLE power mode
};

extern BatteryState batteryState;

#endif