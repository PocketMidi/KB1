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
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <BLEDevice.h>
#include <esp_bt_main.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
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
#include <controls/SleepControl.h>

Adafruit_MCP23X17 mcp_U1;
Adafruit_MCP23X17 mcp_U2;

LEDController ledController;

// I2C mutex for thread-safe access to MCP23017 chips
static SemaphoreHandle_t i2cMutex = NULL;

// LED command queue to serialize all access to ledController from one task
typedef struct {
    uint8_t type; // 0 = set immediate, 1 = set with ramp
    uint8_t color;
    int value;
    unsigned long dur;
} LedCommand;

static QueueHandle_t ledQueue = NULL;

// Enqueue LED set (immediate)
static inline void ledSet(LedColor color, int value) {
    if (!ledQueue) {
        ledController.set(color, value);
        return;
    }
    LedCommand cmd = {0, (uint8_t)color, value, 0};
    xQueueSendToBack(ledQueue, &cmd, 0);
}

// Enqueue LED set with ramp/duration
static inline void ledSetRamp(LedColor color, int value, unsigned long dur) {
    if (!ledQueue) {
        ledController.set(color, value, dur);
        return;
    }
    LedCommand cmd = {1, (uint8_t)color, value, dur};
    xQueueSendToBack(ledQueue, &cmd, 0);
}

// LED update task (runs on a separate core) — sole owner of ledController
void ledTask(void *pvParameters) {
    (void)pvParameters;
    LedCommand cmd;
    while (true) {
        // Process any queued commands first
        while (ledQueue && xQueueReceive(ledQueue, &cmd, 0) == pdTRUE) {
            if (cmd.type == 0) {
                ledController.set((LedColor)cmd.color, cmd.value);
            } else {
                ledController.set((LedColor)cmd.color, cmd.value, cmd.dur);
            }
        }
        // Always call update from this task (with I2C mutex protection)
        if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
            ledController.update();
            xSemaphoreGive(i2cMutex);
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

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
    .keyMapping = 0, // 0 = Natural, 1 = Compact
};
ScaleManager scaleManager(scaleSettings);

//----------------------------------
// Chord Settings
//----------------------------------
ChordSettings chordSettings = {
    .playMode = PlayMode::SCALE,
    .chordType = ChordType::MAJOR,
    .strumEnabled = false,
    .velocitySpread = 8,
    .strumSpeed = 45,
};

//----------------------------------
// KeyboardControl Setup
//----------------------------------
KeyboardControl<decltype(MIDI), decltype(octaveControl)> keyboardControl(
    MIDI,
    octaveControl,
    scaleManager,
    chordSettings
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
    .minCCValue = 16,
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
    .minCCValue = 89,
    .maxCCValue = 89,
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
    64000,
    MIDI
);

SystemSettings systemSettings = {
    .lightSleepTimeout = 90,    // 90 seconds
    .deepSleepTimeout = 330,    // 330 seconds (5.5 minutes)
    .bleTimeout = 600,          // 600 seconds (10 minutes)
    .idleConfirmTimeout = 2,    // 2 seconds
};

Preferences preferences;
BluetoothController* bluetoothControllerPtr = nullptr;

//----------------------------------
// Sleep / Deep Sleep Management
//----------------------------------
unsigned long lastActivityMillis = 0;
unsigned long deepSleepIdleMs = 330000; // Default: 5.5 minutes, updated from systemSettings
bool deepSleepTriggered = false;

// Require a short, confirmed quiet window before starting the deep-sleep countdown
unsigned long idleConfirmMs = 2000; // Default: 2s, updated from systemSettings
unsigned long idleStartMillis = 0;
bool idleConfirmed = false;
unsigned long idleConfirmTime = 0;

// Forward declare touch wake callback so it can be referenced by sleep routines
void IRAM_ATTR touchWakeCallback();

// Enter a light-sleep mode with a simple LED heartbeat sequence.
// The sequence runs while the MCU repeatedly enters short light-sleep intervals
// (3s) to conserve power. If touch wake is sensed, return to normal operation.
// If no input for the configured light-sleep window (5 minutes), fall back
// to the deep-sleep path.
// Light / deep-sleep implementations moved to controls/SleepControl.h

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
    preferences.getBytes("chord", &chordSettings, sizeof(ChordSettings));
    preferences.getBytes("system", &systemSettings, sizeof(SystemSettings));

    scaleManager.setScale(scaleSettings.scaleType);
    scaleManager.setRootNote(scaleSettings.rootNote);

    // Update timing variables from system settings (convert seconds to milliseconds)
    deepSleepIdleMs = systemSettings.deepSleepTimeout * 1000UL;
    idleConfirmMs = systemSettings.idleConfirmTimeout * 1000UL;

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
        ledSet(LedColor::PINK, PINK_PWM_MAX);
        delay(onTime);
        ledSet(LedColor::PINK, 0);
        delay(gapTime);

        ledSet(LedColor::BLUE, PWM_MAX);
        delay(onTime);
        ledSet(LedColor::BLUE, 0);
        delay(gapTime);

        ledSet(LedColor::OCTAVE_DOWN, 1);
        delay(onTime);
        ledSet(LedColor::OCTAVE_DOWN, 0);
        delay(gapTime);

        ledSet(LedColor::OCTAVE_UP, 1);
        delay(onTime);
        ledSet(LedColor::OCTAVE_UP, 0);
        delay(gapTime);

        // Backward wave (stop at Blue, don’t repeat Pink here)
        ledSet(LedColor::OCTAVE_DOWN, 1);
        delay(onTime);
        ledSet(LedColor::OCTAVE_DOWN, 0);
        delay(gapTime);

        ledSet(LedColor::BLUE, PWM_MAX);
        delay(onTime);
        ledSet(LedColor::BLUE, 0);

        delay(cyclePause);
    }

    // End of startup sequence — keep the wave/chase only.
    // Short pause so the last wave step is perceptible before normal operation.
    delay(100);
}

