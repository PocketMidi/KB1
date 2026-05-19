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

        // Disable Pattern Selector (201) when not in ARP mode
        // Disable Swing (202) when not in ARP mode
        // Disable Latch (207) when not in ARP mode
        bool isArpMode = (_chordSettings.playMode == PlayMode::ARP);
        if (_settings.ccNumber == 201 && !isArpMode) {
            return;
        }
        if (_settings.ccNumber == 202 && !isArpMode) {
            return;
        }
        if (_settings.ccNumber == 207 && !isArpMode) {
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
                                // 202 = KB1 Expression: Swing (0-50 firmware, 50-100% UI)
                                int swing = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 0, 50);
                                _chordSettings.strumSwing = constrain(swing, 0, 50);
                                SERIAL_PRINT("SW"); SERIAL_PRINTLN(_chordSettings.strumSwing);
                            } else if (_settings.ccNumber == 203) {
                                // 203 = KB1 Expression: Velocity Spread (10-100%)
                                int velocitySpread = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 10, 100);
                                _chordSettings.velocitySpread = constrain(velocitySpread, 10, 100);
                                SERIAL_PRINT("VS"); SERIAL_PRINTLN(_chordSettings.velocitySpread);
                            } else if (_settings.ccNumber == 204) {
                                // 204 = KB1 Expression: Chord Swing (10-100)
                                int chordSwing = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 10, 100);
                                _chordSettings.gateValue = constrain(chordSwing, 10, 100);
                                SERIAL_PRINT("CS"); SERIAL_PRINTLN(_chordSettings.gateValue);
                                if (notifyChordSettingsCallback) {
                                    notifyChordSettingsCallback();
                                }
                            } else if (_settings.ccNumber == 205) {
                                // 205 = KB1 Expression: Note Range / voicing (1-3 octave spread)
                                int voicing = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 1, 3);
                                voicing = constrain(voicing, 1, 3);
                                _chordSettings.voicing = voicing;
                                SERIAL_PRINT("NoteRange="); SERIAL_PRINTLN(voicing);
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
                            } else if (_settings.ccNumber == 207) {
                                // 207 = KB1 Expression: ARP Latch toggle (0=momentary, 1=latch)
                                _chordSettings.arpLatchMode = (sendVal >= 64) ? 1 : 0;
                                SERIAL_PRINT("Latch="); SERIAL_PRINTLN(_chordSettings.arpLatchMode);
                                if (notifyChordSettingsCallback) {
                                    notifyChordSettingsCallback();
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
                                
                                // Notify BLE clients of the change
                                if (notifyChordSettingsCallback) {
                                    notifyChordSettingsCallback();
                                }
                                
                                // Minimal logging (1 line only for speed)
                                char buf[8];
                                snprintf(buf, sizeof(buf), "P%d", _chordSettings.strumPattern);
                                SERIAL_PRINTLN(buf);
                            } else if (_settings.ccNumber == 204) {
                                // Chord Swing (CC 204): step through 10-100 on each press
                                static unsigned long lastChordSwingChange = 0;
                                unsigned long now = millis();
                                if (now - lastChordSwingChange < _toggleDebounceTime) {
                                    _wasPressed = isPressed;
                                    break;
                                }
                                lastChordSwingChange = now;
                                
                                int minSwing = map(_settings.minCCValue, 0, 127, 10, 100);
                                int maxSwing = map(_settings.maxCCValue, 0, 127, 10, 100);
                                minSwing = constrain(minSwing, 10, 100);
                                maxSwing = constrain(maxSwing, 10, 100);
                                
                                // Step based on offsetTime: 0=forward, >0=reverse
                                bool isForward = (_settings.offsetTime == 0);
                                int step = max(1, (maxSwing - minSwing) / 10);
                                int current = _chordSettings.gateValue;
                                if (isForward) {
                                    current += step;
                                    if (current > maxSwing) current = minSwing;
                                } else {
                                    current -= step;
                                    if (current < minSwing) current = maxSwing;
                                }
                                _chordSettings.gateValue = constrain(current, 10, 100);
                                
                                _ledController.set(isForward ? LedColor::PINK : LedColor::BLUE, 255);
                                _ledController.set(isForward ? LedColor::PINK : LedColor::BLUE, 0, 300);
                                
                                int sendVal2 = map(_chordSettings.gateValue, 10, 100, _settings.minCCValue, _settings.maxCCValue);
                                _midi.sendControlChange(_settings.ccNumber, sendVal2, 1);
                                _lastCCTouchValue = sendVal2;
                                
                                SERIAL_PRINT("CS"); SERIAL_PRINTLN(_chordSettings.gateValue);
                                if (notifyChordSettingsCallback) {
                                    notifyChordSettingsCallback();
                                }
                            } else if (_settings.ccNumber == 205) {
                                // Note Range (CC 205): cycle through voicing 1→2→3→1 (octave spread)
                                static unsigned long lastVoicingChange = 0;
                                unsigned long now = millis();
                                if (now - lastVoicingChange < _toggleDebounceTime) {
                                    _wasPressed = isPressed;
                                    break;
                                }
                                lastVoicingChange = now;

                                int currentVoicing = constrain(_chordSettings.voicing, 1, 3);

                                // Get min/max from settings (maps 0-127 to 1-3)
                                int minVoicing = map(_settings.minCCValue, 0, 127, 1, 3);
                                int maxVoicing = map(_settings.maxCCValue, 0, 127, 1, 3);
                                minVoicing = constrain(minVoicing, 1, 3);
                                maxVoicing = constrain(maxVoicing, 1, 3);

                                // Cycle based on offsetTime: 0=forward, >0=reverse
                                bool isForward = (_settings.offsetTime == 0);
                                if (isForward) {
                                    currentVoicing++;
                                    if (currentVoicing > maxVoicing) currentVoicing = minVoicing;
                                } else {
                                    currentVoicing--;
                                    if (currentVoicing < minVoicing) currentVoicing = maxVoicing;
                                }
                                currentVoicing = constrain(currentVoicing, 1, 3);

                                // Set LED color based on direction
                                _ledController.set(isForward ? LedColor::PINK : LedColor::BLUE, 255);
                                _ledController.set(isForward ? LedColor::BLUE : LedColor::PINK, 0);
                                _ledController.set(isForward ? LedColor::PINK : LedColor::BLUE, 0, 300);

                                _chordSettings.voicing = currentVoicing;

                                // Send MIDI CC (map voicing back to MIDI range)
                                int sendVal205 = map(currentVoicing, 1, 3, _settings.minCCValue, _settings.maxCCValue);
                                _midi.sendControlChange(_settings.ccNumber, sendVal205, 1);
                                _lastCCTouchValue = sendVal205;

                                SERIAL_PRINT("NoteRange="); SERIAL_PRINTLN(currentVoicing);
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
                            } else if (_settings.ccNumber == 207) {
                                // Latch (CC 207): toggle arpLatchMode 0↔1 on each tap
                                static unsigned long lastLatchChange = 0;
                                unsigned long now = millis();
                                if (now - lastLatchChange < _toggleDebounceTime) {
                                    _wasPressed = isPressed;
                                    break;
                                }
                                lastLatchChange = now;
                                
                                _chordSettings.arpLatchMode = (_chordSettings.arpLatchMode == 0) ? 1 : 0;
                                
                                // Pink = latch on, Blue = momentary
                                bool isLatched = (_chordSettings.arpLatchMode == 1);
                                _ledController.set(isLatched ? LedColor::PINK : LedColor::BLUE, 255);
                                _ledController.set(isLatched ? LedColor::PINK : LedColor::BLUE, 0, 300);
                                
                                int sendVal = isLatched ? _settings.maxCCValue : _settings.minCCValue;
                                _midi.sendControlChange(_settings.ccNumber, sendVal, 1);
                                _lastCCTouchValue = sendVal;
                                
                                SERIAL_PRINT("Latch="); SERIAL_PRINTLN(_chordSettings.arpLatchMode);
                                if (notifyChordSettingsCallback) {
                                    notifyChordSettingsCallback();
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
                                    // 202 = KB1 Expression: Swing (0-50 firmware, 50-100% UI)
                                    int swing = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 0, 50);
                                    _chordSettings.strumSwing = constrain(swing, 0, 50);
                                    SERIAL_PRINT("SW"); SERIAL_PRINTLN(_chordSettings.strumSwing);
                                } else if (_settings.ccNumber == 203) {
                                    // 203 = KB1 Expression: Velocity Spread (8-100%)
                                    int velocitySpread = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 10, 100);
                                    _chordSettings.velocitySpread = constrain(velocitySpread, 10, 100);
                                    SERIAL_PRINT("VS"); SERIAL_PRINTLN(_chordSettings.velocitySpread);
                                } else if (_settings.ccNumber == 204) {
                                    // 204 = KB1 Expression: Chord Swing (10-100)
                                    int chordSwing = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 10, 100);
                                    _chordSettings.gateValue = constrain(chordSwing, 10, 100);
                                    SERIAL_PRINT("CS"); SERIAL_PRINTLN(_chordSettings.gateValue);
                                    if (notifyChordSettingsCallback) {
                                        notifyChordSettingsCallback();
                                    }
                                } else if (_settings.ccNumber == 205) {
                                    // 205 = KB1 Expression: Note Range / voicing (1-3 octave spread)
                                    int voicing = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 1, 3);
                                    voicing = constrain(voicing, 1, 3);
                                    _chordSettings.voicing = voicing;
                                    SERIAL_PRINT("NoteRange="); SERIAL_PRINTLN(voicing);
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
                        // REV mode (offsetTime > 0): invert mapping — release returns to maxCC instead of minCC
                        int ccValue;
                        if (_settings.offsetTime > 0) {
                            ccValue = _settings.maxCCValue - (int)(percentage * (_settings.maxCCValue - _settings.minCCValue));
                        } else {
                            ccValue = _settings.minCCValue + (int)(percentage * (_settings.maxCCValue - _settings.minCCValue));
                        }

                        if (_lastCCTouchValue != ccValue) {
                            int sendVal = constrain(ccValue, 0, 127);
                            
                            // Handle KB1 Expression parameters
                            if (_settings.ccNumber == 200) {
                                // 200 = KB1 Expression: Strum Speed (4-360ms)
                                int strumSpeed = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 360, 4);
                                _chordSettings.strumSpeed = constrain(strumSpeed, 4, 360);
                                SERIAL_PRINT("SS"); SERIAL_PRINTLN(_chordSettings.strumSpeed);
                            } else if (_settings.ccNumber == 202) {
                                // 202 = KB1 Expression: Swing (0-50 firmware, 50-100% UI)
                                int swing = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 0, 50);
                                _chordSettings.strumSwing = constrain(swing, 0, 50);
                                SERIAL_PRINT("SW"); SERIAL_PRINTLN(_chordSettings.strumSwing);
                            } else if (_settings.ccNumber == 203) {
                                // 203 = KB1 Expression: Velocity Spread (10-100%)
                                int velocitySpread = map(sendVal, _settings.minCCValue, _settings.maxCCValue, 10, 100);
                                _chordSettings.velocitySpread = constrain(velocitySpread, 10, 100);
                                SERIAL_PRINT("VS"); SERIAL_PRINTLN(_chordSettings.velocitySpread);
                            } else if (_settings.ccNumber == 204) {
                                // 204 = KB1 Expression: Chord Swing (10-100)
                                int chordSwing = map(sendVal, 0, 127, 10, 100);
                                _chordSettings.gateValue = constrain(chordSwing, 10, 100);
                                SERIAL_PRINT("CS"); SERIAL_PRINTLN(_chordSettings.gateValue);
                                if (notifyChordSettingsCallback) {
                                    notifyChordSettingsCallback();
                                }
                            } else if (_settings.ccNumber == 205) {
                                // 205 = KB1 Expression: Note Range / voicing (1-3 octave spread)
                                int voicing = map(sendVal, 0, 127, 1, 3);
                                voicing = constrain(voicing, 1, 3);
                                _chordSettings.voicing = voicing;
                                SERIAL_PRINT("NoteRange="); SERIAL_PRINTLN(voicing);
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