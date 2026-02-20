#include <bt/CharacteristicCallbacks.h>
#include <bt/BluetoothController.h>
#include <objects/Globals.h>
#include <music/ScaleManager.h>
#include <objects/Settings.h>
#include <cstring>

GenericSettingsCallback::GenericSettingsCallback(
    BluetoothController* controller,
    Preferences& preferences,
    void* dest,
    size_t destSize,
    const char* prefKey,
    ScaleManager* scaleManager)
    : _controller(controller), _preferences(preferences), _dest(dest), _destSize(destSize), _prefKey(prefKey), _scaleManager(scaleManager)
{
}

void GenericSettingsCallback::onWrite(BLECharacteristic *pCharacteristic) {
    const std::string rxValue = pCharacteristic->getValue();

    if (_controller) {
        _controller->updateLastActivity();
    }

    // Basic length check
    if (rxValue.length() != _destSize) {
        SERIAL_PRINT("Invalid data length for ");
        SERIAL_PRINTLN(_prefKey.c_str());
        return;
    }

    // Perform copy and persist
    memcpy(_dest, rxValue.data(), _destSize);
    _preferences.putBytes(_prefKey.c_str(), _dest, _destSize);

    // If this was scale settings, update the scale manager
    if (_scaleManager && _prefKey == "scale") {
        ScaleSettings* s = static_cast<ScaleSettings*>(_dest);
        _scaleManager->setScale(s->scaleType);
        _scaleManager->setRootNote(s->rootNote);
    }

    // SERIAL_PRINT(_prefKey.c_str()); SERIAL_PRINTLN(" updated and saved.");
}

MidiSettingsCallback::MidiSettingsCallback(BluetoothController* controller)
    : _controller(controller)
{
}

void MidiSettingsCallback::onWrite(BLECharacteristic *pCharacteristic) {
    const std::string rxValue = pCharacteristic->getValue();
    if (_controller) {
        _controller->updateLastActivity();
    }

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
                SERIAL_PRINT("✓ Sent MIDI CC #");
                SERIAL_PRINT(ccNumber);
                SERIAL_PRINT(" = ");
                SERIAL_PRINTLN(ccValue);
            } else {
                SERIAL_PRINT("✗ Invalid CC range: CC#");
                SERIAL_PRINT(ccNumber);
                SERIAL_PRINT(" Value=");
                SERIAL_PRINTLN(ccValue);
            }
        } else {
            SERIAL_PRINTLN("Invalid MIDI CC string format. Expected 'CC_NUMBER,VALUE'.");
        }
    }
}

KeepAliveCallback::KeepAliveCallback(BluetoothController* controller)
    : _controller(controller)
{
}

void KeepAliveCallback::onWrite(BLECharacteristic *pCharacteristic) {
    if (_controller) {
        // Refresh keep-alive timestamp and ensure it's active
        _controller->refreshKeepAlive();
        SERIAL_PRINTLN("Keep-alive ping received.");
    }
}
