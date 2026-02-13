#ifndef CHARACTERISTIC_CALLBACKS_H
#define CHARACTERISTIC_CALLBACKS_H

#include <Preferences.h>
#include <BLECharacteristic.h>
#include <music/ScaleManager.h>
#include <objects/Settings.h>
#include <string>

class BluetoothController;

// Generic callback that writes incoming bytes directly into a provided destination
class GenericSettingsCallback final : public BLECharacteristicCallbacks {
public:
    GenericSettingsCallback(BluetoothController* controller,
                            Preferences& preferences,
                            void* dest,
                            size_t destSize,
                            const char* prefKey,
                            ScaleManager* scaleManager = nullptr);
    void onWrite(BLECharacteristic *pCharacteristic) override;

private:
    BluetoothController* _controller;
    Preferences& _preferences;
    void* _dest;
    size_t _destSize;
    std::string _prefKey;
    ScaleManager* _scaleManager;
};

// MIDI callback keeps original behavior for parsing CC strings
class MidiSettingsCallback final : public BLECharacteristicCallbacks {
public:
    MidiSettingsCallback(BluetoothController* controller);
    void onWrite(BLECharacteristic *pCharacteristic) override;
private:
    BluetoothController* _controller;
};

// Keep-alive callback for app ping communication
class KeepAliveCallback final : public BLECharacteristicCallbacks {
public:
    KeepAliveCallback(BluetoothController* controller);
    void onWrite(BLECharacteristic *pCharacteristic) override;
private:
    BluetoothController* _controller;
};

#endif