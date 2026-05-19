#ifndef SETTINGS_H
#define SETTINGS_H

#include <objects/Globals.h>

// Struct for Lever settings
struct LeverSettings {
    int ccNumber;
    int minCCValue;
    int maxCCValue;
    int stepSize;
    LeverFunctionMode functionMode;
    ValueMode valueMode;
    unsigned long onsetTime;
    unsigned long offsetTime;
    InterpolationType onsetType;
    InterpolationType offsetType;
};

// Struct for LeverPush settings
struct LeverPushSettings {
    int ccNumber;
    int minCCValue;
    int maxCCValue;
    LeverPushFunctionMode functionMode;
    unsigned long onsetTime;
    unsigned long offsetTime;
    InterpolationType onsetType;
    InterpolationType offsetType;
};

// Struct for Touch settings
struct TouchSettings {
    int ccNumber;
    int minCCValue;
    int maxCCValue;
    TouchFunctionMode functionMode;
    int threshold; // touch sensor threshold (higher = less sensitive)
    unsigned long offsetTime; // For pattern selector direction: 0=FWD, >0=REV
};

// Struct for Scale settings
struct ScaleSettings {
    ScaleType scaleType;
    int rootNote;
    int keyMapping; // 0 = Natural, 1 = Compact
};

// Struct for Chord/Keyboard settings
struct ChordSettings {
    PlayMode playMode;          // SCALE, CHORD, or ARP
    ChordType chordType;        // MAJOR, MINOR, etc.
    bool strumEnabled;          // false = chord (all notes together), true = strum (cascaded)
    int velocitySpread;         // 10-100 (percentage) - velocity variation for chord notes
    int strumSpeed;             // 4-360 (milliseconds) - delay between notes in strum mode / arp speed
    int strumPattern;           // 0-7 - pattern index (0 = use chord type, 1-7 = interval patterns) / arp build mode (0-5)
    int strumSwing;             // 0-50 (percentage) - swing amount for arp timing (0 = straight 50% UI, 50 = max swing 100% UI)
    int gateValue;              // 10-100 (percentage) - CHORD swing amount (10 = straight 50/50, 100 = swung ~66/33)
    int voicing;                // 1-3 (octave range: 1x, 2x, 3x)
    int arpUserMode;            // 0 = CHORD mode (chord-based arp), 1 = USER mode (user-defined sequence)
    int arpLatchMode;           // 0 = MOMENTARY (stop on any key release), 1 = LATCHED (stop on last key release)
};

// Struct for System/Power settings
struct SystemSettings {
    int lightSleepTimeout;  // in seconds
    int deepSleepTimeout;   // in seconds
    int bleTimeout;         // in seconds
    int idleConfirmTimeout; // in seconds
};

// ============================================
// Preset System Structures
// ============================================

#define MAX_PRESET_SLOTS 8
#define PRESET_NAME_MAX_LEN 32

/**
 * Preset metadata stored in NVS
 * Allows listing presets without loading full data
 */
struct PresetMetadata {
    char name[PRESET_NAME_MAX_LEN];
    uint32_t timestamp;      // Unix timestamp
    uint8_t isValid;         // 0 = empty slot, 1 = valid preset
    uint8_t padding[3];      // Align to 4-byte boundary
} __attribute__((packed));

/**
 * Complete preset data bundle
 * All device settings in one atomic structure
 */
struct PresetData {
    LeverSettings lever1;
    LeverPushSettings leverPush1;
    LeverSettings lever2;
    LeverPushSettings leverPush2;
    TouchSettings touch;
    ScaleSettings scale;
    ChordSettings chord;
    SystemSettings system;
} __attribute__((packed));

#endif
