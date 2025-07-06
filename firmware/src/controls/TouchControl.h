#ifndef TOUCH_CONTROLS_H
#define TOUCH_CONTROLS_H

#include <Arduino.h>

template<typename MidiTransport>
class TouchControl {
public:
    TouchControl(
        int touchPin,
        TouchSettings& settings,
        int sensorMin,
        int sensorMax,
        int threshold,
        MidiTransport& midi
    ) :
        _touchPin(touchPin),
        _settings(settings),
        _sensorMin(sensorMin),
        _sensorMax(sensorMax),
        _threshold(threshold),
        _midi(midi),
        _lastCCTouchValue(-1),
        _lastTouchToggle(0),
        _touchDebounceTime(250),
        _toggleState(false),
        _wasPressed(false)
    {}

    void update() {
        int touchValue = touchRead(_touchPin);

        switch (_settings.functionMode) {
            case TouchFunctionMode::HOLD: {
                bool currentState = touchValue > _threshold;
                if (currentState != _toggleState) {
                    _toggleState = currentState;
                    SERIAL_PRINT("TouchFunctionMode::HOLD :"); SERIAL_PRINTLN(_toggleState);
                    _midi.sendControlChange(_settings.ccNumber, _toggleState ? _settings.maxCCValue : _settings.minCCValue, 1);
                }
                break;
            }
            case TouchFunctionMode::TOGGLE: {
                bool isPressed = touchValue > _threshold;
                if (isPressed && !_wasPressed) {
                    _toggleState = !_toggleState;
                    _lastTouchToggle = millis();
                    SERIAL_PRINT("TouchFunctionMode::TOGGLE :"); SERIAL_PRINTLN(_toggleState);
                    _midi.sendControlChange(_settings.ccNumber, _toggleState ? _settings.maxCCValue : _settings.minCCValue, 1);
                }
                _wasPressed = isPressed;
                break;
            }
            case TouchFunctionMode::CONTINUOUS: {
                int clampedSensorValue = max(_sensorMin, min(_sensorMax, touchValue));
                float percentage = (float)(clampedSensorValue - _sensorMin) / (_sensorMax - _sensorMin);
                int ccValue = _settings.minCCValue + (int)(percentage * (_settings.maxCCValue - _settings.minCCValue));

                if (_lastCCTouchValue != ccValue) {
                    SERIAL_PRINT("Sending CC "); SERIAL_PRINT(_settings.ccNumber);
                    SERIAL_PRINT(", Value: "); SERIAL_PRINTLN(ccValue);
                    _midi.sendControlChange(_settings.ccNumber, ccValue, 1);
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
    TouchSettings& _settings;
    int _sensorMin;
    int _sensorMax;
    int _threshold;
    MidiTransport& _midi;

    int _lastCCTouchValue;
    unsigned long _lastTouchToggle;
    unsigned long _touchDebounceTime;
    bool _toggleState;
    bool _wasPressed;
};

#endif