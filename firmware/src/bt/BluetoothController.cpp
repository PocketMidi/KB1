#include <bt/BluetoothController.h>
#include <bt/ServerCallbacks.h>
#include <bt/CharacteristicCallbacks.h>
#include <bt/SecurityCallbacks.h>
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
    _pSystemSettingsCharacteristic(nullptr),
    _pMidiCharacteristic(nullptr),
    _pKeepAliveCharacteristic(nullptr),
    _pService(nullptr),
    _isEnabled(false),
    _lastToggleTime(0),
    _lastActivity(0),
    _modemSleeping(false),
    _lastKeepAlivePing(0),
    _keepAliveActive(false),
    _keepAliveGracePeriod(KEEPALIVE_GRACE_PERIOD_MS)
{
}

void BluetoothController::enable() {
    if (!_isEnabled) {
        SERIAL_PRINTLN("Enabling Bluetooth...");

        BLEDevice::init("KB1");
        BLEDevice::setSecurityCallbacks(new SecurityCallbacks());

        esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;
        esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));

        esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
        esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));

        _pServer = BLEDevice::createServer();
        _pServer->setCallbacks(new ServerCallbacks(this));

        _pService = _pServer->createService(BLEUUID(SERVICE_UUID), 25, 0);

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
            BLECharacteristic::PROPERTY_NOTIFY |
            BLECharacteristic::PROPERTY_INDICATE
        );
        _pMidiCharacteristic->addDescriptor(new BLE2902());
        _pMidiCharacteristic->setCallbacks(new MidiSettingsCallback(this));

        _pKeepAliveCharacteristic = _pService->createCharacteristic(
            KEEPALIVE_UUID,
            BLECharacteristic::PROPERTY_WRITE
        );
        _pKeepAliveCharacteristic->addDescriptor(new BLE2902());
        _pKeepAliveCharacteristic->setCallbacks(new KeepAliveCallback(this));

        // Start the service
        _pService->start();

        _pAdvertising = _pServer->getAdvertising();
        _pAdvertising->addServiceUUID(SERVICE_UUID);
        _pAdvertising->setScanResponse(true);
        _pAdvertising->setMinPreferred(0x06);
        _pAdvertising->setMinPreferred(0x12);
        BLEDevice::startAdvertising();
        SERIAL_PRINTLN("Waiting for a BLE client connection...");
        _isEnabled = true;
        _lastToggleTime = millis();
        _lastActivity = millis();
        _modemSleeping = false;
        SERIAL_PRINTLN("Exiting modem sleep due to Bluetooth enable.");
        // Blink LEDs for Bluetooth enabled
        _ledController.pulse(LedColor::BLUE, 333, 2000);
        _ledController.pulse(LedColor::PINK, 333, 2000);
    }
}

void BluetoothController::disable() {
    if (_isEnabled) {
        SERIAL_PRINTLN("Disabling Bluetooth...");
        if (_deviceConnected) {
            _pServer->disconnect(0);
        }
        BLEDevice::deinit(false);
        _isEnabled = false;
        _lastToggleTime = millis();
        _lastActivity = millis();
        _modemSleeping = false;
        SERIAL_PRINTLN("Bluetooth Disabled.");
        SERIAL_PRINTLN("Exiting modem sleep due to Bluetooth disable.");
        // Blink LEDs for Bluetooth disabled
        _ledController.pulse(LedColor::BLUE, 1000, 2000);
        _ledController.pulse(LedColor::PINK, 1000, 2000);
    }
}

void BluetoothController::setDeviceConnected(bool connected) {
    _deviceConnected = connected;
    if (connected) {
        SERIAL_PRINTLN("BLE client connected.");
    } else {
        SERIAL_PRINTLN("BLE client disconnected.");
    }
    // Treat connection event as activity (will still allow sleep after idleThreshold)
    updateLastActivity();
}

void BluetoothController::updateLastActivity() {
    _lastActivity = millis();
    SERIAL_PRINT("BLE activity at ms: "); SERIAL_PRINTLN(_lastActivity);
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
}
