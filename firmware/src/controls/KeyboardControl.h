#ifndef KEYBOARD_CONTROL_H
#define KEYBOARD_CONTROL_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include <MIDI.h>
#include <objects/Globals.h>
#include <led/LEDController.h>
#include <music/ScaleManager.h>
#include <music/StrumPatterns.h>

template<typename MidiTransport, typename OctaveControlType>
class KeyboardControl {
public:
    KeyboardControl(MidiTransport& midi, OctaveControlType& octaveCtrl, ScaleManager& scaleManager, ChordSettings& chordSettings)
        : _midi(midi),
          _octaveControl(octaveCtrl),
          _scaleManager(scaleManager),
          _chordSettings(chordSettings),
          _currentVelocity(85),
          _minVelocity(15),
          _arpActive(false),
          _arpRootNote(0),
          _arpPattern(nullptr),
          _arpPatternLength(0),
          _arpCurrentIndex(0),
          _arpLastNoteTime(0),
          _arpCurrentNote(-1),
          _strumActive(false),
          _activeStrumKey(0),
          _strumInProgress(false),
          _strumCount(0),
          _strumCurrentIndex(0),
          _strumLastNoteTime(0)
    {
        memset(_isNoteOn, false, sizeof(_isNoteOn));
        memset(_strumNotes, 0, sizeof(_strumNotes));
        memset(_strumVelocities, 0, sizeof(_strumVelocities));
        memset(_activeChordNotes, 0, sizeof(_activeChordNotes));
        memset(_activeChordCount, 0, sizeof(_activeChordCount));

        // Initialize keys array
        _keys[0] = {59, 4, true, true, &mcp_U1, "SW1 (B)"};
        _keys[1] = {60, 14, true, true, &mcp_U1, "SW2 (C)"};
        _keys[2] = {61, 1, true, true, &mcp_U1, "SB1 (C#)"};
        _keys[3] = {62, 13, true, true, &mcp_U1, "SW3 (D)"};
        _keys[4] = {63, 2, true, true, &mcp_U1, "SB2 (D#)"};
        _keys[5] = {64, 12, true, true, &mcp_U1, "SW4 (E)"};
        _keys[6] = {65, 11, true, true, &mcp_U1, "SW5 (F)"};
        _keys[7] = {66, 0, true, true, &mcp_U1, "SB3 (F#)"};
        _keys[8] = {67, 10, true, true, &mcp_U1, "SW6 (G)"};
        _keys[9] = {68, 3, true, true, &mcp_U1, "SB4 (G#)"};
        _keys[10] = {69, 9, true, true, &mcp_U1, "SW7 (A)"};
        _keys[11] = {70, 14, true, true, &mcp_U2, "SB5 (A#)"};
        _keys[12] = {71, 8, true, true, &mcp_U1, "SW8 (B)"};
        _keys[13] = {72, 11, true, true, &mcp_U2, "SW9 (C)"};
        _keys[14] = {73, 13, true, true, &mcp_U2, "SB6 (C#)"};
        _keys[15] = {74, 10, true, true, &mcp_U2, "SW10 (D)"};
        _keys[16] = {75, 12, true, true, &mcp_U2, "SB7 (D#)"};
        _keys[17] = {76, 9, true, true, &mcp_U2, "SW11 (E)"};
        _keys[18] = {77, 8, true, true, &mcp_U2, "SW12 (F)"};
    }

    static constexpr unsigned long KEY_DEBOUNCE_MS = 10; // ms
    static constexpr int MAX_CHORD_NOTES = 16;  // Support 3x voicing (5 notes × 3 octaves = 15)

    void begin() {
        // Configure all pins as INPUT_PULLUP first
        for (auto & key : _keys) {
            key.mcp->pinMode(key.pin, INPUT_PULLUP);
        }
        
        // Then do ONE bulk read to initialize states (2 I2C transactions instead of 19)
        GPIOCache initialCache;
        initialCache.u1_pins = mcp_U1.readGPIOAB();
        initialCache.u2_pins = mcp_U2.readGPIOAB();
        initialCache.timestamp = micros();
        
        // Set initial states from bulk read
        for (auto & key : _keys) {
            bool raw;
            if (key.mcp == &mcp_U1) {
                raw = initialCache.isU1PinLow(key.pin);
            } else {  // key.mcp == &mcp_U2
                raw = initialCache.isU2PinLow(key.pin);
            }
            key.lastReading = raw;
            key.debouncedState = raw;
            key.state = raw;
            key.prevState = raw;
            key.lastDebounceTime = millis();
        }
        
        resetAllKeys();
    }

