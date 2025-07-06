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
          _s3s4PressStartTime(0), _bluetoothToggleTriggered(false) {}

    void setBluetoothController(BluetoothController* controller) { _bluetoothController = controller; }

    void begin() {
        mcp.pinMode(S3_PIN, INPUT_PULLUP);
        mcp.pinMode(S4_PIN, INPUT_PULLUP);
    }

    void update() {
        const bool s3State = !mcp.digitalRead(S3_PIN);
        const bool s4State = !mcp.digitalRead(S4_PIN);

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
            if (!s3State && !s4State) {
                if (areS3S4Pressed) {
                    areS3S4Pressed = false;
                    _s3s4PressStartTime = 0;
                    _bluetoothToggleTriggered = false;
                }
            }

            // Handle individual octave shifts on release
            if (!s3State && isS3Pressed) {
                shiftOctave(-1);
                SERIAL_PRINTLN("S3 Released! Octave Down by 1");
                isS3Pressed = false;
            } else if (s3State) {
                isS3Pressed = true;
            }

            if (!s4State && isS4Pressed) {
                shiftOctave(1);
                SERIAL_PRINTLN("S4 Released! Octave Up by 1");
                isS4Pressed = false;
            } else if (s4State) {
                isS4Pressed = true;
            }
        }
    }

    int getOctave() const {
        return currentOctave;
    }

private:
    void shiftOctave(int shift) {
        currentOctave += shift;
        if (currentOctave < -3) {
            currentOctave = -3;
        } else if (currentOctave > 3) {
            currentOctave = 3;
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

    static const int S3_PIN = 4;
    static const int S4_PIN = 6;
};

#endif