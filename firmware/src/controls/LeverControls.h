#ifndef LEVER_CONTROLS_H
#define LEVER_CONTROLS_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>

#include <controls/OctaveControl.h>
#include <controls/KeyboardControl.h>
#include <objects/Settings.h>

class ScaleManager;  // Forward declaration

template<class MidiTransport>
class LeverControls {
public:
    LeverControls(
        Adafruit_MCP23X17* mcpLeft,
        Adafruit_MCP23X17* mcpRight,
        int leftPin,
        int rightPin,
        LeverSettings& settings,
        MidiTransport& midi,
        LEDController& ledController,
        LedColor ledColor,
        KeyboardControl<MidiTransport, OctaveControl<Adafruit_MCP23X17, LEDController>>& keyboardControl,
        ChordSettings& chordSettings,
        ScaleManager& scaleManager
    );

    void update();
    void setFunctionMode(LeverFunctionMode functionMode);
    void setValueMode(ValueMode valueMode);
    void setOnsetTime(unsigned long time);
    void setOffsetTime(unsigned long time);
    void setCCNumber(int number);
    int getCCNumber();
    void setMinCCValue(int number);
    void setMaxCCValue(int number);
    void setStepSize(int size);
    void setOnsetInterpolationType(InterpolationType type);
    void setOffsetInterpolationType(InterpolationType type);
    void setValue(int value);
    void syncValue(); // Re-sync internal value when settings change

private:
    MidiTransport& _midi;
    Adafruit_MCP23X17* _mcpLeft;
    Adafruit_MCP23X17* _mcpRight;
    int _leftPin;
    int _rightPin;
    LeverSettings& _settings;
    KeyboardControl<MidiTransport, OctaveControl<Adafruit_MCP23X17, LEDController>>& _keyboardControl;
    LEDController& _ledController;
    LedColor _ledColor;
    ChordSettings& _chordSettings;
    ScaleManager& _scaleManager;

    bool _isPressed;
    int _lastSentValue;
    int _currentValue;
    int _targetValue;

    unsigned long _rampStartTime;
    int _rampStartValue;

    void handleInput();
    void updateValue();
};

template<class MidiTransport>
LeverControls<MidiTransport>::LeverControls(
    Adafruit_MCP23X17* mcpLeft,
    Adafruit_MCP23X17* mcpRight,
    int leftPin,
    int rightPin,
    LeverSettings& settings,
    MidiTransport& midi,
    LEDController& ledController,
    LedColor ledColor,
    KeyboardControl<MidiTransport, OctaveControl<Adafruit_MCP23X17, LEDController>>& keyboardControl,
    ChordSettings& chordSettings,
    ScaleManager& scaleManager)
    :
    _midi(midi),
    _mcpLeft(mcpLeft),
    _mcpRight(mcpRight),
    _leftPin(leftPin),
    _rightPin(rightPin),
    _settings(settings),
    _ledController(ledController),
    _ledColor(ledColor),
    _keyboardControl(keyboardControl),
    _chordSettings(chordSettings),
    _scaleManager(scaleManager),
    _isPressed(false),
    _rampStartTime(0)
    {
        // Default initialization
        if (_settings.valueMode == ValueMode::BIPOLAR) {
            _currentValue = 64;
            _targetValue = 64;
            _rampStartValue = 64;
        } else {
            _currentValue = _settings.minCCValue;
            _targetValue = _settings.minCCValue;
            _rampStartValue = _settings.minCCValue;
        }
        // If this lever is configured to control velocity (internal marker ccNumber==128),
        // initialize its current/target/ramp values from the keyboard's current velocity
        // so it starts in sync instead of defaulting to 64.
        if (_settings.ccNumber == 128) {
            int vel = _keyboardControl.getVelocity();
            _currentValue = vel;
            _targetValue = vel;
            _rampStartValue = vel;
            SERIAL_PRINT("L128="); SERIAL_PRINTLN(vel);
        }
        // CC 200 (Strum Speed) syncs from current strumSpeed value
        else if (_settings.ccNumber == 200) {
            int speedMs = abs(_chordSettings.strumSpeed);
            int midiValue = map(speedMs, 5, 360, 127, 0);  // Reversed: lower ms = higher MIDI
            _currentValue = midiValue;
            _targetValue = midiValue;
            _rampStartValue = midiValue;
            SERIAL_PRINT("L200="); SERIAL_PRINT(midiValue); 
            SERIAL_PRINT("("); SERIAL_PRINT(speedMs); SERIAL_PRINTLN("ms)");
        }

        _lastSentValue = _currentValue;
    }

