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
#include <esp_system.h>
#include <driver/gpio.h>
#include <soc/usb_serial_jtag_reg.h>
#include <soc/usb_serial_jtag_struct.h>
#include <bt/BluetoothController.h>
#include <objects/Constants.h>
#include <objects/Globals.h>
#include <objects/Settings.h>
#include <led/LEDController.h>
#include <music/ScaleManager.h>
#include <music/StrumPatterns.h>
#include <controls/KeyboardControl.h>
#include <controls/LeverControls.h>
#include <controls/TouchControl.h>
#include <controls/LeverPushControls.h>
#include "controls/OctaveControl.h"
#include <controls/BLEGestureControl.h>
#include <controls/SleepControl.h>

Adafruit_MCP23X17 mcp_U1;
Adafruit_MCP23X17 mcp_U2;

// Global flag for serial connection detection
#ifdef SERIAL_PRINT_ENABLED
bool serialConnected = false;
bool cpuStatsEnabled = false;  // Toggle CPU profiling output
#endif

LEDController ledController;

// Custom strum pattern storage (pattern 7 - modifiable via BLE)
CustomPattern customPattern = {
    .intervals = {0, 2, 4, 5, 7, 9, 11, 12, 0, 0, 0, 0, 0, 0, 0, 0},
    .length = 8
};

// Lever cooldown after BLE toggle (prevents MIDI output during lever release)
unsigned long leverCooldownUntil = 0;

// Callbacks for syncing lever values when settings change via BLE
void (*syncLever1Callback)() = nullptr;
void (*syncLeverPush1Callback)() = nullptr;
void (*syncLever2Callback)() = nullptr;
void (*syncLeverPush2Callback)() = nullptr;

// Callback for notifying BLE when chord settings change from firmware
void (*notifyChordSettingsCallback)() = nullptr;

// Callback for resetting pattern controls when shape mode is disabled
void (*resetPatternControlsCallback)() = nullptr;

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
    ledController
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
    .velocitySpread = 10,
    .strumSpeed = 80,   // 80ms default (moderate-fast, range: ±5ms=fast at edges, ±360ms=slow at center)
    .strumPattern = 0,  // Default to chord type
    .strumSwing = 0,    // No swing by default
    .voicing = 1        // 1x octave range by default
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
    keyboardControl,
    chordSettings
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
    keyboardControl,
    chordSettings
);

//----------------------------------
// Lever 2 Setup
//----------------------------------
LeverSettings lever2Settings = {
    .ccNumber = 128,
    .minCCValue = 13,
    .maxCCValue = 127,
    .stepSize = 6,
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
    keyboardControl,
    chordSettings
);

//----------------------------------
// LeverPush 2 Setup
//----------------------------------
LeverPushSettings leverPush2Settings = {
    .ccNumber = 128,
    .minCCValue = 85,
    .maxCCValue = 85,
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
        keyboardControl,
        chordSettings
);

//----------------------------------
// Touch Sensor Setup
//----------------------------------
TouchSettings touchSettings = {
    .ccNumber = 1,
    .minCCValue = 64,
    .maxCCValue = 127,
    .functionMode = TouchFunctionMode::CONTINUOUS,
    .threshold = 36800,  // 20% (range: 30000-64000 maps to 0-100%)
    .offsetTime = 0,  // FWD mode (forward cycling for Pattern Selector)
};
TouchControl<decltype(MIDI)> touch(
    T1,
    touchSettings,
    30000,
    64000,
    MIDI,
    chordSettings,
    ledController
);

SystemSettings systemSettings = {
    .lightSleepTimeout = 300,   // 5 minutes
    .deepSleepTimeout = 120,    // 2 minutes
    .bleTimeout = 600,          // 10 minutes
    .idleConfirmTimeout = 2,    // 2 seconds
};

Preferences preferences;
BluetoothController* bluetoothControllerPtr = nullptr;

// BLE Gesture Control - cross-lever gesture to toggle BLE
BLEGestureControl* bleGestureControl = nullptr;

// Battery state tracking
BatteryState batteryState = {
    .accumulatedDischargeMs = 0,
    .lastUpdateMs = 0,
    .accumulatedChargeMs = 0,
    .chargeSessionStartMs = 0,
    .lastSaveMs = 0,
    .calibrationTimestamp = 0,  // 0 = never calibrated
    .lastUsbState = false,
    .isFullyCharged = false,
    .isChargingMode = false,  // true only if valid sequence: boot on battery -> plug USB
    .estimatedPercentage = 254,  // 254 = uncalibrated (needs full charge)
    .activeTimeMs = 0,
    .lightSleepTimeMs = 0,
    .deepSleepTimeMs = 0
};

// Track if USB was connected at boot (bypass mode - no charging)
bool usbConnectedAtBoot = false;
bool firstBatteryUpdate = true;  // Flag to detect USB at boot on first update

//----------------------------------
// Sleep / Deep Sleep Management
//----------------------------------
unsigned long lastActivityMillis = 0;
unsigned long lightSleepIdleMs = 300000; // Default: 5 minutes
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

