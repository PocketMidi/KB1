#include <bt/BluetoothController.h>
#include <bt/ServerCallbacks.h>
#include <bt/CharacteristicCallbacks.h>
#include <bt/SecurityCallbacks.h>
#include <bt/PresetCallbacks.h>
#include <objects/Constants.h>
#include <objects/Globals.h>
#include <objects/Settings.h>
#include <esp_wifi.h>

BluetoothController::BluetoothController(
    Preferences& preferences,
    ScaleManager& scaleManager,
    LEDController& ledController,
    LeverSettings& lever1Settings,
    LeverPushSettings& leverPush1Settings,
    LeverSettings& lever2Settings,
    LeverPushSettings& leverPush2Settings,
    TouchSettings& touchSettings,
    ScaleSettings& scaleSettings,
    ChordSettings& chordSettings,
    SystemSettings& systemSettings)
    :
    _preferences(preferences),
    _scaleManager(scaleManager),
    _ledController(ledController),
    _lever1Settings(lever1Settings),
    _leverPush1Settings(leverPush1Settings),
    _lever2Settings(lever2Settings),
    _leverPush2Settings(leverPush2Settings),
    _touchSettings(touchSettings),
    _scaleSettings(scaleSettings),
    _chordSettings(chordSettings),
    _systemSettings(systemSettings),

    _pServer(nullptr),
    _pAdvertising(nullptr),
    _deviceConnected(false),
    _pLever1SettingsCharacteristic(nullptr),
    _pLeverPush1SettingsCharacteristic(nullptr),
    _pLever2SettingsCharacteristic(nullptr),
    _pLeverPush2SettingsCharacteristic(nullptr),
    _pTouchSettingsCharacteristic(nullptr),
    _pScaleSettingsCharacteristic(nullptr),
    _pChordSettingsCharacteristic(nullptr),
    _pStrumIntervalsCharacteristic(nullptr),
    _pSystemSettingsCharacteristic(nullptr),
    _pMidiCharacteristic(nullptr),
    _pKeepAliveCharacteristic(nullptr),
    _pFirmwareVersionCharacteristic(nullptr),
    _pBatteryStatusCharacteristic(nullptr),
    _pPresetSaveCharacteristic(nullptr),
    _pPresetLoadCharacteristic(nullptr),
    _pPresetListCharacteristic(nullptr),
    _pPresetDeleteCharacteristic(nullptr),
    _pService(nullptr),
    _isEnabled(false),
    _lastToggleTime(0),
    _lastActivity(0),
    _modemSleeping(false),
    _currentPowerMode(CONFIGURATION),  // Start in config mode for responsive initial setup
    _lastModeChangeMs(0),
    _hasRemoteAddress(false),
    _reconnectEligible(false),
    _reconnectMode(false),
    _reconnectStartMs(0),
    _lastKeepAlivePing(0),
    _keepAliveActive(false),
    _keepAliveGracePeriod(KEEPALIVE_GRACE_PERIOD_MS)
{
    memset(_remoteAddress, 0, 6);  // Clear BLE address
}

