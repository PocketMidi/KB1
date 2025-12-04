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
unsigned long pinkLedPressStart = 0;
unsigned long pinkLedReleaseStart = 0;
float pinkLedBrightness = 0.0f;
bool pinkLedWasPressed = false;
const int PWM_MAX = 255;
const int PWM_MIN = 0;
const int PINK_PWM_MAX = PWM_MAX / 4; // Pink LED max brightness at 25%
// Ramp step sizes: doubled to make transitions twice as fast
const float BRIGHTNESS_STEP_UP = (PINK_PWM_MAX / (50.0f / 10.0f)) * 2.0f; // ramp up in 50ms (10ms loop)
const float BRIGHTNESS_STEP_DOWN = (PINK_PWM_MAX / (150.0f / 10.0f)) * 2.0f; // ramp down in 150ms (10ms loop)
const float BLUE_LEFT_STEP_UP = (PWM_MAX / (50.0f / 10.0f)) * 2.0f; // ramp up in 50ms (10ms loop)
const float BLUE_LEFT_STEP_DOWN = (PWM_MAX / (150.0f / 10.0f)) * 2.0f; // ramp down in 150ms (10ms loop)

// Blue LED PWM control variables
const int BLUE_LED_PWM_PIN = 7; // Update if different
unsigned long blueLedPressStart = 0;
unsigned long blueLedReleaseStart = 0;
float blueLedBrightness = 0.0f;
bool blueLedWasPressed = false;

// Lever push state tracking for blue and pink LED effects
bool leverPushWasPressed = false;
bool leverPushIsPressed = false;
unsigned long pinkPulseStart = 0;
bool pinkPulseActive = false;
const int PINK_PULSE_MAX = PINK_PWM_MAX;
// Make the pink pulse fade twice as fast as well
const float PINK_PULSE_STEP_UP = 4.0f;
const float PINK_PULSE_STEP_DOWN = 4.0f;
const int PINK_PULSE_DURATION = 80; // ms at peak

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
    .minCCValue = 32,
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
    .minCCValue = 0,
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
    .minCCValue = 81,
    .maxCCValue = 127,
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
};
TouchControl<decltype(MIDI)> touch(
    T1,
    touchSettings,
    26000,
    155000, 
    26000,
    MIDI
);

Preferences preferences;
BluetoothController* bluetoothControllerPtr = nullptr;

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
        ledController.set(LedColor::PINK, true);
        delay(onTime);
        ledController.set(LedColor::PINK, false);
        delay(gapTime);

        ledController.set(LedColor::BLUE, true);
        delay(onTime);
        ledController.set(LedColor::BLUE, false);
        delay(gapTime);

        ledController.set(LedColor::OCTAVE_DOWN, true);
        delay(onTime);
        ledController.set(LedColor::OCTAVE_DOWN, false);
        delay(gapTime);

        ledController.set(LedColor::OCTAVE_UP, true);
        delay(onTime);
        ledController.set(LedColor::OCTAVE_UP, false);
        delay(gapTime);

        // Backward wave (stop at Blue, don’t repeat Pink here)
        ledController.set(LedColor::OCTAVE_DOWN, true);
        delay(onTime);
        ledController.set(LedColor::OCTAVE_DOWN, false);
        delay(gapTime);

        ledController.set(LedColor::BLUE, true);
        delay(onTime);
        ledController.set(LedColor::BLUE, false);

        delay(cyclePause);
    }

    // End of startup sequence — keep the wave/chase only.
    // Short pause so the last wave step is perceptible before normal operation.
    delay(100);
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

    ledController.begin(LedColor::OCTAVE_UP, 7, &mcp_U2);
    ledController.begin(LedColor::OCTAVE_DOWN, 5, &mcp_U2);

    ledController.begin(LedColor::BLUE, BLUE_LED_PWM_PIN);
    ledController.begin(LedColor::PINK, PINK_LED_PWM_PIN);
    pinMode(PINK_LED_PWM_PIN, OUTPUT); // Ensure PWM pin is set
    pinMode(BLUE_LED_PWM_PIN, OUTPUT); // Ensure PWM pin is set

    // Run LED startup sequence
    startupPulseSequence();

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
}

