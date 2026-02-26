#include <music/ScaleManager.h>
#include <objects/Globals.h>

const ScaleDefinition ScaleManager::_allScales[] = {
    {ScaleType::CHROMATIC, "Chromatic", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}},
    {ScaleType::MAJOR, "Major", {0, 2, 4, 5, 7, 9, 11}},
    {ScaleType::MINOR, "Minor", {0, 2, 3, 5, 7, 8, 10}},
    {ScaleType::HARMONIC_MINOR, "Harmonic Minor", {0, 2, 3, 5, 7, 8, 11}},
    {ScaleType::MELODIC_MINOR_ASC, "Melodic Minor", {0, 2, 3, 5, 7, 9, 11}},
    {ScaleType::PENTATONIC_MAJOR, "Pentatonic Major", {0, 2, 4, 7, 9}},
    {ScaleType::PENTATONIC_MINOR, "Pentatonic Minor", {0, 3, 5, 7, 10}},
    {ScaleType::BLUES, "Blues Minor", {0, 3, 5, 6, 7, 10}},
    {ScaleType::DORIAN, "Dorian", {0, 2, 3, 5, 7, 9, 10}},
    {ScaleType::PHRYGIAN, "Phrygian", {0, 1, 3, 5, 7, 8, 10}},
    {ScaleType::LYDIAN, "Lydian", {0, 2, 4, 6, 7, 9, 11}},
    {ScaleType::MIXOLYDIAN, "Mixolydian", {0, 2, 4, 5, 7, 9, 10}},
    {ScaleType::LOCRIAN, "Locrian", {0, 1, 3, 5, 6, 8, 10}},
    {ScaleType::PHRYGIAN_DOMINANT, "Phrygian Dominant", {0, 1, 4, 5, 7, 8, 10}},
    {ScaleType::WHOLE_TONE, "Whole Tone", {0, 2, 4, 6, 8, 10}},
    {ScaleType::DIMINISHED, "Diminished", {0, 2, 3, 5, 6, 8, 9, 11}},
    {ScaleType::BLUES_MAJOR, "Blues Major", {0, 2, 3, 4, 7, 9}},
    {ScaleType::HIRAJOSHI, "Hirajoshi", {0, 2, 3, 7, 8}},
    {ScaleType::IN_SEN, "In Sen", {0, 1, 5, 7, 10}},
    {ScaleType::DOUBLE_HARMONIC, "Double Harmonic", {0, 1, 4, 5, 7, 8, 11}},
    {ScaleType::SUPER_LOCRIAN, "Super Locrian", {0, 1, 3, 4, 6, 8, 10}}
};

ScaleManager::ScaleManager(ScaleSettings& settings) :
    _settings(settings),
    _whiteKeysQuantize(true) {
    updateScaleIntervals();
}

void ScaleManager::setScale(ScaleType type) {
    if (_settings.scaleType != type) {
        _settings.scaleType = type;
    }
    updateScaleIntervals();
}

void ScaleManager::setRootNote(int root) {
    if (_settings.rootNote != root) {
        _settings.rootNote = root;
    }
}

int ScaleManager::quantizeNote(int note) const {
    if (_settings.scaleType == ScaleType::CHROMATIC) {
        return note;
    }

    if (_whiteKeysQuantize) {
        int closestWhiteKey = getClosestWhiteKey(note);
        return getClosestNoteInScale(closestWhiteKey, _currentScaleIntervals, _settings.rootNote);
    }

    return getClosestNoteInScale(note, _currentScaleIntervals, _settings.rootNote);
}

int ScaleManager::getCompactModeNote(int keyIndex) const {
    // In compact mode, map white key indices to sequential scale degrees
    if (_currentScaleIntervals.empty()) {
        return _settings.rootNote + keyIndex; // Fallback to chromatic
    }
    
    // Calculate which scale degree and octave
    int scaleSize = _currentScaleIntervals.size();
    int scaleDegree = keyIndex % scaleSize;
    int octaveOffset = keyIndex / scaleSize;
    
    // Return root + interval + octave offset
    return _settings.rootNote + _currentScaleIntervals[scaleDegree] + (octaveOffset * 12);
}

void ScaleManager::updateScaleIntervals() {
    _currentScaleIntervals.clear();
    SERIAL_PRINTLN(static_cast<int>(_settings.scaleType));
    for (const auto& scale : _allScales) {
        if (scale.type == _settings.scaleType) {
            _currentScaleIntervals = scale.intervals;
            break;
        }
    }
}

int ScaleManager::getClosestNoteInScale(int note, const std::vector<int>& intervals, int rootNote) {
    int bestNote = -1;
    int minDistance = 1000;

    // Search across multiple octaves to find best match
    for (int octaveOffset = -2; octaveOffset <= 2; ++octaveOffset) {
        for (int interval : intervals) {
            int candidate = rootNote + interval + (octaveOffset * 12);

            if (candidate < 0 || candidate > 127)
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

bool ScaleManager::isWhiteKey(int note) {
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


