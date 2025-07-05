#include <bt/BluetoothController.h>
#include <bt/ServerCallbacks.h>
#include <bt/CharacteristicCallbacks.h>
#include <bt/SecurityCallbacks.h>
#include <objects/Constants.h>
#include <objects/Globals.h>

BluetoothController::BluetoothController(
    Preferences& preferences,
    ScaleManager& scaleManager,
    LEDController& ledController,
    int& ccNumberSWD1LeftRight,
    int& ccNumberSWD1Center,
    int& ccNumberSWD2LeftRight,
    int& ccNumberSWD2Center)
    : _preferences(preferences),
      _scaleManager(scaleManager),
      _ledController(ledController),
      _ccNumberSWD1LeftRight(ccNumberSWD1LeftRight),
      _ccNumberSWD1Center(ccNumberSWD1Center),
      _ccNumberSWD2LeftRight(ccNumberSWD2LeftRight),
      _ccNumberSWD2Center(ccNumberSWD2Center),
      _pServer(nullptr),
      _pAdvertising(nullptr),
      _deviceConnected(false),
      _pSWD1LRCCCharacteristic(nullptr),
      _pSWD1CenterCCCharacteristic(nullptr),
      _pSWD2LRCCCharacteristic(nullptr),
      _pSWD2CenterCCCharacteristic(nullptr),
      _pMidiCcCharacteristic(nullptr),
      _pRootNoteCharacteristic(nullptr),
      _pScaleTypeCharacteristic(nullptr),
      _pService(nullptr),
      _isEnabled(false)
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

        _pService = _pServer->createService(SERVICE_UUID);

        auto valSWD1LR = static_cast<uint8_t>(_ccNumberSWD1LeftRight);
        _pSWD1LRCCCharacteristic = _pService->createCharacteristic(
            SWD1_LR_CC_CHAR_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY
        );
        _pSWD1LRCCCharacteristic->addDescriptor(new BLE2902());
        _pSWD1LRCCCharacteristic->setCallbacks(new CharacteristicCallbacks(this, _preferences, _scaleManager, _ccNumberSWD1LeftRight, _ccNumberSWD1Center, _ccNumberSWD2LeftRight, _ccNumberSWD2Center));
        _pSWD1LRCCCharacteristic->setValue(&valSWD1LR, 1);

        auto valSWD1Center = static_cast<uint8_t>(_ccNumberSWD1Center);
        _pSWD1CenterCCCharacteristic = _pService->createCharacteristic(
            SWD1_CENTER_CC_CHAR_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY
        );
        _pSWD1CenterCCCharacteristic->addDescriptor(new BLE2902());
        _pSWD1CenterCCCharacteristic->setCallbacks(new CharacteristicCallbacks(this, _preferences, _scaleManager, _ccNumberSWD1LeftRight, _ccNumberSWD1Center, _ccNumberSWD2LeftRight, _ccNumberSWD2Center));
        _pSWD1CenterCCCharacteristic->setValue(&valSWD1Center, 1);

        auto valSWD2LR = static_cast<uint8_t>(_ccNumberSWD2LeftRight);
        _pSWD2LRCCCharacteristic = _pService->createCharacteristic(
            SWD2_LR_CC_CHAR_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY
        );
        _pSWD2LRCCCharacteristic->addDescriptor(new BLE2902());
        _pSWD2LRCCCharacteristic->setCallbacks(new CharacteristicCallbacks(this, _preferences, _scaleManager, _ccNumberSWD1LeftRight, _ccNumberSWD1Center, _ccNumberSWD2LeftRight, _ccNumberSWD2Center));
        _pSWD2LRCCCharacteristic->setValue(&valSWD2LR, 1);

        auto valSWD2Center = static_cast<uint8_t>(_ccNumberSWD2Center);
        _pSWD2CenterCCCharacteristic = _pService->createCharacteristic(
            SWD2_CENTER_CC_CHAR_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY
        );
        _pSWD2CenterCCCharacteristic->addDescriptor(new BLE2902());
        _pSWD2CenterCCCharacteristic->setCallbacks(new CharacteristicCallbacks(this, _preferences, _scaleManager, _ccNumberSWD1LeftRight, _ccNumberSWD1Center, _ccNumberSWD2LeftRight, _ccNumberSWD2Center));
        _pSWD2CenterCCCharacteristic->setValue(&valSWD2Center, 1);

        // Create MIDI CC Characteristic (Write Without Response for efficiency)
        _pMidiCcCharacteristic = _pService->createCharacteristic(
            MIDI_CC_CHAR_UUID,
            BLECharacteristic::PROPERTY_WRITE_NR // WRITE_NR for "Write Without Response"
        );
        _pMidiCcCharacteristic->setCallbacks(new CharacteristicCallbacks(this, _preferences, _scaleManager, _ccNumberSWD1LeftRight, _ccNumberSWD1Center, _ccNumberSWD2LeftRight, _ccNumberSWD2Center));

        _pRootNoteCharacteristic = _pService->createCharacteristic(
            ROOT_NOTE_CHAR_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY
        );
        _pRootNoteCharacteristic->addDescriptor(new BLE2902());
        _pRootNoteCharacteristic->setCallbacks(new CharacteristicCallbacks(this, _preferences, _scaleManager, _ccNumberSWD1LeftRight, _ccNumberSWD1Center, _ccNumberSWD2LeftRight, _ccNumberSWD2Center));
        auto initialRootNote = static_cast<uint8_t>(_scaleManager.getRootNote());
        _pRootNoteCharacteristic->setValue(&initialRootNote, 1);

        _pScaleTypeCharacteristic = _pService->createCharacteristic(
            SCALE_TYPE_CHAR_UUID,
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY
        );
        _pScaleTypeCharacteristic->addDescriptor(new BLE2902());
        _pScaleTypeCharacteristic->setCallbacks(new CharacteristicCallbacks(this, _preferences, _scaleManager, _ccNumberSWD1LeftRight, _ccNumberSWD1Center, _ccNumberSWD2LeftRight, _ccNumberSWD2Center));
        auto initialScaleType = static_cast<uint8_t>(_scaleManager.getScaleType());
        _pScaleTypeCharacteristic->setValue(&initialScaleType, 1);

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
        #ifdef SERIAL_PRINT_ENABLED
        BLEDevice::deinit(false);
        #else
        BLEDevice::deinit(true);
        #endif

        _isEnabled = false;
        SERIAL_PRINTLN("Bluetooth Disabled.");
        // Blink LEDs for Bluetooth disabled
        _ledController.pulse(LedColor::BLUE, 1000, 2000);
        _ledController.pulse(LedColor::PINK, 1000, 2000);
    }
}

void BluetoothController::setDeviceConnected(bool connected) {
    _deviceConnected = connected;
}
