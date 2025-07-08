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
    .onsetTime = 1000,
    .offsetTime = 1000,
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
    .minCCValue = 0,
    .maxCCValue = 127,
    .functionMode = LeverPushFunctionMode::INTERPOLATED,
    .onsetTime = 200,
    .offsetTime = 200,
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
    .ccNumber = 7,
    .minCCValue = 0,
    .maxCCValue = 127,
    .stepSize = 2,
    .functionMode = LeverFunctionMode::INCREMENTAL,
    .valueMode = ValueMode::BIPOLAR,
    .onsetTime = 200,
    .offsetTime = 200,
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
    .ccNumber = 7,
    .minCCValue = 81,
    .maxCCValue = 127,
    .functionMode = LeverPushFunctionMode::RESET,
    .onsetTime = 200,
    .offsetTime = 200,
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
    .ccNumber = 51,
    .minCCValue = 0,
    .maxCCValue = 127,
    .functionMode = TouchFunctionMode::HOLD,
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
//
//  Setup
//
//---------------------------------------------------
void setup() {
    SERIAL_BEGIN();
    SERIAL_PRINTLN("Serial monitor started.");

    // initialize digital pin LED_BUILTIN as an output.
    pinMode(LED_BUILTIN, OUTPUT);
    // Keep built-in LED on continuously (active low for some boards)
    digitalWrite(LED_BUILTIN, LOW);

    // Initialize Preferences
    if (!preferences.begin("kb1-settings", false)) {
        SERIAL_PRINTLN("Error initializing Preferences. Rebooting...");
        ESP.restart();
    } else {
        SERIAL_PRINTLN("Preferences initialized successfully.");
    }

    loadSettings(); // Load settings after Preferences are initialized

    // Start MCP_U1
    if (!mcp_U1.begin_I2C(0x20)) {
        SERIAL_PRINTLN("Error initializing U1.");
        // ReSharper disable once CppDFAEndlessLoop
        while (true) {}
    }
    // Start MCP_U2
    if (!mcp_U2.begin_I2C(0x21)) {
        SERIAL_PRINTLN("Error initializing U2.");
        // ReSharper disable once CppDFAEndlessLoop
        while (true) {}
    }

    // Add a small delay to allow MCP2 to fully initialize
    delay(100);

    // Configure SWD buttons on U1 as inputs with pull-up
    mcp_U1.pinMode(SWD1_LEFT_PIN, INPUT_PULLUP);
    mcp_U1.pinMode(SWD1_CENTER_PIN, INPUT_PULLUP);

    // Configure SWD buttons on U2 as inputs with pull-up
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

    ledController.begin(LedColor::BLUE, 7);
    ledController.begin(LedColor::PINK, 8);

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
//  Loops
//
//---------------------------------------------------
void loop() {
    ledController.update();
    // Small delay to avoid watchdog timer issues
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

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
