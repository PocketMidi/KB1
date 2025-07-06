#ifndef LEVER_CONTROLS_H
#define LEVER_CONTROLS_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>

#include <controls/OctaveControl.h>
#include <controls/KeyboardControl.h>
#include <objects/Settings.h>

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
        KeyboardControl<MidiTransport, OctaveControl<Adafruit_MCP23X17, LEDController>>& keyboardControl
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
    KeyboardControl<MidiTransport, OctaveControl<Adafruit_MCP23X17, LEDController>>& keyboardControl)
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
    _isPressed(false),
    _rampStartTime(0)
    {
        if (_settings.valueMode == ValueMode::BIPOLAR) {
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
void LeverControls<MidiTransport>::update() {
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
    _settings.ccNumber = number;
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
void LeverControls<MidiTransport>::handleInput() {
    bool leftState = !_mcpLeft->digitalRead(_leftPin);
    bool rightState = !_mcpRight->digitalRead(_rightPin);
    int oldTargetValue = _targetValue;

    if (_settings.functionMode == LeverFunctionMode::INCREMENTAL) {
        if (leftState && !_isPressed) {
            _currentValue = max(_settings.minCCValue, _currentValue - _settings.stepSize);
            _isPressed = true;
        } else if (rightState && !_isPressed) {
            _currentValue = min(_settings.maxCCValue, _currentValue + _settings.stepSize);
            _isPressed = true;
        } else if (!leftState && !rightState && _isPressed) {
            _isPressed = false;
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
                    _targetValue = 64;
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
                    _targetValue = 64;
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

    if (_currentValue != _lastSentValue) {
        SERIAL_PRINT("Sending CC "); SERIAL_PRINT(_settings.ccNumber);
        SERIAL_PRINT(", Value: "); SERIAL_PRINTLN(_currentValue);
        _midi.sendControlChange(_settings.ccNumber, _currentValue, 1);
        if (_settings.ccNumber == 7) {
            _keyboardControl.setVelocity(_currentValue);
        }

        // _ledController.set(_ledColor, _currentValue);
        _lastSentValue = _currentValue;
    }
}

#endif