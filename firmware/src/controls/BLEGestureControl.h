#ifndef BLE_GESTURE_CONTROL_H
#define BLE_GESTURE_CONTROL_H

#include <Arduino.h>
#include "led/LEDController.h"
#include "bt/BluetoothController.h"
#include "objects/Globals.h"

/**
 * BLEGestureControl - Cross-lever gesture detection for BLE toggle
 * 
 * Gesture: Push both levers toward center simultaneously
 * - Lever 1 (left lever) pushed RIGHT
 * - Lever 2 (right lever) pushed LEFT
 * 
 * Hold for 3 seconds to toggle BLE with progressive LED feedback:
 * - Octave arrows ON immediately (gesture detected)
 * - Pink/Blue LEDs pulse with increasing speed as hold progresses
 * - Final flash on activation
 */
class BLEGestureControl {
public:
    BLEGestureControl(
        LEDController& ledController,
        BluetoothController* bluetoothController = nullptr
    ) : _ledController(ledController),
        _bluetoothController(bluetoothController),
        _gestureStartTime(0),
        _gestureActive(false),
        _toggleTriggered(false),
        _lastFeedbackPhase(-1),
        _feedbackActive(false)
    {}

    void setBluetoothController(BluetoothController* controller) {
        _bluetoothController = controller;
    }

    /**
     * Call this in the main input loop with lever states already read
     * @param lever1Right true if lever 1 is pushed right (toward center)
     * @param lever2Left true if lever 2 is pushed left (toward center)
     * @param keyboardActive true if any keyboard key is currently pressed
     * @return true if gesture is currently active (levers held in cross position)
     */
    bool update(bool lever1Right, bool lever2Left, bool keyboardActive) {
        // Cross gesture: lever1 pushed RIGHT + lever2 pushed LEFT
        bool crossGestureDetected = lever1Right && lever2Left;
        
        unsigned long now = millis();

        // Abort gesture if keyboard is active (prevents accidental triggers during play)
        if (keyboardActive && _gestureActive && !_toggleTriggered) {
            abortGesture();
            SERIAL_PRINTLN("BLE Gesture: Aborted - keyboard active");
            return false;
        }

        if (crossGestureDetected && !keyboardActive) {
            if (!_gestureActive) {
                // Gesture just started
                _gestureActive = true;
                _gestureStartTime = now;
                _toggleTriggered = false;
                _lastFeedbackPhase = -1;
                _feedbackActive = true;
                
                // Immediately turn on octave arrows to confirm gesture detected
                _ledController.set(LedColor::OCTAVE_UP, 255);
                _ledController.set(LedColor::OCTAVE_DOWN, 255);
                
                SERIAL_PRINTLN("BLE Gesture: Cross-lever detected, hold for 3s...");
            }

            unsigned long holdDuration = now - _gestureStartTime;
            
            // Progressive feedback based on hold duration (skip if already triggered)
            if (!_toggleTriggered) {
                updateProgressiveFeedback(holdDuration);
            }

            // Check if hold threshold reached
            if (!_toggleTriggered && holdDuration >= HOLD_DURATION_MS) {
                triggerBLEToggle();
                _toggleTriggered = true;
            }
        } else if (!crossGestureDetected) {
            // Gesture released
            if (_gestureActive) {
                _gestureActive = false;
                
                // Turn off feedback LEDs if toggle wasn't triggered
                if (!_toggleTriggered && _feedbackActive) {
                    _ledController.set(LedColor::OCTAVE_UP, 0);
                    _ledController.set(LedColor::OCTAVE_DOWN, 0);
                    // Stop any ongoing pulse
                    _ledController.set(LedColor::PINK, 0);
                    _ledController.set(LedColor::BLUE, 0);
                    SERIAL_PRINTLN("BLE Gesture: Released before activation");
                }
                
                _feedbackActive = false;
                _lastFeedbackPhase = -1;
            }
        }

        return _gestureActive;
    }