// USB Power Detection (uses USB peripheral, not Serial CDC)
// Returns true if USB VBUS power is connected (works with computer or wall charger)
bool isUsbPowered() {
    // ESP32-S3 XIAO doesn't expose VBUS sensing, so we use multiple detection methods
    
    static uint32_t lastFrameCount = 0;
    static unsigned long lastCheckMs = 0;
    static bool lastState = false;
    
    unsigned long now = millis();
    
    // Check every 100ms to avoid excessive polling
    if (now - lastCheckMs < 100) {
        return lastState;
    }
    
    uint32_t currentFrame = USB_SERIAL_JTAG.fram_num.sof_frame_index;
    
    // Method 1: USB frame counter (works when connected to computer)
    bool frameActive = (currentFrame != lastFrameCount);
    
    // Method 2: Serial CDC available (also indicates USB connection to computer)
    bool serialActive = (bool)Serial;
    
    // Method 3: Check if we recently detected USB (sticky detection for wall chargers)
    // Once USB is detected, assume it stays connected until explicitly unplugged
    // This handles wall chargers that don't enumerate as USB devices
    static bool stickyUsb = false;
    if (frameActive || serialActive) {
        stickyUsb = true;  // USB detected, remember it
    }
    
    // For wall charger detection: if we've never seen USB frames/serial, but device is running,
    // we might be on wall charger. But we can't distinguish from battery power this way.
    // Solution: Use user-initiated detection at boot (check immediately after power-on)
    
    bool connected = frameActive || serialActive || stickyUsb;
    
    lastFrameCount = currentFrame;
    lastCheckMs = now;
    lastState = connected;
    
    return connected;
}

//---------------------------------------------------
// Battery Monitoring Functions
//---------------------------------------------------

// Load battery state from NVS
void loadBatteryState() {
    batteryState.accumulatedDischargeMs = preferences.getUInt("batDischMs", 0);
    batteryState.isFullyCharged = preferences.getBool("batFull", false);
    batteryState.estimatedPercentage = preferences.getUChar("batPct", 254);  // Default: uncalibrated
    batteryState.calibrationTimestamp = preferences.getUInt("batCalTime", 0);  // 0 = never
    batteryState.accumulatedChargeMs = preferences.getULong("batAccChgMs", 0);  // Resume accumulated charge time
    batteryState.lastUsbState = preferences.getBool("lastUsbState", false);  // Track USB state across reboots
    usbConnectedAtBoot = preferences.getBool("usbAtBoot", false);  // Preserve bypass mode flag across sleep/wake
    
    // Load power state tracking (added in v1.5.0-patch to fix calibration persistence)
    batteryState.activeTimeMs = preferences.getUInt("batActiveMs", 0);
    batteryState.lightSleepTimeMs = preferences.getUInt("batLightMs", 0);
    batteryState.deepSleepTimeMs = preferences.getUInt("batDeepMs", 0);
    
    if (batteryState.estimatedPercentage == 254) {
        SERIAL_PRINTLN("Battery uncalibrated - charge fully to establish baseline");
    } else {
        SERIAL_PRINT("Battery state loaded: ");
        SERIAL_PRINT(batteryState.estimatedPercentage);
        SERIAL_PRINTLN("%");
    }
    
    if (batteryState.accumulatedChargeMs > 0) {
        SERIAL_PRINT("Accumulated charge time: ");
        SERIAL_PRINT(batteryState.accumulatedChargeMs / 1000);
        SERIAL_PRINT("s / ");
        SERIAL_PRINT(BATTERY_FULL_CHARGE_MS / 1000);
        SERIAL_PRINTLN("s");
    }
}

// Save battery state to NVS
void saveBatteryState() {
    preferences.putUInt("batDischMs", batteryState.accumulatedDischargeMs);
    preferences.putBool("batFull", batteryState.isFullyCharged);
    preferences.putUChar("batPct", batteryState.estimatedPercentage);
    preferences.putUInt("batCalTime", batteryState.calibrationTimestamp);
    preferences.putULong("batAccChgMs", batteryState.accumulatedChargeMs);  // Persist accumulated charge time
    preferences.putBool("lastUsbState", batteryState.lastUsbState);  // Track USB state across reboots
    preferences.putBool("usbAtBoot", usbConnectedAtBoot);  // Preserve bypass mode flag across sleep/wake
    
    // Save power state tracking (added in v1.5.0-patch to fix calibration persistence)
    preferences.putUInt("batActiveMs", batteryState.activeTimeMs);
    preferences.putUInt("batLightMs", batteryState.lightSleepTimeMs);
    preferences.putUInt("batDeepMs", batteryState.deepSleepTimeMs);
    
    batteryState.lastSaveMs = millis();
    
    SERIAL_PRINTLN("Saved");
}

// Debug: Save charging debug info to NVS
void saveChargingDebug(const char* event, bool usbDetected, bool chargingMode, bool sleepPrevented) {
    static uint8_t debugIndex = 0;
    
    // Store last 5 debug events in circular buffer
    char key[16];
    snprintf(key, sizeof(key), "dbg%d_evt", debugIndex);
    preferences.putString(key, event);
    
    snprintf(key, sizeof(key), "dbg%d_time", debugIndex);
    preferences.putUInt(key, millis() / 1000);  // Seconds since boot
    
    snprintf(key, sizeof(key), "dbg%d_usb", debugIndex);
    preferences.putBool(key, usbDetected);
    
    snprintf(key, sizeof(key), "dbg%d_chrg", debugIndex);
    preferences.putBool(key, chargingMode);
    
    snprintf(key, sizeof(key), "dbg%d_slp", debugIndex);
    preferences.putBool(key, sleepPrevented);
    
    debugIndex = (debugIndex + 1) % 5;  // Circular buffer of 5 entries
    
    // Minimal event logging
    SERIAL_PRINT(event);
    SERIAL_PRINT(" | USB:");
    SERIAL_PRINT(usbDetected);
    SERIAL_PRINT(" | Chg:");
    SERIAL_PRINT(chargingMode);
    SERIAL_PRINT(" | Slp:");
    SERIAL_PRINTLN(sleepPrevented);
}

