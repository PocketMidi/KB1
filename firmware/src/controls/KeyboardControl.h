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
          _arpDirection(1),
          _arpLastPattern(-1),
          _arpShufflePos(0),
          _pitchBendOffset(0),
          _pitchBendOverlapUs(25000),
          _sustainActive(false),
          _sustainReleaseDelayMs(200),
          _arpUserLatchPanicArmed(false),
          _strumActive(false),
          _activeStrumKey(0),
          _strumInProgress(false),
          _strumCount(0),
          _strumCurrentIndex(0),
          _strumLastNoteTime(0),
          _userArpCount(0)
    {
        memset(_isNoteOn, false, sizeof(_isNoteOn));
        memset(_baseNote, 0, sizeof(_baseNote));
        memset(_userArpNotes, 0, sizeof(_userArpNotes));
        memset(_strumNotes, 0, sizeof(_strumNotes));
        memset(_strumVelocities, 0, sizeof(_strumVelocities));
        memset(_strumNoteOffTimes, 0, sizeof(_strumNoteOffTimes));
        memset(_activeChordNotes, 0, sizeof(_activeChordNotes));
        memset(_activeChordCount, 0, sizeof(_activeChordCount));
        memset(_pbPendingOffNote, 0, sizeof(_pbPendingOffNote));
        memset(_pbPendingOffDueUs, 0, sizeof(_pbPendingOffDueUs));
        memset(_pbPendingOffActive, false, sizeof(_pbPendingOffActive));
        memset(_sustainPendingOffNote, 0, sizeof(_sustainPendingOffNote));
        memset(_sustainPendingOffDueUs, 0, sizeof(_sustainPendingOffDueUs));
        memset(_sustainPendingOffActive, false, sizeof(_sustainPendingOffActive));

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

    static constexpr unsigned long KEY_PRESS_DEBOUNCE_MS = 10;    // ms - keep fast for responsive press
    static constexpr unsigned long KEY_RELEASE_DEBOUNCE_MS = 30;   // ms - longer to survive BLE-induced I2C stalls
    static constexpr unsigned long ARP_USER_LATCH_PANIC_HOLD_MS = 900; // ms
    static constexpr int MAX_CHORD_NOTES = 16;  // Support 3x voicing (5 notes × 3 octaves = 15)
    static constexpr int MAX_PB_PENDING_OFFS = 24;
    static constexpr int MAX_SUSTAIN_PENDING_OFFS = 128;

    // Set the note overlap duration for pitch bend mode retriggers.
    // Called by LeverControls whenever the lever is in PITCH_BEND mode.
    void setPitchBendOverlapUs(unsigned long us) { _pitchBendOverlapUs = us; }

    // Returns true when the arpeggiator is actively running (including latch mode with keys released)
    bool isArpActive() const { return _arpActive; }

    // Sustain timing mode: while active, each key release gets its own delayed NoteOff.
    bool isSustainActive() const { return _sustainActive; }
    void setSustainActive(bool active, unsigned long releaseMs = 0) {
        if (active) {
            // Capture duration used for per-release scheduling.
            _sustainReleaseDelayMs = releaseMs;
            if (!_sustainActive) {
                // Lock pitch bend neutral when sustain engages.
                setPitchBendOffset(0);
                _sustainActive = true;
            }
            return;
        }

        if (_sustainActive) {
            _sustainActive = false;
            // Keep existing scheduled tails; just stop scheduling new ones.
            // Update stored duration for next activation.
            _sustainReleaseDelayMs = releaseMs;
        }
    }

    // Pitch bend offset in semitones (-2 to +2). Set by LeverControls PITCH_BEND mode.
    void setPitchBendOffset(int semitones) {
        // Sustain mode freezes pitch bend to avoid accidental lever movement side effects.
        if (_sustainActive) return;
        semitones = constrain(semitones, -2, 2);
        if (semitones == _pitchBendOffset) return;
        int oldOffset = _pitchBendOffset;
        _pitchBendOffset = semitones;
        // Retrigger held SCALE notes with minimal latency.
        // NoteOn before NoteOff creates a tiny overlap that sounds smoother than off-then-on.
        if (_chordSettings.playMode == PlayMode::SCALE) {
            for (int i = 0; i < 128; i++) {
                if (_isNoteOn[i]) {
                    int quantized = _baseNote[i]; // Use stored note — correct for both natural and compact mode
                    int oldNote = constrain(quantized + oldOffset, 0, 127);
                    int newNote = constrain(quantized + semitones, 0, 127);
                    if (oldNote != newNote) {
                        _midi.sendNoteOn(newNote, _currentVelocity, 1);
                        schedulePitchBendNoteOff(oldNote);
                    }
                }
            }
        }
    }

    void begin() {
        memset(_keyPressStartMs, 0, sizeof(_keyPressStartMs));
        memset(_keyLongPressHandled, false, sizeof(_keyLongPressHandled));
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
                // Black keys repeat their lower adjacent white key (same note, parallel octave behaviour)
                int whiteKeyPosition = getWhiteKeyPosition(keyIndex);
                if (whiteKeyPosition < 0) {
                    whiteKeyPosition = getBlackKeyLowerWhitePosition(keyIndex);
                }
                if (whiteKeyPosition >= 0) {
                    quantizedNote = _scaleManager.getCompactModeNote(whiteKeyPosition) + (_octaveControl.getOctave() * 12);
                } else {
                    // Fallback: quantize normally
                    quantizedNote = _scaleManager.quantizeNote(note + _octaveControl.getOctave() * 12);
                }
            } else {
                // Natural mode: quantize to nearest scale note
                quantizedNote = _scaleManager.quantizeNote(note + _octaveControl.getOctave() * 12);
            }
            
            int bentNote = constrain(quantizedNote + _pitchBendOffset, 0, 127);
            _midi.sendNoteOn(bentNote, _currentVelocity, channel);
            char buf[16];
            snprintf(buf, sizeof(buf), "N%dv%d", bentNote, _currentVelocity);
            SERIAL_PRINTLN(buf);
            _isNoteOn[note] = true;
            _baseNote[note] = quantizedNote; // Store pre-offset note for pitch bend retrigger
        } else {
            // Chord mode - use chromatic notes (no scale quantization)
            int rootNote = note + (_octaveControl.getOctave() * 12);
            
            // Get chord intervals (expanded based on voicing setting)
            const int8_t* intervals;
            int intervalCount;
            getExpandedIntervals(intervals, intervalCount);
            
            // Check if we're in ARP mode (PlayMode::ARP)
            bool isArpMode = _chordSettings.playMode == PlayMode::ARP;
            
            if (isArpMode) {
                // ARP MODE: Looping arpeggiator (monophonic)
                
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
                
                // In ARP USER mode, capture note press order
                if (isArpMode && _chordSettings.arpUserMode == 1) {
                    // USER mode: toggle note in/out of sequence on press
                    bool noteAlreadyTracked = false;
                    for (int i = 0; i < _userArpCount; i++) {
                        if (_userArpNotes[i] == rootNote) {
                            noteAlreadyTracked = true;
                            // Already in sequence: remove it (toggle off)
                            for (int j = i; j < _userArpCount - 1; j++) {
                                _userArpNotes[j] = _userArpNotes[j + 1];
                            }
                            _userArpCount--;
                            SERIAL_PRINT("ArpU:Rem"); SERIAL_PRINTLN(rootNote);
                            break;
                        }
                    }
                    if (!noteAlreadyTracked && _userArpCount < 8) {
                        _userArpNotes[_userArpCount++] = rootNote;
                        SERIAL_PRINT("ArpU:Add"); SERIAL_PRINTLN(rootNote);
                    }
                } else if (_chordSettings.arpLatchMode == 1 && wasAlreadyRunning && rootNote == _arpRootNote) {
                    // CHORD latch: same key pressed again → stop arp (toggle off)
                    SERIAL_PRINTLN("Arp:LatchStop");
                    stopArpeggiator();
                    _isNoteOn[note] = false;
                    return;
                }
                
                // Update arpeggiator state
                _arpActive = true;
                _arpRootNote = rootNote;
                _arpPattern = intervals;
                _arpPatternLength = intervalCount;
                if (!wasAlreadyRunning) {
                    _arpUserLatchPanicArmed = false;
                }
                
                // Keep current index if already running, otherwise start from beginning
                if (!wasAlreadyRunning) {
                    int p = _chordSettings.strumPattern;
                    if (p == 2) {
                        _arpCurrentIndex = intervalCount > 0 ? intervalCount - 1 : 0;
                        _arpDirection = -1;
                    } else if (p == 4) {
                        generateArpContract(intervalCount);
                        _arpDirection = 1;
                    } else if (p == 5) {
                        generateArpExpand(intervalCount);
                        _arpDirection = 1;
                    } else if (p == 6) {
                        // Random: generate a fresh shuffle on every new key press
                        generateArpShuffle(intervalCount);
                        _arpDirection = 1;
                    } else {
                        _arpCurrentIndex = 0;
                        _arpDirection = 1;
                    }
                    _arpLastPattern = p;
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
            }  // End of ARP else block
        }
    }

    void stopMidiNote(const byte note, int keyIndex = -1) {
        // If arpeggiator is active, handle based on latch mode
        if (_arpActive) {
            _isNoteOn[note] = false;
            
            // Remove note from user sequence if in USER mode and MOMENTARY
            // (LATCHED USER mode: sequence only changes on press, not release)
            if (_chordSettings.playMode == PlayMode::ARP && _chordSettings.arpUserMode == 1
                && _chordSettings.arpLatchMode == 0) {
                int rootNote = note + (_octaveControl.getOctave() * 12);
                for (int i = 0; i < _userArpCount; i++) {
                    if (_userArpNotes[i] == rootNote) {
                        for (int j = i; j < _userArpCount - 1; j++) {
                            _userArpNotes[j] = _userArpNotes[j + 1];
                        }
                        _userArpCount--;
                        break;
                    }
                }
            }
            
            // Check latch mode: MOMENTARY (0) stops when last key released, LATCHED (1) keeps playing
            if (_chordSettings.arpLatchMode == 0) {
                bool shouldStop = false;
                if (_chordSettings.arpUserMode == 1) {
                    // USER mode: stop when sequence is empty
                    shouldStop = (_userArpCount == 0);
                } else {
                    // CHORD mode: stop only when ALL keys are released
                    bool anyKeyHeld = false;
                    for (int i = 0; i < 128; i++) {
                        if (_isNoteOn[i]) { anyKeyHeld = true; break; }
                    }
                    if (!anyKeyHeld) {
                        shouldStop = true;
                    } else {
                        // At least one key still held — transpose arp to that key
                        for (int i = 0; i < 128; i++) {
                            if (_isNoteOn[i]) {
                                int newRoot = i + (_octaveControl.getOctave() * 12);
                                if (newRoot != _arpRootNote) {
                                    // Cut current note and retarget root
                                    if (_arpCurrentNote >= 0) {
                                        _midi.sendNoteOff(_arpCurrentNote, 0, 1);
                                        _arpCurrentNote = -1;
                                    }
                                    _arpRootNote = newRoot;
                                    char buf[16];
                                    snprintf(buf, sizeof(buf), "Arp:Root>%d", newRoot);
                                    SERIAL_PRINTLN(buf);
                                }
                                break;
                            }
                        }
                    }
                }
                if (shouldStop) {
                    stopArpeggiator();
                }
            }
            // LATCHED (1): Keep playing after key release.
            // Root note updates on next key press. Arp stops only when mode changes.
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
                    // Black keys repeat their lower adjacent white key (same note, parallel octave behaviour)
                    int whiteKeyPosition = getWhiteKeyPosition(keyIndex);
                    if (whiteKeyPosition < 0) {
                        whiteKeyPosition = getBlackKeyLowerWhitePosition(keyIndex);
                    }
                    if (whiteKeyPosition >= 0) {
                        quantizedNote = _scaleManager.getCompactModeNote(whiteKeyPosition) + (_octaveControl.getOctave() * 12);
                    } else {
                        // Fallback: quantize normally
                        quantizedNote = _scaleManager.quantizeNote(note + _octaveControl.getOctave() * 12);
                    }
                } else {
                    // Natural mode: quantize to nearest scale note
                    quantizedNote = _scaleManager.quantizeNote(note + _octaveControl.getOctave() * 12);
                }
                
                int bentNote = constrain(quantizedNote + _pitchBendOffset, 0, 127);
                if (_sustainActive) {
                    // Per-release tail scheduling in KB1.
                    scheduleSustainNoteOff(bentNote, _sustainReleaseDelayMs);
                    char buf[16];
                    snprintf(buf, sizeof(buf), "N%d~", bentNote); // ~ = delayed by sustain timer
                    SERIAL_PRINTLN(buf);
                } else {
                    _midi.sendNoteOff(bentNote, 0, channel);
                    char buf[16];
                    snprintf(buf, sizeof(buf), "N%d-", bentNote);
                    SERIAL_PRINTLN(buf);
                }
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
        _arpDirection = 1;
        _arpLastPattern = -1;
        _arpCurrentNote = -1;
        
        // Clear user sequence
        _userArpCount = 0;
        memset(_userArpNotes, 0, sizeof(_userArpNotes));
    }

    void updateKeyboardState(const GPIOCache& gpioCache) {
        // Process delayed note-offs for pitch bend overlap smoothing.
        processPendingPitchBendNoteOffs();
        // Process delayed note-offs scheduled by sustain timing mode.
        processPendingSustainNoteOffs();

        // Update arpeggiator (for advanced strum looping)
        updateArpeggiator();
        
        // Update basic strum (non-blocking cascade)
        updateStrum();

        unsigned long nowMs = millis();
        
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
                key.lastDebounceTime = nowMs;
                key.lastReading = raw;
            }

            unsigned long debounceMs = raw ? KEY_PRESS_DEBOUNCE_MS : KEY_RELEASE_DEBOUNCE_MS;
            if ((nowMs - key.lastDebounceTime) >= debounceMs) {
                if (raw != key.debouncedState) {
                    key.debouncedState = raw;
                    if (key.debouncedState) {
                        _keyPressStartMs[i] = nowMs;
                        _keyLongPressHandled[i] = false;
                        playMidiNote(key.midi, i);  // Pass key index
                    } else {
                        bool isLongPressRelease =
                            _keyPressStartMs[i] > 0 &&
                            (nowMs - _keyPressStartMs[i]) >= ARP_USER_LATCH_PANIC_HOLD_MS;

                        _keyPressStartMs[i] = 0;

                        // Release-edge panic: consume release after long hold in USER+LATCH ARP.
                        if (!_keyLongPressHandled[i] && isLongPressRelease &&
                            _arpUserLatchPanicArmed &&
                            _arpActive &&
                            _chordSettings.playMode == PlayMode::ARP &&
                            _chordSettings.arpUserMode == 1 &&
                            _chordSettings.arpLatchMode == 1) {
                            _keyLongPressHandled[i] = true;
                            SERIAL_PRINTLN("Arp:PanicLP");
                            stopArpeggiator();
                        } else {
                            _keyLongPressHandled[i] = false;
                            stopMidiNote(key.midi, i);  // Pass key index
                        }
                    }
                }
            }

            key.prevState = key.state;
            key.state = key.debouncedState;
        }

        // Panic stop for runaway ARP: long-press any held key in USER + LATCH mode.
        if (_arpActive &&
            _chordSettings.playMode == PlayMode::ARP &&
            _chordSettings.arpUserMode == 1 &&
            _chordSettings.arpLatchMode == 1) {
            if (!_arpUserLatchPanicArmed && !anyKeyActive()) {
                _arpUserLatchPanicArmed = true;
                SERIAL_PRINTLN("Arp:PanicArmed");
            }

            for (int i = 0; i < MAX_KEYS; ++i) {
                if (_keys[i].debouncedState &&
                    _arpUserLatchPanicArmed &&
                    !_keyLongPressHandled[i] &&
                    _keyPressStartMs[i] > 0 &&
                    (nowMs - _keyPressStartMs[i]) >= ARP_USER_LATCH_PANIC_HOLD_MS) {
                    _keyLongPressHandled[i] = true;
                    SERIAL_PRINTLN("Arp:PanicLP");
                    stopArpeggiator();
                    break;
                }
            }
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
        
        // Check if any currently playing notes need to be turned off (scheduled release timing)
        for (int i = 0; i < _strumCurrentIndex; i++) {
            if (_strumNoteOffTimes[i] > 0 && now >= _strumNoteOffTimes[i]) {
                _midi.sendNoteOff(_strumNotes[i], 0, 1);
                char buf[16];
                snprintf(buf, sizeof(buf), "S%d-off", i);
                SERIAL_PRINTLN(buf);
                _strumNoteOffTimes[i] = 0;  // Mark as handled
            }
        }
        
        // Calculate delay for this note.
        // Repurpose gateValue as CHORD swing amount:
        //   gate=10  -> straight 50/50 split
        //   gate=100 -> triplet-like 66/33 split
        // Note: this does not alter touch-control "Gate" mode naming/behavior.
        // This is independent from ARP swing (strumSwing), which is handled in updateArpeggiator().
        int baseDelay = abs(_chordSettings.strumSpeed);
        auto calculateStepDelay = [&](int noteIndex) -> int {
            int clampedGate = _chordSettings.gateValue;
            if (clampedGate < 10) clampedGate = 10;
            if (clampedGate > 100) clampedGate = 100;

            float swingT = (clampedGate - 10.0f) / 90.0f;     // 0..1
            float longFraction = 0.5f + swingT * (1.0f / 6.0f); // 0.5 .. 0.6667
            float shortFraction = 1.0f - longFraction;          // 0.5 .. 0.3333

            bool isOffBeat = (noteIndex % 2) == 1;
            float pairFraction = isOffBeat ? shortFraction : longFraction;
            int delay = (int)(2.0f * baseDelay * pairFraction);

            if (delay < 4) delay = 4;
            return delay;
        };
        int noteDelay = calculateStepDelay(_strumCurrentIndex);
        
        // Check if enough time has passed to play next note
        if (now - _strumLastNoteTime >= (unsigned long)noteDelay) {
            // Play current note
            _midi.sendNoteOn(_strumNotes[_strumCurrentIndex], _strumVelocities[_strumCurrentIndex], 1);
            char buf[24];
            snprintf(buf, sizeof(buf), "S%d:N%dv%d", _strumCurrentIndex, _strumNotes[_strumCurrentIndex], _strumVelocities[_strumCurrentIndex]);
            SERIAL_PRINTLN(buf);
            
            // Hold each note until just before the next strum step so timing feel comes from
            // swing (onset spacing), not forced staccato note duration.
            int nextIndex = _strumCurrentIndex + 1;
            int nextStepDelay = (nextIndex < _strumCount)
                ? calculateStepDelay(nextIndex)
                : noteDelay;

            unsigned long noteDuration = (nextStepDelay > 1) ? (unsigned long)(nextStepDelay - 1) : 1UL;
            _strumNoteOffTimes[_strumCurrentIndex] = now + noteDuration;
            
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
        if (!_arpActive) {
            return;
        }
        
        // Check if we're in ARP USER mode (user note sequence) or CHORD pattern mode
        bool isArpUserMode = (_chordSettings.playMode == PlayMode::ARP && _chordSettings.arpUserMode == 1);
        
        // Exit if no valid pattern/sequence available
        if (isArpUserMode) {
            if (_userArpCount == 0) return;  // No notes in user sequence yet
        } else {
            if (!_arpPattern || _arpPatternLength == 0) return;  // No pattern available
        }

        unsigned long now = millis();
        
        // Calculate step delay based on strum speed
        int baseDelay = abs(_chordSettings.strumSpeed);
        int noteDelay = baseDelay;
        
        // Apply swing to odd-indexed notes in ALL ARP modes (CHORD and USER)
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
        if (isArpUserMode && _userArpCount > 0) {
            int pat = _chordSettings.strumPattern;

            if (pat == 5 || _userArpCount == 1) {
                // strumPattern 5 = USER PRESS ORDER (cross-arrows icon): play in key-press order
                int noteIndex = _arpCurrentIndex % _userArpCount;
                _arpCurrentNote = _userArpNotes[noteIndex];
                int velocity = calculateChordVelocity(noteIndex, _userArpCount);

                _midi.sendNoteOn(_arpCurrentNote, velocity, 1);
                char buf[24];
                snprintf(buf, sizeof(buf), "ArpU%d:N%dv%d", noteIndex, _arpCurrentNote, velocity);
                SERIAL_PRINTLN(buf);

                _arpLastNoteTime = now;
                _arpCurrentIndex++;
                if (_arpCurrentIndex >= _userArpCount) {
                    _arpCurrentIndex = 0;
                    SERIAL_PRINTLN("ArpU:Loop");
                }
            } else {
                // PITCH-SORTED PATTERNS: sort held notes by pitch, then apply direction
                // Patterns: 1=up, 2=down, 3=updown ping-pong, 6=random, 7=edges-to-center
                int sortedNotes[8];
                int sortedCount = _userArpCount;
                memcpy(sortedNotes, _userArpNotes, sortedCount * sizeof(int));

                // Insertion sort ascending by pitch (max 8 notes, cheap)
                for (int i = 1; i < sortedCount; i++) {
                    int key = sortedNotes[i];
                    int j = i - 1;
                    while (j >= 0 && sortedNotes[j] > key) {
                        sortedNotes[j + 1] = sortedNotes[j];
                        j--;
                    }
                    sortedNotes[j + 1] = key;
                }

                // For Contract (pat==4) and edges-to-center (pat==7), reorder: alternate outer notes inward
                // e.g. [C3,E3,G3,B3] -> [C3,B3,E3,G3]
                if (pat == 4 || pat == 7) {
                    int reordered[8];
                    int lo = 0, hi = sortedCount - 1, k = 0;
                    while (lo <= hi) {
                        reordered[k++] = sortedNotes[lo++];
                        if (lo <= hi) reordered[k++] = sortedNotes[hi--];
                    }
                    memcpy(sortedNotes, reordered, sortedCount * sizeof(int));
                }

                // Clamp index in case note count changed while playing
                if (_arpCurrentIndex >= sortedCount) _arpCurrentIndex = 0;

                // Reset direction/index on pattern change
                if (pat != _arpLastPattern) {
                    _arpLastPattern = pat;
                    if (pat == 2) {
                        _arpCurrentIndex = sortedCount > 0 ? sortedCount - 1 : 0;
                        _arpDirection = -1;
                    } else if (pat == 6) {
                        generateArpShuffle(sortedCount);
                        _arpDirection = 1;
                    } else {
                        _arpCurrentIndex = 0;
                        _arpDirection = 1;
                    }
                }

                // Determine which note to play
                int noteIndex;
                if (pat == 6) {
                    // Random (shuffle): play from pre-generated shuffle, re-shuffle on loop
                    if (_arpShufflePos >= sortedCount) {
                        generateArpShuffle(sortedCount);
                    }
                    noteIndex = _arpShuffleBuffer[_arpShufflePos++];
                } else {
                    noteIndex = _arpCurrentIndex;
                }
                if (noteIndex < 0) noteIndex = 0;
                if (noteIndex >= sortedCount) noteIndex = sortedCount - 1;

                _arpCurrentNote = sortedNotes[noteIndex];
                int velocity = calculateChordVelocity(noteIndex, sortedCount);
                _midi.sendNoteOn(_arpCurrentNote, velocity, 1);
                char buf[32];
                snprintf(buf, sizeof(buf), "ArpS%d(p%d):N%dv%d", noteIndex, pat, _arpCurrentNote, velocity);
                SERIAL_PRINTLN(buf);

                _arpLastNoteTime = now;

                // Advance index for non-random patterns
                if (pat != 6) {
                    if (pat == 2) {
                        // Down: decrement, wrap
                        _arpCurrentIndex--;
                        if (_arpCurrentIndex < 0) _arpCurrentIndex = sortedCount - 1;
                    } else if (pat == 3) {
                        // Up-Down ping-pong
                        _arpCurrentIndex += _arpDirection;
                        if (_arpCurrentIndex >= sortedCount) {
                            _arpDirection = -1;
                            _arpCurrentIndex = max(0, sortedCount - 2);
                        } else if (_arpCurrentIndex < 0) {
                            _arpDirection = 1;
                            _arpCurrentIndex = min(1, sortedCount - 1);
                        }
                    } else {
                        // Up (pat 1), Contract (pat 4), edges-to-center (pat 7): increment linearly
                        _arpCurrentIndex++;
                        if (_arpCurrentIndex >= sortedCount) _arpCurrentIndex = 0;
                    }
                }
            }
        } else {
            // CHORD mode: Calculate note from root + interval
            int interval = _arpPattern[_arpCurrentIndex];
            _arpCurrentNote = _arpRootNote + interval;
            int velocity = calculateChordVelocity(_arpCurrentIndex, _arpPatternLength);
            
            _midi.sendNoteOn(_arpCurrentNote, velocity, 1);
            char buf[24];
            snprintf(buf, sizeof(buf), "Arp%d:N%dv%d", _arpCurrentIndex, _arpCurrentNote, velocity);
            SERIAL_PRINTLN(buf);
            
            _arpLastNoteTime = now;
            
            // Advance index based on strumPattern direction mode
            // Detect pattern change and reset direction/index
            int pat = _chordSettings.strumPattern;
            if (pat != _arpLastPattern) {
                _arpLastPattern = pat;
                if (pat == 2) {
                    _arpCurrentIndex = _arpPatternLength > 0 ? _arpPatternLength - 1 : 0;
                    _arpDirection = -1;
                } else if (pat == 4) {
                    generateArpContract(_arpPatternLength);
                    _arpDirection = 1;
                } else if (pat == 5) {
                    generateArpExpand(_arpPatternLength);
                    _arpDirection = 1;
                } else if (pat == 6) {
                    generateArpShuffle(_arpPatternLength);
                    _arpDirection = 1;
                } else {
                    _arpCurrentIndex = 0;
                    _arpUserLatchPanicArmed = false;
                    _arpDirection = 1;
                }
            } else if (pat == 2) {
                // Down: decrement and wrap
                _arpCurrentIndex--;
                if (_arpCurrentIndex < 0) {
                    _arpCurrentIndex = _arpPatternLength - 1;
                    SERIAL_PRINTLN("Arp:Loop");
                }
            } else if (pat == 3) {
                // Up-Down ping-pong
                _arpCurrentIndex += _arpDirection;
                if (_arpCurrentIndex >= _arpPatternLength) {
                    _arpDirection = -1;
                    _arpCurrentIndex = max(0, _arpPatternLength - 2);
                } else if (_arpCurrentIndex < 0) {
                    _arpDirection = 1;
                    _arpCurrentIndex = min(1, _arpPatternLength - 1);
                }
            } else if (pat == 4 || pat == 5) {
                // Contract/Expand: iterate through pre-computed buffer, restart on loop
                if (_arpShufflePos >= _arpPatternLength) {
                    _arpShufflePos = 0;
                }
                _arpCurrentIndex = _arpShuffleBuffer[_arpShufflePos++];
            } else if (pat == 6) {
                // Random (shuffle): advance through pre-generated shuffle, re-shuffle on loop
                if (_arpShufflePos >= _arpPatternLength) {
                    generateArpShuffle(_arpPatternLength);
                }
                _arpCurrentIndex = _arpShuffleBuffer[_arpShufflePos++];
            } else {
                // Up / chord order (0, 1, 6, 7, default): increment and wrap
                _arpCurrentIndex++;
                if (_arpCurrentIndex >= _arpPatternLength) {
                    _arpCurrentIndex = 0;
                    SERIAL_PRINTLN("Arp:Loop");
                }
            }
        }
    }

private:
    // Fisher-Yates shuffle of indices 0..count-1 into _arpShuffleBuffer
    void generateArpShuffle(int count) {
        count = constrain(count, 1, MAX_CHORD_NOTES);
        for (int i = 0; i < count; i++) _arpShuffleBuffer[i] = i;
        for (int i = count - 1; i > 0; i--) {
            int j = random(0, i + 1);
            int tmp = _arpShuffleBuffer[i];
            _arpShuffleBuffer[i] = _arpShuffleBuffer[j];
            _arpShuffleBuffer[j] = tmp;
        }
        _arpShufflePos = 0;
    }

    // Contract (P4): alternate from outer edges inward — low, high, 2nd-low, 2nd-high...
    void generateArpContract(int count) {
        count = constrain(count, 1, MAX_CHORD_NOTES);
        int lo = 0, hi = count - 1, k = 0;
        while (lo <= hi) {
            _arpShuffleBuffer[k++] = lo++;
            if (lo <= hi) _arpShuffleBuffer[k++] = hi--;
        }
        _arpShufflePos = 0;
    }

    // Expand (P5): from center outward — mid, mid+1, mid-1, mid+2, mid-2...
    void generateArpExpand(int count) {
        count = constrain(count, 1, MAX_CHORD_NOTES);
        int mid = (count - 1) / 2;
        int k = 0;
        _arpShuffleBuffer[k++] = mid;
        for (int d = 1; d < count && k < count; d++) {
            int hi = mid + d;
            int lo = mid - d;
            if (hi < count) _arpShuffleBuffer[k++] = hi;
            if (lo >= 0 && k < count) _arpShuffleBuffer[k++] = lo;
        }
        _arpShufflePos = 0;
    }

    void schedulePitchBendNoteOff(int note, unsigned long delayUs) {
        unsigned long due = micros() + delayUs;
        for (int i = 0; i < MAX_PB_PENDING_OFFS; ++i) {
            if (!_pbPendingOffActive[i]) {
                _pbPendingOffNote[i] = note;
                _pbPendingOffDueUs[i] = due;
                _pbPendingOffActive[i] = true;
                return;
            }
        }
        // Queue full: fail safe by sending immediate off to avoid stuck notes.
        _midi.sendNoteOff(note, 0, 1);
    }

    void schedulePitchBendNoteOff(int note) {
        schedulePitchBendNoteOff(note, _pitchBendOverlapUs);
    }

    void processPendingPitchBendNoteOffs() {
        unsigned long nowUs = micros();
        for (int i = 0; i < MAX_PB_PENDING_OFFS; ++i) {
            if (_pbPendingOffActive[i]) {
                long dt = (long)(nowUs - _pbPendingOffDueUs[i]);
                if (dt >= 0) {
                    _midi.sendNoteOff(_pbPendingOffNote[i], 0, 1);
                    _pbPendingOffActive[i] = false;
                }
            }
        }
    }

    void scheduleSustainNoteOff(int note, unsigned long delayMs) {
        if (delayMs == 0) {
            _midi.sendNoteOff(note, 0, 1);
            return;
        }

        unsigned long due = micros() + (delayMs * 1000UL);
        for (int i = 0; i < MAX_SUSTAIN_PENDING_OFFS; ++i) {
            if (!_sustainPendingOffActive[i]) {
                _sustainPendingOffNote[i] = note;
                _sustainPendingOffDueUs[i] = due;
                _sustainPendingOffActive[i] = true;
                return;
            }
        }

        // Queue full: fail safe by sending immediate off to avoid runaways.
        _midi.sendNoteOff(note, 0, 1);
        SERIAL_PRINTLN("SustainQ:full");
    }

    void processPendingSustainNoteOffs() {
        unsigned long nowUs = micros();
        for (int i = 0; i < MAX_SUSTAIN_PENDING_OFFS; ++i) {
            if (_sustainPendingOffActive[i]) {
                long dt = (long)(nowUs - _sustainPendingOffDueUs[i]);
                if (dt >= 0) {
                    _midi.sendNoteOff(_sustainPendingOffNote[i], 0, 1);
                    _sustainPendingOffActive[i] = false;
                }
            }
        }
    }

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

    // In compact mode, black keys repeat the note of their lower (left) adjacent white key.
    // Hardware key layout (19 keys): white={0,1,3,5,6,8,10,12,13,15,17,18}, black={2,4,7,9,11,14,16}
    // Maps each black key index to the white-key position (0-based) of the key immediately below it.
    int getBlackKeyLowerWhitePosition(int keyIndex) const {
        // black key index -> lower adjacent white key position
        switch (keyIndex) {
            case  2: return 1;   // C#/Db -> C (white pos 1)
            case  4: return 2;   // D#/Eb -> D (white pos 2)
            case  7: return 4;   // F#/Gb -> F (white pos 4)
            case  9: return 5;   // G#/Ab -> G (white pos 5)
            case 11: return 6;   // A#/Bb -> A (white pos 6)
            case 14: return 8;   // C#/Db (oct2) -> C (white pos 8)
            case 16: return 9;   // D#/Eb (oct2) -> D (white pos 9)
            default: return -1;
        }
    }

    // Get chord intervals based on chord type or custom ARP pattern
    // strumPattern 0 = use chord type intervals
    // strumPattern 1-6 = build mode selector (handled in web app PatternBuilder, always results in custom intervals)
    // strumPattern 7 = use custom interval array sent via BLE
    void getChordIntervals(const int8_t*& intervals, int& count) const {
        if (_chordSettings.strumPattern == 7) {
            // Use custom pattern built by PatternBuilder and sent via BLE
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
    unsigned long _keyPressStartMs[MAX_KEYS]{};
    bool _keyLongPressHandled[MAX_KEYS]{};
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
    int _arpDirection;                  // +1 = ascending, -1 = descending (for ping-pong patterns)
    int _arpLastPattern;                // Tracks pattern changes to reset direction/index
    bool _arpUserLatchPanicArmed;       // Arms long-press panic only after a hands-off moment
    int _arpShuffleBuffer[MAX_CHORD_NOTES]; // Shuffled index order for random pattern
    int _arpShufflePos;                 // Current position in shuffle buffer
    int _pitchBendOffset;               // Semitone offset applied to SCALE mode notes (-2 to +2)
    unsigned long _pitchBendOverlapUs;   // Note overlap window in microseconds (configurable per lever)
    int _baseNote[128];                  // Pre-offset quantized note stored at key press (compact+natural safe)
    bool _sustainActive;                 // True while sustain timing mode is active
    unsigned long _sustainReleaseDelayMs; // Per-release NoteOff tail in ms
    int _sustainPendingOffNote[MAX_SUSTAIN_PENDING_OFFS];
    unsigned long _sustainPendingOffDueUs[MAX_SUSTAIN_PENDING_OFFS];
    bool _sustainPendingOffActive[MAX_SUSTAIN_PENDING_OFFS];
    int _pbPendingOffNote[MAX_PB_PENDING_OFFS];      // Delayed note-offs for smooth PB overlap
    unsigned long _pbPendingOffDueUs[MAX_PB_PENDING_OFFS];
    bool _pbPendingOffActive[MAX_PB_PENDING_OFFS];
    
    // Arp USER mode - user-defined note sequence
    int _userArpNotes[8];               // MIDI notes in order pressed (up to 8)
    int _userArpCount;                  // Number of notes in user sequence
    
    // Basic strum state (for monophonic strum behavior)
    bool _strumActive;                  // Is a basic strum/chord currently held
    byte _activeStrumKey;               // Which key is currently active
    
    // Basic strum cascade state (non-blocking playback)
    bool _strumInProgress;              // Is strum cascade currently playing
    int _strumNotes[MAX_CHORD_NOTES];   // MIDI notes to play in strum
    int _strumVelocities[MAX_CHORD_NOTES]; // Velocities for each note
    unsigned long _strumNoteOffTimes[MAX_CHORD_NOTES]; // Scheduled note-off times for active strum notes
    int _strumCount;                    // Number of notes in this strum
    int _strumCurrentIndex;             // Current position in cascade
    unsigned long _strumLastNoteTime;   // Timestamp of last note played
};

#endif