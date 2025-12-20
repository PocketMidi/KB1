/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * It is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details: <http://www.gnu.org/licenses/>.
 *
 */

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include <Preferences.h>
#include <MIDI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <BLEDevice.h>
#include <esp_bt_main.h>
#include <esp_sleep.h>
#include <bt/BluetoothController.h>
#include <objects/Constants.h>
#include <objects/Globals.h>
#include <objects/Settings.h>
#include <led/LEDController.h>
#include <music/ScaleManager.h>
#include <controls/KeyboardControl.h>
#include <controls/LeverControls.h>
#include <controls/TouchControl.h>
#include <controls/LeverPushControls.h>
#include "controls/OctaveControl.h"

Adafruit_MCP23X17 mcp_U1;
Adafruit_MCP23X17 mcp_U2;

LEDController ledController;

// Pink LED PWM control variables
const int PINK_LED_PWM_PIN = 8; // Update if different
// Ramping handled by LEDController via set(color, target, duration)
const int PWM_MAX = 255;
const int PWM_MIN = 0;
const int PINK_PWM_MAX = PWM_MAX / 4; // Pink LED max brightness at 25%

// Blue LED PWM control variables
const int BLUE_LED_PWM_PIN = 7; // Update if different
// (state tracking moved to read loop)
bool blueLedWasPressed = false;

// Lever push state tracking for blue and pink LED effects
bool leverPushWasPressed = false;
bool leverPushIsPressed = false;
// Ramping durations (ms)
const unsigned long PINK_RAMP_UP_MS = 50;
const unsigned long PINK_RAMP_DOWN_MS = 150;
const unsigned long BLUE_RAMP_UP_MS = 50;
const unsigned long BLUE_RAMP_DOWN_MS = 150;

MIDI_CREATE_INSTANCE(HardwareSerial, Serial0, MIDI);

//----------------------------------
// Octave Control Setup
//----------------------------------
OctaveControl<Adafruit_MCP23X17, LEDController> octaveControl(
    mcp_U2,
    ledController,
    nullptr
);

//----------------------------------
// ScaleManager Setup
//----------------------------------
ScaleSettings scaleSettings = {
    .scaleType = ScaleType::CHROMATIC,
    .rootNote = 60,
};
ScaleManager scaleManager(scaleSettings);

//----------------------------------
// KeyboardControl Setup
//----------------------------------
KeyboardControl<decltype(MIDI), decltype(octaveControl)> keyboardControl(
    MIDI,
    octaveControl,
    scaleManager
);

//----------------------------------
// Lever 1 Setup
//----------------------------------
LeverSettings lever1Settings = {
    .ccNumber = 3,
    .minCCValue = 0,
    .maxCCValue = 127,
    .stepSize = 1,
    .functionMode = LeverFunctionMode::INTERPOLATED,
    .valueMode = ValueMode::BIPOLAR,
    .onsetTime = 100,
    .offsetTime = 100,
    .onsetType = InterpolationType::LINEAR,
    .offsetType = InterpolationType::LINEAR,
};
LeverControls<decltype(MIDI)> lever1(
    &mcp_U1,
    &mcp_U2,
    SWD1_LEFT_PIN,
    SWD1_RIGHT_PIN,
    lever1Settings,
    MIDI,
    ledController,
    LedColor::PINK,
    keyboardControl
);

//----------------------------------
// LeverPush 1  Setup
//----------------------------------
LeverPushSettings leverPush1Settings = {
    .ccNumber = 24,
    .minCCValue = 31,
    .maxCCValue = 127,
    .functionMode = LeverPushFunctionMode::INTERPOLATED,
    .onsetTime = 100,
    .offsetTime = 100,
    .onsetType = InterpolationType::LINEAR,
    .offsetType = InterpolationType::LINEAR,
};
LeverPushControls<decltype(MIDI)> leverPush1(
    &mcp_U1,
    SWD1_CENTER_PIN,
    leverPush1Settings,
    lever1,
    MIDI,
    ledController,
    LedColor::PINK,
    keyboardControl
);

//----------------------------------
// Lever 2 Setup
//----------------------------------
LeverSettings lever2Settings = {
    .ccNumber = 128,
    .minCCValue = 15,
    .maxCCValue = 127,
    .stepSize = 8,
    .functionMode = LeverFunctionMode::INCREMENTAL,
    .valueMode = ValueMode::BIPOLAR,
    .onsetTime = 100,
    .offsetTime = 100,
    .onsetType = InterpolationType::LINEAR,
    .offsetType = InterpolationType::LINEAR,
};
LeverControls<decltype(MIDI)> lever2(
    &mcp_U2,
    &mcp_U2,
    SWD2_LEFT_PIN,
    SWD2_RIGHT_PIN,
    lever2Settings,
    MIDI,
    ledController,
    LedColor::BLUE,
    keyboardControl
);