// Debug: Print charging debug log from NVS
void printChargingDebug() {
    SERIAL_PRINTLN("=== CHARGING DEBUG LOG ===");
    for (int i = 0; i < 5; i++) {
        char key[16];
        
        snprintf(key, sizeof(key), "dbg%d_evt", i);
        String event = preferences.getString(key, "");
        if (event.length() == 0) continue;
        
        snprintf(key, sizeof(key), "dbg%d_time", i);
        uint32_t time = preferences.getUInt(key, 0);
        
        snprintf(key, sizeof(key), "dbg%d_usb", i);
        bool usb = preferences.getBool(key, false);
        
        snprintf(key, sizeof(key), "dbg%d_chrg", i);
        bool chrg = preferences.getBool(key, false);
        
        snprintf(key, sizeof(key), "dbg%d_slp", i);
        bool slp = preferences.getBool(key, false);
        
        SERIAL_PRINT("[");
        SERIAL_PRINT(i);
        SERIAL_PRINT("] ");
        SERIAL_PRINT(time);
        SERIAL_PRINT("s: ");
        SERIAL_PRINT(event);
        SERIAL_PRINT(" | USB:");
        SERIAL_PRINT(usb);
        SERIAL_PRINT(" | Chrg:");
        SERIAL_PRINT(chrg);
        SERIAL_PRINT(" | SlpPrev:");
        SERIAL_PRINTLN(slp);
    }
    SERIAL_PRINTLN("==========================");
}

// Calculate battery percentage based on accumulated discharge
// Uses actual power consumption measurements:
// - Active mode: 95mA
// - Light sleep: 2mA  
// - Deep sleep: 0.014mA
uint8_t calculateBatteryPercentage() {
    // Calculate total mAh consumed
    float activeHours = batteryState.activeTimeMs / 3600000.0f;
    float lightSleepHours = batteryState.lightSleepTimeMs / 3600000.0f;
    float deepSleepHours = batteryState.deepSleepTimeMs / 3600000.0f;
    
    float consumedMah = (activeHours * BATTERY_ACTIVE_DRAIN_MA) +
                        (lightSleepHours * BATTERY_LIGHT_SLEEP_DRAIN_MA) +
                        (deepSleepHours * BATTERY_DEEP_SLEEP_DRAIN_MA);
    
    float remainingMah = BATTERY_CAPACITY_MAH - consumedMah;
    
    if (remainingMah <= 0) return 0;
    if (remainingMah >= BATTERY_CAPACITY_MAH) return 100;
    
    return (uint8_t)((remainingMah / BATTERY_CAPACITY_MAH) * 100.0f);
}