// sleep helpers implemented in controls/SleepControl.h


//---------------------------------------------------
//
//  Setup
//
//---------------------------------------------------
void setup() {
    SERIAL_BEGIN();
    delay(2000); // Wait for USB CDC to enumerate
    while (!Serial && millis() < 3000) { delay(10); } // Wait up to 3 seconds for serial connection
    SERIAL_PRINTLN("Serial monitor started.");
    SERIAL_PRINTLN("===========================================");
    SERIAL_PRINTLN("KB1 FIRMWARE v" FIRMWARE_VERSION " - WITH CHORD MODE");
    SERIAL_PRINTLN("Build Date: " __DATE__ " " __TIME__);
    SERIAL_PRINTLN("===========================================");

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

    // Create I2C mutex for thread-safe access to MCP23017 chips
    i2cMutex = xSemaphoreCreateMutex();
    if (i2cMutex == NULL) {
        SERIAL_PRINTLN("Error creating I2C mutex");
        while (true) {}
    }

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

    // Create LED command queue and start LED task on core 0
    ledQueue = xQueueCreate(16, sizeof(LedCommand));
    if (ledQueue == NULL) {
        SERIAL_PRINTLN("Warning: failed to create LED queue");
    }
    // Run ledTask on the same core as readInputs to avoid cross-core I2C access
    xTaskCreatePinnedToCore(ledTask, "ledTask", 4096, nullptr, 1, nullptr, 1);

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
        scaleSettings,
        chordSettings,
        systemSettings
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
    // Check BLE idle and enter modem sleep if needed
    // BLE radio turns off 10 seconds before entering light sleep (DEEP_SLEEP_IDLE_MS: 330 sec - 10 sec = 320 sec)
    if (bluetoothControllerPtr) {
        unsigned long idleThreshold = 320000; // 320 seconds (5 min 20 sec)
        
        // If keep-alive is active, check if we're within grace period
        if (bluetoothControllerPtr->isKeepAliveActive()) {
            unsigned long now = millis();
            unsigned long lastPing = bluetoothControllerPtr->getLastKeepAlivePing();
            unsigned long gracePeriod = bluetoothControllerPtr->getKeepAliveGracePeriod();
            
            // Handle millis() overflow correctly by using unsigned arithmetic
            // This works because unsigned subtraction wraps around correctly
            unsigned long timeSinceLastPing = now - lastPing;
            
            // If we're still within the grace period since last keep-alive ping, extend the idle threshold
            // Note: Each ping calls refreshKeepAlive() which updates _lastActivity, so extending the 
            // threshold to gracePeriod ensures the system won't sleep as long as pings keep coming
            if (timeSinceLastPing < gracePeriod) {
                idleThreshold = gracePeriod;
            } else {
                // Grace period expired, deactivate keep-alive
                bluetoothControllerPtr->setKeepAliveActive(false);
            }
        }
        
        bluetoothControllerPtr->checkIdleAndSleep(idleThreshold);
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

        // Pink/Blue LED: compute targets and use LEDController ramping or touch-driven pulse (with I2C mutex protection)
        bool lever1Right, lever2Right, lever1Left, lever2Left, leverPush1Pressed, leverPush2Pressed;
        if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
            lever1Right = mcp_U2.digitalRead(SWD1_RIGHT_PIN) == LOW;
            lever2Right = mcp_U2.digitalRead(SWD2_RIGHT_PIN) == LOW;
            lever1Left = mcp_U1.digitalRead(SWD1_LEFT_PIN) == LOW;
            lever2Left = mcp_U2.digitalRead(SWD2_LEFT_PIN) == LOW;
            leverPush1Pressed = mcp_U1.digitalRead(SWD1_CENTER_PIN) == LOW;
            leverPush2Pressed = mcp_U2.digitalRead(SWD2_CENTER_PIN) == LOW;
            xSemaphoreGive(i2cMutex);
        }
        
        bool pinkLedPressed = lever1Right || lever2Right;
        bool blueLeftPressed = lever1Left || lever2Left;
        leverPushIsPressed = leverPush1Pressed || leverPush2Pressed;

        static int _lastPinkTarget = -1;
        static int _lastBlueTarget = -1;

        // Touch-driven alternating pulse parameters
        static bool _touchPulseMode = false;
        static unsigned long _pulseStart = 0;
        const unsigned long blueOnMs = 150UL;
        const unsigned long gapMs1 = 5UL;
        const unsigned long pinkOnMs = 150UL;
        const unsigned long gapMs2 = 250UL; // increased for smoother pacing
        const unsigned long totalCycle = blueOnMs + gapMs1 + pinkOnMs + gapMs2;
        const unsigned long pulseRampMs = 60UL; // ramp time for smoother chase

        int computedPinkTarget = pinkLedPressed ? PINK_PWM_MAX : 0;
        int baseLeftTarget = blueLeftPressed ? PWM_MAX : 0;
        int basePushTarget = leverPushIsPressed ? (PWM_MAX / 2) : 0;
        int computedBlueTarget = max(baseLeftTarget, basePushTarget);

        if (touchActive) {
            if (!_touchPulseMode) {
                _touchPulseMode = true;
                _pulseStart = millis();
            }
            unsigned long elapsed = (millis() - _pulseStart) % totalCycle;
            if (elapsed < blueOnMs) {
                // Blue on, pink off
                computedBlueTarget = PWM_MAX;
                computedPinkTarget = 0;
            } else if (elapsed < (blueOnMs + gapMs1)) {
                // Gap after blue
                computedBlueTarget = 0;
                computedPinkTarget = 0;
            } else if (elapsed < (blueOnMs + gapMs1 + pinkOnMs)) {
                // Pink on, blue off
                computedBlueTarget = 0;
                computedPinkTarget = PINK_PWM_MAX;
            } else {
                // Gap after pink
                computedBlueTarget = 0;
                computedPinkTarget = 0;
            }
        } else {
            // Restore normal behavior when not in touch pulse mode
            if (_touchPulseMode) {
                _touchPulseMode = false;
            }
            // computedPinkTarget and computedBlueTarget already set from base inputs
        }

        // Apply pink LED change (use short ramps for pulse mode to create a smooth chase)
        if (computedPinkTarget != _lastPinkTarget) {
            unsigned long dur;
            if (_touchPulseMode) {
                dur = pulseRampMs;
            } else {
                dur = (_lastPinkTarget < 0 || computedPinkTarget > _lastPinkTarget) ? PINK_RAMP_UP_MS : PINK_RAMP_DOWN_MS;
            }
            ledSetRamp(LedColor::PINK, computedPinkTarget, dur);
            _lastPinkTarget = computedPinkTarget;
        }

        // Apply blue LED change (use short ramps for pulse mode to create a smooth chase)
        if (computedBlueTarget != _lastBlueTarget) {
            unsigned long dur;
            if (_touchPulseMode) {
                dur = pulseRampMs;
            } else {
                dur = (_lastBlueTarget < 0 || computedBlueTarget > _lastBlueTarget) ? BLUE_RAMP_UP_MS : BLUE_RAMP_DOWN_MS;
            }
            ledSetRamp(LedColor::BLUE, computedBlueTarget, dur);
            _lastBlueTarget = computedBlueTarget;
        }

        leverPushWasPressed = leverPushIsPressed;        

        // Determine activity: touch active, any keyboard key, any pressed switch, or BLE keep-alive active
        bool keyboardActive = keyboardControl.anyKeyActive();
        bool bleKeepAliveActive = bluetoothControllerPtr && bluetoothControllerPtr->isKeepAliveActive();
        activityDetected = touchActive || keyboardActive || pinkLedPressed || blueLeftPressed || leverPushIsPressed || bleKeepAliveActive;

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
                } else if (millis() - idleStartMillis >= idleConfirmMs) {
                    // We've had a confirmed quiet window — start deep-sleep countdown
                    idleConfirmed = true;
                    idleConfirmTime = millis();
                }
            } else {
                // idleConfirmed true => count towards deep sleep timeout
                if (!deepSleepTriggered && (millis() - idleConfirmTime >= deepSleepIdleMs)) {
                    deepSleepTriggered = true;
                    enterLightSleep(touch, keyboardControl, ledController, bluetoothControllerPtr, touchSettings, lastActivityMillis, PINK_LED_PWM_PIN, BLUE_LED_PWM_PIN, PINK_PWM_MAX, PWM_MAX, PINK_RAMP_UP_MS, PINK_RAMP_DOWN_MS, BLUE_RAMP_UP_MS, BLUE_RAMP_DOWN_MS);
                }
            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// enterDeepSleep() implemented in controls/SleepControl.h