void BluetoothController::enable() {
    if (!_isEnabled) {
        SERIAL_PRINTLN("BLE:Init");

        BLEDevice::init("KB1");
        BLEDevice::setSecurityCallbacks(new SecurityCallbacks());

        esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;
        esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));

        esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
        esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));

        _pServer = BLEDevice::createServer();
        _pServer->setCallbacks(new ServerCallbacks(this));

        _pService = _pServer->createService(BLEUUID(SERVICE_UUID), 50, 0);

        _pLever1SettingsCharacteristic = _pService->createCharacteristic(
            LEVER1_SETTINGS_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE
        );
        _pLever1SettingsCharacteristic->addDescriptor(new BLE2902());
        _pLever1SettingsCharacteristic->setValue((uint8_t*)&_lever1Settings, sizeof(LeverSettings));
        _pLever1SettingsCharacteristic->setCallbacks(new GenericSettingsCallback(
            this,
            _preferences,
            &_lever1Settings,
            sizeof(LeverSettings),
            "lever1",
            nullptr
        ));

        _pLeverPush1SettingsCharacteristic = _pService->createCharacteristic(
            LEVERPUSH1_SETTINGS_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE
        );
        _pLeverPush1SettingsCharacteristic->addDescriptor(new BLE2902());
        _pLeverPush1SettingsCharacteristic->setValue((uint8_t*)&_leverPush1Settings, sizeof(LeverPushSettings));
        _pLeverPush1SettingsCharacteristic->setCallbacks(new GenericSettingsCallback(
            this,
            _preferences,
            &_leverPush1Settings,
            sizeof(LeverPushSettings),
            "leverpush1",
            nullptr
        ));

        _pLever2SettingsCharacteristic = _pService->createCharacteristic(
            LEVER2_SETTINGS_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE
        );
        _pLever2SettingsCharacteristic->addDescriptor(new BLE2902());
        _pLever2SettingsCharacteristic->setValue((uint8_t*)&_lever2Settings, sizeof(LeverSettings));
        _pLever2SettingsCharacteristic->setCallbacks(new GenericSettingsCallback(
            this,
            _preferences,
            &_lever2Settings,
            sizeof(LeverSettings),
            "lever2",
            nullptr
        ));

        _pLeverPush2SettingsCharacteristic = _pService->createCharacteristic(
            LEVERPUSH2_SETTINGS_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE
        );
        _pLeverPush2SettingsCharacteristic->addDescriptor(new BLE2902());
        _pLeverPush2SettingsCharacteristic->setValue((uint8_t*)&_leverPush2Settings, sizeof(LeverPushSettings));
        _pLeverPush2SettingsCharacteristic->setCallbacks(new GenericSettingsCallback(
            this,
            _preferences,
            &_leverPush2Settings,
            sizeof(LeverPushSettings),
            "leverpush2",
            nullptr
        ));

        _pTouchSettingsCharacteristic = _pService->createCharacteristic(
            TOUCH_SETTINGS_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE
        );
        _pTouchSettingsCharacteristic->addDescriptor(new BLE2902());
        _pTouchSettingsCharacteristic->setValue((uint8_t*)&_touchSettings, sizeof(TouchSettings));
        _pTouchSettingsCharacteristic->setCallbacks(new GenericSettingsCallback(
            this,
            _preferences,
            &_touchSettings,
            sizeof(TouchSettings),
            "touch",
            nullptr
        ));

        _pScaleSettingsCharacteristic = _pService->createCharacteristic(
            SCALE_SETTINGS_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE
        );
        _pScaleSettingsCharacteristic->addDescriptor(new BLE2902());
        _pScaleSettingsCharacteristic->setValue((uint8_t*)&_scaleSettings, sizeof(ScaleSettings));
        _pScaleSettingsCharacteristic->setCallbacks(new GenericSettingsCallback(
            this,
            _preferences,
            &_scaleSettings,
            sizeof(ScaleSettings),
            "scale",
            &_scaleManager
        ));

        _pChordSettingsCharacteristic = _pService->createCharacteristic(
            CHORD_SETTINGS_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE
        );
        _pChordSettingsCharacteristic->addDescriptor(new BLE2902());
        _pChordSettingsCharacteristic->setValue((uint8_t*)&_chordSettings, sizeof(ChordSettings));
        _pChordSettingsCharacteristic->setCallbacks(new GenericSettingsCallback(
            this,
            _preferences,
            &_chordSettings,
            sizeof(ChordSettings),
            "chord",
            nullptr
        ));

        _pStrumIntervalsCharacteristic = _pService->createCharacteristic(
            STRUM_INTERVALS_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE
        );
        _pStrumIntervalsCharacteristic->addDescriptor(new BLE2902());
        _pStrumIntervalsCharacteristic->setCallbacks(new StrumIntervalsCallback(this, _preferences));

        _pSystemSettingsCharacteristic = _pService->createCharacteristic(
            SYSTEM_SETTINGS_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE
        );
        _pSystemSettingsCharacteristic->addDescriptor(new BLE2902());
        _pSystemSettingsCharacteristic->setValue((uint8_t*)&_systemSettings, sizeof(SystemSettings));
        _pSystemSettingsCharacteristic->setCallbacks(new GenericSettingsCallback(
            this,
            _preferences,
            &_systemSettings,
            sizeof(SystemSettings),
            "system",
            nullptr
        ));

        _pMidiCharacteristic = _pService->createCharacteristic(
            MIDI_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_WRITE_NR |
            BLECharacteristic::PROPERTY_NOTIFY |
            BLECharacteristic::PROPERTY_INDICATE
        );
        _pMidiCharacteristic->addDescriptor(new BLE2902());
        _pMidiCharacteristic->setCallbacks(new MidiSettingsCallback(this));

        _pKeepAliveCharacteristic = _pService->createCharacteristic(
            KEEPALIVE_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_WRITE_NR |
            BLECharacteristic::PROPERTY_NOTIFY
        );
        _pKeepAliveCharacteristic->addDescriptor(new BLE2902());
        _pKeepAliveCharacteristic->setCallbacks(new KeepAliveCallback(this));

        // Firmware Version Characteristic (Read-only)
        _pFirmwareVersionCharacteristic = _pService->createCharacteristic(
            FIRMWARE_VERSION_UUID,
            BLECharacteristic::PROPERTY_READ
        );
        _pFirmwareVersionCharacteristic->addDescriptor(new BLE2902());
        
        // Set version as string in format "major.minor.patch"
        _pFirmwareVersionCharacteristic->setValue(FIRMWARE_VERSION);

        // Battery Status Characteristic (Read-only with notifications)
        _pBatteryStatusCharacteristic = _pService->createCharacteristic(
            BATTERY_STATUS_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_NOTIFY
        );
        _pBatteryStatusCharacteristic->addDescriptor(new BLE2902());
        
        // Initialize with current battery state
        // Format: [percentage(1), remainingSeconds(4 LE), usbConnected(1), calibrationTimestamp(4 LE)] = 10 bytes
        uint8_t batteryData[10];
        batteryData[0] = batteryState.estimatedPercentage;
        uint32_t remainingSeconds = 0;  // Will be calculated properly on each update
        memcpy(&batteryData[1], &remainingSeconds, 4);
        batteryData[5] = batteryState.lastUsbState ? 1 : 0;
        memcpy(&batteryData[6], &batteryState.calibrationTimestamp, 4);
        _pBatteryStatusCharacteristic->setValue(batteryData, 10);

        // Battery Control Characteristic (Write - for commands like reset/recalibrate)
        _pBatteryControlCharacteristic = _pService->createCharacteristic(
            BATTERY_CONTROL_UUID,
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_WRITE_NR
        );
        _pBatteryControlCharacteristic->setCallbacks(new BatteryControlCallback(this, _preferences));

        // Preset Save Characteristic (Write)
        _pPresetSaveCharacteristic = _pService->createCharacteristic(
            PRESET_SAVE_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_WRITE_NR
        );
        _pPresetSaveCharacteristic->addDescriptor(new BLE2902());
        _pPresetSaveCharacteristic->setCallbacks(new PresetSaveCallback(
            this, _preferences, _lever1Settings, _leverPush1Settings,
            _lever2Settings, _leverPush2Settings, _touchSettings,
            _scaleSettings, _chordSettings, _systemSettings
        ));
        // Preset Load Characteristic (Write)
        _pPresetLoadCharacteristic = _pService->createCharacteristic(
            PRESET_LOAD_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_WRITE_NR
        );
        _pPresetLoadCharacteristic->addDescriptor(new BLE2902());
        _pPresetLoadCharacteristic->setCallbacks(new PresetLoadCallback(
            this, _preferences, _scaleManager, _lever1Settings, _leverPush1Settings,
            _lever2Settings, _leverPush2Settings, _touchSettings,
            _scaleSettings, _chordSettings, _systemSettings
        ));

        // Preset List Characteristic (Read)
        _pPresetListCharacteristic = _pService->createCharacteristic(
            PRESET_LIST_UUID,
            BLECharacteristic::PROPERTY_READ
        );
        _pPresetListCharacteristic->addDescriptor(new BLE2902());
        _pPresetListCharacteristic->setCallbacks(new PresetListCallback(this, _preferences));

        // Preset Delete Characteristic (Write)
        _pPresetDeleteCharacteristic = _pService->createCharacteristic(
            PRESET_DELETE_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_WRITE_NR
        );
        _pPresetDeleteCharacteristic->addDescriptor(new BLE2902());
        _pPresetDeleteCharacteristic->setCallbacks(new PresetDeleteCallback(this, _preferences));

        // Start the service
        _pService->start();

        _pAdvertising = _pServer->getAdvertising();
        _pAdvertising->addServiceUUID(SERVICE_UUID);
        _pAdvertising->setScanResponse(true);
        _pAdvertising->setMinPreferred(0x06);
        _pAdvertising->setMinPreferred(0x12);
        BLEDevice::startAdvertising();
        SERIAL_PRINTLN("BLE:Ready");
        _isEnabled = true;
        _lastToggleTime = millis();
        _lastActivity = millis();
        _modemSleeping = false;
    }
}