    void playMidiNote(const byte note, int keyIndex = -1) {
        constexpr byte channel = 1;
        
        // Check play mode
        if (_chordSettings.playMode == PlayMode::SCALE) {
            // Original scale mode behavior
            int quantizedNote;
            
            // Check if compact mode and we have a valid key index
            if (_scaleManager.getKeyMapping() == 1 && keyIndex >= 0) {
                // Compact mode: map white keys to sequential scale degrees
                int whiteKeyPosition = getWhiteKeyPosition(keyIndex);
                if (whiteKeyPosition >= 0) {
                    // This is a white key, use compact mapping (root starts at leftmost white key)
                    quantizedNote = _scaleManager.getCompactModeNote(whiteKeyPosition) + (_octaveControl.getOctave() * 12);
                } else {
                    // Black key in compact mode, use quantization
                    quantizedNote = _scaleManager.quantizeNote(note + _octaveControl.getOctave() * 12);
                }
            } else {
                // Natural mode: quantize to nearest scale note
                quantizedNote = _scaleManager.quantizeNote(note + _octaveControl.getOctave() * 12);
            }
            
            _midi.sendNoteOn(quantizedNote, _currentVelocity, channel);
            char buf[16];
            snprintf(buf, sizeof(buf), "N%dv%d", quantizedNote, _currentVelocity);
            SERIAL_PRINTLN(buf);
            _isNoteOn[note] = true;
        } else {
            // Chord mode - use chromatic notes (no scale quantization)
            int rootNote = note + (_octaveControl.getOctave() * 12);
            
            // Get chord intervals (expanded based on voicing setting)
            const int8_t* intervals;
            int intervalCount;
            getExpandedIntervals(intervals, intervalCount);
            
            // Check if this is advanced strum mode (pattern > 0)
            // Advanced mode works independently of chord/strum toggle
            bool isAdvancedStrum = _chordSettings.strumPattern > 0;
            
            if (isAdvancedStrum) {
                // ADVANCED STRUM: Looping arpeggiator (monophonic)
                
                // If arpeggiator is already running, smoothly transition to new root note
                // Keep current index to continue pattern seamlessly
                bool wasAlreadyRunning = _arpActive;
                int previousIndex = _arpCurrentIndex;
                
                // Stop currently playing note (if any)
                if (_arpCurrentNote >= 0) {
                    _midi.sendNoteOff(_arpCurrentNote, 0, 1);
                    SERIAL_PRINT("Arp Note Off (interrupted): ");
                    SERIAL_PRINTLN(_arpCurrentNote);
                }
                
                // Update arpeggiator state
                _arpActive = true;
                _arpRootNote = rootNote;
                _arpPattern = intervals;
                _arpPatternLength = intervalCount;
                
                // Keep current index if already running, otherwise start from beginning
                if (!wasAlreadyRunning) {
                    _arpCurrentIndex = 0;
                    _arpLastNoteTime = millis();
                    char buf[16];
                    snprintf(buf, sizeof(buf), "Arp:R%dP%d", rootNote, _chordSettings.strumPattern);
                    SERIAL_PRINTLN(buf);
                } else {
                    // Continue from current position, adjust if pattern length changed
                    if (_arpCurrentIndex >= intervalCount) {
                        _arpCurrentIndex = 0;
                    }
                    char buf[20];
                    snprintf(buf, sizeof(buf), "ArpRoot:%d@%d", rootNote, _arpCurrentIndex);
                    SERIAL_PRINTLN(buf);
                }
                
                _arpCurrentNote = -1;
                _isNoteOn[note] = true;
                
            } else {
                // BASIC STRUM or CHORD: Play once
                
                if (_chordSettings.strumEnabled) {
                    // BASIC STRUM MODE: Non-blocking monophonic strum
                    // Stop previous strum if one is in progress (immediate interrupt)
                    if (_strumInProgress) {
                        stopStrum();
                    }
                    
                    // Mark this strum as active
                    _strumActive = true;
                    _activeStrumKey = note;
                    _activeChordCount[note] = intervalCount;
                    
                    // Prepare strum notes (non-blocking - will play over time)
                    // Determine direction based on speed sign (negative = reverse)
                    bool reverse = _chordSettings.strumSpeed < 0;
                    
                    // Build list of notes and velocities to play
                    _strumCount = intervalCount;
                    for (int i = 0; i < intervalCount; i++) {
                        // Get note index (reverse order if speed is negative)
                        int noteIndex = reverse ? (intervalCount - 1 - i) : i;
                        int chordNote = rootNote + intervals[noteIndex];
                        int velocity = calculateChordVelocity(noteIndex, intervalCount);
                        
                        _strumNotes[i] = chordNote;
                        _strumVelocities[i] = velocity;
                        _activeChordNotes[note][noteIndex] = chordNote;
                    }
                    
                    // Start strum playback (first note plays immediately)
                    _strumInProgress = true;
                    _strumCurrentIndex = 0;
                    _strumLastNoteTime = millis();
                    
                    // Play first note immediately
                    _midi.sendNoteOn(_strumNotes[0], _strumVelocities[0], channel);
                    char buf[24];
                    snprintf(buf, sizeof(buf), "S0:N%dv%d", _strumNotes[0], _strumVelocities[0]);
                    SERIAL_PRINTLN(buf);
                    
                    _strumCurrentIndex = 1;  // Next note to play
                } else {
                    // CHORD MODE: Monophonic (like strum mode)
                    // Stop previous chord if one is active
                    if (_strumActive && _activeStrumKey != note) {
                        // Kill all notes from previous chord
                        for (int i = 0; i < _activeChordCount[_activeStrumKey]; i++) {
                            _midi.sendNoteOff(_activeChordNotes[_activeStrumKey][i], 0, 1);
                        }
                        _activeChordCount[_activeStrumKey] = 0;
                        _isNoteOn[_activeStrumKey] = false;
                    }
                    
                    // Mark this chord as active (reuse strum tracking for both modes)
                    _strumActive = true;
                    _activeStrumKey = note;
                    
                    // Store chord notes for this key
                    _activeChordCount[note] = intervalCount;
                    
                    // Chord mode: send all notes immediately
                    for (int i = 0; i < intervalCount; i++) {
                        int chordNote = rootNote + intervals[i];
                        int velocity = calculateChordVelocity(i, intervalCount);
                        _activeChordNotes[note][i] = chordNote;
                        
                        _midi.sendNoteOn(chordNote, velocity, channel);
                        SERIAL_PRINT("Chord Note ");
                        SERIAL_PRINT(i);
                        SERIAL_PRINT(" On: ");
                        SERIAL_PRINT(chordNote);
                        SERIAL_PRINT(", Velocity: ");
                        SERIAL_PRINTLN(velocity);
                    }
                }
                
                _isNoteOn[note] = true;
            }  // End of isAdvancedStrum else block
        }
    }

