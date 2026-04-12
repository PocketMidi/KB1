#ifndef BLUETOOTH_CONTROLLER_H
#define BLUETOOTH_CONTROLLER_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <Preferences.h>
#include <music/ScaleManager.h>
#include <led/LEDController.h>
#include <esp_gap_ble_api.h>

class ServerCallbacks;
class CharacteristicCallbacks;

// BLE power modes for adaptive connection interval management
enum BLEPowerMode {
    LIVE_PERFORMANCE,   // Live sliders - max responsiveness (7.5-15ms, latency 0)
    CONFIGURATION,      // Settings changes - good responsiveness (30-50ms, latency 1)
    IDLE_CONNECTED      // Music making - max power savings (100-200ms, latency 4)
};

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
        ScaleSettings& scaleSettings,
        ChordSettings& chordSettings,
        SystemSettings& systemSettings
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
    BLECharacteristic* getChordSettingsCharacteristic() { return _pChordSettingsCharacteristic; }
    BLECharacteristic* getStrumIntervalsCharacteristic() { return _pStrumIntervalsCharacteristic; }
    BLECharacteristic* getSystemSettingsCharacteristic() { return _pSystemSettingsCharacteristic; }
    BLECharacteristic* getMidiCharacteristic() { return _pMidiCharacteristic; }
    BLECharacteristic* getKeepAliveCharacteristic() { return _pKeepAliveCharacteristic; }
    BLECharacteristic* getFirmwareVersionCharacteristic() { return _pFirmwareVersionCharacteristic; }
    BLECharacteristic* getBatteryStatusCharacteristic() { return _pBatteryStatusCharacteristic; }
    BLECharacteristic* getBatteryControlCharacteristic() { return _pBatteryControlCharacteristic; }
    
    // Preset characteristic getters
    BLECharacteristic* getPresetSaveCharacteristic() { return _pPresetSaveCharacteristic; }
    BLECharacteristic* getPresetLoadCharacteristic() { return _pPresetLoadCharacteristic; }
    BLECharacteristic* getPresetListCharacteristic() { return _pPresetListCharacteristic; }
    BLECharacteristic* getPresetDeleteCharacteristic() { return _pPresetDeleteCharacteristic; }

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

    // Adaptive power management (new in v1.7.0)
    void setActivityMode(BLEPowerMode mode);
    BLEPowerMode getCurrentPowerMode() const { return _currentPowerMode; }
    void storeRemoteAddress(const uint8_t* address);
    void updateBatteryModeTracking();  // Update battery time tracking when mode changes

    // Smart reconnect after sleep (v1.7.0)
    void setReconnectEligible(bool eligible);
    bool isReconnectEligible() const { return _reconnectEligible; }
    void startReconnectMode();
    bool isReconnectMode() const { return _reconnectMode; }
    unsigned long getReconnectStartMs() const { return _reconnectStartMs; }
    void exitReconnectMode();

    // Keep-alive management
    void setKeepAliveActive(bool active);
    
    // Notify BLE clients when settings change from firmware
    void notifyChordSettings();
    void updateBatteryStatus();  // Update battery status characteristic
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

    // Adaptive power management state (v1.7.0)
    BLEPowerMode _currentPowerMode;
    unsigned long _lastModeChangeMs;
    uint8_t _remoteAddress[6];
    bool _hasRemoteAddress;
    
    // Smart reconnect state (v1.7.0)
    bool _reconnectEligible;   // True if should auto-reconnect on wake
    bool _reconnectMode;        // True if currently in reconnect window
    unsigned long _reconnectStartMs;  // When reconnect mode started

    BLECharacteristic* _pLever1SettingsCharacteristic;
    BLECharacteristic* _pLeverPush1SettingsCharacteristic;
    BLECharacteristic* _pLever2SettingsCharacteristic;
    BLECharacteristic* _pLeverPush2SettingsCharacteristic;
    BLECharacteristic* _pTouchSettingsCharacteristic;
    BLECharacteristic* _pScaleSettingsCharacteristic;
    BLECharacteristic* _pChordSettingsCharacteristic;
    BLECharacteristic* _pStrumIntervalsCharacteristic;
    BLECharacteristic* _pSystemSettingsCharacteristic;
    BLECharacteristic* _pMidiCharacteristic;
    BLECharacteristic* _pKeepAliveCharacteristic;
    BLECharacteristic* _pFirmwareVersionCharacteristic;
    BLECharacteristic* _pBatteryStatusCharacteristic;
    BLECharacteristic* _pBatteryControlCharacteristic;  // For commands like reset/recalibrate
    
    // Preset management characteristics
    BLECharacteristic* _pPresetSaveCharacteristic;
    BLECharacteristic* _pPresetLoadCharacteristic;
    BLECharacteristic* _pPresetListCharacteristic;
    BLECharacteristic* _pPresetDeleteCharacteristic;

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
    ChordSettings& _chordSettings;
    SystemSettings& _systemSettings;

    // Private helper methods
    void updateConnectionParams();  // Apply power mode to BLE connection parameters
};

#endif