void BluetoothController::disable() {
    if (_isEnabled) {
        SERIAL_PRINTLN("BLE:Shutdown");
        if (_deviceConnected) {
            _pServer->disconnect(0);
        }
        BLEDevice::deinit(false);
        _isEnabled = false;
        _lastToggleTime = millis();
        _lastActivity = millis();
        _modemSleeping = false;
        SERIAL_PRINTLN("BLE:Off");
    }
}

void BluetoothController::setDeviceConnected(bool connected) {
    _deviceConnected = connected;
    // Treat connection event as activity (will still allow sleep after idleThreshold)
    updateLastActivity();
}

void BluetoothController::updateLastActivity() {
    _lastActivity = millis();
    // SERIAL_PRINT("BLE activity at ms: "); SERIAL_PRINTLN(_lastActivity);
    if (_modemSleeping) {
        esp_wifi_set_ps(WIFI_PS_NONE);
        _modemSleeping = false;
        SERIAL_PRINTLN("Exiting modem sleep due to BLE activity.");
    }
}

void BluetoothController::checkIdleAndSleep(unsigned long idleThresholdMs) {
    if (!_isEnabled) return;
    // Idle is determined strictly by last characteristic activity timestamp,
    // even if a BLE client is connected.
    unsigned long now = millis();
    if (!_modemSleeping && (now - _lastActivity >= idleThresholdMs)) {
        // Enter modem sleep
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        _modemSleeping = true;
        SERIAL_PRINTLN("Entering modem sleep due to BLE idle.");
    }
}

