#ifndef PRESET_CALLBACKS_H
#define PRESET_CALLBACKS_H

#include <BLEDevice.h>
#include <Preferences.h>
#include <objects/Settings.h>

class BluetoothController;
class ScaleManager;

/**
 * Callback for saving current settings to a preset slot
 * Format: [slot#(1 byte)][name(32 bytes)]
 */
class PresetSaveCallback : public BLECharacteristicCallbacks {
public:
    PresetSaveCallback(
        BluetoothController* controller,
        Preferences& preferences,
        LeverSettings& lever1,
        LeverPushSettings& leverPush1,
        LeverSettings& lever2,
        LeverPushSettings& leverPush2,
        TouchSettings& touch,
        ScaleSettings& scale,
        SystemSettings& system
    );
    
    void onWrite(BLECharacteristic *pCharacteristic) override;

private:
    BluetoothController* _controller;
    Preferences& _preferences;
    LeverSettings& _lever1;
    LeverPushSettings& _leverPush1;
    LeverSettings& _lever2;
    LeverPushSettings& _leverPush2;
    TouchSettings& _touch;
    ScaleSettings& _scale;
    SystemSettings& _system;
};

/**
 * Callback for loading a preset from a slot
 * Format: [slot#(1 byte)]
 */
class PresetLoadCallback : public BLECharacteristicCallbacks {
public:
    PresetLoadCallback(
        BluetoothController* controller,
        Preferences& preferences,
        ScaleManager& scaleManager,
        LeverSettings& lever1,
        LeverPushSettings& leverPush1,
        LeverSettings& lever2,
        LeverPushSettings& leverPush2,
        TouchSettings& touch,
        ScaleSettings& scale,
        SystemSettings& system
    );
    
    void onWrite(BLECharacteristic *pCharacteristic) override;

private:
    BluetoothController* _controller;
    Preferences& _preferences;
    ScaleManager& _scaleManager;
    LeverSettings& _lever1;
    LeverPushSettings& _leverPush1;
    LeverSettings& _lever2;
    LeverPushSettings& _leverPush2;
    TouchSettings& _touch;
    ScaleSettings& _scale;
    SystemSettings& _system;
};

/**
 * Callback for listing all presets
 * Returns metadata for all slots
 */
class PresetListCallback : public BLECharacteristicCallbacks {
public:
    PresetListCallback(BluetoothController* controller, Preferences& preferences);
    void onRead(BLECharacteristic *pCharacteristic) override;

private:
    BluetoothController* _controller;
    Preferences& _preferences;
};

/**
 * Callback for deleting a preset
 * Format: [slot#(1 byte)]
 */
class PresetDeleteCallback : public BLECharacteristicCallbacks {
public:
    PresetDeleteCallback(BluetoothController* controller, Preferences& preferences);
    void onWrite(BLECharacteristic *pCharacteristic) override;

private:
    BluetoothController* _controller;
    Preferences& _preferences;
};

#endif