    void stopMidiNote(const byte note, int keyIndex = -1) {
        // If arpeggiator is active, only stop it when the last key is released
        if (_arpActive) {
            _isNoteOn[note] = false;
            
            // Check if any other keys are still held
            bool otherKeysHeld = false;
            for (int i = 0; i < 128; i++) {
                if (_isNoteOn[i]) {
                    otherKeysHeld = true;
                    break;
                }
            }
            
            // Only stop arpeggiator when the last key is released
            if (!otherKeysHeld) {
                stopArpeggiator();
            }
            return;
        }
        
        if (_isNoteOn[note])
        {
            constexpr byte channel = 1;
            
            if (_chordSettings.playMode == PlayMode::SCALE) {
                // Original scale mode behavior
                int quantizedNote;
                
                // Check if compact mode and we have a valid key index
                if (_scaleManager.getKeyMapping() == 1 && keyIndex >= 0) {
                    // Compact mode: map white keys to sequential scale degrees
                    int whiteKeyPosition = getWhiteKeyPosition(keyIndex);
                    if (whiteKeyPosition >= 0) {
                        // This is a white key, use compact mapping (root starts at leftmost white key)
                        quantizedNote = _scaleManager.getCompactModeNote(whiteKeyPosition) + (_octaveControl.getOctave() * 12);
                    } else {
                        // Black key in compact mode, use quantization
                        quantizedNote = _scaleManager.quantizeNote(note + _octaveControl.getOctave() * 12);
                    }
                } else {
                    // Natural mode: quantize to nearest scale note
                    quantizedNote = _scaleManager.quantizeNote(note + _octaveControl.getOctave() * 12);
                }
                
                _midi.sendNoteOff(quantizedNote, 0, channel);
                char buf[16];
                snprintf(buf, sizeof(buf), "N%d-", quantizedNote);
                SERIAL_PRINTLN(buf);
            } else {
                // Chord mode - use chromatic notes (no scale quantization)
                
                // If this is the active strum key and strum is in progress, stop cascade
                if (_strumActive && _activeStrumKey == note && _strumInProgress) {
                    stopStrum();
                }
                
                // Stop all chord notes
                for (int i = 0; i < _activeChordCount[note]; i++) {
                    int chordNote = _activeChordNotes[note][i];
                    _midi.sendNoteOff(chordNote, 0, channel);
                    char buf[16];
                    snprintf(buf, sizeof(buf), "C%d-", chordNote);
                    SERIAL_PRINTLN(buf);
                }
                _activeChordCount[note] = 0;
                
                // Clear strum active flag if this was the active strum key
                if (_strumActive && _activeStrumKey == note) {
                    _strumActive = false;
                }
            }
            
            _isNoteOn[note] = false;
        }
    }

