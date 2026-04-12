#include <bt/CharacteristicCallbacks.h>
#include <bt/BluetoothController.h>
#include <objects/Globals.h>
#include <music/ScaleManager.h>
#include <music/StrumPatterns.h>
#include <objects/Settings.h>
#include <cstring>

GenericSettingsCallback::GenericSettingsCallback(
    BluetoothController* controller,
    Preferences& preferences,
    void* dest,
    size_t destSize,
    const char* prefKey,
    ScaleManager* scaleManager)
    : _controller(controller), _preferences(preferences), _dest(dest), _destSize(destSize), _prefKey(prefKey), _scaleManager(scaleManager)
{
}

void GenericSettingsCallback::onWrite(BLECharacteristic *pCharacteristic) {
    const std::string rxValue = pCharacteristic->getValue();

    if (_controller) {
        _controller->updateLastActivity();
        _controller->setActivityMode(CONFIGURATION);  // Settings write = config mode
    }

    // Basic length check
    if (rxValue.length() != _destSize) {
        SERIAL_PRINT("Invalid data length for ");
        SERIAL_PRINTLN(_prefKey.c_str());
        return;
    }

    // Perform copy and persist
    memcpy(_dest, rxValue.data(), _destSize);
    _preferences.putBytes(_prefKey.c_str(), _dest, _destSize);

    // Debug: Log lever settings updates to see stepSize
    if (_prefKey == "lever1" || _prefKey == "lever2") {
        LeverSettings* s = static_cast<LeverSettings*>(_dest);
        SERIAL_PRINT("✓ "); SERIAL_PRINT(_prefKey.c_str()); 
        SERIAL_PRINT(" settings updated - CC:"); SERIAL_PRINT(s->ccNumber);
        SERIAL_PRINT(", stepSize:"); SERIAL_PRINT(s->stepSize);
        SERIAL_PRINT(", mode:"); SERIAL_PRINTLN((int)s->functionMode);
    }

    // If this was scale settings, update the scale manager
    if (_scaleManager && _prefKey == "scale") {
        ScaleSettings* s = static_cast<ScaleSettings*>(_dest);
        _scaleManager->setScale(s->scaleType);
        _scaleManager->setRootNote(s->rootNote);
    }
    
    // Sync lever internal values when settings change
    if (_prefKey == "lever1" && syncLever1Callback) {
        syncLever1Callback();
    } else if (_prefKey == "leverpush1" && syncLeverPush1Callback) {
        syncLeverPush1Callback();
    } else if (_prefKey == "lever2" && syncLever2Callback) {
        syncLever2Callback();
    } else if (_prefKey == "leverpush2" && syncLeverPush2Callback) {
        syncLeverPush2Callback();
    }
    // When chord settings change (e.g., strumSpeed), sync all levers
    // in case any are assigned to CC 200 (Strum Speed)
    else if (_prefKey == "chord") {
        if (syncLever1Callback) syncLever1Callback();
        if (syncLeverPush1Callback) syncLeverPush1Callback();
        if (syncLever2Callback) syncLever2Callback();
        if (syncLeverPush2Callback) syncLeverPush2Callback();
        
        // Reset pattern controls if shape mode was disabled (strumPattern = 0)
        if (resetPatternControlsCallback) resetPatternControlsCallback();
    }

    // SERIAL_PRINT(_prefKey.c_str()); SERIAL_PRINTLN(" updated and saved.");
}

MidiSettingsCallback::MidiSettingsCallback(BluetoothController* controller)
    : _controller(controller)
{
}

void MidiSettingsCallback::onWrite(BLECharacteristic *pCharacteristic) {
    const std::string rxValue = pCharacteristic->getValue();
    if (_controller) {
        _controller->updateLastActivity();
        _controller->setActivityMode(LIVE_PERFORMANCE);  // MIDI write = live slider mode
    }

    if (!rxValue.empty()) {
        const auto rxString = String(rxValue.c_str());
        const int commaIndex = rxString.indexOf(',');

        if (commaIndex != -1) {
            const int ccNumber = rxString.substring(0, commaIndex).toInt();
            const int ccValue = rxString.substring(commaIndex + 1).toInt();

            // Throttle serial output: only print every 10th message
            static uint8_t printCounter = 0;
            if (printCounter == 0) {
                SERIAL_PRINT("CC");
                SERIAL_PRINT(ccNumber);
                SERIAL_PRINT("=");
                SERIAL_PRINTLN(ccValue);
            }
            printCounter = (printCounter + 1) % 10;

            if (ccNumber >= 0 && ccNumber <= 127 && ccValue >= 0 && ccValue <= 127) {
                MIDI.sendControlChange(ccNumber, ccValue, 1);
            }
        }
    }
}

KeepAliveCallback::KeepAliveCallback(BluetoothController* controller)
    : _controller(controller)
{
}

void KeepAliveCallback::onWrite(BLECharacteristic *pCharacteristic) {
    if (_controller) {
        // Refresh keep-alive timestamp and ensure it's active
        _controller->refreshKeepAlive();
        SERIAL_PRINTLN("Keep-alive ping received.");
    }
}

StrumIntervalsCallback::StrumIntervalsCallback(BluetoothController* controller, Preferences& preferences)
    : _controller(controller), _preferences(preferences)
{
}

