#ifndef CHARACTERISTIC_CALLBACKS_H
#define CHARACTERISTIC_CALLBACKS_H

#include <Preferences.h>
#include <BLECharacteristic.h>
#include <music/ScaleManager.h>

class BluetoothController;

class CharacteristicCallbacks final : public BLECharacteristicCallbacks {
public:
    CharacteristicCallbacks(
        BluetoothController* controller,
        Preferences& preferences,
        ScaleManager& scaleManager,
        int& ccNumberSWD1LeftRight,
        int& ccNumberSWD1Center,
        int& ccNumberSWD2LeftRight,
        int& ccNumberSWD2Center);
    void onWrite(BLECharacteristic *pCharacteristic) override;

private:
    BluetoothController* _controller;
    Preferences& _preferences;
    ScaleManager& _scaleManager;
    int& _ccNumberSWD1LeftRight;
    int& _ccNumberSWD1Center;
    int& _ccNumberSWD2LeftRight;
    int& _ccNumberSWD2Center;
};

#endif