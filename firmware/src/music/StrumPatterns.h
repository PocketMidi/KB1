#ifndef STRUM_PATTERNS_H
#define STRUM_PATTERNS_H

#include <Arduino.h>

// Maximum number of notes in a pattern
#define MAX_PATTERN_LENGTH 16

// Pattern structure
struct StrumPattern {
    const char* name;
    int8_t intervals[MAX_PATTERN_LENGTH];  // Semitones from root note
    uint8_t length;                        // Number of notes in pattern
};

// Built-in strum patterns
// Pattern 0: Use chord type (null intervals = use current chord voicing)
// Patterns 1-7: Fixed interval patterns

const StrumPattern STRUM_PATTERNS[] = {
    // Pattern 0: Chord (uses chord type setting)
    {
        "Chord",
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        0  // Length 0 indicates use chord type
    },
    
    // Pattern 1: Major Triad Up
    {
        "Triad Up",
        {0, 4, 7, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        4
    },
    
    // Pattern 2: Major Triad Down
    {
        "Triad Down",
        {12, 7, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        4
    },
    
    // Pattern 3: Pentatonic Run
    {
        "Pentatonic",
        {0, 2, 4, 7, 9, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        6
    },
    
    // Pattern 4: Octave Jump
    {
        "Octave Jump",
        {0, 12, 7, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        5
    },
    
    // Pattern 5: Major 7th Arpeggio
    {
        "Major 7th",
        {0, 4, 7, 11, 12, 11, 7, 4, 0, 0, 0, 0, 0, 0, 0, 0},
        8
    },
    
    // Pattern 6: Blues Run
    {
        "Blues Run",
        {0, 3, 5, 6, 7, 10, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        7
    },
    
    // Pattern 7: Custom (can be modified via BLE)
    {
        "Custom",
        {0, 2, 4, 5, 7, 9, 11, 12, 0, 0, 0, 0, 0, 0, 0, 0},
        8
    }
};

const int NUM_STRUM_PATTERNS = 8;

// Mutable custom pattern (pattern 7) - can be modified via BLE
struct CustomPattern {
    int8_t intervals[MAX_PATTERN_LENGTH];
    uint8_t length;
};

// Global custom pattern storage
extern CustomPattern customPattern;

// Get pattern by index (with bounds checking)
inline const StrumPattern* getStrumPattern(int index) {
    if (index < 0 || index >= NUM_STRUM_PATTERNS) {
        return &STRUM_PATTERNS[0]; // Default to chord pattern
    }
    return &STRUM_PATTERNS[index];
}

// Get pattern note at index (applying root note)
inline int getPatternNote(const StrumPattern* pattern, int noteIndex, int rootNote) {
    if (!pattern || noteIndex >= pattern->length) {
        return rootNote; // Default to root
    }
    return rootNote + pattern->intervals[noteIndex];
}

// Set custom pattern intervals (for pattern 7)
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
