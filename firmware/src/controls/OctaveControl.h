#ifndef OCTAVE_CONTROL_H
#define OCTAVE_CONTROL_H

#include "led/LEDController.h"
#include "objects/Globals.h"
#include "bt/BluetoothController.h"

template <typename GpioExpander, typename LedManager>
class OctaveControl {
public:
    OctaveControl(
        GpioExpander& mcp, LedManager& ledController, BluetoothController* bluetoothController)
                : mcp(mcp), ledController(ledController), _bluetoothController(bluetoothController), currentOctave(0),
                    isS3Pressed(false), isS4Pressed(false), areS3S4Pressed(false),
                    _s3s4PressStartTime(0), _bluetoothToggleTriggered(false),
                    _s3PotentialStart(0), _s4PotentialStart(0) {}

    void setBluetoothController(BluetoothController* controller) { _bluetoothController = controller; }

    void begin() {
        mcp.pinMode(S3_PIN, INPUT_PULLUP);
        mcp.pinMode(S4_PIN, INPUT_PULLUP);
    }

    void update() {
        const unsigned long now = millis();
        const bool s3State = !mcp.digitalRead(S3_PIN);
        const bool s4State = !mcp.digitalRead(S4_PIN);

        // If Bluetooth was recently toggled, ignore octave button activity
        // for 4000 ms to avoid accidental octave changes immediately after
        // enabling/disabling Bluetooth.
        if (_bluetoothController) {
            unsigned long lastToggle = _bluetoothController->getLastToggleTime();
            if (lastToggle != 0 && (now - lastToggle) < 4000) {
                // During cooldown, sync internal pressed state to current readings
                // so we ignore both presses and releases for the cooldown period.
                isS3Pressed = s3State;
                isS4Pressed = s4State;
                areS3S4Pressed = s3State && s4State;
                if (!areS3S4Pressed) {
                    _s3s4PressStartTime = 0;
                    _bluetoothToggleTriggered = false;
                }
                return; // still in cooldown
            }
        }

        if (s3State && s4State) {
            if (!areS3S4Pressed) {
                _s3s4PressStartTime = millis();
                areS3S4Pressed = true;
                _bluetoothToggleTriggered = false;
            }
            if (!_bluetoothToggleTriggered && (millis() - _s3s4PressStartTime >= 3000)) {
                if (_bluetoothController) {
                    if (_bluetoothController->isEnabled()) {
                        _bluetoothController->disable();
                        SERIAL_PRINTLN("Bluetooth Disabled by Octave Buttons.");
                    } else {
                        _bluetoothController->enable();
                        SERIAL_PRINTLN("Bluetooth Enabled by Octave Buttons.");
                    }
                }
                _bluetoothToggleTriggered = true;
            }
        } else {
            // Clear simultaneous potential starts when not both pressed
            _s3PotentialStart = (_s3PotentialStart && !s3State) ? 0 : _s3PotentialStart;
            _s4PotentialStart = (_s4PotentialStart && !s4State) ? 0 : _s4PotentialStart;

            // Handle single-button press confirmation (50ms) to avoid false activation
            const unsigned long CONFIRM_MS = 50;

            if (s3State && !s4State) {
                // Start potential timer if not already pressed
                if (!isS3Pressed && _s3PotentialStart == 0) {
                    _s3PotentialStart = now;
                }
                // Confirm after CONFIRM_MS
                if (!isS3Pressed && _s3PotentialStart != 0 && (now - _s3PotentialStart >= CONFIRM_MS)) {
                    // Ensure s4 still not pressed
                    if (!s4State) {
                        isS3Pressed = true;
                    }
                    _s3PotentialStart = 0;
                }
            } else if (s4State && !s3State) {
                if (!isS4Pressed && _s4PotentialStart == 0) {
                    _s4PotentialStart = now;
                }
                if (!isS4Pressed && _s4PotentialStart != 0 && (now - _s4PotentialStart >= CONFIRM_MS)) {
                    if (!s3State) {
                        isS4Pressed = true;
                    }
                    _s4PotentialStart = 0;
                }
            }

            if (!s3State && !s4State) {
                if (areS3S4Pressed) {
                    areS3S4Pressed = false;
                    _s3s4PressStartTime = 0;
                    _bluetoothToggleTriggered = false;
                }
            }

            // Handle individual octave shifts on release
            if (!s3State && isS3Pressed) {
                isS3Pressed = false; // Reset flag immediately on release
                shiftOctave(-1);
                SERIAL_PRINTLN("S3 Released! Octave Down by 1");
            }

            if (!s4State && isS4Pressed) {
                isS4Pressed = false; // Reset flag immediately on release
                shiftOctave(1);
                SERIAL_PRINTLN("S4 Released! Octave Up by 1");
            }
        }
    }

    int getOctave() const {
        return currentOctave;
    }

private:
    void shiftOctave(int shift) {
        currentOctave += shift;
        if (currentOctave < -4) {
            currentOctave = -4;
        } else if (currentOctave > 4) {
            currentOctave = 4;
        }
        
        ledController.pulse(LedColor::OCTAVE_UP, 0);
        ledController.pulse(LedColor::OCTAVE_DOWN, 0);

        if (currentOctave == 0) {
            ledController.set(LedColor::OCTAVE_DOWN, 0);
            ledController.set(LedColor::OCTAVE_UP, 0);
        } else if (currentOctave > 0) {
            ledController.set(LedColor::OCTAVE_DOWN, 0);
            ledController.set(LedColor::OCTAVE_UP, 255);
            if (currentOctave == 1) ledController.pulse(LedColor::OCTAVE_UP, 1500);
            else if (currentOctave == 2) ledController.pulse(LedColor::OCTAVE_UP, 750);
            else if (currentOctave == 3) ledController.pulse(LedColor::OCTAVE_UP, 350);
        } else { // currentOctave < 0
            ledController.set(LedColor::OCTAVE_UP, 0);
            ledController.set(LedColor::OCTAVE_DOWN, 255);
            if (currentOctave == -1) ledController.pulse(LedColor::OCTAVE_DOWN, 1500);
            else if (currentOctave == -2) ledController.pulse(LedColor::OCTAVE_DOWN, 750);
            else if (currentOctave == -3) ledController.pulse(LedColor::OCTAVE_DOWN, 350);
        }
    }

    GpioExpander& mcp;
    LedManager& ledController;
    BluetoothController* _bluetoothController;
    int currentOctave;

    bool isS3Pressed;
    bool isS4Pressed;
    bool areS3S4Pressed;

    unsigned long _s3s4PressStartTime;
    bool _bluetoothToggleTriggered;

    // Potential press timers for debouncing simultaneous press detection
    unsigned long _s3PotentialStart;
    unsigned long _s4PotentialStart;

    static const int S3_PIN = 4;
    static const int S4_PIN = 6;
};

#endif