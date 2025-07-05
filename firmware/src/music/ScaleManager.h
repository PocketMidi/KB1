#ifndef SCALE_MANAGER_H
#define SCALE_MANAGER_H

#include <Arduino.h>
#include <vector>

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

struct ScaleDefinition {
    ScaleType type;
    const char* name;
    std::vector<int> intervals;
};

class ScaleManager {
public:
    static const ScaleDefinition _allScales[];
    ScaleManager();

    void setScale(ScaleType type);
    void setRootNote(int root);
    int quantizeNote(int note) const;
    ScaleType getScaleType() const { return _currentScaleType; }
    int getRootNote() const { return _rootNote; }
    void enableWhiteKeyQuantization(bool enable) { _whiteKeysQuantize = enable; }

private:
    bool _whiteKeysQuantize; // Default true
    ScaleType _currentScaleType; // Default: ScaleType::CHROMATIC
    int _rootNote; // Default: C (MIDI Note Number: 60)
    std::vector<int> _currentScaleIntervals;

    void updateScaleIntervals();
    int getClosestNoteInScale(int note, const std::vector<int>& intervals, int rootNote) const;
    bool isWhiteKey(int note) const;
    int getClosestWhiteKey(int note) const;
};

#endif