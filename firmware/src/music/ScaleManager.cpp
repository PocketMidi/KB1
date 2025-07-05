#include <music/ScaleManager.h>
#include <algorithm>
#include <objects/Globals.h>

const ScaleDefinition ScaleManager::_allScales[] = {
    {ScaleType::CHROMATIC, "Chromatic", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}},
    {ScaleType::MAJOR, "Major", {0, 2, 4, 5, 7, 9, 11}},
    {ScaleType::MINOR, "Minor", {0, 2, 3, 5, 7, 8, 10}},
    {ScaleType::HARMONIC_MINOR, "Harmonic Minor", {0, 2, 3, 5, 7, 8, 11}},
    {ScaleType::MELODIC_MINOR_ASC, "Melodic Minor Ascending", {0, 2, 3, 5, 7, 9, 11}},
    {ScaleType::PENTATONIC_MAJOR, "Pentatonic Major", {0, 2, 4, 7, 9}},
    {ScaleType::PENTATONIC_MINOR, "Pentatonic Minor", {0, 3, 5, 7, 10}},
    {ScaleType::BLUES, "Blues", {0, 3, 5, 6, 7, 10}},
    {ScaleType::DORIAN, "Dorian", {0, 2, 3, 5, 7, 9, 10}},
    {ScaleType::PHRYGIAN, "Phrygian", {0, 1, 3, 5, 7, 8, 10}},
    {ScaleType::LYDIAN, "Lydian", {0, 2, 4, 6, 7, 9, 11}},
    {ScaleType::MIXOLYDIAN, "Mixolydian", {0, 2, 4, 5, 7, 9, 10}},
    {ScaleType::LOCRIAN, "Locrian", {0, 1, 3, 4, 6, 8, 10}}
};

ScaleManager::ScaleManager() :
    _currentScaleType(ScaleType::CHROMATIC),
    _rootNote(60),
    _whiteKeysQuantize(true) {
    updateScaleIntervals();
}

void ScaleManager::setScale(ScaleType type) {
    if (_currentScaleType != type) {
        _currentScaleType = type;
        updateScaleIntervals();
        SERIAL_PRINT("Scale set to: ");
        for (const auto& scale : _allScales) {
            if (scale.type == type) {
                SERIAL_PRINTLN(scale.name);
                break;
            }
        }
    }
}

void ScaleManager::setRootNote(int root) {
    if (_rootNote != root) {
        _rootNote = root;
        SERIAL_PRINT("Root note set to: ");
        SERIAL_PRINTLN(_rootNote);
    }
}

int ScaleManager::quantizeNote(int note) const {
    if (_currentScaleType == ScaleType::CHROMATIC) {
        return note;
    }

    if (_whiteKeysQuantize) {
        static const int whiteKeyOffsets[7] = {0, 2, 4, 5, 7, 9, 11};
        int noteMod = note % 12;

        // Find the white key index
        int whiteKeyIndex = -1;
        for (int i = 0; i < 7; ++i) {
            if (noteMod == whiteKeyOffsets[i]) {
                whiteKeyIndex = i;
                break;
            }
        }

        if (whiteKeyIndex == -1) {
            // Not a white key — shouldn't happen if using white keys only
            return getClosestNoteInScale(note, _currentScaleIntervals, _rootNote);
        }

        // Map white key index to scale degree
        int octaveOffset = (note / 12) - (_rootNote / 12);
        int scaleDegree = whiteKeyIndex % _currentScaleIntervals.size();

        int targetNote = _rootNote + _currentScaleIntervals[scaleDegree] + (octaveOffset * 12);
        return targetNote;
    }

    return getClosestNoteInScale(note, _currentScaleIntervals, _rootNote);
}

void ScaleManager::updateScaleIntervals() {
    _currentScaleIntervals.clear();
    for (const auto& scale : _allScales) {
        if (scale.type == _currentScaleType) {
            _currentScaleIntervals = scale.intervals;
            break;
        }
    }
}

int ScaleManager::getClosestNoteInScale(int note, const std::vector<int>& intervals, int rootNote) const {
    int bestNote = -1;
    int minDistance = 1000;

    // Search across multiple octaves to find best match
    for (int octaveOffset = -2; octaveOffset <= 2; ++octaveOffset) {
        for (int interval : intervals) {
            int candidate = rootNote + interval + (octaveOffset * 12);

            if (candidate < 0 || candidate > 127)
                continue;

            if (_whiteKeysQuantize && !isWhiteKey(candidate))
                continue;

            int distance = abs(note - candidate);
            if (distance < minDistance || (distance == minDistance && candidate < bestNote)) {
                minDistance = distance;
                bestNote = candidate;
            }
        }
    }

    return bestNote != -1 ? bestNote : note; // fallback
}

bool ScaleManager::isWhiteKey(int note) const {
    const bool whiteKeys[12] = {
        // C, C#, D, D#, E, F, F#, G, G#, A, A#, B
        true, false, true, false, true, true, false, true, false, true, false, true
    };
    return whiteKeys[note % 12];
}

int ScaleManager::getClosestWhiteKey(int note) const {
    int up = note;
    int down = note;

    while (up <= 127 || down >= 0) {
        if (up <= 127 && isWhiteKey(up)) return up;
        if (down >= 0 && isWhiteKey(down)) return down;
        ++up;
        --down;
    }

    return note; // fallback
}
