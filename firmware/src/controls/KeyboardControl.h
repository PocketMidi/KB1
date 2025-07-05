#ifndef KEYBOARD_CONTROL_H
#define KEYBOARD_CONTROL_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include <MIDI.h>
#include <objects/Globals.h>
#include <led/LEDController.h>
#include <music/ScaleManager.h>

template<typename MidiTransport, typename OctaveControlType, typename LEDControllerType>
class KeyboardControl {
public:
    KeyboardControl(MidiTransport& midi, OctaveControlType& octaveCtrl, LEDControllerType& ledCtrl, ScaleManager& scaleManager)
        : _midi(midi),
          _octaveControl(octaveCtrl),
          _ledController(ledCtrl),
          _scaleManager(scaleManager),
          _currentVelocity(80),
          _minVelocity(8)
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

    void begin() {
        for (auto & key : _keys) {
            key.mcp->pinMode(key.pin, INPUT_PULLUP);
            key.state = !key.mcp->digitalRead(key.pin);
            key.prevState = key.state;
        }
    }

    void playMidiNote(const byte note) {
        constexpr byte channel = 1;
        int quantizedNote = _scaleManager.quantizeNote(note + _octaveControl.getOctave() * 12);
        _midi.sendNoteOn(quantizedNote, _currentVelocity, channel);
        SERIAL_PRINT("Note On: ");
        SERIAL_PRINT(quantizedNote);
        SERIAL_PRINT(", Velocity: ");
        SERIAL_PRINTLN(_currentVelocity);
        _isNoteOn[note] = true;
    }

    void stopMidiNote(const byte note) {
        if (_isNoteOn[note])
        {
            constexpr byte channel = 1;
            int quantizedNote = _scaleManager.quantizeNote(note + _octaveControl.getOctave() * 12);
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
        if (_currentVelocity < _minVelocity)
            _currentVelocity = _minVelocity;
        else if (_currentVelocity > 127)
            _currentVelocity = 127;

        if (delta > 0) {
            _ledController.set(LedColor::BLUE, 255, 500);
        } else if (delta < 0) {
            _ledController.set(LedColor::BLUE, 0, 3000);
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

    void resetVelocity() {
        _currentVelocity = 80;
        SERIAL_PRINTLN("Velocity Reset to 80");
        _ledController.set(LedColor::BLUE, 51, 250);
        delay(250);
        _ledController.set(LedColor::BLUE, 0, 0);
    }

    void updateKeyboardState() {
        // Read key states & button states on U2
        for (auto & key : _keys) {
            key.state = !key.mcp->digitalRead(key.pin);
        }

        for (auto & key : _keys) {
            if (key.state != key.prevState) {
                if (key.state) {
                    playMidiNote(key.midi);
                    SERIAL_PRINT(key.name);
                    SERIAL_PRINTLN(" Pressed!");
                } else {
                    stopMidiNote(key.midi);
                }
            }
            key.prevState = key.state;
        }
    }

private:
    MidiTransport& _midi;
    OctaveControlType& _octaveControl;
    LEDControllerType& _ledController;
    ScaleManager& _scaleManager; // Add ScaleManager member

    volatile int _currentVelocity;
    int _minVelocity;
    bool _isNoteOn[128]{};
    key _keys[MAX_KEYS];
};

#endif