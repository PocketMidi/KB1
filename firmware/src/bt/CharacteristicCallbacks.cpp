#include <bt/CharacteristicCallbacks.h>
#include <bt/BluetoothController.h>
#include <objects/Globals.h>
#include <music/ScaleManager.h>
#include <objects/Settings.h>

CharacteristicCallbacks::CharacteristicCallbacks(
    BluetoothController* controller,
    Preferences& preferences,
    ScaleManager& scaleManager,
    LeverSettings& lever1Settings,
    LeverPushSettings& leverPush1Settings,
    LeverSettings& lever2Settings,
    LeverPushSettings& leverPush2Settings,
    TouchSettings& touchSettings,
    ScaleSettings& scaleSettings)
    : _controller(controller),
      _preferences(preferences),
      _scaleManager(scaleManager),
      _lever1Settings(lever1Settings),
      _leverPush1Settings(leverPush1Settings),
      _lever2Settings(lever2Settings),
      _leverPush2Settings(leverPush2Settings),
      _touchSettings(touchSettings),
      _scaleSettings(scaleSettings)
{
}

void CharacteristicCallbacks::onWrite(BLECharacteristic *pCharacteristic) {
    const std::string rxValue = pCharacteristic->getValue();

    if (pCharacteristic->getUUID().equals(BLEUUID(LEVER1_SETTINGS_UUID))) {
        if (rxValue.length() == sizeof(LeverSettings)) {
            memcpy(&_lever1Settings, rxValue.data(), sizeof(LeverSettings));
            _preferences.putBytes("lever1", &_lever1Settings, sizeof(LeverSettings));
            SERIAL_PRINTLN("Lever1Settings updated and saved.");
        } else {
            SERIAL_PRINTLN("Invalid data length for Lever1Settings.");
        }
    } else if (pCharacteristic->getUUID().equals(BLEUUID(LEVERPUSH1_SETTINGS_UUID))) {
        if (rxValue.length() == sizeof(LeverPushSettings)) {
            memcpy(&_leverPush1Settings, rxValue.data(), sizeof(LeverPushSettings));
            _preferences.putBytes("leverpush1", &_leverPush1Settings, sizeof(LeverPushSettings));
            SERIAL_PRINTLN("LeverPush1Settings updated and saved.");
        } else {
            SERIAL_PRINTLN("Invalid data length for LeverPush1Settings.");
        }
    } else if (pCharacteristic->getUUID().equals(BLEUUID(LEVER2_SETTINGS_UUID))) {
        if (rxValue.length() == sizeof(LeverSettings)) {
            memcpy(&_lever2Settings, rxValue.data(), sizeof(LeverSettings));
            _preferences.putBytes("lever2", &_lever2Settings, sizeof(LeverSettings));
            SERIAL_PRINTLN("Lever2Settings updated and saved.");
        } else {
            SERIAL_PRINTLN("Invalid data length for Lever2Settings.");
        }
    } else if (pCharacteristic->getUUID().equals(BLEUUID(LEVERPUSH2_SETTINGS_UUID))) {
        if (rxValue.length() == sizeof(LeverPushSettings)) {
            memcpy(&_leverPush2Settings, rxValue.data(), sizeof(LeverPushSettings));
            _preferences.putBytes("leverpush2", &_leverPush2Settings, sizeof(LeverPushSettings));
            SERIAL_PRINTLN("LeverPush2Settings updated and saved.");
        } else {
            SERIAL_PRINTLN("Invalid data length for LeverPush2Settings.");
        }
    } else if (pCharacteristic->getUUID().equals(BLEUUID(TOUCH_SETTINGS_UUID))) {
        if (rxValue.length() == sizeof(TouchSettings)) {
            memcpy(&_touchSettings, rxValue.data(), sizeof(TouchSettings));
            _preferences.putBytes("touch", &_touchSettings, sizeof(TouchSettings));
            SERIAL_PRINTLN("TouchSettings updated and saved.");
        } else {
            SERIAL_PRINTLN("Invalid data length for TouchSettings.");
        }
    } else if (pCharacteristic->getUUID().equals(BLEUUID(SCALE_SETTINGS_UUID))) {
        if (rxValue.length() == sizeof(ScaleSettings)) {
            memcpy(&_scaleSettings, rxValue.data(), sizeof(ScaleSettings));
            _preferences.putBytes("scale", &_scaleSettings, sizeof(ScaleSettings));
            _scaleManager.setScale(_scaleSettings.scaleType);
            _scaleManager.setRootNote(_scaleSettings.rootNote);
            SERIAL_PRINTLN("ScaleSettings updated and saved.");
        } else {
            SERIAL_PRINTLN("Invalid data length for ScaleSettings.");
        }
    } else if (pCharacteristic->getUUID().equals(BLEUUID(MIDI_UUID))) {
        if (!rxValue.empty()) {
            SERIAL_PRINT("Received BLE MIDI CC: ");
            SERIAL_PRINTLN(rxValue.c_str());

            const auto rxString = String(rxValue.c_str());
            const int commaIndex = rxString.indexOf(',');

            if (commaIndex != -1) {
                const int ccNumber = rxString.substring(0, commaIndex).toInt();
                const int ccValue = rxString.substring(commaIndex + 1).toInt();

                if (ccNumber >= 0 && ccNumber <= 127 && ccValue >= 0 && ccValue <= 127) {
                    MIDI.sendControlChange(ccNumber, ccValue, 1);
                }
            } else {
                SERIAL_PRINTLN("Invalid MIDI CC string format. Expected 'CC_NUMBER,VALUE'.");
            }
        }
    }
}