    void updateVelocity(const int delta) {
        const int previousVelocity = _currentVelocity;
        _currentVelocity += delta;
        if (_currentVelocity < _minVelocity) {
            _currentVelocity = _minVelocity;
        } else if (_currentVelocity > 127) {
            _currentVelocity = 127;
        }

        if (_currentVelocity != previousVelocity) {
            if (delta > 0) {
                SERIAL_PRINT("Current Velocity Increased to: ");
            } else {
                SERIAL_PRINT("Current Velocity Decreased to: ");
            }
            SERIAL_PRINTLN(_currentVelocity);
        }
    }

    void setVelocity(const int value) {
        _currentVelocity = value;
        if (_velocityChangeHook) _velocityChangeHook(_currentVelocity);
    }

    int getVelocity() const { return _currentVelocity; }

    // Register a callback to be notified when velocity changes via setVelocity()
    void registerVelocityChangeHook(void (*hook)(int)) { _velocityChangeHook = hook; }

    void resetAllKeys() {
        for (int i = 0; i < 128; ++i) {
            _midi.sendNoteOff(i, 0, 1);
            if (_isNoteOn[i]) {
                _isNoteOn[i] = false;
            }
        }
    }

    // Stop arpeggiator and silence current note (clean cutoff)
    void stopArpeggiator() {
        if (!_arpActive) {
            return;
        }
        
        // Turn off current note if any
        if (_arpCurrentNote >= 0) {
            _midi.sendNoteOff(_arpCurrentNote, 0, 1);
            char buf[16];
            snprintf(buf, sizeof(buf), "Arp:Stop%d", _arpCurrentNote);
            SERIAL_PRINTLN(buf);
        }
        
        // Clear arp state
        _arpActive = false;
        _arpRootNote = 0;
        _arpPattern = nullptr;
        _arpPatternLength = 0;
        _arpCurrentIndex = 0;
        _arpCurrentNote = -1;
    }

    void updateKeyboardState(const GPIOCache& gpioCache) {
        // Update arpeggiator (for advanced strum looping)
        updateArpeggiator();
        
        // Update basic strum (non-blocking cascade)
        updateStrum();
        
        // Debounced key scanning (using cached GPIO states, zero I2C overhead)
        for (int i = 0; i < MAX_KEYS; ++i) {
            auto & key = _keys[i];
            
            // Extract pin state from cache based on which MCP this key is on
            bool raw;
            if (key.mcp == &mcp_U1) {
                raw = gpioCache.isU1PinLow(key.pin);
            } else {  // key.mcp == &mcp_U2
                raw = gpioCache.isU2PinLow(key.pin);
            };
            if (raw != key.lastReading) {
                key.lastDebounceTime = millis();
                key.lastReading = raw;
            }

            if ((millis() - key.lastDebounceTime) >= KEY_DEBOUNCE_MS) {
                if (raw != key.debouncedState) {
                    key.debouncedState = raw;
                    if (key.debouncedState) {
                        playMidiNote(key.midi, i);  // Pass key index
                    } else {
                        stopMidiNote(key.midi, i);  // Pass key index
                    }
                }
            }

            key.prevState = key.state;
            key.state = key.debouncedState;
        }
    }

