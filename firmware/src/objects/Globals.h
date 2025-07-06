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
    }

    // Default constructor needed for array initialization
    key() : midi(0), pin(0), state(false), prevState(false), mcp(nullptr) {
        name[0] = '\0';
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
    LOCRIAN
};

extern Preferences preferences;

#define MAX_KEYS 19


extern MIDI_NAMESPACE::MidiInterface<MIDI_NAMESPACE::SerialMIDI<HardwareSerial>> MIDI;

// Helper macro for serial printing, depends on SERIAL_PRINT_ENABLED from Constants.h
#ifdef SERIAL_PRINT_ENABLED
#define SERIAL_BEGIN() Serial.begin(115200);
#define SERIAL_PRINT(msg) Serial.print(msg);
#define SERIAL_PRINTLN(msg) Serial.println(msg);
#else
#define SERIAL_BEGIN()
#define SERIAL_PRINT(msg)
#define SERIAL_PRINTLN(msg)
#endif

#endif