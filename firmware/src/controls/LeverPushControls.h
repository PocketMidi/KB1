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
        KeyboardControl<MidiTransport, OctaveControl<Adafruit_MCP23X17, LEDController>>& keyboardControl
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

private:
    Adafruit_MCP23X17* _mcp;
    int _centerPin;
    LeverPushSettings& _settings;
    LeverControls<MidiTransport>& _leverControls;
    MidiTransport& _midi;
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
LeverPushControls<MidiTransport>::LeverPushControls(
    Adafruit_MCP23X17* mcp,
    int centerPin,
    LeverPushSettings& settings,
    LeverControls<MidiTransport>& leverControls,
    MidiTransport& midi,
    LEDController& ledController,
    LedColor ledColor,
    KeyboardControl<MidiTransport, OctaveControl<Adafruit_MCP23X17, LEDController>>& keyboardControl)
    :
    _mcp(mcp),
    _centerPin(centerPin),
    _settings(settings),
    _leverControls(leverControls),
    _midi(midi),
    _ledController(ledController),
    _ledColor(ledColor),
    _keyboardControl(keyboardControl),
    _isPressed(false),
    _rampStartTime(0),
    _currentValue(_settings.minCCValue),
    _targetValue(_settings.minCCValue),
    _rampStartValue(_settings.minCCValue),
    _lastSentValue(_settings.minCCValue)
    {}

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
void LeverPushControls<MidiTransport>::handleInput() {
    bool state = !_mcp->digitalRead(_centerPin);
    int oldTargetValue = _targetValue;

    if (_settings.functionMode == LeverPushFunctionMode::RESET) {
        if (state) {
            _currentValue = _settings.minCCValue;
            if (_settings.ccNumber == 128) {
                _keyboardControl.setVelocity(_currentValue);
                if (_leverControls.getCCNumber() == 128) {
                    _leverControls.setValue(_currentValue);
                }
            } else {
                _leverControls.setValue(_currentValue);
            }
        }
    } else if (_settings.functionMode == LeverPushFunctionMode::STATIC) {
        if (state && !_isPressed) {
            _currentValue = _settings.maxCCValue;
            _isPressed = true;
            SERIAL_PRINT("LeverPushFunctionMode::STATIC :"); SERIAL_PRINTLN(_currentValue);
        } else if (!state && _isPressed) {
            _currentValue = _settings.minCCValue;
            _isPressed = false;
            SERIAL_PRINT("LeverPushFunctionMode::STATIC :"); SERIAL_PRINTLN(_currentValue);
        }
        _targetValue = _currentValue;
    } else if (_settings.functionMode == LeverPushFunctionMode::PEAK_AND_DECAY) {
        if (state && !_isPressed) {
            _currentValue = _settings.maxCCValue;
            _targetValue = _settings.maxCCValue;
            _isPressed = true;
            SERIAL_PRINT("LeverPushFunctionMode::JUMP_AND_INTERPOLATE :"); SERIAL_PRINTLN(_currentValue);
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
            SERIAL_PRINT("LeverPushFunctionMode::INTERPOLATED :"); SERIAL_PRINTLN(_currentValue);
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
        SERIAL_PRINT("Sending CC "); SERIAL_PRINT(_settings.ccNumber);
        SERIAL_PRINT(", Value: "); SERIAL_PRINTLN(_currentValue);
        _midi.sendControlChange(_settings.ccNumber, _currentValue, 1);
        if (_settings.ccNumber == 128) {
            _keyboardControl.setVelocity(_currentValue);
        }
        // _ledController.set(_ledColor, _currentValue);
        _lastSentValue = _currentValue;
    }
}

#endif