//----------------------------------
// LeverPush 2 Setup
//----------------------------------
LeverPushSettings leverPush2Settings = {
    .ccNumber = 128,
    .minCCValue = 87,
    .maxCCValue = 87,
    .functionMode = LeverPushFunctionMode::RESET,
    .onsetTime = 100,
    .offsetTime = 100,
    .onsetType = InterpolationType::LINEAR,
    .offsetType = InterpolationType::LINEAR,
};
LeverPushControls<decltype(MIDI)> leverPush2(
        &mcp_U2,
        SWD2_CENTER_PIN,
        leverPush2Settings,
        lever2,
        MIDI,
        ledController,
        LedColor::BLUE,
        keyboardControl
);

//----------------------------------
// Touch Sensor Setup
//----------------------------------
TouchSettings touchSettings = {
    .ccNumber = 1,
    .minCCValue = 64,
    .maxCCValue = 127,
    .functionMode = TouchFunctionMode::CONTINUOUS,
    .threshold = 24000,
};
TouchControl<decltype(MIDI)> touch(
    T1,
    touchSettings,
    24000,
    48000,
    MIDI
);

Preferences preferences;
BluetoothController* bluetoothControllerPtr = nullptr;

// Deep sleep / inactivity tracking
unsigned long lastActivityMillis = 0;
const unsigned long DEEP_SLEEP_IDLE_MS = 300000; // 5 minutes (300000 ms)
bool deepSleepTriggered = false;

// Require a short, confirmed quiet window before starting the deep-sleep countdown
const unsigned long IDLE_CONFIRM_MS = 2000; // require 2s of no activity to confirm idle
unsigned long idleStartMillis = 0;
bool idleConfirmed = false;
unsigned long idleConfirmTime = 0;

// Forward declaration
void enterDeepSleep();

// Touch wake callback (called when touch interrupt fires)
void IRAM_ATTR touchWakeCallback() {
    // empty: waking is handled by hardware wake source
}

void readInputs(void *pvParameters);
void loadSettings() {
    preferences.getBytes("lever1", &lever1Settings, sizeof(LeverSettings));
    preferences.getBytes("leverpush1", &leverPush1Settings, sizeof(LeverPushSettings));
    preferences.getBytes("lever2", &lever2Settings, sizeof(LeverSettings));
    preferences.getBytes("leverpush2", &leverPush2Settings, sizeof(LeverPushSettings));
    preferences.getBytes("touch", &touchSettings, sizeof(TouchSettings));
    preferences.getBytes("scale", &scaleSettings, sizeof(ScaleSettings));

    scaleManager.setScale(scaleSettings.scaleType);
    scaleManager.setRootNote(scaleSettings.rootNote);

    SERIAL_PRINTLN("Settings loaded from Preferences.");
}

//---------------------------------------------------
// Startup LED wave bounce (fast & sharp)
// Sequence: Pink → Blue → Down → Up → Down → Blue
// Bounce back & forth 2×
//---------------------------------------------------
void startupPulseSequence() {
    const int onTime = 80;      // LED ON duration (fast flash)
    const int gapTime = 5;      // quick gap between LEDs
    const int cyclePause = 100; // short pause after full bounce

    // Wave bounce 2×
    for (int i = 0; i < 2; i++) {
        // Forward wave
        ledController.set(LedColor::PINK, PINK_PWM_MAX);
        delay(onTime);
        ledController.set(LedColor::PINK, 0);
        delay(gapTime);

        ledController.set(LedColor::BLUE, PWM_MAX);
        delay(onTime);
        ledController.set(LedColor::BLUE, 0);
        delay(gapTime);

        ledController.set(LedColor::OCTAVE_DOWN, 1);
        delay(onTime);
        ledController.set(LedColor::OCTAVE_DOWN, 0);
        delay(gapTime);

        ledController.set(LedColor::OCTAVE_UP, 1);
        delay(onTime);
        ledController.set(LedColor::OCTAVE_UP, 0);
        delay(gapTime);

        // Backward wave (stop at Blue, don’t repeat Pink here)
        ledController.set(LedColor::OCTAVE_DOWN, 1);
        delay(onTime);
        ledController.set(LedColor::OCTAVE_DOWN, 0);
        delay(gapTime);

        ledController.set(LedColor::BLUE, PWM_MAX);
        delay(onTime);
        ledController.set(LedColor::BLUE, 0);

        delay(cyclePause);
    }

    // End of startup sequence — keep the wave/chase only.
    // Short pause so the last wave step is perceptible before normal operation.
    delay(100);
}