void StrumIntervalsCallback::onWrite(BLECharacteristic *pCharacteristic) {
    const std::string rxValue = pCharacteristic->getValue();

    if (_controller) {
        _controller->updateLastActivity();
        _controller->setActivityMode(CONFIGURATION);  // Strum pattern write = config mode
    }

    // Expect data format: length byte + interval bytes (int8_t array)
    // Max 16 intervals, so max 17 bytes total
    if (rxValue.length() < 1 || rxValue.length() > 17) {
        SERIAL_PRINTLN("Invalid strum intervals data length");
        return;
    }

    uint8_t length = static_cast<uint8_t>(rxValue[0]);
    if (length > MAX_PATTERN_LENGTH) {
        length = MAX_PATTERN_LENGTH;
    }

    // Extract intervals from remaining bytes
    int8_t intervals[MAX_PATTERN_LENGTH];
    for (uint8_t i = 0; i < length && i + 1 < rxValue.length(); i++) {
        intervals[i] = static_cast<int8_t>(rxValue[i + 1]);
    }

    // Set custom pattern
    setCustomPattern(intervals, length);

    // Persist to preferences
    _preferences.putBytes("customStrum", &customPattern, sizeof(CustomPattern));

    SERIAL_PRINT("Strum:CustP");
    SERIAL_PRINTLN(length);
}

BatteryControlCallback::BatteryControlCallback(BluetoothController* controller, Preferences& preferences)
    : _controller(controller), _preferences(preferences)
{
}

void BatteryControlCallback::onWrite(BLECharacteristic *pCharacteristic) {
    const std::string rxValue = pCharacteristic->getValue();

    if (_controller) {
        _controller->updateLastActivity();
        _controller->setActivityMode(CONFIGURATION);  // Battery control = config mode
    }

    if (rxValue.length() < 1 || rxValue.length() > 2) {
        SERIAL_PRINTLN("Invalid battery control command length");
        return;
    }

    uint8_t command = static_cast<uint8_t>(rxValue[0]);
    
    if (command == 0x01) {  // Reset/recalibrate command
        // Reset battery state to uncalibrated
        batteryState.estimatedPercentage = 254;  // Uncalibrated
        batteryState.isFullyCharged = false;
        batteryState.isChargingMode = false;  // Exit charging mode
        batteryState.accumulatedDischargeMs = 0;
        batteryState.activeTimeMs = 0;
        batteryState.lightSleepTimeMs = 0;
        batteryState.deepSleepTimeMs = 0;
        batteryState.accumulatedChargeMs = 0;  // Clear accumulated charge time
        batteryState.chargeSessionStartMs = 0;  // Clear session timer
        batteryState.calibrationTimestamp = 0;  // Never calibrated
        
        // Save to NVS (must save all fields for consistency)
        _preferences.putUInt("batDischMs", 0);
        _preferences.putBool("batFull", false);
        _preferences.putUChar("batPct", 254);
        _preferences.putUInt("batCalTime", 0);
        _preferences.putULong("batAccChgMs", 0);  // Clear accumulated charge time in NVS
        batteryState.lastSaveMs = millis();  // Update save timestamp
        
        SERIAL_PRINTLN("Battery reset to uncalibrated - ready for recalibration");
        
        // If USB is connected, restart charging mode immediately
        // This allows LEDs to start if user resets battery while USB is plugged in
        if (batteryState.lastUsbState) {
            SERIAL_PRINTLN("USB connected - restarting charging mode after reset");
            batteryState.isChargingMode = true;
            batteryState.chargeSessionStartMs = millis();
        }
        
        // Notify UI of battery status change
        if (_controller) {
            _controller->updateBatteryStatus();
        }
    } else if (command == 0x02 && rxValue.length() == 2) {  // Set battery percentage (dev tool)
        uint8_t percentage = static_cast<uint8_t>(rxValue[1]);
        if (percentage > 100) {
            SERIAL_PRINTLN("Bat% out of range");
            return;
        }

        // Set time trackers so calculateBatteryPercentage() returns the correct value.
        // consumedMah = (100 - %) / 100 * capacity
        // activeMs   = consumedMah / activeDrainRate * 3600000
        float consumedMah = ((100.0f - (float)percentage) / 100.0f) * BATTERY_CAPACITY_MAH;
        uint32_t activeMs = (uint32_t)((consumedMah / BATTERY_ACTIVE_DRAIN_MA) * 3600000.0f);

        batteryState.activeTimeMs = activeMs;
        batteryState.lightSleepTimeMs = 0;
        batteryState.deepSleepTimeMs = 0;
        batteryState.accumulatedDischargeMs = activeMs;
        batteryState.estimatedPercentage = percentage;
        batteryState.isFullyCharged = (percentage == 100);
        if (percentage > 0 && batteryState.calibrationTimestamp == 0) {
            batteryState.calibrationTimestamp = millis() / 1000;
        }

        // Save all time-tracker fields to NVS
        _preferences.putUChar("batPct", percentage);
        _preferences.putUInt("batActiveMs", activeMs);
        _preferences.putUInt("batLightMs", 0);
        _preferences.putUInt("batDeepMs", 0);
        _preferences.putUInt("batDischMs", activeMs);
        _preferences.putBool("batFull", percentage == 100);
        if (batteryState.calibrationTimestamp > 0) {
            _preferences.putUInt("batCalTime", batteryState.calibrationTimestamp);
        }
        batteryState.lastSaveMs = millis();

        // Notify UI of battery status change
        if (_controller) {
            _controller->updateBatteryStatus();
        }

        SERIAL_PRINT("Bat% set: ");
        SERIAL_PRINTLN(percentage);
    } else {
        SERIAL_PRINT("Unknown battery control command: ");
        SERIAL_PRINTLN(command);
    }
}

