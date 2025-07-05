#ifndef BLUETOOTH_CONTROLLER_H
#define BLUETOOTH_CONTROLLER_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <Preferences.h>
#include <music/ScaleManager.h>
#include <led/LEDController.h>

class ServerCallbacks;
class CharacteristicCallbacks;

class BluetoothController {
public:
    BluetoothController(
        Preferences& preferences,
        ScaleManager& scaleManager,
        LEDController& ledController,
        int& ccNumberSWD1LeftRight,
        int& ccNumberSWD1Center,
        int& ccNumberSWD2LeftRight,
        int& ccNumberSWD2Center
    );

    LEDController& _ledController;

    void enable();
    void disable();

    // Getters for characteristics (used by CharacteristicCallbacks)
    BLECharacteristic* getSWD1LRCCCharacteristic() { return _pSWD1LRCCCharacteristic; }
    BLECharacteristic* getSWD1CenterCCCharacteristic() { return _pSWD1CenterCCCharacteristic; }
    BLECharacteristic* getSWD2LRCCCharacteristic() { return _pSWD2LRCCCharacteristic; }
    BLECharacteristic* getSWD2CenterCCCharacteristic() { return _pSWD2CenterCCCharacteristic; }
    BLECharacteristic* getMidiCcCharacteristic() { return _pMidiCcCharacteristic; }
    BLECharacteristic* getRootNoteCharacteristic() { return _pRootNoteCharacteristic; }
    BLECharacteristic* getScaleTypeCharacteristic() { return _pScaleTypeCharacteristic; }

    // Methods for ServerCallbacks to update connection status and advertising
    void setDeviceConnected(bool connected);
    bool isEnabled() { return _isEnabled; }
    BLEAdvertising* getAdvertising() { return _pAdvertising; }

private:
    BLEServer* _pServer;
    BLEService* _pService;
    BLEAdvertising* _pAdvertising;
    bool _deviceConnected;
    bool _isEnabled;

    BLECharacteristic* _pSWD1LRCCCharacteristic;
    BLECharacteristic* _pSWD1CenterCCCharacteristic;
    BLECharacteristic* _pSWD2LRCCCharacteristic;
    BLECharacteristic* _pSWD2CenterCCCharacteristic;
    BLECharacteristic* _pMidiCcCharacteristic;
    BLECharacteristic* _pRootNoteCharacteristic;
    BLECharacteristic* _pScaleTypeCharacteristic;

    Preferences& _preferences;
    ScaleManager& _scaleManager;

    // References to global CC number variables
    int& _ccNumberSWD1LeftRight;
    int& _ccNumberSWD1Center;
    int& _ccNumberSWD2LeftRight;
    int& _ccNumberSWD2Center;
};

#endif