#ifndef OCTAVE_CONTROL_H
#define OCTAVE_CONTROL_H

#include "led/LEDController.h"
#include "objects/Globals.h"

template <typename GpioExpander, typename LedManager>
class OctaveControl {
public:
    OctaveControl(
        GpioExpander& mcp, LedManager& ledController)
        : mcp(mcp), ledController(ledController), currentOctave(0),
          isS3Pressed(false), isS4Pressed(false) {}

    void begin() {
        mcp.pinMode(S3_PIN, INPUT_PULLUP);
        mcp.pinMode(S4_PIN, INPUT_PULLUP);
    }

    void update(const GPIOCache& gpioCache) {
        // Extract octave button states from cached GPIO (no I2C overhead)
        // S3 and S4 are on U2 (pins 4 and 6)
        const bool s3State = gpioCache.isU2PinLow(S3_PIN);
        const bool s4State = gpioCache.isU2PinLow(S4_PIN);

        // Handle individual octave shifts on release
        if (!s3State && isS3Pressed) {
            isS3Pressed = false;
            shiftOctave(-1);
            char buf[8];
            snprintf(buf, sizeof(buf), "O%+d", currentOctave);
            SERIAL_PRINTLN(buf);
        } else if (s3State) {
            isS3Pressed = true;
        }

        if (!s4State && isS4Pressed) {
            isS4Pressed = false;
            shiftOctave(1);
            char buf[8];
            snprintf(buf, sizeof(buf), "O%+d", currentOctave);
            SERIAL_PRINTLN(buf);
        } else if (s4State) {
            isS4Pressed = true;
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
    int currentOctave;

    bool isS3Pressed;
    bool isS4Pressed;

    static const int S3_PIN = 4;
    static const int S4_PIN = 6;
};

#endif