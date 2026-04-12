#ifndef LEVER_PUSH_CONTROLS_H
#define LEVER_PUSH_CONTROLS_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include <objects/Globals.h>
#include <objects/Settings.h>
#include <controls/OctaveControl.h>
#include <controls/KeyboardControl.h>

template<class MidiTransport>
class LeverPushControls {
public:
    LeverPushControls(
        Adafruit_MCP23X17* mcp,
        int centerPin,
        LeverPushSettings& settings,
        LeverControls<MidiTransport>& leverControls,
        MidiTransport& midi,
        LEDController& ledController,
        LedColor ledColor,
        KeyboardControl<MidiTransport, OctaveControl<Adafruit_MCP23X17, LEDController>>& keyboardControl,
        ChordSettings& chordSettings
    );

    void update();
    void setCCNumber(int number);
    void setMinCCValue(int number);
    void setMaxCCValue(int number);
    void setFunctionMode(LeverPushFunctionMode functionMode);
    void setOnsetTime(unsigned long time);
    void setOffsetTime(unsigned long time);
    void setOnsetInterpolationType(InterpolationType type);
    void setOffsetInterpolationType(InterpolationType type);
    void syncValue(); // Re-sync internal value when settings change

private:
    Adafruit_MCP23X17* _mcp;
    int _centerPin;
    LeverPushSettings& _settings;
    LeverControls<MidiTransport>& _leverControls;
    MidiTransport& _midi;
    KeyboardControl<MidiTransport, OctaveControl<Adafruit_MCP23X17, LEDController>>& _keyboardControl;
    LEDController& _ledController;
    LedColor _ledColor;
    ChordSettings& _chordSettings;

    bool _isPressed;
    int _lastSentValue;
    int _currentValue;
    int _targetValue;

    unsigned long _rampStartTime;
    int _rampStartValue;
    int _previousCCNumber;

    void handleInput();
    void updateValue();
};

template<class MidiTransport>
LeverPushControls<MidiTransport>::LeverPushControls(
    Adafruit_MCP23X17* mcp,
    int centerPin,
    LeverPushSettings& settings,
    LeverControls<MidiTransport>& leverControls,
    MidiTransport& midi,
    LEDController& ledController,
    LedColor ledColor,
    KeyboardControl<MidiTransport, OctaveControl<Adafruit_MCP23X17, LEDController>>& keyboardControl,
    ChordSettings& chordSettings)
    :
    _mcp(mcp),
    _centerPin(centerPin),
    _settings(settings),
    _leverControls(leverControls),
    _midi(midi),
    _ledController(ledController),
    _ledColor(ledColor),
    _keyboardControl(keyboardControl),
    _chordSettings(chordSettings),
    _isPressed(false),
    _rampStartTime(0),
    _currentValue(_settings.minCCValue),
    _targetValue(_settings.minCCValue),
    _rampStartValue(_settings.minCCValue),
    _lastSentValue(_settings.minCCValue),
    _previousCCNumber(-1)
    {
        // LeverPush is always unipolar - no special init needed for CC 200
    }

template<class MidiTransport>
void LeverPushControls<MidiTransport>::update() {
    handleInput();
    updateValue();
}

template<class MidiTransport>
void LeverPushControls<MidiTransport>::setFunctionMode(LeverPushFunctionMode functionMode) {
    _settings.functionMode = functionMode;
}

template<class MidiTransport>
void LeverPushControls<MidiTransport>::setOnsetTime(unsigned long time) {
    _settings.onsetTime = time;
}

template<class MidiTransport>
void LeverPushControls<MidiTransport>::setOffsetTime(unsigned long time) {
    _settings.offsetTime = time;
}

template<class MidiTransport>
void LeverPushControls<MidiTransport>::setCCNumber(int number) {
    _settings.ccNumber = number;
    
    // When CC number changes TO 200 (Strum Speed), sync from current strumSpeed
    if (number == 200) {
        int strumSpeed = map(_currentValue, _settings.minCCValue, _settings.maxCCValue, 360, 4);
        _chordSettings.strumSpeed = constrain(strumSpeed, 4, 360);
        SERIAL_PRINT("LeverPush assigned to CC 200, syncing strumSpeed to: ");
        SERIAL_PRINTLN(_chordSettings.strumSpeed);
        // Notify BLE clients of the change
        if (notifyChordSettingsCallback) {
            notifyChordSettingsCallback();
        }
    }
}