void BluetoothController::setKeepAliveActive(bool active) {
    _keepAliveActive = active;
    if (active) {
        _lastKeepAlivePing = millis();
        updateLastActivity();
        SERIAL_PRINTLN("Keep-alive activated.");
    } else {
        SERIAL_PRINTLN("Keep-alive deactivated.");
    }
}

void BluetoothController::refreshKeepAlive() {
    _lastKeepAlivePing = millis();
    _keepAliveActive = true;
    updateLastActivity();

    // Send status notification back to web app (battery + USB state)
    // Format: [battery(1), flags(1), pattern(1), octave(1), scale(1), root(1), reserved(4)] = 10 bytes
    // bit0 of flags = USB connected, bit1 = touch calibrated (reserved for future)
    if (_isEnabled && _pKeepAliveCharacteristic && _deviceConnected) {
        uint8_t statusPacket[10] = {0};
        statusPacket[0] = batteryState.estimatedPercentage;
        statusPacket[1] = (batteryState.lastUsbState ? 0x01 : 0x00);
        statusPacket[2] = (uint8_t)_chordSettings.strumPattern;
        statusPacket[3] = 128; // octave offset reserved (128 = center/0)
        statusPacket[4] = (uint8_t)_scaleSettings.scaleType;
        statusPacket[5] = (uint8_t)(_scaleSettings.rootNote % 12);
        // bytes 6-9: reserved
        _pKeepAliveCharacteristic->setValue(statusPacket, 10);
        _pKeepAliveCharacteristic->notify();
    }
}

// Store remote BLE device address for connection parameter updates
void BluetoothController::storeRemoteAddress(const uint8_t* address) {
    if (address) {
        memcpy(_remoteAddress, address, 6);
        _hasRemoteAddress = true;
        if (serialConnected) {
            Serial.print("BLE remote addr: ");
            for (int i = 0; i < 6; i++) {
                if (_remoteAddress[i] < 0x10) Serial.print("0");
                Serial.print(_remoteAddress[i], HEX);
                if (i < 5) Serial.print(":");
            }
            Serial.println();
        }
    }
}

