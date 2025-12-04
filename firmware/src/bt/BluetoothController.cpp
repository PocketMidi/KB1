#include <bt/BluetoothController.h>
#include <bt/ServerCallbacks.h>
#include <bt/CharacteristicCallbacks.h>
#include <bt/SecurityCallbacks.h>
#include <objects/Constants.h>
#include <objects/Globals.h>
#include <objects/Settings.h>

BluetoothController::BluetoothController(
    Preferences& preferences,
    ScaleManager& scaleManager,
    LEDController& ledController,
    LeverSettings& lever1Settings,
    LeverPushSettings& leverPush1Settings,
    LeverSettings& lever2Settings,
    LeverPushSettings& leverPush2Settings,
    TouchSettings& touchSettings,
    ScaleSettings& scaleSettings)
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

    _pServer(nullptr),
    _pAdvertising(nullptr),
    _deviceConnected(false),
    _pLever1SettingsCharacteristic(nullptr),
    _pLeverPush1SettingsCharacteristic(nullptr),
    _pLever2SettingsCharacteristic(nullptr),
    _pLeverPush2SettingsCharacteristic(nullptr),
    _pTouchSettingsCharacteristic(nullptr),
    _pScaleSettingsCharacteristic(nullptr),
    _pMidiCharacteristic(nullptr),
    _pService(nullptr),
    _isEnabled(false),
    _lastToggleTime(0)
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
        _pLever1SettingsCharacteristic->setCallbacks(new CharacteristicCallbacks(
            this,
            _preferences,
            _scaleManager,
            _lever1Settings,
            _leverPush1Settings,
            _lever2Settings,
            _leverPush2Settings,
            _touchSettings,
            _scaleSettings
        ));

        _pLeverPush1SettingsCharacteristic = _pService->createCharacteristic(
            LEVERPUSH1_SETTINGS_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE
        );
        _pLeverPush1SettingsCharacteristic->addDescriptor(new BLE2902());
        _pLeverPush1SettingsCharacteristic->setValue((uint8_t*)&_leverPush1Settings, sizeof(LeverPushSettings));
        _pLeverPush1SettingsCharacteristic->setCallbacks(new CharacteristicCallbacks(
            this,
            _preferences,
            _scaleManager,
            _lever1Settings,
            _leverPush1Settings,
            _lever2Settings,
            _leverPush2Settings,
            _touchSettings,
            _scaleSettings
        ));

        _pLever2SettingsCharacteristic = _pService->createCharacteristic(
            LEVER2_SETTINGS_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE
        );
        _pLever2SettingsCharacteristic->addDescriptor(new BLE2902());
        _pLever2SettingsCharacteristic->setValue((uint8_t*)&_lever2Settings, sizeof(LeverSettings));
        _pLever2SettingsCharacteristic->setCallbacks(new CharacteristicCallbacks(
            this,
            _preferences,
            _scaleManager,
            _lever1Settings,
            _leverPush1Settings,
            _lever2Settings,
            _leverPush2Settings,
            _touchSettings,
            _scaleSettings
        ));

        _pLeverPush2SettingsCharacteristic = _pService->createCharacteristic(
            LEVERPUSH2_SETTINGS_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE
        );
        _pLeverPush2SettingsCharacteristic->addDescriptor(new BLE2902());
        _pLeverPush2SettingsCharacteristic->setValue((uint8_t*)&_leverPush2Settings, sizeof(LeverPushSettings));
        _pLeverPush2SettingsCharacteristic->setCallbacks(new CharacteristicCallbacks(
            this,
            _preferences,
            _scaleManager,
            _lever1Settings,
            _leverPush1Settings,
            _lever2Settings,
            _leverPush2Settings,
            _touchSettings,
            _scaleSettings
        ));

        _pTouchSettingsCharacteristic = _pService->createCharacteristic(
            TOUCH_SETTINGS_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE
        );
        _pTouchSettingsCharacteristic->addDescriptor(new BLE2902());
        _pTouchSettingsCharacteristic->setValue((uint8_t*)&_touchSettings, sizeof(TouchSettings));
        _pTouchSettingsCharacteristic->setCallbacks(new CharacteristicCallbacks(
            this,
            _preferences,
            _scaleManager,
            _lever1Settings,
            _leverPush1Settings,
            _lever2Settings,
            _leverPush2Settings,
            _touchSettings,
            _scaleSettings
        ));

        _pScaleSettingsCharacteristic = _pService->createCharacteristic(
            SCALE_SETTINGS_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE
        );
        _pScaleSettingsCharacteristic->addDescriptor(new BLE2902());
        _pScaleSettingsCharacteristic->setValue((uint8_t*)&_scaleSettings, sizeof(ScaleSettings));
        _pScaleSettingsCharacteristic->setCallbacks(new CharacteristicCallbacks(
            this,
            _preferences,
            _scaleManager,
            _lever1Settings,
            _leverPush1Settings,
            _lever2Settings,
            _leverPush2Settings,
            _touchSettings,
            _scaleSettings
        ));

        _pMidiCharacteristic = _pService->createCharacteristic(
            MIDI_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY |
            BLECharacteristic::PROPERTY_INDICATE
        );
        _pMidiCharacteristic->addDescriptor(new BLE2902());
        _pMidiCharacteristic->setCallbacks(new CharacteristicCallbacks(
            this,
            _preferences,
            _scaleManager,
            _lever1Settings,
            _leverPush1Settings,
            _lever2Settings,
            _leverPush2Settings,
            _touchSettings,
            _scaleSettings
        ));

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
        SERIAL_PRINTLN("Bluetooth Disabled.");
        // Blink LEDs for Bluetooth disabled
        _ledController.pulse(LedColor::BLUE, 1000, 2000);
        _ledController.pulse(LedColor::PINK, 1000, 2000);
    }
}

void BluetoothController::setDeviceConnected(bool connected) {
    _deviceConnected = connected;
}
