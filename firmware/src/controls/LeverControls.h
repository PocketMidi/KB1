#ifndef LEVER_CONTROLS_H
#define LEVER_CONTROLS_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>

enum class LeverMode {
    UNIPOLAR,
    BIPOLAR
};

enum class LeverFunctionMode {
    INTERPOLATED,
    JUMP_AND_INTERPOLATE,
    INCREMENTAL,
};

enum class InterpolationType {
    LINEAR,
    ACCELERATING,
    DECELERATING
};

template<class MidiTransport>
class LeverControls {
public:
    LeverControls(
        Adafruit_MCP23X17* mcpLeft,
        int leftPin,
        Adafruit_MCP23X17* mcpRight,
        int rightPin,
        int ccNumber,
        int minCCValue,
        int maxCCValue,
        int stepSize,
        LeverFunctionMode functionMode,
        LeverMode mode,
        unsigned long onsetTime,
        unsigned long offsetTime,
        InterpolationType onsetInterpolationType,
        InterpolationType offsetInterpolationType,
        MidiTransport& midi
    );

    void update();
    void setFunctionMode(LeverFunctionMode functionMode);
    void setLeverMode(LeverMode leverMode);
    void setOnsetTime(unsigned long time);
    void setOffsetTime(unsigned long time);
    void setCCNumber(int number);
    void setMinCCValue(int number);
    void setMaxCCValue(int number);
    void setStepSize(int size);
    void setOnsetInterpolationType(InterpolationType type);
    void setOffsetInterpolationType(InterpolationType type);

private:
    MidiTransport& _midi;
    Adafruit_MCP23X17* _mcpLeft;
    Adafruit_MCP23X17* _mcpRight;
    int _leftPin;
    int _rightPin;
    int _ccNumber;
    LeverMode _leverMode;
    LeverFunctionMode _functionMode;
    unsigned long _onsetTime;
    unsigned long _offsetTime;
    InterpolationType _onsetInterpolationType;
    InterpolationType _offsetInterpolationType;

    int _currentValue;
    int _targetValue;
    bool _isPressed;

    unsigned long _rampStartTime;
    int _rampStartValue;

    int _minCCValue;
    int _maxCCValue;

    int _stepSize;

    int _lastSentValue;

    void handleInput();
    void updateValue();
};

template<class MidiTransport>
LeverControls<MidiTransport>::LeverControls(
    Adafruit_MCP23X17* mcpLeft,
    int leftPin,
    Adafruit_MCP23X17* mcpRight,
    int rightPin,
    int ccNumber,
    int minCCValue,
    int maxCCValue,
    int stepSize,
    LeverFunctionMode functionMode,
    LeverMode mode,
    unsigned long onsetTime,
    unsigned long offsetTime,
    InterpolationType onsetInterpolationType,
    InterpolationType offsetInterpolationType,
    MidiTransport& midi)
    :
    _midi(midi),
    _mcpLeft(mcpLeft),
    _leftPin(leftPin),
    _mcpRight(mcpRight),
    _rightPin(rightPin),
    _ccNumber(ccNumber),
    _minCCValue(minCCValue),
    _maxCCValue(maxCCValue),
    _stepSize(stepSize),
    _functionMode(functionMode),
    _leverMode(mode),
    _onsetTime(onsetTime),
    _offsetTime(offsetTime),
    _onsetInterpolationType(onsetInterpolationType),
    _offsetInterpolationType(offsetInterpolationType),
    _isPressed(false),
    _rampStartTime(0)
    {
        if (_leverMode == LeverMode::BIPOLAR) {
            _currentValue = 64;
            _targetValue = 64;
            _rampStartValue = 64;
        } else {
            _currentValue = _minCCValue;
            _targetValue = _minCCValue;
            _rampStartValue = _minCCValue;
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
    _functionMode = functionMode;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::setLeverMode(LeverMode leverMode) {
    _leverMode = leverMode;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::setOnsetTime(unsigned long time) {
    _onsetTime = time;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::setOffsetTime(unsigned long time) {
    _offsetTime = time;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::setCCNumber(int number) {
    _ccNumber = number;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::setMinCCValue(int number) {
    _minCCValue = number;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::setMaxCCValue(int number) {
    _maxCCValue = number;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::setStepSize(int size) {
    _stepSize = size;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::setOnsetInterpolationType(InterpolationType type) {
    _onsetInterpolationType = type;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::setOffsetInterpolationType(InterpolationType type) {
    _offsetInterpolationType = type;
}

template<class MidiTransport>
void LeverControls<MidiTransport>::handleInput() {
    bool leftState = !_mcpLeft->digitalRead(_leftPin);
    bool rightState = !_mcpRight->digitalRead(_rightPin);
    int oldTargetValue = _targetValue;

    if (_functionMode == LeverFunctionMode::INCREMENTAL) {
        if (leftState && !_isPressed) {
            _currentValue = max(_minCCValue, _currentValue - _stepSize);
            _isPressed = true;
        } else if (rightState && !_isPressed) {
            _currentValue = min(_maxCCValue, _currentValue + _stepSize);
            _isPressed = true;
        } else if (!leftState && !rightState && _isPressed) {
            _isPressed = false;
        }
        _targetValue = _currentValue;
    } else if (_functionMode == LeverFunctionMode::JUMP_AND_INTERPOLATE) {
        if (leftState) {
            _currentValue = _minCCValue;
            _targetValue = _minCCValue;
            _isPressed = true;
        } else if (rightState) {
            _currentValue = _maxCCValue;
            _targetValue = _maxCCValue;
            _isPressed = true;
        } else {
            if (_isPressed) {
                if (_leverMode == LeverMode::BIPOLAR) {
                    _targetValue = 64;
                } else if (_leverMode == LeverMode::UNIPOLAR) {
                    _targetValue = _minCCValue;
                }
                _isPressed = false;
            }
        }

        if (_targetValue != oldTargetValue) {
            _rampStartTime = millis();
            _rampStartValue = _currentValue;
        }
    } else if (_functionMode == LeverFunctionMode::INTERPOLATED) {
        if (leftState) {
            _targetValue = _minCCValue;
            _isPressed = true;
        } else if (rightState) {
            _targetValue = _maxCCValue;
            _isPressed = true;
        }
        else {
            if (_isPressed) {
                if (_leverMode == LeverMode::BIPOLAR) {
                    _targetValue = 64;
                } else if (_leverMode == LeverMode::UNIPOLAR) {
                    _targetValue = _minCCValue;
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
    if (_functionMode == LeverFunctionMode::INTERPOLATED) {
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