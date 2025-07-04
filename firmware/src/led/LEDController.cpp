#include "LEDController.h"

LEDController::LEDController() {
    // Initialize LED states
    pinkLed.pin = -1;
    pinkLed.mode = LedMode::STATIC;
    pinkLed.currentBrightness = 0;
    pinkLed.targetBrightness = 0;
    pinkLed.startTime = 0;
    pinkLed.duration = 0;
    pinkLed.pulseSpeed = 0;
    pinkLed.lastPulseMillis = 0;
    pinkLed.fadeAmount = 1;

    blueLed.pin = -1;
    blueLed.mode = LedMode::STATIC;
    blueLed.currentBrightness = 0;
    blueLed.targetBrightness = 0;
    blueLed.startTime = 0;
    blueLed.duration = 0;
    blueLed.pulseSpeed = 0;
    blueLed.lastPulseMillis = 0;
    blueLed.fadeAmount = 1;
}

void LEDController::begin(LedColor color, int pin) {
    LedState* led = nullptr;
    if (color == LedColor::PINK) {
        led = &pinkLed;
    } else if (color == LedColor::BLUE) {
        led = &blueLed;
    }

    if (led) {
        led->pin = pin;
        pinMode(led->pin, OUTPUT);
        analogWrite(led->pin, led->currentBrightness);
    }
}

void LEDController::set(LedColor color, int targetBrightness, unsigned long duration) {
    LedState* led = nullptr;
    if (color == LedColor::PINK) {
        led = &pinkLed;
    } else if (color == LedColor::BLUE) {
        led = &blueLed;
    }

    if (led) {
        led->mode = LedMode::STATIC;
        led->targetBrightness = constrain(targetBrightness, 0, 255);
        led->startTime = millis();
        led->duration = duration;
        if (duration == 0) {
            led->currentBrightness = led->targetBrightness;
            analogWrite(led->pin, led->currentBrightness);
        }
    }
}

void LEDController::pulse(LedColor color, unsigned long pulseSpeed) {
    LedState* led = nullptr;
    if (color == LedColor::PINK) {
        led = &pinkLed;
    } else if (color == LedColor::BLUE) {
        led = &blueLed;
    }

    if (led) {
        led->mode = LedMode::PULSE;
        led->pulseSpeed = pulseSpeed;
        led->lastPulseMillis = millis();
        led->fadeAmount = 1; // Reset fade direction
    }
}

void LEDController::update() {
    updateLed(pinkLed);
    updateLed(blueLed);
}

void LEDController::updateLed(LedState& led) {
    if (led.pin == -1) return; // LED not initialized

    unsigned long currentTime = millis();

    if (led.mode == LedMode::STATIC) {
        if (led.duration > 0 && led.currentBrightness != led.targetBrightness) {
            unsigned long elapsedTime = currentTime - led.startTime;
            if (elapsedTime < led.duration) {
                led.currentBrightness = map(elapsedTime, 0, led.duration, led.currentBrightness, led.targetBrightness);
                analogWrite(led.pin, led.currentBrightness);
            } else {
                led.currentBrightness = led.targetBrightness;
                analogWrite(led.pin, led.currentBrightness);
            }
        }
    } else if (led.mode == LedMode::PULSE) {
        if (currentTime - led.lastPulseMillis >= led.pulseSpeed) {
            led.lastPulseMillis = currentTime;
            
            // Simple breathing effect: fade up and down
            led.currentBrightness += led.fadeAmount;
            if (led.currentBrightness <= 0 || led.currentBrightness >= 255) {
                led.fadeAmount *= -1; // Reverse direction
            }
            analogWrite(led.pin, led.currentBrightness);
        }
    }
}