    // Return true if any key is currently pressed (debounced state)
    bool anyKeyActive() const {
        for (const auto & key : _keys) {
            if (key.debouncedState) return true;
        }
        return false;
    }

    // Stop current strum in progress (for immediate interrupt)
    void stopStrum() {
        if (!_strumInProgress) {
            return;
        }
        
        // No note-offs needed - we let notes ring until key release
        // Just stop the cascade
        _strumInProgress = false;
        _strumCurrentIndex = 0;
        _strumCount = 0;
    }
    
    // Update basic strum state (non-blocking cascade playback)
    void updateStrum() {
        if (!_strumInProgress || _strumCurrentIndex >= _strumCount) {
            if (_strumInProgress) {
                _strumInProgress = false;  // Strum complete
            }
            return;
        }
        
        unsigned long now = millis();
        
        // Calculate delay for this note
        int baseDelay = abs(_chordSettings.strumSpeed);
        int noteDelay = baseDelay;
        
        // Apply swing to odd-indexed notes (every other note)
        if (_chordSettings.strumSwing > 0 && (_strumCurrentIndex % 2) == 1) {
            int swingDelay = (baseDelay * _chordSettings.strumSwing) / 200;
            noteDelay += swingDelay;
        }
        
        // Check if enough time has passed to play next note
        if (now - _strumLastNoteTime >= (unsigned long)noteDelay) {
            // Play current note
            _midi.sendNoteOn(_strumNotes[_strumCurrentIndex], _strumVelocities[_strumCurrentIndex], 1);
            char buf[24];
            snprintf(buf, sizeof(buf), "S%d:N%dv%d", _strumCurrentIndex, _strumNotes[_strumCurrentIndex], _strumVelocities[_strumCurrentIndex]);
            SERIAL_PRINTLN(buf);
            
            // Move to next note
            _strumCurrentIndex++;
            _strumLastNoteTime = now;
            
            // Check if strum is complete
            if (_strumCurrentIndex >= _strumCount) {
                _strumInProgress = false;
            }
        }
    }
    
    // Update arpeggiator state (called from updateKeyboardState)
    void updateArpeggiator() {
        if (!_arpActive || !_arpPattern || _arpPatternLength == 0) {
            return;
        }

        unsigned long now = millis();
        
        // Calculate note duration based on strum speed and swing (use absolute value for timing)
        int baseDelay = abs(_chordSettings.strumSpeed);
        int noteDelay = baseDelay;
        
        // Apply swing to odd-indexed notes
        if (_chordSettings.strumSwing > 0 && (_arpCurrentIndex % 2) == 1) {
            int swingDelay = (baseDelay * _chordSettings.strumSwing) / 200;
            noteDelay += swingDelay;
        }
        
        // Check if it's time for the next note
        if (now - _arpLastNoteTime < (unsigned long)noteDelay) {
            return;
        }
        
        // Turn off previous note
        if (_arpCurrentNote >= 0) {
            _midi.sendNoteOff(_arpCurrentNote, 0, 1);
            char buf[16];
            snprintf(buf, sizeof(buf), "Arp-%d", _arpCurrentNote);
            SERIAL_PRINTLN(buf);
        }
        
        // Play next note in pattern
        int interval = _arpPattern[_arpCurrentIndex];
        _arpCurrentNote = _arpRootNote + interval;
        int velocity = calculateChordVelocity(_arpCurrentIndex, _arpPatternLength);
        
        _midi.sendNoteOn(_arpCurrentNote, velocity, 1);
        char buf[24];
        snprintf(buf, sizeof(buf), "Arp%d:N%dv%d", _arpCurrentIndex, _arpCurrentNote, velocity);
        SERIAL_PRINTLN(buf);
        
        _arpLastNoteTime = now;
        _arpCurrentIndex++;
        
        // Loop seamlessly back to start
        if (_arpCurrentIndex >= _arpPatternLength) {
            _arpCurrentIndex = 0;
            SERIAL_PRINTLN("Arp:Loop");
        }
    }

private:
    // Map key index to white key position (-1 if not a white key)
    // White keys are: SW1-SW12 (indices: 0,1,3,5,6,8,10,12,13,15,17,18)
    int getWhiteKeyPosition(int keyIndex) const {
        const int whiteKeyIndices[] = {0, 1, 3, 5, 6, 8, 10, 12, 13, 15, 17, 18};
        for (int i = 0; i < 12; ++i) {
            if (whiteKeyIndices[i] == keyIndex) {
                return i;  // Return position among white keys (0-11)
            }
        }
        return -1;  // Not a white key
    }

