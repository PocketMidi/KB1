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
    int onsetTime;
    int offsetTime;
    InterpolationType onsetType;
    InterpolationType offsetType;
};

// Struct for Touch settings
struct TouchSettings {
    int ccNumber;
    int minCCValue;
    int maxCCValue;
    TouchFunctionMode functionMode;
};

// Struct for Scale settings
struct ScaleSettings {
    ScaleType scaleType;
    int rootNote;
};

#endif