// Set activity mode and update connection parameters
// Conservative timeouts to avoid lag during configuration
void BluetoothController::setActivityMode(BLEPowerMode mode) {
    if (mode == _currentPowerMode) {
        return;  // No change needed
    }
    
    // Update battery tracking for time spent in previous mode
    updateBatteryModeTracking();
    
    unsigned long now = millis();
    BLEPowerMode oldMode = _currentPowerMode;  // Save for logging
    _currentPowerMode = mode;
    _lastModeChangeMs = now;
    
    // Also update lastModeChangeMs in battery state for consistency
    batteryState.lastModeChangeMs = now;
    
    const char* modeName = "";
    switch(mode) {
        case LIVE_PERFORMANCE:
            modeName = "LIVE";
            break;
        case CONFIGURATION:
            modeName = "CONFIG";
            break;
        case IDLE_CONNECTED:
            modeName = "IDLE";
            break;
    }
    SERIAL_PRINT("BLE power mode: ");
    SERIAL_PRINTLN(modeName);
    
    // Update connection parameters if we have a connection
    if (_deviceConnected) {
        updateConnectionParams();
    }
}

// Update battery time tracking when BLE power mode changes
void BluetoothController::updateBatteryModeTracking() {
    if (!_deviceConnected) {
        return;  // Nothing to track if not connected
    }
    
    unsigned long now = millis();
    unsigned long elapsed = now - batteryState.lastModeChangeMs;
    
    // Accumulate time spent in the current mode
    switch(_currentPowerMode) {
        case LIVE_PERFORMANCE:
            batteryState.bleLiveTimeMs += elapsed;
            break;
        case CONFIGURATION:
            batteryState.bleConfigTimeMs += elapsed;
            break;
        case IDLE_CONNECTED:
            batteryState.bleIdleTimeMs += elapsed;
            break;
    }
}

// Update BLE connection parameters based on current power mode
void BluetoothController::updateConnectionParams() {
    if (!_deviceConnected) {
        return;
    }
    
    uint16_t minInt, maxInt, latency;
    
    switch(_currentPowerMode) {
        case LIVE_PERFORMANCE:
            // Max responsiveness for live sliders
            minInt = 6;    // 7.5ms
            maxInt = 12;   // 15ms
            latency = 0;   // No latency
            break;
            
        case CONFIGURATION:
            // Good responsiveness for settings changes
            minInt = 24;   // 30ms
            maxInt = 40;   // 50ms
            latency = 1;   // Can skip 1 event (60-100ms worst case)
            break;
            
        case IDLE_CONNECTED:
            // Max power savings for music-making periods
            minInt = 80;   // 100ms
            maxInt = 160;  // 200ms
            latency = 4;   // Can skip 4 events (400-800ms response time)
            break;
    }
    
    // Get remote address from server if we don't have it yet
    // For ESP32 BLE Arduino, we need to get it from the peer devices
    if (!_hasRemoteAddress && _pServer) {
        std::map<uint16_t, conn_status_t> peer_devices = _pServer->getPeerDevices(false);
        if (!peer_devices.empty()) {
            // Get first (and likely only) connected device
            auto it = peer_devices.begin();
            // For ESP32, the remote_bda should be available in conn_status_t
            // If not directly accessible, we'll need to use GAP APIs
            // For now, mark as available and use the conn_id approach
            _hasRemoteAddress = true;
            SERIAL_PRINTLN("BLE: Using connection for param update");
        }
    }
    
    // If we still don't have connection info, skip update
    if (!_hasRemoteAddress) {
        SERIAL_PRINTLN("BLE: No connection info for param update");
        return;
    }
    
    // Prepare connection parameter update request
    // Note: With ESP32 BLE, we can try updating without explicit address
    // The BLE stack should apply to the current connection
    esp_ble_conn_update_params_t params = {0};
    // If we don't have the exact address, the BLE stack may still accept 
    // the update if there's only one active connection
    memset(params.bda, 0, 6);  // Zero address may work for single connection
    params.min_int = minInt;
    params.max_int = maxInt;
    params.latency = latency;
    params.timeout = 400;  // 4 seconds supervision timeout
    
    // Request the connection parameter update
    // Note: Central (phone/tablet) may accept or reject this request
    esp_err_t result = esp_ble_gap_update_conn_params(&params);
    
    if (result == ESP_OK) {
        SERIAL_PRINT("  Requested: ");
        SERIAL_PRINT(minInt * 1.25f);
        SERIAL_PRINT("-");
        SERIAL_PRINT(maxInt * 1.25f);
        SERIAL_PRINT("ms, latency=");
        SERIAL_PRINTLN(latency);
    } else {
        if (serialConnected) {
            Serial.print("  Param update failed: 0x");
            Serial.println(result, HEX);
        }
    }
}