// Update battery monitoring (call from main loop)
void updateBatteryMonitoring() {
    unsigned long now = millis();
    bool usbNow = isUsbPowered();  // Detect USB power (not serial terminal)
    bool wasUsbConnected = batteryState.lastUsbState;
    
    // Detect USB plug/unplug events
    bool usbJustPlugged = usbNow && !wasUsbConnected;
    bool usbJustUnplugged = !usbNow && wasUsbConnected;
    
    if (usbJustPlugged) {
        // USB just connected - check if valid charging sequence
        SERIAL_PRINT("USB plug detected - usbAtBoot=");
        SERIAL_PRINTLN(usbConnectedAtBoot);
        if (!usbConnectedAtBoot) {
            // Valid sequence: Device was on battery, now USB plugged in
            batteryState.isChargingMode = true;
            
            // Start new charging session (accumulated time already loaded from NVS)
            batteryState.chargeSessionStartMs = now;
            
            unsigned long totalChargeDuration = batteryState.accumulatedChargeMs;
            unsigned long remainingMs = BATTERY_FULL_CHARGE_MS > totalChargeDuration ? 
                                       BATTERY_FULL_CHARGE_MS - totalChargeDuration : 0;
            
            if (totalChargeDuration == 0) {
                SERIAL_PRINTLN("USB connected - charging started (valid sequence)");
            } else {
                SERIAL_PRINTLN("USB connected - charging resumed");
            }
            
            SERIAL_PRINT("Chg: ");
            SERIAL_PRINT(totalChargeDuration / 1000);
            SERIAL_PRINT("/");
            SERIAL_PRINT(BATTERY_FULL_CHARGE_MS / 1000);
            SERIAL_PRINTLN("s");
            
            saveChargingDebug("USB_PLUG", true, true, true);
        } else {
            // Invalid sequence: USB was connected at boot (bypass mode)
            batteryState.isChargingMode = false;
            // NOTE: Preserve accumulatedChargeMs across bypass mode - allows resume after proper sequence
            SERIAL_PRINTLN("USB detected (bypass mode - NOT charging)");
            SERIAL_PRINTLN("Unplug USB, power cycle device, then plug USB to charge");
            saveChargingDebug("USB_BYPASS", true, false, false);
        }
    }
    
    if (usbJustUnplugged) {
        // USB just disconnected - save accumulated charge time
        if (batteryState.isChargingMode && batteryState.chargeSessionStartMs > 0) {
            // Add current session time to accumulated total
            unsigned long sessionDuration = now - batteryState.chargeSessionStartMs;
            batteryState.accumulatedChargeMs += sessionDuration;
            unsigned long totalChargeDuration = batteryState.accumulatedChargeMs;
            
            // Calculate mAh gained from total charge time (charge current = 100mA)
            float chargeHours = totalChargeDuration / 3600000.0f;
            float mAhGained = chargeHours * BATTERY_CHARGE_CURRENT_MA;
            
            if (totalChargeDuration >= BATTERY_FULL_CHARGE_MS) {
                // Full charge cycle complete - calibrate to 100%
                batteryState.isFullyCharged = true;
                batteryState.accumulatedDischargeMs = 0;
                batteryState.activeTimeMs = 0;
                batteryState.lightSleepTimeMs = 0;
                batteryState.deepSleepTimeMs = 0;
                batteryState.estimatedPercentage = 100;
                batteryState.calibrationTimestamp = now / 1000;
                batteryState.accumulatedChargeMs = 0;  // Clear accumulated time (calibration complete)
                
                SERIAL_PRINT("Full charge complete! ");
                SERIAL_PRINT(mAhGained);
                SERIAL_PRINTLN("mAh added (calibrated to 100%)");
            } else if (batteryState.isFullyCharged) {
                // Partial charge on calibrated battery - add capacity incrementally
                // Convert mAh gained to equivalent discharge time (use active mode rate for conservative estimate)
                float equivalentActiveHours = mAhGained / BATTERY_ACTIVE_DRAIN_MA;
                unsigned long equivalentActiveMs = equivalentActiveHours * 3600000.0f;
                
                // Subtract from accumulated discharge (capped at 0)
                if (batteryState.accumulatedDischargeMs > equivalentActiveMs) {
                    batteryState.accumulatedDischargeMs -= equivalentActiveMs;
                } else {
                    batteryState.accumulatedDischargeMs = 0;
                }
                
                // Also reduce activeTimeMs proportionally (since we track both)
                if (batteryState.activeTimeMs > equivalentActiveMs) {
                    batteryState.activeTimeMs -= equivalentActiveMs;
                } else {
                    batteryState.activeTimeMs = 0;
                }
                
                // Recalculate percentage (cap at 100%)
                batteryState.estimatedPercentage = calculateBatteryPercentage();
                if (batteryState.estimatedPercentage > 100) {
                    batteryState.estimatedPercentage = 100;
                }
                
                // Clear accumulated charge time after applying to calibrated battery
                batteryState.accumulatedChargeMs = 0;
                
                SERIAL_PRINT("Partial charge: ");
                SERIAL_PRINT(totalChargeDuration / 1000);
                SERIAL_PRINT("s, +");
                SERIAL_PRINT(mAhGained);
                SERIAL_PRINT("mAh → ");
                SERIAL_PRINT(batteryState.estimatedPercentage);
                SERIAL_PRINTLN("%");
            } else {
                // Partial charge, never calibrated - keep accumulated time for next session
                SERIAL_PRINT("Partial charge: ");
                SERIAL_PRINT(totalChargeDuration / 1000);
                SERIAL_PRINT("s / ");
                SERIAL_PRINT(BATTERY_FULL_CHARGE_MS / 1000);
                SERIAL_PRINT("s - ");
                unsigned long remaining = BATTERY_FULL_CHARGE_MS - totalChargeDuration;
                SERIAL_PRINT(remaining / 1000);
                SERIAL_PRINTLN("s remaining for calibration");
                batteryState.estimatedPercentage = 254;
                // Don't clear accumulatedChargeMs - let it persist for next charge session
            }
            
            batteryState.chargeSessionStartMs = 0;  // Clear session timer (charge session ended)
            saveBatteryState();  // Persist updated accumulated time
        }
        
        batteryState.isChargingMode = false;
    }
    
    // Update battery state
    if (usbNow) {
        if (batteryState.isChargingMode && batteryState.chargeSessionStartMs > 0) {
            // Valid charging mode - calculate total charge time
            unsigned long sessionDuration = now - batteryState.chargeSessionStartMs;
            unsigned long totalChargeDuration = batteryState.accumulatedChargeMs + sessionDuration;
            
            if (totalChargeDuration >= BATTERY_FULL_CHARGE_MS) {
                // Charging complete - always exit charging mode (stops LEDs, allows sleep)
                if (!batteryState.isFullyCharged) {
                    // First time reaching full charge - update battery state
                    batteryState.isFullyCharged = true;
                    batteryState.accumulatedDischargeMs = 0;
                    batteryState.activeTimeMs = 0;
                    batteryState.lightSleepTimeMs = 0;
                    batteryState.deepSleepTimeMs = 0;
                    batteryState.estimatedPercentage = 100;
                    batteryState.calibrationTimestamp = now / 1000;  // Store as seconds since boot (approximation)
                    batteryState.accumulatedChargeMs = 0;  // Clear accumulated time (calibration complete)
                    
                    SERIAL_PRINTLN("Battery fully charged! Calibration complete.");
                    saveBatteryState();
                }
                
                // Always exit charging mode when time completes (even if already fully charged)
                batteryState.isChargingMode = false;
                batteryState.chargeSessionStartMs = 0;
                SERIAL_PRINTLN("Charging mode ended - LEDs off, sleep enabled");
            } else {
                // Show charging progress every 30s and save state
                static unsigned long lastProgressPrint = 0;
                if (now - lastProgressPrint >= 30000) {
                    SERIAL_PRINT("Chg: ");
                    SERIAL_PRINT(totalChargeDuration / 1000);
                    SERIAL_PRINT("/");
                    SERIAL_PRINT(BATTERY_FULL_CHARGE_MS / 1000);
                    SERIAL_PRINTLN("s");
                    lastProgressPrint = now;
                    
                    // Save accumulated time periodically (resume from here if power lost)
                    batteryState.accumulatedChargeMs += sessionDuration;
                    batteryState.chargeSessionStartMs = now;  // Reset session timer
                    saveBatteryState();
                }
                
                // Calculate current charge progress for BLE reporting (if battery is calibrated)
                if (batteryState.isFullyCharged) {
                    // Calculate mAh gained so far
                    float chargeHours = totalChargeDuration / 3600000.0f;
                    float mAhGained = chargeHours * BATTERY_CHARGE_CURRENT_MA;
                    
                    // Convert to equivalent discharge time (use active mode rate)
                    float equivalentActiveHours = mAhGained / BATTERY_ACTIVE_DRAIN_MA;
                    unsigned long equivalentActiveMs = equivalentActiveHours * 3600000.0f;
                    
                    // Calculate what the percentage would be with this charge
                    // Start from the snapshot values at charge start
                    unsigned long tempActiveMs = batteryState.activeTimeMs;
                    if (tempActiveMs > equivalentActiveMs) {
                        tempActiveMs -= equivalentActiveMs;
                    } else {
                        tempActiveMs = 0;
                    }
                    
                    // Calculate percentage using temporary reduced discharge time
                    float activeHours = tempActiveMs / 3600000.0f;
                    float lightSleepHours = batteryState.lightSleepTimeMs / 3600000.0f;
                    float deepSleepHours = batteryState.deepSleepTimeMs / 3600000.0f;
                    
                    float consumedMah = (activeHours * BATTERY_ACTIVE_DRAIN_MA) +
                                       (lightSleepHours * BATTERY_LIGHT_SLEEP_DRAIN_MA) +
                                       (deepSleepHours * BATTERY_DEEP_SLEEP_DRAIN_MA);
                    
                    float remainingMah = BATTERY_CAPACITY_MAH - consumedMah;
                    uint8_t currentPercentage = (uint8_t)((remainingMah / BATTERY_CAPACITY_MAH) * 100.0f);
                    
                    if (currentPercentage > 100) currentPercentage = 100;
                    if (currentPercentage < 0) currentPercentage = 0;
                    
                    batteryState.estimatedPercentage = currentPercentage;
                } else {
                    // Uncalibrated - stay at 254 during charging
                    batteryState.estimatedPercentage = 254;
                }
            }
        } else {
            // Bypass mode - show last known percentage (not charging, just powered by USB)
            // Percentage stays as loaded from NVS (254 if uncalibrated, or last known %)
        }
    } else {
        // On battery power - accumulate discharge
        if (batteryState.lastUpdateMs > 0 && batteryState.estimatedPercentage != 254) {
            unsigned long elapsedMs = now - batteryState.lastUpdateMs;
            
            // For now, assume active mode (could be enhanced with sleep state tracking)
            batteryState.activeTimeMs += elapsedMs;
            batteryState.accumulatedDischargeMs += elapsedMs;
            
            // Recalculate percentage (skip if uncalibrated)
            batteryState.estimatedPercentage = calculateBatteryPercentage();
        }
        
        // Save to NVS every 5 minutes to reduce wear (preserves uncalibrated state)
        if (now - batteryState.lastSaveMs >= 300000) {  // 5 minutes
            saveBatteryState();
        }
    }
    
    batteryState.lastUsbState = usbNow;
    batteryState.lastUpdateMs = now;
}

