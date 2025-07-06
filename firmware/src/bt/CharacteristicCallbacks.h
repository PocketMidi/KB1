#ifndef CHARACTERISTIC_CALLBACKS_H
#define CHARACTERISTIC_CALLBACKS_H

#include <Preferences.h>
#include <BLECharacteristic.h>
#include <music/ScaleManager.h>
#include <objects/Settings.h>

class BluetoothController;

class CharacteristicCallbacks final : public BLECharacteristicCallbacks {
public:
    CharacteristicCallbacks(
        BluetoothController* controller,
        Preferences& preferences,
        ScaleManager& scaleManager,
        LeverSettings& lever1Settings,
        LeverPushSettings& lever1PushSettings,
        LeverSettings& lever2Settings,
        LeverPushSettings& lever2PushSettings,
        TouchSettings& touchSettings,
        ScaleSettings& scaleSettings);
    void onWrite(BLECharacteristic *pCharacteristic) override;

private:
    BluetoothController* _controller;
    Preferences& _preferences;
    ScaleManager& _scaleManager;
    LeverSettings& _lever1Settings;
    LeverPushSettings& _leverPush1Settings;
    LeverSettings& _lever2Settings;
    LeverPushSettings& _leverPush2Settings;
    TouchSettings& _touchSettings;
    ScaleSettings& _scaleSettings;
};

#endif