//---------------------------------------------------
//
//  Loop
//
//---------------------------------------------------
void loop() {
    ledController.update();
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

        // Smooth PWM brightness logic for pink LED (right)
        bool lever1Right = mcp_U2.digitalRead(SWD1_RIGHT_PIN) == LOW;
        bool lever2Right = mcp_U2.digitalRead(SWD2_RIGHT_PIN) == LOW;
        bool pinkLedPressed = lever1Right || lever2Right;

        if (pinkLedPressed) {
            if (pinkLedBrightness < PINK_PWM_MAX) {
                pinkLedBrightness += BRIGHTNESS_STEP_UP;
                if (pinkLedBrightness > PINK_PWM_MAX) pinkLedBrightness = PINK_PWM_MAX;
            }
        } else {
            if (pinkLedBrightness > PWM_MIN) {
                pinkLedBrightness -= BRIGHTNESS_STEP_DOWN;
                if (pinkLedBrightness < PWM_MIN) pinkLedBrightness = PWM_MIN;
            }
        }
        analogWrite(PINK_LED_PWM_PIN, (int)pinkLedBrightness);

        // Smooth PWM brightness logic for blue LED (left)
        bool lever1Left = mcp_U1.digitalRead(SWD1_LEFT_PIN) == LOW;
        bool lever2Left = mcp_U2.digitalRead(SWD2_LEFT_PIN) == LOW;
        bool blueLedPressed = lever1Left || lever2Left;

        static bool blueLedAtMax = false;
        static float blueLeftBrightness = 0.0f;
        if (blueLedPressed) {
            if (blueLeftBrightness < PWM_MAX) {
                blueLeftBrightness += BLUE_LEFT_STEP_UP;
                if (blueLeftBrightness > PWM_MAX) blueLeftBrightness = (float)PWM_MAX;
            }
        } else {
            if (blueLeftBrightness > PWM_MIN) {
                blueLeftBrightness -= BLUE_LEFT_STEP_DOWN;
                if (blueLeftBrightness < PWM_MIN) blueLeftBrightness = (float)PWM_MIN;
            }
        }
        analogWrite(BLUE_LED_PWM_PIN, (int)(blueLeftBrightness + 0.5f));

        // Lever push logic for blue LED
        bool leverPush1Pressed = mcp_U1.digitalRead(SWD1_CENTER_PIN) == LOW;
        bool leverPush2Pressed = mcp_U2.digitalRead(SWD2_CENTER_PIN) == LOW;
        leverPushIsPressed = leverPush1Pressed || leverPush2Pressed;
        unsigned long now = millis();

        static float bluePushBrightness = 0.0f;
        static unsigned long pinkBlinkStart = 0;
        static bool pinkBlinkActive = false;
        static bool pinkBlinkPending = false;

        if (leverPushIsPressed) {
            bluePushBrightness = PWM_MAX / 2; // Dim blue LED by half on push
        } else {
            bluePushBrightness = PWM_MIN; // Immediate off after release
        }

        // Combine blue LED brightness from lever left and push
        float blueLedFinalBrightness = max(blueLeftBrightness, bluePushBrightness);
        analogWrite(BLUE_LED_PWM_PIN, (int)blueLedFinalBrightness);

        // Pink LED blink at 31 brightness, 100ms after push release
        if (!leverPushIsPressed && leverPushWasPressed) {
            pinkBlinkPending = true;
            pinkBlinkStart = now;
        }
        if (pinkBlinkPending && (now - pinkBlinkStart >= 100)) {
            pinkBlinkActive = true;
            pinkBlinkPending = false;
            pinkLedBrightness = 31.0f / 2.0f; // Dim pink LED by half on release blink
        }
        if (pinkBlinkActive) {
            pinkLedBrightness -= PINK_PULSE_STEP_DOWN;
            if (pinkLedBrightness <= PWM_MIN) {
                pinkLedBrightness = PWM_MIN;
                pinkBlinkActive = false;
            }
            analogWrite(PINK_LED_PWM_PIN, (int)pinkLedBrightness);
        }
        leverPushWasPressed = leverPushIsPressed;

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