    bool isGestureActive() const {
        return _gestureActive;
    }

private:
    static const unsigned long HOLD_DURATION_MS = 3000;  // 3 seconds to toggle
    
    // Pulse speeds (ms per cycle) - gets faster as hold progresses
    static const unsigned long PULSE_SPEED_PHASE1 = 800;   // 0-1s: slow
    static const unsigned long PULSE_SPEED_PHASE2 = 500;   // 1-2s: medium
    static const unsigned long PULSE_SPEED_PHASE3 = 250;   // 2-3s: fast
    static const unsigned long PHASE_DURATION = 1000;      // 1 second per phase
    
    // LED brightness constants
    static const int LED_MAX_BRIGHTNESS = 255;
    static const int PINK_MAX_BRIGHTNESS = 64;  // ~25% of 255

    LEDController& _ledController;
    BluetoothController* _bluetoothController;

    unsigned long _gestureStartTime;
    bool _gestureActive;
    bool _toggleTriggered;
    int _lastFeedbackPhase;
    bool _feedbackActive;

    void abortGesture() {
        _gestureActive = false;
        _feedbackActive = false;
        _lastFeedbackPhase = -1;
        // Turn off all feedback LEDs
        _ledController.set(LedColor::OCTAVE_UP, 0);
        _ledController.set(LedColor::OCTAVE_DOWN, 0);
        _ledController.set(LedColor::PINK, 0);
        _ledController.set(LedColor::BLUE, 0);
    }

    void updateProgressiveFeedback(unsigned long holdDuration) {
        // Determine current phase (0, 1, or 2)
        int phase = min((int)(holdDuration / PHASE_DURATION), 2);
        
        // Only update pulse if phase changed
        if (phase != _lastFeedbackPhase) {
            _lastFeedbackPhase = phase;
            
            unsigned long pulseSpeed;
            switch (phase) {
                case 0:
                    pulseSpeed = PULSE_SPEED_PHASE1;
                    break;
                case 1:
                    pulseSpeed = PULSE_SPEED_PHASE2;
                    break;
                case 2:
                default:
                    pulseSpeed = PULSE_SPEED_PHASE3;
                    break;
            }
            
            // Start pulsing pink and blue at current phase speed
            // Duration of 0 means pulse indefinitely until we stop it
            _ledController.pulse(LedColor::PINK, pulseSpeed, 0);
            _ledController.pulse(LedColor::BLUE, pulseSpeed, 0);
            
            SERIAL_PRINT("BLE Gesture: Phase ");
            SERIAL_PRINT(phase + 1);
            SERIAL_PRINT("/3 - pulse speed ");
            SERIAL_PRINT(pulseSpeed);
            SERIAL_PRINTLN("ms");
        }
    }

    void triggerBLEToggle() {
        // Stop all pulsing and turn off LEDs immediately - gesture complete
        _ledController.set(LedColor::OCTAVE_UP, 0);
        _ledController.set(LedColor::OCTAVE_DOWN, 0);
        _ledController.set(LedColor::PINK, 0);
        _ledController.set(LedColor::BLUE, 0);
        _feedbackActive = false;
        
        // Set lever cooldown to prevent MIDI output during lever release
        leverCooldownUntil = millis() + 1500;  // 1.5 second cooldown
        
        SERIAL_PRINTLN("BLE Gesture: Activation threshold reached!");
        
        if (_bluetoothController) {
            if (_bluetoothController->isEnabled()) {
                _bluetoothController->disable();
                SERIAL_PRINTLN("Bluetooth Disabled by Cross-Lever Gesture.");
            } else {
                _bluetoothController->enable();
                SERIAL_PRINTLN("Bluetooth Enabled by Cross-Lever Gesture.");
            }
        }
        // BluetoothController enable/disable handles its own LED feedback
    }
};

#endif // BLE_GESTURE_CONTROL_H
