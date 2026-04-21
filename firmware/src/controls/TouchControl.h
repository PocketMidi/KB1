#ifndef TOUCH_CONTROL_H
#define TOUCH_CONTROL_H

#include <Arduino.h>
#include <objects/Settings.h>
#include <led/LEDController.h>

class ScaleManager;  // Forward declaration

template<typename MidiTransport>
class TouchControl {
public:
    TouchControl(
        int touchPin,
        TouchSettings& settings,
        int sensorMin,
        int sensorMax,
        MidiTransport& midi,
        ChordSettings& chordSettings,
        LEDController& ledController,
        ScaleManager& scaleManager
    ) :
        _touchPin(touchPin),
        _settings(settings),
        _sensorMin(sensorMin),
        _sensorMax(sensorMax),
        _midi(midi),
        _chordSettings(chordSettings),
        _ledController(ledController),
        _scaleManager(scaleManager),
        _lastCCTouchValue(-1),
        _lastTouchToggle(0),
        _touchDebounceTime(250),      // 250ms for Hold/Continuous (EMI rejection)
        _toggleDebounceTime(50),       // 50ms for Toggle mode (5× faster pattern selection)
        _toggleState(false),
        _wasPressed(false),
        _smoothedValue(0.0f),
        _smoothingAlpha(.075f),
        _previousCCNumber(-1)
    {}