    // Get chord intervals based on chord type or strum pattern
    void getChordIntervals(const int8_t*& intervals, int& count) const {
        // Check if a strum pattern is selected (pattern > 0)
        // Pattern 0 = use chord type intervals
        // Pattern 1-6 = predefined patterns
        // Pattern 7 = custom pattern
        if (_chordSettings.strumPattern > 0 && _chordSettings.strumPattern < 7) {
            // Use predefined strum pattern (1-6)
            const StrumPattern* pattern = getStrumPattern(_chordSettings.strumPattern);
            intervals = pattern->intervals;
            count = pattern->length;
            return;
        } else if (_chordSettings.strumPattern == 7) {
            // Use custom pattern  
            const int8_t* customIntervals;
            uint8_t customLength;
            getCustomPattern(customIntervals, customLength);
            intervals = customIntervals;
            count = customLength;
            return;
        }
        
        // Pattern 0 or invalid: use chord type intervals
        // Define chord interval arrays locally (semitones from root)
        static const int8_t CHORD_MAJOR[] = {0, 4, 7};
        static const int8_t CHORD_MINOR[] = {0, 3, 7};
        static const int8_t CHORD_DIMINISHED[] = {0, 3, 6};
        static const int8_t CHORD_AUGMENTED[] = {0, 4, 8};
        static const int8_t CHORD_SUS2[] = {0, 2, 7};
        static const int8_t CHORD_SUS4[] = {0, 5, 7};
        static const int8_t CHORD_POWER[] = {0, 7, 12};
        static const int8_t CHORD_MAJOR7[] = {0, 4, 7, 11};
        static const int8_t CHORD_MINOR7[] = {0, 3, 7, 10};
        static const int8_t CHORD_DOM7[] = {0, 4, 7, 10};
        static const int8_t CHORD_MAJOR_ADD9[] = {0, 4, 7, 14};
        static const int8_t CHORD_MINOR_ADD9[] = {0, 3, 7, 14};
        static const int8_t CHORD_MAJOR6[] = {0, 4, 7, 9};
        static const int8_t CHORD_MINOR6[] = {0, 3, 7, 9};
        static const int8_t CHORD_MAJOR9[] = {0, 4, 7, 11, 14};
        
        switch (_chordSettings.chordType) {
            case ChordType::MAJOR:
                intervals = CHORD_MAJOR;
                count = 3;
                break;
            case ChordType::MINOR:
                intervals = CHORD_MINOR;
                count = 3;
                break;
            case ChordType::DIMINISHED:
                intervals = CHORD_DIMINISHED;
                count = 3;
                break;
            case ChordType::AUGMENTED:
                intervals = CHORD_AUGMENTED;
                count = 3;
                break;
            case ChordType::SUS2:
                intervals = CHORD_SUS2;
                count = 3;
                break;
            case ChordType::SUS4:
                intervals = CHORD_SUS4;
                count = 3;
                break;
            case ChordType::POWER:
                intervals = CHORD_POWER;
                count = 3;
                break;
            case ChordType::MAJOR7:
                intervals = CHORD_MAJOR7;
                count = 4;
                break;
            case ChordType::MINOR7:
                intervals = CHORD_MINOR7;
                count = 4;
                break;
            case ChordType::DOM7:
                intervals = CHORD_DOM7;
                count = 4;
                break;
            case ChordType::MAJOR_ADD9:
                intervals = CHORD_MAJOR_ADD9;
                count = 4;
                break;
            case ChordType::MINOR_ADD9:
                intervals = CHORD_MINOR_ADD9;
                count = 4;
                break;
            case ChordType::MAJOR6:
                intervals = CHORD_MAJOR6;
                count = 4;
                break;
            case ChordType::MINOR6:
                intervals = CHORD_MINOR6;
                count = 4;
                break;
            case ChordType::MAJOR9:
                intervals = CHORD_MAJOR9;
                count = 5;
                break;
            default:
                intervals = CHORD_MAJOR;
                count = 3;
                break;
        }
    }

