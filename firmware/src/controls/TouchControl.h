#ifndef TOUCH_CONTROLS_H
#define TOUCH_CONTROLS_H

#include <Arduino.h>

enum class TouchFunctionMode {
    HOLD,
    TOGGLE,
    CONTINUOUS,
};

template<typename MidiTransport>
class TouchControl {
public:
    TouchControl(
        int touchPin,
        int ccNumber,
        int minCCValue,
        int maxCCValue,
        int sensorMin,
        int sensorMax,
        int threshold,
        TouchFunctionMode functionMode,
        MidiTransport& midi
    ) :
        _touchPin(touchPin),
        _ccNumber(ccNumber),
        _minCCValue(minCCValue),
        _maxCCValue(maxCCValue),
        _sensorMin(sensorMin),
        _sensorMax(sensorMax),
        _threshold(threshold),
        _functionMode(functionMode),
        _midi(midi),
        _lastCCTouchValue(-1),
        _lastTouchToggle(0),
        _touchDebounceTime(250),
        _toggleState(false),
        _wasPressed(false)
    {}

    void update() {
        int touchValue = touchRead(_touchPin);

        switch (_functionMode) {
            case TouchFunctionMode::HOLD: {
                bool currentState = touchValue > _threshold;
                if (currentState != _toggleState) {
                    _toggleState = currentState;
                    SERIAL_PRINT("TouchFunctionMode::HOLD :"); SERIAL_PRINTLN(_toggleState);
                    _midi.sendControlChange(_ccNumber, _toggleState ? _maxCCValue : _minCCValue, 1);
                }
                break;
            }
            case TouchFunctionMode::TOGGLE: {
                bool isPressed = touchValue > _threshold;
                if (isPressed && !_wasPressed) {
                    _toggleState = !_toggleState;
                    _lastTouchToggle = millis();
                    SERIAL_PRINT("TouchFunctionMode::TOGGLE :"); SERIAL_PRINTLN(_toggleState);
                    _midi.sendControlChange(_ccNumber, _toggleState ? _maxCCValue : _minCCValue, 1);
                }
                _wasPressed = isPressed;
                break;
            }
            case TouchFunctionMode::CONTINUOUS: {
                int clampedSensorValue = max(_sensorMin, min(_sensorMax, touchValue));
                float percentage = (float)(clampedSensorValue - _sensorMin) / (_sensorMax - _sensorMin);
                int ccValue = _minCCValue + (int)(percentage * (_maxCCValue - _minCCValue));

                if (_lastCCTouchValue != ccValue) {
                    SERIAL_PRINT("Sending CC "); SERIAL_PRINT(_ccNumber);
                    SERIAL_PRINT(", Value: "); SERIAL_PRINTLN(ccValue);
                    _midi.sendControlChange(_ccNumber, ccValue, 1);
                    _lastCCTouchValue = ccValue;
                }
                break;
            }
            default:
                break;
        }
    }

private:
    int _touchPin;
    int _ccNumber;
    int _minCCValue;
    int _maxCCValue;
    int _sensorMin;
    int _sensorMax;
    int _threshold;
    TouchFunctionMode _functionMode;
    MidiTransport& _midi;

    int _lastCCTouchValue;
    unsigned long _lastTouchToggle;
    unsigned long _touchDebounceTime;
    bool _toggleState;
    bool _wasPressed;
};

#endif