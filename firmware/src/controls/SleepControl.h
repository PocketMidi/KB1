// SleepControl.h
// Lightweight sleep helper functions extracted from main.cpp
#pragma once

#include <Arduino.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <BLEDevice.h>

#include <controls/TouchControl.h>
#include <controls/KeyboardControl.h>
#include <led/LEDController.h>
#include <bt/BluetoothController.h>
#include <objects/Settings.h>

// These functions are implemented as templates so they can be used with the
// project's templated control types without requiring concrete typedefs here.

// Forward-declare touch wake callback implemented in main.cpp
extern void IRAM_ATTR touchWakeCallback();

// Forward declaration so enterLightSleep can call enterDeepSleep below
template<typename TouchT, typename KeyboardT>
inline void enterDeepSleep(TouchT &touch,
                           LEDController &ledController,
                           BluetoothController* &bluetoothControllerPtr,
                           TouchSettings &touchSettings,
                           unsigned long lastActivityMillis,
                           const uint8_t PINK_LED_PWM_PIN,
                           const uint8_t BLUE_LED_PWM_PIN,
                           const int PINK_PWM_MAX,
                           const int PWM_MAX);

template<typename TouchT, typename KeyboardT>
inline void sleepBlinkOnce(LEDController &ledController, int PINK_PWM_MAX, int PWM_MAX) {
    const int onTime = 80; // LED ON duration
    const int gapTime = 5; // small gap

    // Pink
    ledController.set(LedColor::PINK, PINK_PWM_MAX);
    delay(onTime);
    ledController.set(LedColor::PINK, 0);
    delay(gapTime);

    // Blue
    ledController.set(LedColor::BLUE, PWM_MAX);
    delay(onTime);
    ledController.set(LedColor::BLUE, 0);
    delay(gapTime);

    // Octave down
    ledController.set(LedColor::OCTAVE_DOWN, 1);
    delay(onTime);
    ledController.set(LedColor::OCTAVE_DOWN, 0);
    delay(gapTime);

    // Octave up
    ledController.set(LedColor::OCTAVE_UP, 1);
    delay(onTime);
    ledController.set(LedColor::OCTAVE_UP, 0);
    delay(gapTime);
}