template<class MidiTransport>
void LeverPushControls<MidiTransport>::setMinCCValue(int number) {
    _settings.minCCValue = number;
}

template<class MidiTransport>
void LeverPushControls<MidiTransport>::setMaxCCValue(int number) {
    _settings.maxCCValue = number;
}

template<class MidiTransport>
void LeverPushControls<MidiTransport>::setOnsetInterpolationType(InterpolationType type) {
    _settings.onsetType = type;
}

template<class MidiTransport>
void LeverPushControls<MidiTransport>::setOffsetInterpolationType(InterpolationType type) {
    _settings.offsetType = type;
}

template<class MidiTransport>
void LeverPushControls<MidiTransport>::syncValue() {
    // Re-initialize value based on current settings (called after BLE updates)
    // LeverPush is always unipolar - just reset to min value
    _currentValue = _settings.minCCValue;
    _targetValue = _settings.minCCValue;
    _rampStartValue = _settings.minCCValue;
    _lastSentValue = _settings.minCCValue;
}

template<class MidiTransport>
void LeverPushControls<MidiTransport>::handleInput() {
    // Reset pattern state if CC changed away from Pattern Selector (201)
    if (_previousCCNumber == 201 && _settings.ccNumber != 201) {
        _lastSentValue = _settings.minCCValue;
        _currentValue = _settings.minCCValue;
        _targetValue = _settings.minCCValue;
        _isPressed = false;
        SERIAL_PRINTLN("LeverPush: Exited Pattern Selector mode, state reset");
    }
    _previousCCNumber = _settings.ccNumber;

    // Disable Pattern Selector (201) and Swing (202) when not in strum:shape mode
    // Only active when playMode=CHORD AND strumEnabled=true AND strumPattern>0
    bool isStrumShapeMode = (_chordSettings.playMode == PlayMode::CHORD && 
                              _chordSettings.strumEnabled && 
                              _chordSettings.strumPattern > 0);
    bool isPatternOrSwing = (_settings.ccNumber == 201 || _settings.ccNumber == 202);
    if (isPatternOrSwing && !isStrumShapeMode) {
        // Silently ignore input when not in strum:shape mode
        return;
    }

    bool state = !_mcp->digitalRead(_centerPin);
    int oldTargetValue = _targetValue;

    if (_settings.functionMode == LeverPushFunctionMode::RESET) {
        if (state) {
            _currentValue = _settings.minCCValue;
            if (_settings.ccNumber == 128) {
                int vel = constrain(_currentValue, 0, 127);
                _keyboardControl.setVelocity(vel);
                if (_leverControls.getCCNumber() == 128) {
                    _leverControls.setValue(vel);
                }
            } else {
                _leverControls.setValue(_currentValue);
            }
        }
    } else if (_settings.functionMode == LeverPushFunctionMode::STATIC) {
        // Special handling for Pattern Selector (201): cycle through patterns instead of toggle
        if (_settings.ccNumber == 201) {
            if (state && !_isPressed) {
                // Use discrete MIDI values for each pattern (0, 25, 51, 76, 102, 127)
                // This avoids rounding errors from continuous mapping
                const int patternMidi[] = {0, 25, 51, 76, 102, 127};
                
                // Find current pattern by matching nearest MIDI value
                int currentPattern = 1;
                int minDiff = 127;
                for (int i = 0; i < 6; i++) {
                    int diff = abs(_currentValue - patternMidi[i]);
                    if (diff < minDiff) {
                        minDiff = diff;
                        currentPattern = i + 1;
                    }
                }
                
                // Get min/max from settings
                int minPattern = 1 + (_settings.minCCValue * 5 / 127);  // Map 0-127 to 0-5, add 1
                int maxPattern = 1 + (_settings.maxCCValue * 5 / 127);
                minPattern = constrain(minPattern, 1, 6);
                maxPattern = constrain(maxPattern, 1, 6);
                
                // Cycle pattern based on offsetTime: 0=forward, >0=reverse
                if (_settings.offsetTime == 0) {
                    // Forward: increment pattern (wrap around from max to min)
                    currentPattern++;
                    if (currentPattern > maxPattern) currentPattern = minPattern;
                } else {
                    // Reverse: decrement pattern (wrap around from min to max)
                    currentPattern--;
                    if (currentPattern < minPattern) currentPattern = maxPattern;
                }
                currentPattern = constrain(currentPattern, 1, 6);
                
                // Set discrete MIDI value for the pattern
                _currentValue = patternMidi[currentPattern - 1];
                _isPressed = true;
                
                SERIAL_PRINT("P");
                SERIAL_PRINT(currentPattern);
                SERIAL_PRINT("=");
                SERIAL_PRINTLN(_currentValue);
            } else if (!state && _isPressed) {
                // Don't change value on release - keep current pattern
                _isPressed = false;
            }
        } else {
            // Normal toggle behavior for other parameters
            if (state && !_isPressed) {
                _currentValue = _settings.maxCCValue;
                _isPressed = true;
            } else if (!state && _isPressed) {
                _currentValue = _settings.minCCValue;
                _isPressed = false;
            }
        }
        _targetValue = _currentValue;
    } else if (_settings.functionMode == LeverPushFunctionMode::PEAK_AND_DECAY) {
        if (state && !_isPressed) {
            _currentValue = _settings.maxCCValue;
            _targetValue = _settings.maxCCValue;
            _isPressed = true;
        } else if (!state && _isPressed) {
            _targetValue = _settings.minCCValue;
            _isPressed = false;
        }

        if (_targetValue != oldTargetValue) {
            _rampStartTime = millis();
            _rampStartValue = _currentValue;
        }
    } else if (_settings.functionMode == LeverPushFunctionMode::INTERPOLATED) {
        if (state && !_isPressed) {
            _targetValue = _settings.maxCCValue;
            _isPressed = true;
        } else if (!state && _isPressed) {
            _targetValue = _settings.minCCValue;
            _isPressed = false;
        }

        if (_targetValue != oldTargetValue) {
            _rampStartTime = millis();
            _rampStartValue = _currentValue;
        }
    }
}

