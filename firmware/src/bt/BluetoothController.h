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
        LeverSettings& lever1Settings,
        LeverPushSettings& leverPush1Settings,
        LeverSettings& lever2Settings,
        LeverPushSettings& leverPush2Settings,
        TouchSettings& touchSettings,
        ScaleSettings& scaleSettings
    );

    LEDController& _ledController;

    void enable();
    void disable();

    // Getters for characteristics (used by CharacteristicCallbacks)
    BLECharacteristic* getLever1SettingsCharacteristic() { return _pLever1SettingsCharacteristic; }
    BLECharacteristic* getLeverPush1SettingsCharacteristic() { return _pLeverPush1SettingsCharacteristic; }
    BLECharacteristic* getLever2SettingsCharacteristic() { return _pLever2SettingsCharacteristic; }
    BLECharacteristic* getLeverPush2SettingsCharacteristic() { return _pLeverPush2SettingsCharacteristic; }
    BLECharacteristic* getTouchSettingsCharacteristic() { return _pTouchSettingsCharacteristic; }
    BLECharacteristic* getScaleSettingsCharacteristic() { return _pScaleSettingsCharacteristic; }
    BLECharacteristic* getMidiCharacteristic() { return _pMidiCharacteristic; }
    BLECharacteristic* getKeepAliveCharacteristic() { return _pKeepAliveCharacteristic; }

    // Access the last enable/disable timestamp for cooldown checks
    unsigned long getLastToggleTime() const { return _lastToggleTime; }


    // Methods for ServerCallbacks to update connection status and advertising
    void setDeviceConnected(bool connected);
    bool isEnabled() { return _isEnabled; }
    BLEAdvertising* getAdvertising() { return _pAdvertising; }

    // Activity/sleep control (public API)
    void updateLastActivity();
    unsigned long getLastActivity() const { return _lastActivity; }
    void checkIdleAndSleep(unsigned long idleThresholdMs);

    // Keep-alive management
    void setKeepAliveActive(bool active);
    bool isKeepAliveActive() const { return _keepAliveActive; }
    unsigned long getLastKeepAlivePing() const { return _lastKeepAlivePing; }
    unsigned long getKeepAliveGracePeriod() const { return _keepAliveGracePeriod; }
    void refreshKeepAlive();

private:
    BLEServer* _pServer;
    BLEService* _pService;
    BLEAdvertising* _pAdvertising;
    bool _deviceConnected;
    bool _isEnabled;
    unsigned long _lastToggleTime;
    unsigned long _lastActivity;
    bool _modemSleeping;

    BLECharacteristic* _pLever1SettingsCharacteristic;
    BLECharacteristic* _pLeverPush1SettingsCharacteristic;
    BLECharacteristic* _pLever2SettingsCharacteristic;
    BLECharacteristic* _pLeverPush2SettingsCharacteristic;
    BLECharacteristic* _pTouchSettingsCharacteristic;
    BLECharacteristic* _pScaleSettingsCharacteristic;
    BLECharacteristic* _pMidiCharacteristic;
    BLECharacteristic* _pKeepAliveCharacteristic;

    // Keep-alive state
    unsigned long _lastKeepAlivePing;
    bool _keepAliveActive;
    unsigned long _keepAliveGracePeriod;

    Preferences& _preferences;
    ScaleManager& _scaleManager;


    // References to settings structs
    LeverSettings& _lever1Settings;
    LeverPushSettings& _leverPush1Settings;
    LeverSettings& _lever2Settings;
    LeverPushSettings& _leverPush2Settings;
    TouchSettings& _touchSettings;
    ScaleSettings& _scaleSettings;
};

#endif