// Single blink sequence used just before entering deep sleep
void sleepBlinkOnce() {
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


//---------------------------------------------------
//
//  Setup
//
//---------------------------------------------------
void setup() {
    SERIAL_BEGIN();
    SERIAL_PRINTLN("Serial monitor started.");

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW); // Keep LED on

    if (!preferences.begin("kb1-settings", false)) {
        SERIAL_PRINTLN("Error initializing Preferences. Rebooting...");
        ESP.restart();
    } else {
        SERIAL_PRINTLN("Preferences initialized successfully.");
    }

    loadSettings();

    if (!mcp_U1.begin_I2C(0x20)) {
        SERIAL_PRINTLN("Error initializing U1.");
        while (true) {}
    }
    if (!mcp_U2.begin_I2C(0x21)) {
        SERIAL_PRINTLN("Error initializing U2.");
        while (true) {}
    }

    delay(100); // Allow MCP to settle

    mcp_U1.pinMode(SWD1_LEFT_PIN, INPUT_PULLUP);
    mcp_U1.pinMode(SWD1_CENTER_PIN, INPUT_PULLUP);

    mcp_U2.pinMode(SWD1_RIGHT_PIN, INPUT_PULLUP);
    mcp_U2.pinMode(SWD2_LEFT_PIN, INPUT_PULLUP);
    mcp_U2.pinMode(SWD2_CENTER_PIN, INPUT_PULLUP);
    mcp_U2.pinMode(SWD2_RIGHT_PIN, INPUT_PULLUP);

    xTaskCreatePinnedToCore(readInputs, "readInputs", 4096, nullptr, 1, nullptr, 1);

    MIDI.begin(1);

    octaveControl.begin();
    keyboardControl.begin();

    // Register velocity hook to keep lever2 in sync when velocity changes
    auto velocityHook = [](int v) {
        lever2.setValue(v);
    };
    keyboardControl.registerVelocityChangeHook(velocityHook);

    ledController.begin(LedColor::OCTAVE_UP, 7, &mcp_U2);
    ledController.begin(LedColor::OCTAVE_DOWN, 5, &mcp_U2);

    ledController.begin(LedColor::BLUE, BLUE_LED_PWM_PIN);
    ledController.begin(LedColor::PINK, PINK_LED_PWM_PIN);
    pinMode(PINK_LED_PWM_PIN, OUTPUT); // Ensure PWM pin is set
    pinMode(BLUE_LED_PWM_PIN, OUTPUT); // Ensure PWM pin is set

    // Run LED startup sequence
    startupPulseSequence();

    // Initialize Bluetooth controller

    bluetoothControllerPtr = new BluetoothController(
        preferences,
        scaleManager,
        ledController,
        lever1Settings,
        leverPush1Settings,
        lever2Settings,
        leverPush2Settings,
        touchSettings,
        scaleSettings
    );

    octaveControl.setBluetoothController(bluetoothControllerPtr);

    // Initialize activity timer
    lastActivityMillis = millis();
}

