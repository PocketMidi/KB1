#ifndef LEVER_CONTROLS_H
#define LEVER_CONTROLS_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>

#include <MIDI.h>

enum class FunctionMode {
    INTERPOLATED,
    MANUAL
};

enum class LeverMode {
    UNIPOLAR,
    BIPOLAR
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
        FunctionMode functionMode,
        LeverMode mode,
        unsigned long onsetTime,
        unsigned long offsetTime,
        MidiTransport& midi
    );

    void update();
    void setFunctionMode(FunctionMode functionMode);
    void setLeverMode(LeverMode leverMode);
    void setOnsetTime(unsigned long time);
    void setOffsetTime(unsigned long time);
    void setCCNumber(int number);
    void setMinCCValue(int number);
    void setMaxCCValue(int number);
    void setStepSize(int size);

private:
    MidiTransport& _midi;
    Adafruit_MCP23X17* _mcpLeft;
    Adafruit_MCP23X17* _mcpRight;
    int _leftPin;
    int _rightPin;
    int _ccNumber;
    LeverMode _leverMode;
    FunctionMode _functionMode;
    unsigned long _onsetTime;
    unsigned long _offsetTime;

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
    FunctionMode functionMode,
    LeverMode mode,
    unsigned long onsetTime,
    unsigned long offsetTime,
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
void LeverControls<MidiTransport>::setFunctionMode(FunctionMode functionMode) {
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
void LeverControls<MidiTransport>::handleInput() {
    bool leftState = !_mcpLeft->digitalRead(_leftPin);
    bool rightState = !_mcpRight->digitalRead(_rightPin);
    int oldTargetValue = _targetValue;

    if (_functionMode == FunctionMode::MANUAL) {
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
    } else {
        if (leftState) {
            _targetValue = _minCCValue;
            _isPressed = true;
        } else if (rightState) {
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
    }
}

template<class MidiTransport>
void LeverControls<MidiTransport>::updateValue() {
    if (_functionMode == FunctionMode::INTERPOLATED) {
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
            float totalValueChange = _targetValue - _rampStartValue;
            _currentValue = _rampStartValue + (int)(progress * totalValueChange);
        }
    }

    if (_currentValue != _lastSentValue) {
        SERIAL_PRINT("Sending CC ");
        SERIAL_PRINT(_ccNumber);
        SERIAL_PRINT(", Value: ");
        SERIAL_PRINTLN(_currentValue);
        _midi.sendControlChange(_ccNumber, _currentValue, 1);
        _lastSentValue = _currentValue;
    }
}

#endif //LEVER_CONTROLS_H