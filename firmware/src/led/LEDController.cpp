#include "LEDController.h"

// Helper function for linear interpolation
float lerp(float a, float b, float f) {
    return a + f * (b - a);
}

LEDController::LEDController() {
    // Initialize LED states
    pinkLed.pin = -1;
    pinkLed.mcp = nullptr;
    pinkLed.mode = LedMode::STATIC;
    pinkLed.currentBrightness = 0;
    pinkLed.targetBrightness = 0;
    pinkLed.startBrightness = 0;
    pinkLed.startTime = 0;
    pinkLed.duration = 0;
    pinkLed.pulseDuration = 0;

    blueLed.pin = -1;
    blueLed.mcp = nullptr;
    blueLed.mode = LedMode::STATIC;
    blueLed.currentBrightness = 0;
    blueLed.targetBrightness = 0;
    blueLed.startBrightness = 0;
    blueLed.startTime = 0;
    blueLed.duration = 0;
    blueLed.pulseDuration = 0;

    octaveUpLed.pin = -1;
    octaveUpLed.mcp = nullptr;
    octaveUpLed.mode = LedMode::STATIC;
    octaveUpLed.currentBrightness = 0;
    octaveUpLed.targetBrightness = 0;
    octaveUpLed.startBrightness = 0;
    octaveUpLed.startTime = 0;
    octaveUpLed.duration = 0;
    octaveUpLed.pulseDuration = 0;

    octaveDownLed.pin = -1;
    octaveDownLed.mcp = nullptr;
    octaveDownLed.mode = LedMode::STATIC;
    octaveDownLed.currentBrightness = 0;
    octaveDownLed.targetBrightness = 0;
    octaveDownLed.startBrightness = 0;
    octaveDownLed.startTime = 0;
    octaveDownLed.duration = 0;
    octaveDownLed.pulseDuration = 0;
}

void LEDController::begin(LedColor color, int pin, Adafruit_MCP23X17* mcp) {
    LedState* led = nullptr;
    switch (color) {
        case LedColor::PINK:
            led = &pinkLed;
            break;
        case LedColor::BLUE:
            led = &blueLed;
            break;
        case LedColor::OCTAVE_UP:
            led = &octaveUpLed;
            break;
        case LedColor::OCTAVE_DOWN:
            led = &octaveDownLed;
            break;
    }

    if (led) {
        led->pin = pin;
        led->mcp = mcp;
        if (mcp) {
            mcp->pinMode(pin, OUTPUT);
            mcp->digitalWrite(pin, HIGH); // Off for active-low
        } else {
            pinMode(led->pin, OUTPUT);
            analogWrite(led->pin, led->currentBrightness);
        }
    }
}

void LEDController::set(LedColor color, int targetBrightness, unsigned long duration) {
    LedState* led = nullptr;
    switch (color) {
        case LedColor::PINK:
            led = &pinkLed;
            break;
        case LedColor::BLUE:
            led = &blueLed;
            break;
        case LedColor::OCTAVE_UP:
            led = &octaveUpLed;
            break;
        case LedColor::OCTAVE_DOWN:
            led = &octaveDownLed;
            break;
    }

    if (led) {
        led->mode = LedMode::STATIC;
        led->targetBrightness = constrain(targetBrightness, 0, 255);
        led->startBrightness = led->currentBrightness;
        led->startTime = millis();
        led->duration = duration;
        if (duration == 0) {
            led->currentBrightness = led->targetBrightness;
            if (led->mcp) {
                led->mcp->digitalWrite(led->pin, led->currentBrightness > 0 ? LOW : HIGH);
            } else {
                analogWrite(led->pin, led->currentBrightness);
            }
        }
    }
}

void LEDController::pulse(LedColor color, unsigned long duration) {
    LedState* led = nullptr;
    switch (color) {
        case LedColor::PINK:
            led = &pinkLed;
            break;
        case LedColor::BLUE:
            led = &blueLed;
            break;
        case LedColor::OCTAVE_UP:
            led = &octaveUpLed;
            break;
        case LedColor::OCTAVE_DOWN:
            led = &octaveDownLed;
            break;
    }

    if (led) {
        led->mode = LedMode::PULSE;
        led->pulseDuration = duration;
        led->startTime = millis();
    }
}

void LEDController::update() {
    updateLed(pinkLed);
    updateLed(blueLed);
    updateLed(octaveUpLed);
    updateLed(octaveDownLed);
}

void LEDController::updateLed(LedState& led) {
    if (led.pin == -1) return; // LED not initialized

    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - led.startTime;

    if (led.mcp) { // MCP-controlled LED (digital, active-low)
        if (led.mode == LedMode::STATIC) {
            if (elapsedTime >= led.duration) {
                led.currentBrightness = led.targetBrightness;
                led.mcp->digitalWrite(led.pin, led.currentBrightness > 0 ? LOW : HIGH);
            } else {
                // No interpolation for MCP, just wait until duration ends
            }
        } else if (led.mode == LedMode::PULSE) {
            float progress = (float)(elapsedTime % led.pulseDuration) / (float)led.pulseDuration;
            int state = progress < 0.5f ? LOW : HIGH; // 50% duty cycle
            led.mcp->digitalWrite(led.pin, state);
            led.currentBrightness = (state == LOW) ? 255 : 0;
        }
    } else { // GPIO-controlled LED (analog)
        if (led.mode == LedMode::STATIC) {
            if (elapsedTime < led.duration) {
                float progress = (float)elapsedTime / (float)led.duration;
                led.currentBrightness = (int)lerp(led.startBrightness, led.targetBrightness, progress);
                analogWrite(led.pin, led.currentBrightness);
            } else {
                led.currentBrightness = led.targetBrightness;
                analogWrite(led.pin, led.currentBrightness);
            }
        } else if (led.mode == LedMode::PULSE) {
            float progress = (float)(elapsedTime % led.pulseDuration) / (float)led.pulseDuration;
            float brightness;
            if (progress < 0.5f) {
                // Fade in
                brightness = 2.0f * progress; // Scale progress from [0, 0.5) to [0, 1)
            } else {
                // Fade out
                brightness = 2.0f * (1.0f - progress); // Scale progress from [0.5, 1) to (1, 0]
            }
            led.currentBrightness = (int)(brightness * 255.0f);
            analogWrite(led.pin, led.currentBrightness);
        }
    }
}
