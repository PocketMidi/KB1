#ifndef STRUM_PATTERNS_H
#define STRUM_PATTERNS_H

#include <Arduino.h>

// Maximum number of notes in a custom ARP pattern (sent via BLE from PatternBuilder)
#define MAX_PATTERN_LENGTH 16

// Custom pattern storage (pattern index 7) - built by PatternBuilder, sent via BLE
// Patterns 1-6 are build modes (up/down/updown/exclusive/inclusive/user) - handled in web app
// Pattern 0 = use chord type intervals (no custom pattern)
// Pattern 7 = use this custom interval array
struct CustomPattern {
    int8_t intervals[MAX_PATTERN_LENGTH];
    uint8_t length;
};

// Global custom pattern storage
extern CustomPattern customPattern;

// Set custom pattern intervals (sent from PatternBuilder via BLE)
inline void setCustomPattern(const int8_t* intervals, uint8_t length) {
    if (length > MAX_PATTERN_LENGTH) {
        length = MAX_PATTERN_LENGTH;
    }
    customPattern.length = length;
    for (uint8_t i = 0; i < length; i++) {
        customPattern.intervals[i] = intervals[i];
    }
    // Clear remaining slots
    for (uint8_t i = length; i < MAX_PATTERN_LENGTH; i++) {
        customPattern.intervals[i] = 0;
    }
}

// Get custom pattern intervals and length
inline void getCustomPattern(const int8_t*& intervals, uint8_t& length) {
    intervals = customPattern.intervals;
    length = customPattern.length;
}

#endif // STRUM_PATTERNS_H