//---------------------------------------------------
//
//  Loop
//
//---------------------------------------------------
void loop() {
    ledController.update();
    // Check BLE idle and enter modem sleep if needed (90 seconds = 90000 ms)
    if (bluetoothControllerPtr) {
        bluetoothControllerPtr->checkIdleAndSleep(90000);
    }
    vTaskDelay(1 / portTICK_PERIOD_MS);
}
[[noreturn]] void readInputs(void *pvParameters) {
    while (true) {
        touch.update();
        lever1.update();
        lever2.update();
        leverPush1.update();
        leverPush2.update();
        octaveControl.update();
        keyboardControl.updateKeyboardState();

        // Query touch sensor active state (affects LED behavior)
        bool touchActive = touch.isActive();

        // Activity detection: any active input counts as activity
        bool activityDetected = false;

        // Pink LED: compute target and use LEDController ramping
        bool lever1Right = mcp_U2.digitalRead(SWD1_RIGHT_PIN) == LOW;
        bool lever2Right = mcp_U2.digitalRead(SWD2_RIGHT_PIN) == LOW;
        bool pinkLedPressed = lever1Right || lever2Right;
        static int _lastPinkTarget = -1;
        int _pinkTarget = pinkLedPressed ? PINK_PWM_MAX : 0;
        if (touchActive) {
            _pinkTarget = PINK_PWM_MAX;
        }
        if (_pinkTarget != _lastPinkTarget) {
            unsigned long dur = (_pinkTarget > _lastPinkTarget) ? PINK_RAMP_UP_MS : PINK_RAMP_DOWN_MS;
            ledController.set(LedColor::PINK, _pinkTarget, dur);
            _lastPinkTarget = _pinkTarget;
        }

        // Blue LED: compute left/push targets and use LEDController ramping
        bool lever1Left = mcp_U1.digitalRead(SWD1_LEFT_PIN) == LOW;
        bool lever2Left = mcp_U2.digitalRead(SWD2_LEFT_PIN) == LOW;
        bool blueLeftPressed = lever1Left || lever2Left;

        // Lever push logic for blue LED
        bool leverPush1Pressed = mcp_U1.digitalRead(SWD1_CENTER_PIN) == LOW;
        bool leverPush2Pressed = mcp_U2.digitalRead(SWD2_CENTER_PIN) == LOW;
        leverPushIsPressed = leverPush1Pressed || leverPush2Pressed;

        static int _lastBlueTarget = -1;
        int leftTarget = blueLeftPressed ? PWM_MAX : 0;
        int pushTarget = leverPushIsPressed ? (PWM_MAX / 2) : 0;
        int _blueTarget = max(leftTarget, pushTarget);
        if (touchActive) {
            _blueTarget = PWM_MAX;
        }
        if (_blueTarget != _lastBlueTarget) {
            unsigned long dur = (_blueTarget > _lastBlueTarget) ? BLUE_RAMP_UP_MS : BLUE_RAMP_DOWN_MS;
            ledController.set(LedColor::BLUE, _blueTarget, dur);
            _lastBlueTarget = _blueTarget;
        }

        leverPushWasPressed = leverPushIsPressed;        

        // Determine activity: touch active, any keyboard key, or any pressed switch
        bool keyboardActive = keyboardControl.anyKeyActive();
        activityDetected = touchActive || keyboardActive || pinkLedPressed || blueLeftPressed || leverPushIsPressed;

        if (activityDetected) {
            // Reset idle confirmation and mark last activity time
            idleStartMillis = 0;
            idleConfirmed = false;
            lastActivityMillis = millis();
            deepSleepTriggered = false;
        } else {
            // No activity detected currently — start/continue idle confirmation
            if (!idleConfirmed) {
                if (idleStartMillis == 0) {
                    idleStartMillis = millis();
                } else if (millis() - idleStartMillis >= IDLE_CONFIRM_MS) {
                    // We've had a confirmed quiet window — start deep-sleep countdown
                    idleConfirmed = true;
                    idleConfirmTime = millis();
                }
            } else {
                // idleConfirmed true => count towards deep sleep timeout
                if (!deepSleepTriggered && (millis() - idleConfirmTime >= DEEP_SLEEP_IDLE_MS)) {
                    deepSleepTriggered = true;
                    enterDeepSleep();
                }
            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void enterDeepSleep() {
    const uint32_t uptimeMs = millis();
    const uint32_t idleMs = (uptimeMs > lastActivityMillis) ? (uptimeMs - lastActivityMillis) : 0;
    const uint8_t wakePin = T1;
    const uint16_t wakeThreshold = 40; // may need tuning
    const uint32_t freeHeap = ESP.getFreeHeap();

    SERIAL_PRINTLN("--- Deep Sleep Report ---");
    SERIAL_PRINT("Uptime (ms): "); SERIAL_PRINTLN(uptimeMs);
    SERIAL_PRINT("Idle before sleep (ms): "); SERIAL_PRINTLN(idleMs);
    SERIAL_PRINT("Touch wake pin: "); SERIAL_PRINTLN(wakePin);
    SERIAL_PRINT("Touch wake threshold: "); SERIAL_PRINTLN(wakeThreshold);
    SERIAL_PRINT("Free heap (bytes): "); SERIAL_PRINTLN(freeHeap);
    SERIAL_PRINTLN("-------------------------");

    // Configure touch interrupt for wake
    touchAttachInterrupt(wakePin, touchWakeCallback, wakeThreshold);

    // Enable touch pad wakeup from sleep (pin and threshold required)
    touchSleepWakeUpEnable(wakePin, wakeThreshold);

    // Single blink to indicate we're about to sleep
    sleepBlinkOnce();

    SERIAL_PRINTLN("Entering deep sleep now.");
    delay(50);

    esp_deep_sleep_start();
}
