#ifndef TOUCH_CONTROLS_H
#define TOUCH_CONTROLS_H

#include <Arduino.h>
#include <objects/Settings.h>

template<typename MidiTransport>
class TouchControl {
public:
    TouchControl(
        int touchPin,
        TouchSettings& settings,
        int sensorMin,
        int sensorMax,
        MidiTransport& midi
    ) :
        _touchPin(touchPin),
        _settings(settings),
        _sensorMin(sensorMin),
        _sensorMax(sensorMax),
        _midi(midi),
        _lastCCTouchValue(-1),
        _lastTouchToggle(0),
        _touchDebounceTime(250),
        _toggleState(false),
        _wasPressed(false),
        _smoothedValue(0.0f),
        _smoothingAlpha(.075f)
    {}

    void update() {
        int raw = touchRead(_touchPin);

        // Initialize smoothed value on first run
        if (_smoothedValue == 0.0f) {
            _smoothedValue = (float)raw;
        }

        // Exponential moving average smoothing
        _smoothedValue = (_smoothingAlpha * (float)raw) + ((1.0f - _smoothingAlpha) * _smoothedValue);
        int touchValue = (int)(_smoothedValue + 0.5f);

        // Hysteresis thresholds: on = settings.threshold, off = 75% of threshold (but not below sensor min)
        int onThreshold = _settings.threshold;
        int offThreshold = max(_sensorMin, (int)(_settings.threshold * 0.75f));

        switch (_settings.functionMode) {
            case TouchFunctionMode::HOLD: {
                        // Use hysteresis: require smoothed value to cross on/off thresholds
                        bool currentState = _toggleState;
                        if (touchValue > onThreshold) {
                            currentState = true;
                        } else if (touchValue < offThreshold) {
                            currentState = false;
                        }

                        if (currentState != _toggleState) {
                            _toggleState = currentState;
                            SERIAL_PRINT("TouchFunctionMode::HOLD :"); SERIAL_PRINTLN(_toggleState);
                            int sendVal = constrain(_toggleState ? _settings.maxCCValue : _settings.minCCValue, 0, 127);
                            _midi.sendControlChange(_settings.ccNumber, sendVal, 1);
                        }
                break;
            }
            case TouchFunctionMode::TOGGLE: {
                        // Use smoothed value for press detection with hysteresis
                        bool isPressed = (touchValue > onThreshold);
                        if (isPressed && !_wasPressed) {
                            _toggleState = !_toggleState;
                            _lastTouchToggle = millis();
                            SERIAL_PRINT("TouchFunctionMode::TOGGLE :"); SERIAL_PRINTLN(_toggleState);
                            int sendVal = constrain(_toggleState ? _settings.maxCCValue : _settings.minCCValue, 0, 127);
                            _midi.sendControlChange(_settings.ccNumber, sendVal, 1);
                        }
                        // Update _wasPressed: true if smoothed value is above offThreshold
                        _wasPressed = (touchValue > offThreshold);
                break;
            }
            case TouchFunctionMode::CONTINUOUS: {
                        // Use smoothed reading for continuous mapping to reduce jitter
                        int clampedSensorValue = max(_sensorMin, min(_sensorMax, touchValue));
                        float percentage = 0.0f;
                        if (_sensorMax != _sensorMin) {
                            percentage = (float)(clampedSensorValue - _sensorMin) / (float)(_sensorMax - _sensorMin);
                        }
                        int ccValue = _settings.minCCValue + (int)(percentage * (_settings.maxCCValue - _settings.minCCValue));

                        if (_lastCCTouchValue != ccValue) {
                            int sendVal = constrain(ccValue, 0, 127);
                            SERIAL_PRINT("Sending CC "); SERIAL_PRINT(_settings.ccNumber);
                            SERIAL_PRINT(", Value: "); SERIAL_PRINTLN(sendVal);
                            _midi.sendControlChange(_settings.ccNumber, sendVal, 1);
                            _lastCCTouchValue = sendVal;
                        }
                break;
            }
            default:
                break;
        }
    }

    // Returns whether the touch sensor is currently considered "active".
    // Call this after `update()` so `_smoothedValue` and state are up-to-date.
    bool isActive() {
        int touchValue = (int)(_smoothedValue + 0.5f);
        int onThreshold = _settings.threshold;
        int offThreshold = max(_sensorMin, (int)(_settings.threshold * 0.75f));
        switch (_settings.functionMode) {
            case TouchFunctionMode::CONTINUOUS:
                return touchValue > offThreshold;
            case TouchFunctionMode::HOLD:
            case TouchFunctionMode::TOGGLE:
            default:
                return _toggleState || _wasPressed;
        }
    }

private:
    int _touchPin;
    TouchSettings& _settings;
    int _sensorMin;
    int _sensorMax;
    // threshold now comes from `_settings.threshold` so no local copy needed
    MidiTransport& _midi;

    int _lastCCTouchValue;
    unsigned long _lastTouchToggle;
    unsigned long _touchDebounceTime;
    bool _toggleState;
    bool _wasPressed;
    // Smoothing state
    float _smoothedValue;
    float _smoothingAlpha;
};

#endif