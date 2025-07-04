#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>

enum class LedColor {
    PINK,
    BLUE
};

enum class LedMode {
    STATIC,
    PULSE
};

class LEDController {
public:
    LEDController();
    void begin(LedColor color, int pin);
    void set(LedColor color, int targetBrightness, unsigned long duration = 0);
    void pulse(LedColor color, unsigned long pulseSpeed);
    void update();

private:
    struct LedState {
        int pin;
        LedMode mode;
        int currentBrightness;
        int targetBrightness;
        unsigned long startTime;
        unsigned long duration; // For interpolation
        unsigned long pulseSpeed; // For pulsing
        unsigned long lastPulseMillis; // For pulsing
        int fadeAmount; // For pulsing
    };

    LedState pinkLed;
    LedState blueLed;

    void updateLed(LedState& led);
};

#endif