template<typename TouchT, typename KeyboardT>
inline void enterLightSleep(TouchT &touch,
                            KeyboardT &keyboardControl,
                            LEDController &ledController,
                            BluetoothController* &bluetoothControllerPtr,
                            TouchSettings &touchSettings,
                            unsigned long &lastActivityMillis,
                            const uint8_t PINK_LED_PWM_PIN,
                            const uint8_t BLUE_LED_PWM_PIN,
                            const int PINK_PWM_MAX,
                            const int PWM_MAX,
                            const unsigned long PINK_RAMP_UP_MS,
                            const unsigned long PINK_RAMP_DOWN_MS,
                            const unsigned long BLUE_RAMP_UP_MS,
                            const unsigned long BLUE_RAMP_DOWN_MS) {
    SERIAL_PRINTLN("Preparing to enter light sleep mode...");

    const uint32_t LIGHT_SLEEP_MAX_MS = 90000UL; // 90 seconds
    uint32_t lightSleepMaxMs = LIGHT_SLEEP_MAX_MS;
    const uint32_t cycleSleepMs = 2000UL; // 2 seconds between sequences
    const uint32_t pinkOnMs = 150UL;
    const uint32_t gapMs = 5UL;
    const uint32_t blueOnMs = 150UL;

    const uint8_t wakePin = T1;
    const uint32_t cfgThresh = touchSettings.threshold;
    uint32_t wakeCalc = cfgThresh / 2;
    const uint16_t wakeThreshold = (wakeCalc > 0x7FFF) ? 0x7FFF : (uint16_t)wakeCalc;

    uint32_t startMs = millis();

    touchAttachInterrupt(wakePin, touchWakeCallback, wakeThreshold);
    touchSleepWakeUpEnable(wakePin, wakeThreshold);

    // Turn off octave LEDs
    ledController.set(LedColor::OCTAVE_UP, 0);
    ledController.set(LedColor::OCTAVE_DOWN, 0);
    ledController.update();

    bool btWasEnabled = false;
    if (bluetoothControllerPtr) {
        btWasEnabled = bluetoothControllerPtr->isEnabled();
        if (btWasEnabled) bluetoothControllerPtr->disable();
    }

    SERIAL_PRINTLN("Entering light-sleep loop (will fallback to deep-sleep after timeout)...");

    while ((millis() - startMs) < lightSleepMaxMs) {
        if (touch.isActive() || keyboardControl.anyKeyActive() || (millis() - lastActivityMillis < 1000)) {
            SERIAL_PRINTLN("Activity detected before light sleep; aborting.");
            break;
        }

        ledController.set(LedColor::PINK, PINK_PWM_MAX);
        delay(pinkOnMs);
        ledController.set(LedColor::PINK, 0);
        delay(gapMs);

        ledController.set(LedColor::BLUE, PWM_MAX);
        delay(blueOnMs);
        ledController.set(LedColor::BLUE, 0);
        ledController.update();
        delay(5);
        if (digitalRead(BLUE_LED_PWM_PIN) == HIGH) {
            digitalWrite(BLUE_LED_PWM_PIN, LOW);
        }

        esp_sleep_enable_timer_wakeup(cycleSleepMs * 1000ULL);
        touchSleepWakeUpEnable(wakePin, wakeThreshold);

        SERIAL_PRINTLN("Light sleeping for 3s...");
        esp_sleep_enable_touchpad_wakeup();
        esp_light_sleep_start();

        esp_sleep_wakeup_cause_t wakeReason = esp_sleep_get_wakeup_cause();
        if (wakeReason == ESP_SLEEP_WAKEUP_TOUCHPAD) {
            SERIAL_PRINTLN("Woke from light sleep: touchpad");
            lastActivityMillis = millis();
            break;
        } else if (wakeReason == ESP_SLEEP_WAKEUP_TIMER) {
            SERIAL_PRINTLN("Woke from light sleep: timer");
        } else if (wakeReason != ESP_SLEEP_WAKEUP_UNDEFINED) {
            SERIAL_PRINT("Woke from light sleep, reason: ");
            SERIAL_PRINTLN((int)wakeReason);
        }

        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);

        delay(10);
    }

    ledController.begin(LedColor::BLUE, BLUE_LED_PWM_PIN);
    ledController.begin(LedColor::PINK, PINK_LED_PWM_PIN);

    if (bluetoothControllerPtr && btWasEnabled) bluetoothControllerPtr->enable();

    gpio_hold_dis((gpio_num_t)PINK_LED_PWM_PIN);
    gpio_hold_dis((gpio_num_t)BLUE_LED_PWM_PIN);

    if ((millis() - startMs) >= lightSleepMaxMs) {
        SERIAL_PRINTLN("No activity during light sleep window — entering deep sleep.");
        if (!touch.isActive() && !keyboardControl.anyKeyActive()) {
            enterDeepSleep<TouchT, KeyboardT>(touch, ledController, bluetoothControllerPtr, touchSettings, lastActivityMillis, PINK_LED_PWM_PIN, BLUE_LED_PWM_PIN, PINK_PWM_MAX, PWM_MAX);
        } else {
            SERIAL_PRINTLN("Activity detected after light sleep window; not entering deep sleep.");
        }
    } else {
        SERIAL_PRINTLN("Exiting light sleep and resuming operation.");
    }
}

template<typename TouchT, typename KeyboardT>
inline void enterDeepSleep(TouchT &touch,
                           LEDController &ledController,
                           BluetoothController* &bluetoothControllerPtr,
                           TouchSettings &touchSettings,
                           unsigned long lastActivityMillis,
                           const uint8_t PINK_LED_PWM_PIN,
                           const uint8_t BLUE_LED_PWM_PIN,
                           const int PINK_PWM_MAX,
                           const int PWM_MAX) {
    const uint32_t uptimeMs = millis();
    const uint32_t idleMs = (uptimeMs > lastActivityMillis) ? (uptimeMs - lastActivityMillis) : 0;
    const uint8_t wakePin = T1;
    const uint32_t cfgThresh = touchSettings.threshold;
    uint32_t wakeCalc = cfgThresh / 2;
    const uint16_t wakeThreshold = (wakeCalc > 0x7FFF) ? 0x7FFF : (uint16_t)wakeCalc;
    const uint32_t freeHeap = ESP.getFreeHeap();

    SERIAL_PRINTLN("--- Deep Sleep Report ---");
    SERIAL_PRINT("Uptime (ms): "); SERIAL_PRINTLN(uptimeMs);
    SERIAL_PRINT("Idle before sleep (ms): "); SERIAL_PRINTLN(idleMs);
    SERIAL_PRINT("Touch wake pin: "); SERIAL_PRINTLN(wakePin);
    SERIAL_PRINT("Touch wake threshold: "); SERIAL_PRINTLN(wakeThreshold);
    SERIAL_PRINT("Free heap (bytes): "); SERIAL_PRINTLN(freeHeap);
    SERIAL_PRINTLN("-------------------------");

    touchAttachInterrupt(wakePin, touchWakeCallback, wakeThreshold);
    touchSleepWakeUpEnable(wakePin, wakeThreshold);

    sleepBlinkOnce<TouchT, KeyboardT>(ledController, PINK_PWM_MAX, PWM_MAX);

    SERIAL_PRINTLN("Entering deep sleep now.");
    delay(50);
    esp_deep_sleep_start();
}