    // Expand chord intervals across octaves based on voicing setting
    void getExpandedIntervals(const int8_t*& intervals, int& count) const {
        // Get base chord intervals
        const int8_t* baseIntervals;
        int baseCount;
        getChordIntervals(baseIntervals, baseCount);
        
        // Get voicing multiplier (1-3)
        int voicing = constrain(_chordSettings.voicing, 1, 3);
        
        if (voicing == 1) {
            // No expansion needed
            intervals = baseIntervals;
            count = baseCount;
            return;
        }
        
        // Expand intervals across octaves
        static int8_t expandedIntervals[MAX_CHORD_NOTES];
        int expandedCount = 0;
        
        // Build root-forward voicing (repeat chord pattern at +12, +24 semitones)
        for (int octave = 0; octave < voicing; octave++) {
            int octaveOffset = octave * 12;
            for (int i = 0; i < baseCount; i++) {
                if (expandedCount < MAX_CHORD_NOTES) {
                    expandedIntervals[expandedCount++] = baseIntervals[i] + octaveOffset;
                }
            }
        }
        
        intervals = expandedIntervals;
        count = expandedCount;
    }

    // Calculate velocity for chord note based on velocity spread setting (exponential)
    int calculateChordVelocity(int noteIndex, int totalNotes) const {
        if (_chordSettings.velocitySpread == 0 || noteIndex == 0) {
            // No spread, or root note always gets full velocity
            return _currentVelocity;
        }
        
        // Exponential reduction: each subsequent note is a percentage of previous
        // spreadFactor determines how much each note reduces (0-100% maps to 0-50% reduction per step)
        float spreadFactor = _chordSettings.velocitySpread / 200.0f; // 0.0 to 0.5
        float reductionPerStep = 1.0f - spreadFactor; // Multiply by this for each step
        
        float velocity = _currentVelocity;
        for (int i = 0; i < noteIndex; i++) {
            velocity *= reductionPerStep;
        }
        
        int finalVelocity = (int)velocity;
        
        // Ensure velocity stays within valid range
        if (finalVelocity < _minVelocity) finalVelocity = _minVelocity;
        if (finalVelocity > 127) finalVelocity = 127;
        
        return finalVelocity;
    }

    MidiTransport& _midi;
    OctaveControlType& _octaveControl;
    ScaleManager& _scaleManager;
    ChordSettings& _chordSettings;

    volatile int _currentVelocity;
    int _minVelocity;
    bool _isNoteOn[128]{};
    key _keys[MAX_KEYS];
    void (*_velocityChangeHook)(int) = nullptr;
    
    // Chord tracking
    int _activeChordNotes[128][MAX_CHORD_NOTES];  // Store active chord notes for each key
    int _activeChordCount[128];  // Count of active chord notes for each key
    
    // Arpeggiator state (for advanced strum mode with pattern > 0)
    bool _arpActive;                    // Is arpeggiator running
    int _arpRootNote;                   // Root note for arpeggio
    const int8_t* _arpPattern;          // Pointer to interval pattern
    int _arpPatternLength;              // Number of intervals in pattern
    int _arpCurrentIndex;               // Current position in pattern
    unsigned long _arpLastNoteTime;     // Timestamp of last note
    int _arpCurrentNote;                // Currently playing MIDI note (-1 if none)
    
    // Basic strum state (for monophonic strum behavior)
    bool _strumActive;                  // Is a basic strum/chord currently held
    byte _activeStrumKey;               // Which key is currently active
    
    // Basic strum cascade state (non-blocking playback)
    bool _strumInProgress;              // Is strum cascade currently playing
    int _strumNotes[MAX_CHORD_NOTES];   // MIDI notes to play in strum
    int _strumVelocities[MAX_CHORD_NOTES]; // Velocities for each note
    int _strumCount;                    // Number of notes in strum
    int _strumCurrentIndex;             // Current position in cascade
    unsigned long _strumLastNoteTime;   // Timestamp of last note played
};

#endif