template<class MidiTransport>
void LeverPushControls<MidiTransport>::updateValue() {
    if (_settings.functionMode == LeverPushFunctionMode::INTERPOLATED || _settings.functionMode == LeverPushFunctionMode::PEAK_AND_DECAY) {
        if (_currentValue == _targetValue && _currentValue == _lastSentValue) {
            return;
        }

        unsigned long currentTime = millis();
        unsigned long rampDuration = _isPressed ? _settings.onsetTime : _settings.offsetTime;
        unsigned long elapsedTime = currentTime - _rampStartTime;

        if (rampDuration == 0 || elapsedTime >= rampDuration) {
            _currentValue = _targetValue;
        } else {
            float progress = (float)elapsedTime / (float)rampDuration;

            if (_isPressed) {
                if (_settings.onsetType == InterpolationType::EXPONENTIAL) {
                    progress = pow(progress, 2);
                } else if (_settings.onsetType == InterpolationType::LOGARITHMIC) {
                    progress = 1 - pow(1 - progress, 2);
                }
            } else {
                if (_settings.offsetType == InterpolationType::EXPONENTIAL) {
                    progress = pow(progress, 2);
                } else if (_settings.offsetType == InterpolationType::LOGARITHMIC) {
                    progress = 1 - pow(1 - progress, 2);
                }
            }

            float totalValueChange = _targetValue - _rampStartValue;
            _currentValue = _rampStartValue + (int)(progress * totalValueChange);
        }
    }

    if (_currentValue != _lastSentValue) {
        int sendVal = constrain(_currentValue, 0, 127);
        if (_settings.ccNumber == 128) {
            SERIAL_PRINT("Velocity set: "); SERIAL_PRINTLN(sendVal);
            _keyboardControl.setVelocity(sendVal);
        } else if (_settings.ccNumber == 200) {
            // 200 = KB1 Expression: Strum Speed (bipolar: ±4 to ±360ms)
            // CC 200 is bipolar: 0-63=reverse(-360 to -4ms), 64=center, 65-127=forward(+4 to +360ms)
            int strumSpeed;
            if (sendVal < 64) {
                // Reverse mode: MIDI 0-63 maps to -360ms to -4ms (slow to fast reverse)
                strumSpeed = -map(sendVal, 0, 63, 360, 4);
            } else {
                // Forward mode: MIDI 65-127 maps to +4ms to +360ms (fast to slow forward)
                strumSpeed = map(sendVal, 127, 65, 360, 4);
            }
            _chordSettings.strumSpeed = constrain(strumSpeed, -360, 360);
            SERIAL_PRINT("KB1 Expression: Strum Speed set to "); SERIAL_PRINTLN(_chordSettings.strumSpeed);
            // Notify BLE clients of the change
            if (notifyChordSettingsCallback) {
                notifyChordSettingsCallback();
            }
        } else if (_settings.ccNumber == 201) {
            // 201 = KB1 Expression: Pattern Selector (1-6)
            // Only active when in strum:shape mode
            bool isStrumShapeMode = (_chordSettings.playMode == PlayMode::CHORD && 
                                      _chordSettings.strumEnabled && 
                                      _chordSettings.strumPattern > 0);
            if (!isStrumShapeMode) {
                // Silently ignore when not in strum:shape mode
                SERIAL_PRINTLN("LeverPush CC 201: Ignored (not in strum:shape mode)");
            } else {
                // Use discrete MIDI values to avoid rounding errors
                const int patternMidi[] = {0, 25, 51, 76, 102, 127};
                
                // Find pattern by matching nearest MIDI value
                int pattern = 1;
                int minDiff = 127;
                for (int i = 0; i < 6; i++) {
                    int diff = abs(sendVal - patternMidi[i]);
                    if (diff < minDiff) {
                        minDiff = diff;
                        pattern = i + 1;
                    }
                }
                
                // Detect direction change and set LED (pink=forward, blue=reverse)
                static int lastPattern = -1;
                if (lastPattern != -1 && pattern != lastPattern) {
                    // Handle wraparound: 6->1 is forward, 1->6 is reverse
                    bool isForward = (pattern > lastPattern) || (lastPattern == 6 && pattern == 1);
                    if (lastPattern == 1 && pattern == 6) isForward = false;  // 1->6 wraparound is reverse
                    
                    // Flash on instantly, then fade out over 300ms
                    _ledController.set(isForward ? LedColor::PINK : LedColor::BLUE, 255);  // On immediately
                    _ledController.set(isForward ? LedColor::BLUE : LedColor::PINK, 0);    // Off immediately
                    // Queue the fade-out (will execute after the instant on)
                    _ledController.set(isForward ? LedColor::PINK : LedColor::BLUE, 0, 300);
                }
                lastPattern = pattern;
                
                _chordSettings.strumPattern = constrain(pattern, 1, 6);
                SERIAL_PRINT("P"); SERIAL_PRINTLN(_chordSettings.strumPattern);
            }
        } else if (_settings.ccNumber == 202) {
            // 202 = KB1 Expression: Swing (0-100%)
            // Only active when in strum:shape mode
            bool isStrumShapeMode = (_chordSettings.playMode == PlayMode::CHORD && 
                                      _chordSettings.strumEnabled && 
                                      _chordSettings.strumPattern > 0);
            if (!isStrumShapeMode) {
                // Silently ignore when not in strum:shape mode
                SERIAL_PRINTLN("LP202:Skip");
            } else {
                int swing = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 0, 100);
                _chordSettings.strumSwing = constrain(swing, 0, 100);
                SERIAL_PRINT("SW"); SERIAL_PRINTLN(_chordSettings.strumSwing);
            }
        } else if (_settings.ccNumber == 203) {
            // 203 = KB1 Expression: Velocity Spread (8-100%)
            int velocitySpread = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 10, 100);
            _chordSettings.velocitySpread = constrain(velocitySpread, 10, 100);
            SERIAL_PRINT("VS"); SERIAL_PRINTLN(_chordSettings.velocitySpread);
        } else {
            // Throttle CC output (max once per 300ms to reduce interpolation spam)
            static unsigned long lastPushCCPrint = 0;
            unsigned long now = millis();
            if (now - lastPushCCPrint >= 300) {
                SERIAL_PRINT("C");
                SERIAL_PRINT(_settings.ccNumber);
                SERIAL_PRINT("=");
                SERIAL_PRINTLN(sendVal);
                lastPushCCPrint = now;
            }
            _midi.sendControlChange(_settings.ccNumber, sendVal, 1);
        }
        // _ledController.set(_ledColor, _currentValue);
        _lastSentValue = _currentValue;
    }
}

#endif