void loadSettings() {
    preferences.getBytes("lever1", &lever1Settings, sizeof(LeverSettings));
    preferences.getBytes("leverpush1", &leverPush1Settings, sizeof(LeverPushSettings));
    preferences.getBytes("lever2", &lever2Settings, sizeof(LeverSettings));
    preferences.getBytes("leverpush2", &leverPush2Settings, sizeof(LeverPushSettings));
    preferences.getBytes("touch", &touchSettings, sizeof(TouchSettings));
    preferences.getBytes("scale", &scaleSettings, sizeof(ScaleSettings));
    preferences.getBytes("chord", &chordSettings, sizeof(ChordSettings));
    preferences.getBytes("customStrum", &customPattern, sizeof(CustomPattern));
    preferences.getBytes("system", &systemSettings, sizeof(SystemSettings));

    scaleManager.setScale(scaleSettings.scaleType);
    scaleManager.setRootNote(scaleSettings.rootNote);

    // Update timing variables from system settings (convert seconds to milliseconds)
    lightSleepIdleMs = systemSettings.lightSleepTimeout * 1000UL;
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

//---------------------------------------------------
// Charging LED breathing (very slow ambient)
// Simple alternating: Pink ↔ Blue (repeat)
// Non-blocking state machine with fading for continuous loop during charging
//---------------------------------------------------
// Charging pattern state
enum ChargePatternState {
    CHARGE_PINK,
    CHARGE_BLUE
};

ChargePatternState chargePatternState = CHARGE_PINK;
unsigned long lastChargePatternUpdate = 0;
const unsigned long CHARGE_LED_DURATION = 1200; // 1200ms per LED (slow breathing)
const unsigned long CHARGE_FADE_DURATION = 1200; // 1200ms fade - matches duration for continuous breathing
const int CHARGE_BRIGHTNESS_PINK = PINK_PWM_MAX / 3; // ~33% of pink max
const int CHARGE_BRIGHTNESS_NORMAL = PWM_MAX / 3;    // ~33% of normal max

void updateChargingLEDPattern() {
    unsigned long currentTime = millis();
    
    // Update pattern every CHARGE_LED_DURATION ms
    if (currentTime - lastChargePatternUpdate < CHARGE_LED_DURATION) {
        return; // Not time to advance yet
    }
    
    lastChargePatternUpdate = currentTime;
    
    // Simple alternating pattern Pink <-> Blue
    switch (chargePatternState) {
        case CHARGE_PINK:
            ledSetRamp(LedColor::PINK, CHARGE_BRIGHTNESS_PINK, CHARGE_FADE_DURATION);
            ledSetRamp(LedColor::BLUE, 0, CHARGE_FADE_DURATION);
            chargePatternState = CHARGE_BLUE;
            break;
        case CHARGE_BLUE:
            ledSetRamp(LedColor::BLUE, CHARGE_BRIGHTNESS_NORMAL, CHARGE_FADE_DURATION);
            ledSetRamp(LedColor::PINK, 0, CHARGE_FADE_DURATION);
            chargePatternState = CHARGE_PINK;
            break;
    }
}

// sleep helpers implemented in controls/SleepControl.h


//---------------------------------------------------
//
//  Setup
//
//---------------------------------------------------
void setup() {
    SERIAL_BEGIN();  // Instant check, no waiting

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW); // Keep LED on
    
    // Check USB status immediately at boot (for bypass mode detection)
    // Do this BEFORE any delays or other initialization
    delay(150);  // Allow USB SOF frames to start (50ms was too short for some hosts)
    bool usbAtBootEarlyDetection = isUsbPowered();  // Capture BEFORE loadBatteryState() overwrites it
    usbConnectedAtBoot = usbAtBootEarlyDetection;
    firstBatteryUpdate = false;  // No longer needed since we check at boot

    if (!preferences.begin("kb1-settings", false)) {
        SERIAL_PRINTLN("Error initializing Preferences. Rebooting...");
        ESP.restart();
    } else {
        SERIAL_PRINTLN("Preferences initialized successfully.");
    }
    
    // Print charging debug log from previous session
    #ifdef SERIAL_PRINT_ENABLED
    printChargingDebug();
    #endif
    
    // Track boots without serial terminal
    #ifdef SERIAL_PRINT_ENABLED
    if (serialConnected) {
        // Terminal connected at startup - show boot count and reset
        unsigned int bootsWithoutSerial = preferences.getUInt("bootsNoSerial", 0);
        
        SERIAL_PRINTLN("Serial monitor started.");
        SERIAL_PRINTLN("===========================================");
        SERIAL_PRINTLN("KB1 FIRMWARE v" FIRMWARE_VERSION " - WITH CHORD MODE");
        SERIAL_PRINTLN("Build Date: " __DATE__ " " __TIME__);
        if (bootsWithoutSerial > 0) {
            SERIAL_PRINT("Boots without serial since last connection: ");
            SERIAL_PRINTLN(bootsWithoutSerial);
        }
        SERIAL_PRINTLN("===========================================");
        
        // Reset counter
        preferences.putUInt("bootsNoSerial", 0);
    } else {
        // No terminal - increment boot counter
        unsigned int bootsWithoutSerial = preferences.getUInt("bootsNoSerial", 0);
        preferences.putUInt("bootsNoSerial", bootsWithoutSerial + 1);
    }
    #endif

    loadSettings();
    loadBatteryState();  // Restores lastUsbState and usbConnectedAtBoot from NVS
    
    // Only restore usbConnectedAtBoot if waking from deep sleep
    // On fresh power-on, detect USB fresh to enable proper charging detection
    esp_reset_reason_t resetReason = esp_reset_reason();
    if (resetReason != ESP_RST_DEEPSLEEP) {
        // Fresh boot - restore early hardware detection (loadBatteryState overwrote it with stale NVS)
        usbConnectedAtBoot = usbAtBootEarlyDetection;
        batteryState.lastUsbState = usbAtBootEarlyDetection;  // Prevents fake "just plugged" event
        SERIAL_PRINTLN("Fresh boot - USB state detected at boot");
    } else {
        SERIAL_PRINTLN("Wake from sleep - USB state restored from NVS");
    }

    // Set I2C speed BEFORE initializing I2C devices
    Wire.setClock(400000);  // 400kHz fast mode
    delay(50);  // Brief stabilization delay
    SERIAL_PRINTLN("I2C bus speed: 400kHz");

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

    // Create I/O input reading task on Core 1 (Protocol CPU)
    // Touch sensor requires Core 1 access (hardware peripheral affinity)
    // Priority 2 (higher than LED task) for minimal input latency
    xTaskCreatePinnedToCore(readInputs, "readInputs", 4096, nullptr, 2, nullptr, 1);

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

    // Create LED command queue and start LED task on core 1
    ledQueue = xQueueCreate(16, sizeof(LedCommand));
    if (ledQueue == NULL) {
        SERIAL_PRINTLN("Warning: failed to create LED queue");
    }
    // Create LED update task on Core 1 (same as inputs)
    // CRITICAL: I2C devices (MCP23017 LEDs) require same core to avoid mutex race conditions
    // Moving to Core 0 causes "Unfinished Repeated Start transaction!" assertion failures
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

    // Set up callbacks for syncing lever values when settings change via BLE
    syncLever1Callback = []() { lever1.syncValue(); };
    syncLeverPush1Callback = []() { leverPush1.syncValue(); };
    syncLever2Callback = []() { lever2.syncValue(); };
    syncLeverPush2Callback = []() { leverPush2.syncValue(); };
    
    // Set up callback for notifying BLE when chord settings change from firmware
    notifyChordSettingsCallback = []() { 
        if (bluetoothControllerPtr) {
            bluetoothControllerPtr->notifyChordSettings(); 
        }
    };

    // Set up callback to stop arpeggiator when shape mode is disabled
    resetPatternControlsCallback = []() {
        // If shape mode is disabled (strumPattern = 0), stop the arpeggiator
        if (chordSettings.strumPattern == 0) {
            keyboardControl.stopArpeggiator();
            SERIAL_PRINTLN("Arpeggiator stopped - shape mode disabled");
        }
    };

    // Initialize BLE gesture control (cross-lever activation)
    bleGestureControl = new BLEGestureControl(ledController, bluetoothControllerPtr);

    // Initialize activity timer
    lastActivityMillis = millis();
    
    // Initialize battery monitoring and check for boot state
    // Check USB multiple times with delays to allow CDC initialization and USB host enumeration
    // Checks BOTH serial CDC (terminal open) AND USB power (frame counter, catches no-terminal case)
    batteryState.lastUpdateMs = millis();
    for (int i = 0; i < 20; i++) {  // 20 checks × 50ms = 1 second total
        bool serialNow = (bool)Serial;
        bool usbPoweredNow = isUsbPowered();

        if (serialNow || usbPoweredNow) {
            usbConnectedAtBoot = true;
            batteryState.lastUsbState = true;
            firstBatteryUpdate = false;  // Don't check again in updateBatteryMonitoring

            if (serialNow) {
                SERIAL_PRINTLN("USB detected at boot (serial) - bypass mode (NOT charging)");
            } else {
                SERIAL_PRINTLN("USB detected at boot (no serial) - bypass mode (NOT charging)");
            }
            SERIAL_PRINTLN("For charging: Power on device first, THEN plug USB");
            // DO NOT reset estimatedPercentage - preserve calibration in bypass mode

            // Save to NVS immediately so sleep/wake preserves bypass mode
            saveBatteryState();
            break;
        }
        delay(50);  // Wait 50ms between checks
    }
    
    // Call updateBatteryMonitoring to establish baseline if USB wasn't detected in loop
    if (!usbConnectedAtBoot) {
        updateBatteryMonitoring();
    }
}

