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
          s3(), s4(), lastShiftMs(0) {}

    void begin() {
        mcp.pinMode(S3_PIN, INPUT_PULLUP);
        mcp.pinMode(S4_PIN, INPUT_PULLUP);
        // Suppressed until endBootSuppression() is called - see that method for why.
        bootSuppressed = true;
    }

    // Call once the boot-time LED sequence (startupPulseSequence) has finished. That
    // sequence writes OCTAVE_UP/OCTAVE_DOWN on the same MCP chip as these button pins for
    // over a second, and a fixed post-begin() delay can't reliably outlast it - confirmed
    // by boot diagnostics showing spurious edges while the LED sequence was still running.
    void endBootSuppression() {
        bootSuppressed = false;
    }

    void update(const GPIOCache& gpioCache) {
        // Extract octave button states from cached GPIO (no I2C overhead)
        // S3 and S4 are on U2 (pins 4 and 6)
        const unsigned long nowMs = millis();
        handleButton(s3, gpioCache.isU2PinLow(S3_PIN), -1, nowMs);
        handleButton(s4, gpioCache.isU2PinLow(S4_PIN), 1, nowMs);
    }

    int getOctave() const {
        return currentOctave;
    }

private:
    struct ButtonState {
        bool lastReading = false;
        bool debouncedState = false;
        bool armed = false;
        unsigned long lastChangeMs = 0;
    };

    // Shift on the debounced press edge, then lock out retriggers for RETRIGGER_LOCKOUT_MS.
    // The lockout (not the release debounce) is what rejects contact bounce.
    void handleButton(ButtonState& btn, bool raw, int shift, unsigned long nowMs) {
        if (raw != btn.lastReading) {
            btn.lastReading = raw;
            btn.lastChangeMs = nowMs;
        }

        const unsigned long debounceMs = raw ? PRESS_DEBOUNCE_MS : RELEASE_DEBOUNCE_MS;
        if ((nowMs - btn.lastChangeMs) < debounceMs) {
            return;
        }

        // MCP pins can read LOW until the pullups settle at boot; ignore edges until a
        // stable release has been observed so the settle doesn't look like a press.
        if (!btn.armed) {
            if (!raw && !bootSuppressed) {
                btn.armed = true;
                btn.debouncedState = false;
            }
            return;
        }

        // Suppress any shift while the boot LED sequence is still running on the same
        // MCP chip, regardless of arm state.
        if (bootSuppressed) {
            return;
        }

        if (raw == btn.debouncedState) {
            return;
        }

        btn.debouncedState = raw;
        if (!raw) {
            return;  // act on press only
        }

        if (nowMs - lastShiftMs < RETRIGGER_LOCKOUT_MS) {
            return;
        }
        lastShiftMs = nowMs;

        shiftOctave(shift);
        char buf[8];
        snprintf(buf, sizeof(buf), "O%+d", currentOctave);
        SERIAL_PRINTLN(buf);
    }

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

    ButtonState s3;
    ButtonState s4;
    unsigned long lastShiftMs;
    bool bootSuppressed = true;

    static const int S3_PIN = 4;
    static const int S4_PIN = 6;

    static constexpr unsigned long PRESS_DEBOUNCE_MS = 4;
    static constexpr unsigned long RELEASE_DEBOUNCE_MS = 40;
    static constexpr unsigned long RETRIGGER_LOCKOUT_MS = 180;
};

#endif