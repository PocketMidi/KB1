#ifndef SCALE_MANAGER_H
#define SCALE_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <objects/Settings.h>

struct ScaleDefinition {
    ScaleType type;
    const char* name;
    std::vector<int> intervals;
};

class ScaleManager {
public:
    static const ScaleDefinition _allScales[];
    ScaleManager(ScaleSettings& settings);

    void setScale(ScaleType type);
    void setRootNote(int root);
    int quantizeNote(int note) const;
    ScaleType getScaleType() const { return _settings.scaleType; }
    int getRootNote() const { return _settings.rootNote; }
    void enableWhiteKeyQuantization(bool enable) { _whiteKeysQuantize = enable; }

private:
    bool _whiteKeysQuantize; // Default true
    ScaleSettings& _settings;
    std::vector<int> _currentScaleIntervals;

    void updateScaleIntervals();

    static int getClosestNoteInScale(int note, const std::vector<int>& intervals, int rootNote);

    static bool isWhiteKey(int note);
    int getClosestWhiteKey(int note) const;
};

#endif