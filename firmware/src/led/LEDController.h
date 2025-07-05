#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>

enum class LedColor {
    PINK,
    BLUE,
    OCTAVE_UP,
    OCTAVE_DOWN
};

enum class LedMode {
    STATIC,
    PULSE
};

class LEDController {
public:
    LEDController();
    void begin(LedColor color, int pin, Adafruit_MCP23X17* mcp = nullptr);
    void set(LedColor color, int targetBrightness, unsigned long duration = 0);
    void pulse(LedColor color, unsigned long pulseSpeed, unsigned long totalPulsationDuration = 0);
    void update();

private:
    struct LedState {
        int pin;
        Adafruit_MCP23X17* mcp;
        LedMode mode;
        int currentBrightness;
        int targetBrightness;
        int startBrightness;
        unsigned long startTime;
        unsigned long duration; // For interpolation or total pulsation duration
        unsigned long pulseDuration; // For individual pulse cycle duration
    };

    LedState pinkLed{};
    LedState blueLed{};
    LedState octaveUpLed{};
    LedState octaveDownLed{};

    static void updateLed(LedState& led);
};

#endif