// Smart reconnect management (v1.7.0)
void BluetoothController::setReconnectEligible(bool eligible) {
    _reconnectEligible = eligible;
    if (!eligible) {
        _reconnectMode = false;
        _reconnectStartMs = 0;
    }
    SERIAL_PRINT("BLE reconnect eligible: ");
    SERIAL_PRINTLN(eligible ? "YES" : "NO");
}

void BluetoothController::startReconnectMode() {
    if (!_reconnectEligible) {
        return;
    }
    
    SERIAL_PRINTLN("BLE:ReconnectMode");
    _reconnectMode = true;
    _reconnectStartMs = millis();
    
    // Enable BLE in IDLE mode for power-efficient reconnection
    if (!_isEnabled) {
        enable();
        setActivityMode(IDLE_CONNECTED);  // Start power-efficient (35mA)
    }
}

void BluetoothController::exitReconnectMode() {
    if (_reconnectMode) {
        SERIAL_PRINTLN("BLE reconnect mode ended");
        _reconnectMode = false;
        _reconnectEligible = false;
        _reconnectStartMs = 0;
    }
}

void BluetoothController::notifyChordSettings() {
    if (_isEnabled && _pChordSettingsCharacteristic && _deviceConnected) {
        _pChordSettingsCharacteristic->setValue((uint8_t*)&_chordSettings, sizeof(ChordSettings));
        _pChordSettingsCharacteristic->notify();
        // SERIAL_PRINTLN("Notified chord settings to BLE client");
    }
}

void BluetoothController::notifyScaleSettings() {
    if (_isEnabled && _pScaleSettingsCharacteristic && _deviceConnected) {
        _pScaleSettingsCharacteristic->setValue((uint8_t*)&_scaleSettings, sizeof(ScaleSettings));
        _pScaleSettingsCharacteristic->notify();
        // SERIAL_PRINTLN("Notified scale settings to BLE client");
    }
}

void BluetoothController::updateBatteryStatus() {
    if (_isEnabled && _pBatteryStatusCharacteristic) {
        // Calculate remaining runtime in seconds
        uint32_t remainingSeconds = 0;
        
        if (batteryState.estimatedPercentage < 255) {  // Not charging
            // Estimate remaining runtime based on current percentage and active drain
            float remainingMah = (batteryState.estimatedPercentage / 100.0f) * BATTERY_CAPACITY_MAH;
            float remainingHours = remainingMah / BATTERY_ACTIVE_DRAIN_MA;  // Conservative estimate using active drain
            remainingSeconds = (uint32_t)(remainingHours * 3600.0f);
        }
        
        // Format: [percentage(1), remainingSeconds(4 LE), usbConnected(1), calibrationTimestamp(4 LE)] = 10 bytes
        uint8_t batteryData[10];
        batteryData[0] = batteryState.estimatedPercentage;  // 0-100, 254 (uncalibrated), or 255 (charging)
        memcpy(&batteryData[1], &remainingSeconds, 4);       // Remaining seconds (little-endian)
        batteryData[5] = batteryState.lastUsbState ? 1 : 0;  // USB connection status
        memcpy(&batteryData[6], &batteryState.calibrationTimestamp, 4);  // Calibration timestamp (little-endian)
        
        _pBatteryStatusCharacteristic->setValue(batteryData, 10);
        
        // Notify if device is connected
        if (_deviceConnected) {
            _pBatteryStatusCharacteristic->notify();
        }
    }
}
