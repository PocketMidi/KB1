#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include <Preferences.h>
#include <MIDI.h>
#include <BLECharacteristic.h>
#include <objects/Constants.h>
#include <BLEServer.h>

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

enum class InterpolationType {
    LINEAR,
    ACCELERATING,
    DECELERATING
};

extern Preferences preferences;

#define MAX_KEYS 19
extern key keys[MAX_KEYS];

extern MIDI_NAMESPACE::MidiInterface<MIDI_NAMESPACE::SerialMIDI<HardwareSerial>> MIDI;

// BLE Globals
extern BLEServer* pServer;
extern bool deviceConnected;
extern BLECharacteristic* pSWD1LRCCCharacteristic;
extern BLECharacteristic* pSWD1CenterCCCharacteristic;
extern BLECharacteristic* pSWD2LRCCCharacteristic;
extern BLECharacteristic* pSWD2CenterCCCharacteristic;
extern BLECharacteristic* pMidiCcCharacteristic;
extern BLECharacteristic* pScaleTypeCharacteristic;
extern BLECharacteristic* pRootNoteCharacteristic;

extern int ccNumberSWD1LeftRight;
extern int ccNumberSWD1Center;
extern int ccNumberSWD2LeftRight;
extern int ccNumberSWD2Center;

extern volatile int currentOctave;
extern volatile int currentVelocity;
extern bool sustain;
extern int minVelocity;

extern volatile bool shiftModeActive;
extern int threshold;
extern bool touch1detected;

extern volatile bool prevS3State;
extern volatile bool prevS4State;

extern bool prevSWD1LeftState;
extern bool prevSWD1CenterState;
extern bool prevSWD1RightState;

extern volatile bool prevSWD2LeftState;
extern volatile bool prevSWD2CenterState;
extern volatile bool prevSWD2RightState;
extern bool isSwd2LeftPressed;
extern bool isSwd2RightPressed;
extern bool isSwd2CenterPressed;
extern const int swd2Interval;

extern const int bluePin;
extern int blueBrightness;
extern unsigned long previousTime;
extern const unsigned long ledOffDelay;

extern TimerHandle_t upTimer;
extern TimerHandle_t downTimer;

extern bool isNoteOn[128];

extern const int pinkPin;


// Helper macro for serial printing, depends on SERIAL_PRINT_ENABLED from Constants.h
#ifdef SERIAL_PRINT_ENABLED
#define SERIAL_BEGIN() Serial.begin(115200);
#define SERIAL_PRINT(msg) Serial.print(msg)
#define SERIAL_PRINTLN(msg) Serial.println(msg)
#else
#define SERIAL_PRINT(msg)
#define SERIAL_PRINTLN(msg)
#endif

#endif