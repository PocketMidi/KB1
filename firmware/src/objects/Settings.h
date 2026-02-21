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
};

// Struct for Scale settings
struct ScaleSettings {
    ScaleType scaleType;
    int rootNote;
    int keyMapping; // 0 = Natural, 1 = Compact
};

// Struct for Chord/Keyboard settings
struct ChordSettings {
    PlayMode playMode;          // SCALE or CHORD
    ChordType chordType;        // MAJOR, MINOR, etc.
    bool strumEnabled;          // false = chord (all notes together), true = strum (cascaded)
    int velocitySpread;         // 0-100 (percentage) - velocity variation for chord notes
    int strumSpeed;             // 5-100 (milliseconds) - delay between notes in strum mode
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