    void update() {
        // Reset pattern state if CC changed away from Pattern Selector (201)
        if (_previousCCNumber == 201 && _settings.ccNumber != 201) {
            _lastCCTouchValue = -1;
            _toggleState = false;
            _wasPressed = false;
            SERIAL_PRINTLN("Touch: Exited Pattern Selector mode, state reset");
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

        int raw = touchRead(_touchPin);

        // Initialize smoothed value on first run
        if (_smoothedValue == 0.0f) {
            _smoothedValue = (float)raw;
        }

        // Exponential moving average smoothing
        // Use lighter smoothing for Toggle mode to enable rapid re-triggering
        float smoothingAlpha = (_settings.functionMode == TouchFunctionMode::TOGGLE) ? 0.3f : _smoothingAlpha;
        _smoothedValue = (smoothingAlpha * (float)raw) + ((1.0f - smoothingAlpha) * _smoothedValue);
        int touchValue = (int)(_smoothedValue + 0.5f);

        // Hysteresis thresholds: on = settings.threshold, off = 75% of threshold (but not below sensor min)
        // For Toggle mode, use tighter hysteresis (50%) to allow faster release detection
        int onThreshold = _settings.threshold;
        float offRatio = (_settings.functionMode == TouchFunctionMode::TOGGLE) ? 0.5f : 0.75f;
        int offThreshold = max(_sensorMin, (int)(_settings.threshold * offRatio));

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
                            
                            // Handle KB1 Expression parameters
                            if (_settings.ccNumber == 200) {
                                // 200 = KB1 Expression: Strum Speed (4-360ms)
                                int strumSpeed = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 360, 4);
                                _chordSettings.strumSpeed = constrain(strumSpeed, 4, 360);
                                SERIAL_PRINT("SS"); SERIAL_PRINTLN(_chordSettings.strumSpeed);
                            } else if (_settings.ccNumber == 202) {
                                // 202 = KB1 Expression: Swing (0-100%)
                                int swing = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 0, 100);
                                _chordSettings.strumSwing = constrain(swing, 0, 100);
                                SERIAL_PRINT("SW"); SERIAL_PRINTLN(_chordSettings.strumSwing);
                            } else if (_settings.ccNumber == 203) {
                                // 203 = KB1 Expression: Velocity Spread (10-100%)
                                int velocitySpread = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 10, 100);
                                _chordSettings.velocitySpread = constrain(velocitySpread, 10, 100);
                                SERIAL_PRINT("VS"); SERIAL_PRINTLN(_chordSettings.velocitySpread);
                            } else if (_settings.ccNumber == 204) {
                                // 204 = KB1 Expression: Scale Type (0-20)
                                int scaleIndex = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 0, 20);
                                scaleIndex = constrain(scaleIndex, 0, 20);
                                _scaleManager.setScale((ScaleType)scaleIndex);
                                SERIAL_PRINT("ScaleType="); SERIAL_PRINTLN(scaleIndex);
                                if (notifyScaleSettingsCallback) {
                                    notifyScaleSettingsCallback();
                                }
                            } else if (_settings.ccNumber == 205) {
                                // 205 = KB1 Expression: Chord Type (0-14)
                                int chordIndex = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 0, 14);
                                chordIndex = constrain(chordIndex, 0, 14);
                                _chordSettings.chordType = (ChordType)chordIndex;
                                SERIAL_PRINT("ChordType="); SERIAL_PRINTLN(chordIndex);
                                if (notifyChordSettingsCallback) {
                                    notifyChordSettingsCallback();
                                }
                            } else if (_settings.ccNumber == 206) {
                                // 206 = KB1 Expression: Root Note (60-71, MIDI notes C through B)
                                int rootNote = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 60, 71);
                                rootNote = constrain(rootNote, 60, 71);
                                _scaleManager.setRootNote(rootNote);
                                SERIAL_PRINT("RootNote="); SERIAL_PRINTLN(rootNote);
                                if (notifyScaleSettingsCallback) {
                                    notifyScaleSettingsCallback();
                                }
                            } else {
                                _midi.sendControlChange(_settings.ccNumber, sendVal, 1);
                            }
                        }
                break;
            }
            case TouchFunctionMode::TOGGLE: {
                        // Use smoothed value for press detection with hysteresis
                        bool isPressed = (touchValue > onThreshold);
                        if (isPressed && !_wasPressed) {
                            // Special handling for Pattern Selector (CC 201): cycle through patterns
                            if (_settings.ccNumber == 201) {
                                // Rate limiting with adaptive debounce: 50ms for Toggle mode (5× faster than previous 250ms)
                                // Allows rapid pattern cycling while preventing bounce
                                static unsigned long lastPatternChange = 0;
                                unsigned long now = millis();
                                if (now - lastPatternChange < _toggleDebounceTime) {  // Use 50ms for Toggle mode
                                    _wasPressed = isPressed;  // Update state to prevent re-trigger
                                    break;
                                }
                                lastPatternChange = now;
                                // Use discrete MIDI values for each pattern (0, 25, 51, 76, 102, 127)
                                const int patternMidi[] = {0, 25, 51, 76, 102, 127};
                                
                                // Get current pattern from chord settings (shared state with other controls)
                                int currentPattern = constrain(_chordSettings.strumPattern, 1, 6);
                                
                                // Get min/max from settings
                                int minPattern = 1 + (_settings.minCCValue * 5 / 127);  // Map 0-127 to 0-5, add 1
                                int maxPattern = 1 + (_settings.maxCCValue * 5 / 127);
                                minPattern = constrain(minPattern, 1, 6);
                                maxPattern = constrain(maxPattern, 1, 6);
                                
                                // Cycle pattern based on offsetTime: 0=forward, >0=reverse
                                bool isForward = false;
                                if (_settings.offsetTime == 0) {
                                    // Forward: increment pattern (wrap around from max to min)
                                    currentPattern++;
                                    if (currentPattern > maxPattern) currentPattern = minPattern;
                                    isForward = true;
                                } else {
                                    // Reverse: decrement pattern (wrap around from min to max)
                                    currentPattern--;
                                    if (currentPattern < minPattern) currentPattern = maxPattern;
                                    isForward = false;
                                }
                                currentPattern = constrain(currentPattern, 1, 6);
                                
                                // Set LED color based on direction (override default behavior)
                                // Flash on instantly, then fade out over 300ms
                                _ledController.set(isForward ? LedColor::PINK : LedColor::BLUE, 255);  // On immediately
                                _ledController.set(isForward ? LedColor::BLUE : LedColor::PINK, 0);    // Off immediately
                                // Queue the fade-out (will execute after the instant on)
                                _ledController.set(isForward ? LedColor::PINK : LedColor::BLUE, 0, 300);
                                
                                // Set discrete MIDI value for the pattern
                                int sendVal = patternMidi[currentPattern - 1];
                                _midi.sendControlChange(_settings.ccNumber, sendVal, 1);
                                _lastCCTouchValue = sendVal;
                                
                                // Update internal chord settings to match the pattern change
                                _chordSettings.strumPattern = constrain(currentPattern, 1, 6);
                                
                                // Minimal logging (1 line only for speed)
                                char buf[8];
                                snprintf(buf, sizeof(buf), "P%d", _chordSettings.strumPattern);
                                SERIAL_PRINTLN(buf);
                            } else if (_settings.ccNumber == 204) {
                                // Scale Type (CC 204): cycle through scale types (0-20)
                                static unsigned long lastScaleChange = 0;
                                unsigned long now = millis();
                                if (now - lastScaleChange < _toggleDebounceTime) {
                                    _wasPressed = isPressed;
                                    break;
                                }
                                lastScaleChange = now;
                                
                                // Get current scale type (0-20)
                                int currentScale = (int)_scaleManager.getScaleType();
                                
                                // Get min/max from settings (maps 0-127 to 0-20)
                                int minScale = map(_settings.minCCValue, 0, 127, 0, 20);
                                int maxScale = map(_settings.maxCCValue, 0, 127, 0, 20);
                                minScale = constrain(minScale, 0, 20);
                                maxScale = constrain(maxScale, 0, 20);
                                
                                // Cycle based on offsetTime: 0=forward, >0=reverse
                                bool isForward = (_settings.offsetTime == 0);
                                if (isForward) {
                                    currentScale++;
                                    if (currentScale > maxScale) currentScale = minScale;
                                } else {
                                    currentScale--;
                                    if (currentScale < minScale) currentScale = maxScale;
                                }
                                currentScale = constrain(currentScale, 0, 20);
                                
                                // Set LED color based on direction
                                _ledController.set(isForward ? LedColor::PINK : LedColor::BLUE, 255);
                                _ledController.set(isForward ? LedColor::BLUE : LedColor::PINK, 0);
                                _ledController.set(isForward ? LedColor::PINK : LedColor::BLUE, 0, 300);
                                
                                // Apply scale change
                                _scaleManager.setScale((ScaleType)currentScale);
                                
                                // Send MIDI CC (map scale index back to MIDI range using settings)
                                int sendVal = map(currentScale, 0, 20, _settings.minCCValue, _settings.maxCCValue);
                                _midi.sendControlChange(_settings.ccNumber, sendVal, 1);
                                _lastCCTouchValue = sendVal;
                                
                                SERIAL_PRINT("ScaleType="); SERIAL_PRINTLN(currentScale);
                                if (notifyScaleSettingsCallback) {
                                    notifyScaleSettingsCallback();
                                }
                            } else if (_settings.ccNumber == 205) {
                                // Chord Type (CC 205): cycle through chord types (0-14)
                                static unsigned long lastChordChange = 0;
                                unsigned long now = millis();
                                if (now - lastChordChange < _toggleDebounceTime) {
                                    _wasPressed = isPressed;
                                    break;
                                }
                                lastChordChange = now;
                                
                                // Get current chord type (0-14)
                                int currentChord = (int)_chordSettings.chordType;
                                
                                // Get min/max from settings (maps 0-127 to 0-14)
                                int minChord = map(_settings.minCCValue, 0, 127, 0, 14);
                                int maxChord = map(_settings.maxCCValue, 0, 127, 0, 14);
                                minChord = constrain(minChord, 0, 14);
                                maxChord = constrain(maxChord, 0, 14);
                                
                                // Cycle based on offsetTime: 0=forward, >0=reverse
                                bool isForward = (_settings.offsetTime == 0);
                                if (isForward) {
                                    currentChord++;
                                    if (currentChord > maxChord) currentChord = minChord;
                                } else {
                                    currentChord--;
                                    if (currentChord < minChord) currentChord = maxChord;
                                }
                                currentChord = constrain(currentChord, 0, 14);
                                
                                // Set LED color based on direction
                                _ledController.set(isForward ? LedColor::PINK : LedColor::BLUE, 255);
                                _ledController.set(isForward ? LedColor::BLUE : LedColor::PINK, 0);
                                _ledController.set(isForward ? LedColor::PINK : LedColor::BLUE, 0, 300);
                                
                                // Apply chord change
                                _chordSettings.chordType = (ChordType)currentChord;
                                
                                // Send MIDI CC (map chord index back to MIDI range using settings)
                                int sendVal = map(currentChord, 0, 14, _settings.minCCValue, _settings.maxCCValue);
                                _midi.sendControlChange(_settings.ccNumber, sendVal, 1);
                                _lastCCTouchValue = sendVal;
                                
                                SERIAL_PRINT("ChordType="); SERIAL_PRINTLN(currentChord);
                                if (notifyChordSettingsCallback) {
                                    notifyChordSettingsCallback();
                                }
                            } else if (_settings.ccNumber == 206) {
                                // Root Note (CC 206): cycle through root notes (60-71, MIDI notes C through B)
                                static unsigned long lastRootChange = 0;
                                unsigned long now = millis();
                                if (now - lastRootChange < _toggleDebounceTime) {
                                    _wasPressed = isPressed;
                                    break;
                                }
                                lastRootChange = now;
                                
                                // Get current root note (MIDI 60-71)
                                int currentRoot = _scaleManager.getRootNote();
                                
                                // Get min/max from settings (maps 0-127 to 60-71)
                                int minRoot = map(_settings.minCCValue, 0, 127, 60, 71);
                                int maxRoot = map(_settings.maxCCValue, 0, 127, 60, 71);
                                minRoot = constrain(minRoot, 60, 71);
                                maxRoot = constrain(maxRoot, 60, 71);
                                
                                // Cycle based on offsetTime: 0=forward, >0=reverse
                                bool isForward = (_settings.offsetTime == 0);
                                if (isForward) {
                                    currentRoot++;
                                    if (currentRoot > maxRoot) currentRoot = minRoot;
                                } else {
                                    currentRoot--;
                                    if (currentRoot < minRoot) currentRoot = maxRoot;
                                }
                                currentRoot = constrain(currentRoot, 60, 71);
                                
                                // Set LED color based on direction
                                _ledController.set(isForward ? LedColor::PINK : LedColor::BLUE, 255);
                                _ledController.set(isForward ? LedColor::BLUE : LedColor::PINK, 0);
                                _ledController.set(isForward ? LedColor::PINK : LedColor::BLUE, 0, 300);
                                
                                // Apply root note change
                                _scaleManager.setRootNote(currentRoot);
                                
                                // Send MIDI CC (map root MIDI note back to MIDI range using settings)
                                int sendVal = map(currentRoot, 60, 71, _settings.minCCValue, _settings.maxCCValue);
                                _midi.sendControlChange(_settings.ccNumber, sendVal, 1);
                                _lastCCTouchValue = sendVal;
                                
                                SERIAL_PRINT("RootNote="); SERIAL_PRINTLN(currentRoot);
                                if (notifyScaleSettingsCallback) {
                                    notifyScaleSettingsCallback();
                                }
                            } else {
                                // Normal toggle behavior for other parameters
                                _toggleState = !_toggleState;
                                _lastTouchToggle = millis();
                                SERIAL_PRINT("TouchFunctionMode::TOGGLE :"); SERIAL_PRINTLN(_toggleState);
                                int sendVal = constrain(_toggleState ? _settings.maxCCValue : _settings.minCCValue, 0, 127);
                                
                                // Handle KB1 Expression parameters
                                if (_settings.ccNumber == 200) {
                                    // 200 = KB1 Expression: Strum Speed (4-360ms)
                                    int strumSpeed = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 360, 4);
                                    _chordSettings.strumSpeed = constrain(strumSpeed, 4, 360);
                                    SERIAL_PRINT("SS"); SERIAL_PRINTLN(_chordSettings.strumSpeed);
                                } else if (_settings.ccNumber == 202) {
                                    // 202 = KB1 Expression: Swing (0-100%)
                                    int swing = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 0, 100);
                                    _chordSettings.strumSwing = constrain(swing, 0, 100);
                                    SERIAL_PRINT("SW"); SERIAL_PRINTLN(_chordSettings.strumSwing);
                                } else if (_settings.ccNumber == 203) {
                                    // 203 = KB1 Expression: Velocity Spread (8-100%)
                                    int velocitySpread = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 10, 100);
                                    _chordSettings.velocitySpread = constrain(velocitySpread, 10, 100);
                                    SERIAL_PRINT("VS"); SERIAL_PRINTLN(_chordSettings.velocitySpread);
                                } else if (_settings.ccNumber == 204) {
                                    // 204 = KB1 Expression: Scale Type (0-20)
                                    int scaleIndex = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 0, 20);
                                    scaleIndex = constrain(scaleIndex, 0, 20);
                                    _scaleManager.setScale((ScaleType)scaleIndex);
                                    SERIAL_PRINT("ScaleType="); SERIAL_PRINTLN(scaleIndex);
                                    if (notifyScaleSettingsCallback) {
                                        notifyScaleSettingsCallback();
                                    }
                                } else if (_settings.ccNumber == 205) {
                                    // 205 = KB1 Expression: Chord Type (0-14)
                                    int chordIndex = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 0, 14);
                                    chordIndex = constrain(chordIndex, 0, 14);
                                    _chordSettings.chordType = (ChordType)chordIndex;
                                    SERIAL_PRINT("ChordType="); SERIAL_PRINTLN(chordIndex);
                                    if (notifyChordSettingsCallback) {
                                        notifyChordSettingsCallback();
                                    }
                                } else if (_settings.ccNumber == 206) {
                                    // 206 = KB1 Expression: Root Note (60-71, MIDI notes C through B)
                                    int rootNote = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 60, 71);
                                    rootNote = constrain(rootNote, 60, 71);
                                    _scaleManager.setRootNote(rootNote);
                                    SERIAL_PRINT("RootNote="); SERIAL_PRINTLN(rootNote);
                                    if (notifyScaleSettingsCallback) {
                                        notifyScaleSettingsCallback();
                                    }
                                } else {
                                    _midi.sendControlChange(_settings.ccNumber, sendVal, 1);
                                }
                            }
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
                            
                            // Handle KB1 Expression parameters
                            if (_settings.ccNumber == 200) {
                                // 200 = KB1 Expression: Strum Speed (4-360ms)
                                int strumSpeed = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 360, 4);
                                _chordSettings.strumSpeed = constrain(strumSpeed, 4, 360);
                                SERIAL_PRINT("SS"); SERIAL_PRINTLN(_chordSettings.strumSpeed);
                            } else if (_settings.ccNumber == 202) {
                                // 202 = KB1 Expression: Swing (0-100%)
                                int swing = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 0, 100);
                                _chordSettings.strumSwing = constrain(swing, 0, 100);
                                SERIAL_PRINT("SW"); SERIAL_PRINTLN(_chordSettings.strumSwing);
                            } else if (_settings.ccNumber == 203) {
                                // 203 = KB1 Expression: Velocity Spread (10-100%)
                                int velocitySpread = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 10, 100);
                                _chordSettings.velocitySpread = constrain(velocitySpread, 10, 100);
                                SERIAL_PRINT("VS"); SERIAL_PRINTLN(_chordSettings.velocitySpread);
                            } else if (_settings.ccNumber == 204) {
                                // 204 = KB1 Expression: Scale Type (0-20)
                                int scaleIndex = map(sendVal, 0, 127, 0, 20);
                                scaleIndex = constrain(scaleIndex, 0, 20);
                                _scaleManager.setScale((ScaleType)scaleIndex);
                                SERIAL_PRINT("ScaleType="); SERIAL_PRINTLN(scaleIndex);
                                if (notifyScaleSettingsCallback) {
                                    notifyScaleSettingsCallback();
                                }
                            } else if (_settings.ccNumber == 205) {
                                // 205 = KB1 Expression: Chord Type (0-14)
                                int chordIndex = map(sendVal, 0, 127, 0, 14);
                                chordIndex = constrain(chordIndex, 0, 14);
                                _chordSettings.chordType = (ChordType)chordIndex;
                                SERIAL_PRINT("ChordType="); SERIAL_PRINTLN(chordIndex);
                                if (notifyChordSettingsCallback) {
                                    notifyChordSettingsCallback();
                                }
                            } else if (_settings.ccNumber == 206) {
                                // 206 = KB1 Expression: Root Note (0-11)
                                int rootNote = map(sendVal, 0, 127, 0, 11);
                                rootNote = constrain(rootNote, 0, 11);
                                _scaleManager.setRootNote(rootNote);
                                SERIAL_PRINT("RootNote="); SERIAL_PRINTLN(rootNote);
                                if (notifyScaleSettingsCallback) {
                                    notifyScaleSettingsCallback();
                                }
                            } else {
                                // Throttled serial output for continuous mode (print every 500ms max)
                                static unsigned long lastContinuousPrint = 0;
                                unsigned long now = millis();
                                if (now - lastContinuousPrint >= 500) {
                                    SERIAL_PRINT("Touch CC"); SERIAL_PRINT(_settings.ccNumber);
                                    SERIAL_PRINT("="); SERIAL_PRINTLN(sendVal);
                                    lastContinuousPrint = now;
                                }
                                _midi.sendControlChange(_settings.ccNumber, sendVal, 1);
                            }
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
    ChordSettings& _chordSettings;
    LEDController& _ledController;
    ScaleManager& _scaleManager;

    int _lastCCTouchValue;
    unsigned long _lastTouchToggle;
    unsigned long _touchDebounceTime;     // Debounce for Hold/Continuous modes (250ms)
    unsigned long _toggleDebounceTime;    // Debounce for Toggle mode (50ms, faster re-trigger)
    bool _toggleState;
    bool _wasPressed;
    int _previousCCNumber;
    // Smoothing state
    float _smoothedValue;
    float _smoothingAlpha;
};

#endif