#ifndef LEVER_PUSH_CONTROLS_H
#define LEVER_PUSH_CONTROLS_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include "objects/Globals.h"

enum class LeverPushFunctionMode {
    INTERPOLATED,
    JUMP_AND_INTERPOLATE,
    STATIC,
    RESET,
};

template<class MidiTransport>
class LeverPushControls {
public:
    LeverPushControls(
        Adafruit_MCP23X17* mcp,
        int centerPin,
        int ccNumber,
        int minCCValue,
        int maxCCValue,
        LeverPushFunctionMode functionMode,
        unsigned long onsetTime,
        unsigned long offsetTime,
        InterpolationType onsetInterpolationType,
        InterpolationType offsetInterpolationType,
        MidiTransport& midi,
        LeverControls<MidiTransport>& leverControls
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
    MidiTransport& _midi;
    LeverControls<MidiTransport>& _leverControls;
    Adafruit_MCP23X17* _mcp;
    int _centerPin;

    LeverPushFunctionMode _functionMode;
    InterpolationType _onsetInterpolationType;
    InterpolationType _offsetInterpolationType;

    bool _isPressed;
    int _lastSentValue;
    int _currentValue;
    int _targetValue;

    unsigned long _onsetTime;
    unsigned long _offsetTime;
    unsigned long _rampStartTime;
    int _rampStartValue;
    int _ccNumber;
    int _minCCValue;
    int _maxCCValue;

    void handleInput();
    void updateValue();
};

template<class MidiTransport>
LeverPushControls<MidiTransport>::LeverPushControls(
    Adafruit_MCP23X17* mcp,
    int centerPin,
    int ccNumber,
    int minCCValue,
    int maxCCValue,
    LeverPushFunctionMode functionMode,
    unsigned long onsetTime,
    unsigned long offsetTime,
    InterpolationType onsetInterpolationType,
    InterpolationType offsetInterpolationType,
    MidiTransport& midi,
    LeverControls<MidiTransport>& leverControls)
    :
    _leverControls(leverControls),
    _midi(midi),
    _mcp(mcp),
    _centerPin(centerPin),
    _ccNumber(ccNumber),
    _minCCValue(minCCValue),
    _maxCCValue(maxCCValue),
    _functionMode(functionMode),
    _onsetTime(onsetTime),
    _offsetTime(offsetTime),
    _onsetInterpolationType(onsetInterpolationType),
    _offsetInterpolationType(offsetInterpolationType),
    _isPressed(false),
    _rampStartTime(0),
    _currentValue(minCCValue),
    _targetValue(minCCValue),
    _rampStartValue(minCCValue),
    _lastSentValue(minCCValue)
    {

    }

template<class MidiTransport>
void LeverPushControls<MidiTransport>::update() {
    handleInput();
    updateValue();
}

template<class MidiTransport>
void LeverPushControls<MidiTransport>::setFunctionMode(LeverPushFunctionMode functionMode) {
    _functionMode = functionMode;
}

template<class MidiTransport>
void LeverPushControls<MidiTransport>::setOnsetTime(unsigned long time) {
    _onsetTime = time;
}

template<class MidiTransport>
void LeverPushControls<MidiTransport>::setOffsetTime(unsigned long time) {
    _offsetTime = time;
}

template<class MidiTransport>
void LeverPushControls<MidiTransport>::setCCNumber(int number) {
    _ccNumber = number;
}

template<class MidiTransport>
void LeverPushControls<MidiTransport>::setMinCCValue(int number) {
    _minCCValue = number;
}

template<class MidiTransport>
void LeverPushControls<MidiTransport>::setMaxCCValue(int number) {
    _maxCCValue = number;
}

template<class MidiTransport>
void LeverPushControls<MidiTransport>::setOnsetInterpolationType(InterpolationType type) {
    _onsetInterpolationType = type;
}

template<class MidiTransport>
void LeverPushControls<MidiTransport>::setOffsetInterpolationType(InterpolationType type) {
    _offsetInterpolationType = type;
}

template<class MidiTransport>
void LeverPushControls<MidiTransport>::handleInput() {
    bool state = !_mcp->digitalRead(_centerPin);
    int oldTargetValue = _targetValue;

    if (_functionMode == LeverPushFunctionMode::RESET) {
        if (state) {
            _currentValue = _minCCValue;
            _leverControls.setValue(_currentValue);
        }
    } else if (_functionMode == LeverPushFunctionMode::STATIC) {
        if (state && !_isPressed) {
            _currentValue = _maxCCValue;
            _isPressed = true;
            SERIAL_PRINT("LeverPushFunctionMode::STATIC :"); SERIAL_PRINTLN(_currentValue);
        } else if (!state && _isPressed) {
            _currentValue = _minCCValue;
            _isPressed = false;
            SERIAL_PRINT("LeverPushFunctionMode::STATIC :"); SERIAL_PRINTLN(_currentValue);
        }
        _targetValue = _currentValue;
    } else if (_functionMode == LeverPushFunctionMode::JUMP_AND_INTERPOLATE) {
        if (state && !_isPressed) {
            _currentValue = _maxCCValue;
            _targetValue = _maxCCValue;
            _isPressed = true;
            SERIAL_PRINT("LeverPushFunctionMode::JUMP_AND_INTERPOLATE :"); SERIAL_PRINTLN(_currentValue);
        } else if (!state && _isPressed) {
            _targetValue = _minCCValue;
            _isPressed = false;
        }

        if (_targetValue != oldTargetValue) {
            _rampStartTime = millis();
            _rampStartValue = _currentValue;
        }
    } else if (_functionMode == LeverPushFunctionMode::INTERPOLATED) {
        if (state && !_isPressed) {
            _targetValue = _maxCCValue;
            _isPressed = true;
            SERIAL_PRINT("LeverPushFunctionMode::INTERPOLATED :"); SERIAL_PRINTLN(_currentValue);
        } else if (!state && _isPressed) {
            _targetValue = _minCCValue;
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
    if (_functionMode == LeverPushFunctionMode::INTERPOLATED || _functionMode == LeverPushFunctionMode::JUMP_AND_INTERPOLATE) {
        if (_currentValue == _targetValue && _currentValue == _lastSentValue) {
            return;
        }

        unsigned long currentTime = millis();
        unsigned long rampDuration = _isPressed ? _onsetTime : _offsetTime;
        unsigned long elapsedTime = currentTime - _rampStartTime;

        if (rampDuration == 0 || elapsedTime >= rampDuration) {
            _currentValue = _targetValue;
        } else {
            float progress = (float)elapsedTime / (float)rampDuration;

            if (_isPressed) {
                if (_onsetInterpolationType == InterpolationType::ACCELERATING) {
                    progress = pow(progress, 2);
                } else if (_onsetInterpolationType == InterpolationType::DECELERATING) {
                    progress = 1 - pow(1 - progress, 2);
                }
            } else {
                if (_offsetInterpolationType == InterpolationType::ACCELERATING) {
                    progress = pow(progress, 2);
                } else if (_offsetInterpolationType == InterpolationType::DECELERATING) {
                    progress = 1 - pow(1 - progress, 2);
                }
            }

            float totalValueChange = _targetValue - _rampStartValue;
            _currentValue = _rampStartValue + (int)(progress * totalValueChange);
        }
    }

    if (_currentValue != _lastSentValue) {
        SERIAL_PRINT("Sending CC "); SERIAL_PRINT(_ccNumber);
        SERIAL_PRINT(", Value: "); SERIAL_PRINTLN(_currentValue);
        _midi.sendControlChange(_ccNumber, _currentValue, 1);
        _lastSentValue = _currentValue;
    }
}

#endif