//---------------------------------------------------
//
//  Loop
//
//---------------------------------------------------
void loop() {
    // Update battery monitoring (tracks USB connection and discharge)
    static unsigned long lastBatteryUpdate = 0;
    
    // Fast polling (1s) for first 60 seconds for responsive USB detection, then slow down to 30s
    unsigned long pollInterval = (millis() < 60000) ? 1000 : 30000;
    
    if (millis() - lastBatteryUpdate > pollInterval) {
        updateBatteryMonitoring();
        
        // Update BLE characteristic if enabled
        if (bluetoothControllerPtr) {
            bluetoothControllerPtr->updateBatteryStatus();
        }
        
        lastBatteryUpdate = millis();
    }
    
    // Update charging LED pattern when actively charging
    // Check both isChargingMode AND current USB state for immediate response
    static bool wasCharging = false;
    static bool chargingLEDsEnabled = false; // Guard flag
    static uint32_t lastUsbFrameCheck = 0;
    static uint8_t usbDisconnectCount = 0;  // Debounce counter for USB disconnect
    const uint8_t USB_DISCONNECT_THRESHOLD = 3;  // Require 3 consecutive misses before stopping
    
    // Real-time USB check using frame counter (same method as battery monitoring)
    uint32_t currentFrame = USB_SERIAL_JTAG.fram_num.sof_frame_index;
    bool frameActive = (currentFrame != lastUsbFrameCheck);
    bool serialActive = (bool)Serial;
    bool usbCurrentlyConnected = frameActive || serialActive;
    lastUsbFrameCheck = currentFrame;
    
    // Debounce USB disconnect detection to prevent false negatives during blocking operations
    if (!usbCurrentlyConnected) {
        usbDisconnectCount++;
    } else {
        usbDisconnectCount = 0;  // Reset counter if USB detected
    }
    
    bool usbActuallyDisconnected = (usbDisconnectCount >= USB_DISCONNECT_THRESHOLD);
    
    if (batteryState.isChargingMode && !usbActuallyDisconnected) {
        if (!chargingLEDsEnabled) {
            // Just started charging pattern
            chargingLEDsEnabled = true;
            usbDisconnectCount = 0;  // Reset disconnect counter when starting
            SERIAL_PRINT("Charging LEDs started - usbAtBoot=");
            SERIAL_PRINT(usbConnectedAtBoot);
            SERIAL_PRINT(" isChargingMode=");
            SERIAL_PRINT(batteryState.isChargingMode);
            SERIAL_PRINT(" usbConnected=");
            SERIAL_PRINTLN(!usbActuallyDisconnected);
        }
        updateChargingLEDPattern();
        wasCharging = true;
    } else {
        // Stop immediately when USB disconnects OR charging mode exits
        if (wasCharging || chargingLEDsEnabled) {
            // Disable pattern FIRST to prevent any new ramp commands
            chargingLEDsEnabled = false;
            usbDisconnectCount = 0;  // Reset counter on intentional stop
            chargePatternState = CHARGE_PINK; // Reset state machine
            lastChargePatternUpdate = 0; // Reset timing
            
            // Force clear all LEDs immediately (override any pending ramps)
            ledSet(LedColor::PINK, 0);
            ledSet(LedColor::BLUE, 0);
            ledSet(LedColor::OCTAVE_DOWN, 0);
            ledSet(LedColor::OCTAVE_UP, 0);
            delay(10); // Brief delay to let queued commands process
            // Send again to ensure they're cleared
            ledSet(LedColor::PINK, 0);
            ledSet(LedColor::BLUE, 0);
            ledSet(LedColor::OCTAVE_DOWN, 0);
            ledSet(LedColor::OCTAVE_UP, 0);
            delay(10); // Another delay
            // Third clear for stubborn ramps
            ledSet(LedColor::PINK, 0);
            ledSet(LedColor::BLUE, 0);
            
            SERIAL_PRINT("Charging LEDs stopped - isChargingMode=");
            SERIAL_PRINT(batteryState.isChargingMode);
            SERIAL_PRINT(" usbConnected=");
            SERIAL_PRINTLN(!usbActuallyDisconnected);
            wasCharging = false;
            SERIAL_PRINTLN("Charging LEDs stopped");
        }
    }
    
    // Periodically check if USB CDC terminal is connected (every 5 seconds)
    #ifdef SERIAL_PRINT_ENABLED
    static unsigned long lastSerialCheck = 0;
    static bool wasConnected = false;
    
    if (millis() - lastSerialCheck > 5000) {
        bool nowConnected = (bool)Serial;
        
        // Report when terminal first connects (for verification)
        if (nowConnected && !wasConnected) {
            unsigned long uptimeMs = millis();
            unsigned int bootsWithoutSerial = preferences.getUInt("bootsNoSerial", 0);
            
            Serial.println("=== Serial Connected ===");
            Serial.print("Firmware: v"); Serial.println(FIRMWARE_VERSION);
            Serial.print("Uptime: "); Serial.print(uptimeMs / 1000); Serial.println("s");
            if (bootsWithoutSerial > 0) {
                Serial.print("Boots without serial: "); Serial.println(bootsWithoutSerial);
            }
            Serial.println("Serial auto-detect: Active");
            
            // Battery status
            Serial.print("Battery: ");
            if (batteryState.estimatedPercentage == 254) {
                Serial.println("Uncalibrated (needs full 5hr charge)");
                if (usbConnectedAtBoot) {
                    Serial.println("⚠️  USB at boot = bypass mode (NOT charging)");
                    Serial.println("⚠️  For charging: Power on FIRST, then plug USB");
                }
            } else if (batteryState.estimatedPercentage == 255) {
                if (batteryState.isChargingMode) {
                    Serial.println("USB powered (charging)");
                } else {
                    Serial.println("USB powered (bypass - NOT charging)");
                    Serial.println("⚠️  USB at boot = bypass mode");
                    Serial.println("⚠️  For charging: Unplug USB, power cycle, then plug USB");
                }
            } else {
                Serial.print(batteryState.estimatedPercentage);
                Serial.print("%, USB: ");
                Serial.println(batteryState.lastUsbState ? "Connected" : "Disconnected");
            }
            
            Serial.println("========================");
            
            // Reset counter when terminal connects during runtime
            preferences.putUInt("bootsNoSerial", 0);
        }
        
        serialConnected = nowConnected;
        wasConnected = nowConnected;
        lastSerialCheck = millis();
    }
    #endif
    
    // Periodically print CPU usage stats (every 10 seconds)
    // NOTE: Disabled due to linker issues with vTaskGetRunTimeStats on ESP32-S3
    // #ifdef SERIAL_PRINT_ENABLED
    // if (cpuStatsEnabled && serialConnected) {
    //     static unsigned long lastStatsTime = 0;
    //     if (millis() - lastStatsTime > 10000) {
    //         char statsBuffer[1024];
    //         vTaskGetRunTimeStats(statsBuffer);
    //         
    //         Serial.println("\n========== CPU STATS ==========");
    //         Serial.println("Task            Abs Time    %Time");
    //         Serial.println("-------------------------------");
    //         Serial.println(statsBuffer);
    //         Serial.println("===============================\n");
    //         
    //         lastStatsTime = millis();
    //     }
    // }
    // 
    // // Check for serial commands (toggle CPU stats on/off)
    // if (serialConnected && Serial.available()) {
    //     String cmd = Serial.readStringUntil('\n');
    //     cmd.trim();
    //     if (cmd == "cpu") {
    //         cpuStatsEnabled = !cpuStatsEnabled;
    //         Serial.print("CPU stats ");
    //         Serial.println(cpuStatsEnabled ? "ENABLED (10s interval)" : "DISABLED");
    //     } else if (cmd == "help") {
    //         Serial.println("Commands:");
    //         Serial.println("  cpu  - Toggle CPU profiling stats");
    //         Serial.println("  help - Show this message");
    //     }
    // }
    // #endif
    
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
        
        // Read lever states FIRST (for BLE gesture detection before lever MIDI processing)
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
        
        // BLE gesture detection BEFORE lever updates to suppress MIDI during gesture
        bool bleGestureActive = false;
        if (bleGestureControl) {
            bool keyboardActiveForGesture = keyboardControl.anyKeyActive();
            bleGestureActive = bleGestureControl->update(lever1Right, lever2Left, keyboardActiveForGesture);
        }
        
        // Only update levers if gesture not active (cooldown already checked inside lever update)
        if (!bleGestureActive) {
            lever1.update();
            lever2.update();
        }
        
        leverPush1.update();
        leverPush2.update();
        octaveControl.update();
        keyboardControl.updateKeyboardState();

        // Query touch sensor active state (affects LED behavior)
        bool touchActive = touch.isActive();

        // Activity detection: any active input counts as activity
        bool activityDetected = false;

        // Pink/Blue LED: compute targets and use LEDController ramping
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

        // Disable LED cycling for Toggle mode (including Pattern Selector) to avoid blocking/distracting animation
        bool isToggleMode = (touchSettings.functionMode == TouchFunctionMode::TOGGLE);
        
        if (touchActive && !isToggleMode) {
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
        
        // Prevent sleep during charging
        bool activelyCharging = batteryState.isChargingMode && (batteryState.chargeSessionStartMs > 0);
        
        activityDetected = touchActive || keyboardActive || pinkLedPressed || blueLeftPressed || leverPushIsPressed || bleKeepAliveActive || activelyCharging;

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
                if (!deepSleepTriggered && (millis() - idleConfirmTime >= lightSleepIdleMs)) {
                    // Log debug info before entering sleep
                    bool usbNow = isUsbPowered();
                    saveChargingDebug("ENTER_SLEEP", usbNow, batteryState.isChargingMode, activelyCharging);
                    
                    deepSleepTriggered = true;
                    enterLightSleep(touch, keyboardControl, ledController, bluetoothControllerPtr, touchSettings, lastActivityMillis, PINK_LED_PWM_PIN, BLUE_LED_PWM_PIN, PINK_PWM_MAX, PWM_MAX, PINK_RAMP_UP_MS, PINK_RAMP_DOWN_MS, BLUE_RAMP_UP_MS, BLUE_RAMP_DOWN_MS);
                }
            }
        }
        
        // Periodic debug logging (every 60s) to track charging state
        static unsigned long lastDebugLog = 0;
        if (batteryState.isChargingMode && (millis() - lastDebugLog > 60000)) {
            bool usbNow = isUsbPowered();
            saveChargingDebug("CHARGE_60S", usbNow, batteryState.isChargingMode, activelyCharging);
            lastDebugLog = millis();
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// enterDeepSleep() implemented in controls/SleepControl.h