template<class MidiTransport>
void LeverControls<MidiTransport>::update() {
    // Skip lever processing during cooldown (after BLE toggle)
    if (millis() < leverCooldownUntil) {
        return;
    }
    handleInput();
    updateValue();
}

template<class MidiTransport>
void LeverControls<MidiTransport>::setFunctionMode(LeverFunctionMode functionMode) {
    _settings.functionMode = functionMode;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::setValueMode(ValueMode valueMode) {
    _settings.valueMode = valueMode;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::setOnsetTime(unsigned long time) {
    _settings.onsetTime = time;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::setOffsetTime(unsigned long time) {
    _settings.offsetTime = time;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::setCCNumber(int number) {
    int oldCC =_settings.ccNumber;
    _settings.ccNumber = number;
    
    // When CC changes TO 200, sync from current strumSpeed magnitude
    if (number == 200 && oldCC != 200) {
        int speedMs = abs(_chordSettings.strumSpeed);
        int midiValue = map(speedMs, 5, 360, 127, 0);  // Reversed: lower ms = higher MIDI
        _currentValue = midiValue;
        _targetValue = midiValue;
        _rampStartValue = midiValue;
        SERIAL_PRINT("Lever assigned to CC 200, synced to MIDI: ");
        SERIAL_PRINTLN(midiValue);
    }
}

template<class MidiTransport>
int LeverControls<MidiTransport>::getCCNumber() {
    return _settings.ccNumber;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::setMinCCValue(int number) {
    _settings.minCCValue = number;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::setMaxCCValue(int number) {
    _settings.maxCCValue = number;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::setStepSize(int size) {
    SERIAL_PRINT("LeverControls::setStepSize called: "); SERIAL_PRINTLN(size);
    _settings.stepSize = size;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::setOnsetInterpolationType(InterpolationType type) {
    _settings.onsetType = type;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::setOffsetInterpolationType(InterpolationType type) {
    _settings.offsetType = type;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::setValue(int value) {
    _currentValue = value;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::syncValue() {
   // Re-initialize value based on current settings (called after BLE updates)
    if (_settings.ccNumber == 128) {
        int vel = _keyboardControl.getVelocity();
        _currentValue = vel;
        _targetValue = vel;
        _rampStartValue = vel;
    }
    // CC 200 (Strum Speed) syncs from current strumSpeed magnitude
    else if (_settings.ccNumber == 200) {
        int speedMs = abs(_chordSettings.strumSpeed);
        int midiValue = map(speedMs, 5, 360, 127, 0);  // Reversed: lower ms = higher MIDI
        _currentValue = midiValue;
        _targetValue = midiValue;
        _rampStartValue = midiValue;
    }
    else if (_settings.valueMode == ValueMode::BIPOLAR) {
        _currentValue = 64;
        _targetValue = 64;
        _rampStartValue = 64;
    } else {
        _currentValue = _settings.minCCValue;
        _targetValue = _settings.minCCValue;
        _rampStartValue = _settings.minCCValue;
    }
    
    _lastSentValue = _currentValue;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::handleInput() {
    bool leftState = !_mcpLeft->digitalRead(_leftPin);
    bool rightState = !_mcpRight->digitalRead(_rightPin);
    int oldTargetValue = _targetValue;

    if (_settings.functionMode == LeverFunctionMode::INCREMENTAL) {
        // Special handling for discrete cycling parameters (204, 205, 206)
        // Step in discrete space instead of MIDI space for accurate control
        if (_settings.ccNumber == 204) {
            // Scale Type (0-20): Apply directly to avoid MIDI remapping errors
            if (leftState && !_isPressed) {
                int currentScale = (int)_scaleManager.getScaleType();
                currentScale = max(0, currentScale - 1);
                _scaleManager.setScale((ScaleType)currentScale);
                _currentValue = map(currentScale, 0, 20, _settings.minCCValue, _settings.maxCCValue);
                _lastSentValue = _currentValue; // Skip send() remapping
                _isPressed = true;
                SERIAL_PRINT("ScaleType="); SERIAL_PRINTLN(currentScale);
                if (notifyScaleSettingsCallback) {
                    notifyScaleSettingsCallback();
                }
            } else if (rightState && !_isPressed) {
                int currentScale = (int)_scaleManager.getScaleType();
                currentScale = min(20, currentScale + 1);
                _scaleManager.setScale((ScaleType)currentScale);
                _currentValue = map(currentScale, 0, 20, _settings.minCCValue, _settings.maxCCValue);
                _lastSentValue = _currentValue;
                _isPressed = true;
                SERIAL_PRINT("ScaleType="); SERIAL_PRINTLN(currentScale);
                if (notifyScaleSettingsCallback) {
                    notifyScaleSettingsCallback();
                }
            } else if (!leftState && !rightState && _isPressed) {
                _isPressed = false;
            }
        } else if (_settings.ccNumber == 205) {
            // Chord Type (0-14): Apply directly to avoid MIDI remapping errors
            if (leftState && !_isPressed) {
                int currentChord = (int)_chordSettings.chordType;
                currentChord = max(0, currentChord - 1);
                _chordSettings.chordType = (ChordType)currentChord;
                _currentValue = map(currentChord, 0, 14, _settings.minCCValue, _settings.maxCCValue);
                _lastSentValue = _currentValue;
                _isPressed = true;
                SERIAL_PRINT("ChordType="); SERIAL_PRINTLN(currentChord);
                if (notifyChordSettingsCallback) {
                    notifyChordSettingsCallback();
                }
            } else if (rightState && !_isPressed) {
                int currentChord = (int)_chordSettings.chordType;
                currentChord = min(14, currentChord + 1);
                _chordSettings.chordType = (ChordType)currentChord;
                _currentValue = map(currentChord, 0, 14, _settings.minCCValue, _settings.maxCCValue);
                _lastSentValue = _currentValue;
                _isPressed = true;
                SERIAL_PRINT("ChordType="); SERIAL_PRINTLN(currentChord);
                if (notifyChordSettingsCallback) {
                    notifyChordSettingsCallback();
                }
            } else if (!leftState && !rightState && _isPressed) {
                _isPressed = false;
            }
        } else if (_settings.ccNumber == 206) {
            // Root Note (0-11): Apply directly to avoid MIDI remapping errors
            // Root note is stored as MIDI note 60-71 (C through B)
            if (leftState && !_isPressed) {
                int currentRoot = _scaleManager.getRootNote();
                currentRoot = max(60, currentRoot - 1);
                _scaleManager.setRootNote(currentRoot);
                _currentValue = map(currentRoot, 60, 71, _settings.minCCValue, _settings.maxCCValue);
                _lastSentValue = _currentValue;
                _isPressed = true;
                SERIAL_PRINT("RootNote="); SERIAL_PRINTLN(currentRoot);
                if (notifyScaleSettingsCallback) {
                    notifyScaleSettingsCallback();
                }
            } else if (rightState && !_isPressed) {
                int currentRoot = _scaleManager.getRootNote();
                currentRoot = min(71, currentRoot + 1);
                _scaleManager.setRootNote(currentRoot);
                _currentValue = map(currentRoot, 60, 71, _settings.minCCValue, _settings.maxCCValue);
                _lastSentValue = _currentValue;
                _isPressed = true;
                SERIAL_PRINT("RootNote="); SERIAL_PRINTLN(currentRoot);
                if (notifyScaleSettingsCallback) {
                    notifyScaleSettingsCallback();
                }
            } else if (!leftState && !rightState && _isPressed) {
                _isPressed = false;
            }
        } else {
            // Normal INCREMENTAL mode: step in MIDI space
            if (leftState && !_isPressed) {
                _currentValue = max(_settings.minCCValue, _currentValue - _settings.stepSize);
                _isPressed = true;
                SERIAL_PRINT("L");
                SERIAL_PRINT(_currentValue);
                SERIAL_PRINTLN("<");
            } else if (rightState && !_isPressed) {
                _currentValue = min(_settings.maxCCValue, _currentValue + _settings.stepSize);
                _isPressed = true;
                SERIAL_PRINT("L");
                SERIAL_PRINT(_currentValue);
                SERIAL_PRINTLN(">");
            } else if (!leftState && !rightState && _isPressed) {
                _isPressed = false;
            }
        }
        _targetValue = _currentValue;
    } else if (_settings.functionMode == LeverFunctionMode::PEAK_AND_DECAY) {
        if (leftState) {
            _currentValue = _settings.minCCValue;
            _targetValue = _settings.minCCValue;
            _isPressed = true;
        } else if (rightState) {
            _currentValue = _settings.maxCCValue;
            _targetValue = _settings.maxCCValue;
            _isPressed = true;
        } else {
            if (_isPressed) {
                if (_settings.valueMode == ValueMode::BIPOLAR) {
                    _targetValue = (_settings.minCCValue + _settings.maxCCValue) / 2;
                } else if (_settings.valueMode == ValueMode::UNIPOLAR) {
                    _targetValue = _settings.minCCValue;
                }
                _isPressed = false;
            }
        }

        if (_targetValue != oldTargetValue) {
            _rampStartTime = millis();
            _rampStartValue = _currentValue;
        }
    } else if (_settings.functionMode == LeverFunctionMode::INTERPOLATED) {
        if (leftState) {
            _targetValue = _settings.minCCValue;
            _isPressed = true;
        } else if (rightState) {
            _targetValue = _settings.maxCCValue;
            _isPressed = true;
        }
        else {
            if (_isPressed) {
                if (_settings.valueMode == ValueMode::BIPOLAR) {
                    _targetValue = (_settings.minCCValue + _settings.maxCCValue) / 2;
                } else if (_settings.valueMode == ValueMode::UNIPOLAR) {
                    _targetValue = _settings.minCCValue;
                }
            }
            _isPressed = false;
        }

        if (_targetValue != oldTargetValue) {
            _rampStartTime = millis();
            _rampStartValue = _currentValue;
        }
    } else if (_settings.functionMode == LeverFunctionMode::PITCH_BEND || _settings.ccNumber == 208) {
        // PITCH_BEND: left=min semitone, right=max semitone, center=0 on release
        // Uses minCCValue/maxCCValue so the user-configured range is respected
        if (leftState) {
            _targetValue = _settings.minCCValue;
            _isPressed = true;
        } else if (rightState) {
            _targetValue = _settings.maxCCValue;
            _isPressed = true;
        } else {
            if (_isPressed) {
                _targetValue = 64; // Always return to semitone 0 (center)
            }
            _isPressed = false;
        }
    }
}

template<class MidiTransport>
void LeverControls<MidiTransport>::updateValue() {
    if (_settings.functionMode == LeverFunctionMode::INTERPOLATED || _settings.functionMode == LeverFunctionMode::PEAK_AND_DECAY) {
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

    if (_settings.functionMode == LeverFunctionMode::PITCH_BEND || _settings.ccNumber == 208) {
        // Bypass interpolation — map _targetValue directly to semitone offset
        if (_targetValue == _lastSentValue) return;
        
        // Convert MIDI CC values to cents (-100..+100)
        auto midiToCents = [](int midi) -> int {
            return round((midi / 127.0f) * 200.0f) - 100;
        };
        
        // Get cents bounds from user's configured min/max
        int minCents = midiToCents(_settings.minCCValue);
        int maxCents = midiToCents(_settings.maxCCValue);
        
        // Map current lever position to target cents within configured range
        float normalizedPosition = 0.0;
        if (_settings.maxCCValue > _settings.minCCValue) {
            normalizedPosition = (float)(_targetValue - _settings.minCCValue) / 
                                (float)(_settings.maxCCValue - _settings.minCCValue);
            normalizedPosition = constrain(normalizedPosition, 0.0, 1.0);
        }
        
        float mappedCents = minCents + (normalizedPosition * (maxCents - minCents));
        int snappedCents = (int)round(mappedCents / 10.0f) * 10;
        snappedCents = constrain(snappedCents, minCents, maxCents);

        // Note retrigger at semitone boundaries: threshold at ±50 cents.
        int semitoneOffset = (snappedCents >= 50) ? 1 : (snappedCents <= -50) ? -1 : 0;

        // Use onsetTime (ms) as the note overlap duration. Default 25ms if unset.
        unsigned long overlapUs = (_settings.onsetTime > 0)
            ? (unsigned long)_settings.onsetTime * 1000UL
            : 25000UL;
        _keyboardControl.setPitchBendOverlapUs(overlapUs);
        _keyboardControl.setPitchBendOffset(semitoneOffset);
        _currentValue = _targetValue;
        _lastSentValue = _targetValue;
        return;
    }

    if (_currentValue != _lastSentValue) {
        int sendVal = constrain(_currentValue, 0, 127);
        if (_settings.ccNumber == 128) {
            // 128 is reserved as an internal marker for velocity control.
            // Do NOT send a Control Change for velocity — velocity is part
            // of Note On messages. Instead update the keyboard's velocity
            // and update the cached sent value to avoid repeats.
            SERIAL_PRINT("V");
            SERIAL_PRINTLN(sendVal);
            _keyboardControl.setVelocity(sendVal);
        } else if (_settings.ccNumber == 200) {
            // 200 = KB1 Expression: Strum Speed (unipolar magnitude, direction set via UI)
            // MIDI 0-127 maps to 360-5ms magnitude (REVERSED: right lever = faster = lower ms)
            // Direction (sign) is controlled via web UI FWD/REV toggle
            int speedMs = map(sendVal, 0, 127, 360, 5);
            // Preserve current direction sign
            if (_chordSettings.strumSpeed < 0) {
                _chordSettings.strumSpeed = -speedMs;
            } else {
                _chordSettings.strumSpeed = speedMs;
            }
            SERIAL_PRINT("SS"); SERIAL_PRINTLN(_chordSettings.strumSpeed);
            // Notify BLE clients of the change
            if (notifyChordSettingsCallback) {
                notifyChordSettingsCallback();
            }
        } else if (_settings.ccNumber == 201) {
            // 201 = KB1 Expression: Pattern Selector (1-6)
            // Only active when in arp:pattern mode
            bool isStrumShapeMode = (_chordSettings.playMode == PlayMode::CHORD && 
                                      _chordSettings.strumEnabled && 
                                      _chordSettings.strumPattern > 0);
            if (!isStrumShapeMode) {
                // Silently ignore when not in arp:pattern mode
                SERIAL_PRINTLN("L201:Skip");
            } else {
                // Map CC value (0-127) to pattern range (1-6)
                int pattern = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 1, 6);
                _chordSettings.strumPattern = constrain(pattern, 1, 6);
                SERIAL_PRINT("P"); SERIAL_PRINTLN(_chordSettings.strumPattern);
            }
        } else if (_settings.ccNumber == 202) {
            // 202 = KB1 Expression: Swing (0-50 firmware, 50-100% UI)
            // Active in arp:pattern mode only
            bool isStrumShapeMode = (_chordSettings.playMode == PlayMode::CHORD && 
                                      _chordSettings.strumEnabled && 
                                      _chordSettings.strumPattern > 0);
            bool isArpMode = (_chordSettings.playMode == PlayMode::ARP);
            if (!isStrumShapeMode && !isArpMode) {
                // Silently ignore when not in a swing-capable mode
                SERIAL_PRINTLN("L202:Skip");
            } else {
                // Map CC value (0-127) to swing range (0-50 firmware, 50-100% UI)
                int swing = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 0, 50);
                _chordSettings.strumSwing = constrain(swing, 0, 50);
                SERIAL_PRINT("SW"); SERIAL_PRINTLN(_chordSettings.strumSwing);
            }
        } else if (_settings.ccNumber == 203) {
            // 203 = KB1 Expression: Velocity Spread (8-100%)
            // Map CC value (0-127) to velocity spread range (8-100)
            int velocitySpread = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 10, 100);
            _chordSettings.velocitySpread = constrain(velocitySpread, 10, 100);
            SERIAL_PRINT("VS"); SERIAL_PRINTLN(_chordSettings.velocitySpread);
        } else if (_settings.ccNumber == 204) {
            // 204 = KB1 Expression: Scale Type (0-20)
            // Map user's CC range to scale type 0-20
            int scaleIndex = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 0, 20);
            scaleIndex = constrain(scaleIndex, 0, 20);
            _scaleManager.setScale((ScaleType)scaleIndex);
            SERIAL_PRINT("ScaleType="); SERIAL_PRINTLN(scaleIndex);
            // Notify BLE clients of the change
            if (notifyScaleSettingsCallback) {
                notifyScaleSettingsCallback();
            }
        } else if (_settings.ccNumber == 205) {
            // 205 = KB1 Expression: Chord Type (0-14)
            // Map user's CC range to chord type 0-14
            int chordIndex = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 0, 14);
            chordIndex = constrain(chordIndex, 0, 14);
            _chordSettings.chordType = (ChordType)chordIndex;
            SERIAL_PRINT("ChordType="); SERIAL_PRINTLN(chordIndex);
            // Notify BLE clients of the change
            if (notifyChordSettingsCallback) {
                notifyChordSettingsCallback();
            }
        } else if (_settings.ccNumber == 206) {
            // 206 = KB1 Expression: Root Note (60-71, MIDI notes C through B)
            // Map user's CC range to root note 60-71
            int rootNote = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 60, 71);
            rootNote = constrain(rootNote, 60, 71);
            _scaleManager.setRootNote(rootNote);
            SERIAL_PRINT("RootNote="); SERIAL_PRINTLN(rootNote);
            // Notify BLE clients of the change
            if (notifyScaleSettingsCallback) {
                notifyScaleSettingsCallback();
            }
        } else {
            // Throttle CC output (max once per 300ms to reduce interpolation spam)
            static unsigned long lastLeverCCPrint = 0;
            unsigned long now = millis();
            if (now - lastLeverCCPrint >= 300) {
                SERIAL_PRINT("C");
                SERIAL_PRINT(_settings.ccNumber);
                SERIAL_PRINT("=");
                SERIAL_PRINTLN(sendVal);
                lastLeverCCPrint = now;
            }
            _midi.sendControlChange(_settings.ccNumber, sendVal, 1);
        }

        // _ledController.set(_ledColor, sendVal);
        _lastSentValue = _currentValue;
    }
}

#endif