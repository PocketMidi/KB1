#ifndef KEYBOARD_CONTROL_H
#define KEYBOARD_CONTROL_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include <MIDI.h>
#include <objects/Globals.h>
#include <led/LEDController.h>
#include <music/ScaleManager.h>

template<typename MidiTransport, typename OctaveControlType>
class KeyboardControl {
public:
    KeyboardControl(MidiTransport& midi, OctaveControlType& octaveCtrl, ScaleManager& scaleManager)
        : _midi(midi),
          _octaveControl(octaveCtrl),
          _scaleManager(scaleManager),
          _currentVelocity(89),
          _minVelocity(15)
    {
        memset(_isNoteOn, false, sizeof(_isNoteOn));

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

    void begin() {
        for (auto & key : _keys) {
            key.mcp->pinMode(key.pin, INPUT_PULLUP);
            bool raw = !key.mcp->digitalRead(key.pin);
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
        SERIAL_PRINT("Note On: ");
        SERIAL_PRINT(quantizedNote);
        SERIAL_PRINT(", Velocity: ");
        SERIAL_PRINTLN(_currentVelocity);
        _isNoteOn[note] = true;
    }

    void stopMidiNote(const byte note, int keyIndex = -1) {
        if (_isNoteOn[note])
        {
            constexpr byte channel = 1;
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
            SERIAL_PRINT("Note Off: ");
            SERIAL_PRINT(quantizedNote);
            SERIAL_PRINTLN(", Velocity: 0");
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

    void updateKeyboardState() {
        // Debounced key scanning
        for (int i = 0; i < MAX_KEYS; ++i) {
            auto & key = _keys[i];
            bool raw = !key.mcp->digitalRead(key.pin);
            if (raw != key.lastReading) {
                key.lastDebounceTime = millis();
                key.lastReading = raw;
            }

            if ((millis() - key.lastDebounceTime) >= KEY_DEBOUNCE_MS) {
                if (raw != key.debouncedState) {
                    key.debouncedState = raw;
                    if (key.debouncedState) {
                        playMidiNote(key.midi, i);  // Pass key index
                        SERIAL_PRINT(key.name);
                        SERIAL_PRINTLN(" Pressed!");
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

    MidiTransport& _midi;
    OctaveControlType& _octaveControl;
    ScaleManager& _scaleManager; // Add ScaleManager member

    volatile int _currentVelocity;
    int _minVelocity;
    bool _isNoteOn[128]{};
    key _keys[MAX_KEYS];
    void (*_velocityChangeHook)(int) = nullptr;
};

#endif