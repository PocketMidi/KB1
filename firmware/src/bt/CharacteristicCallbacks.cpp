#include <bt/CharacteristicCallbacks.h>
#include <bt/BluetoothController.h>
#include <objects/Globals.h>
#include <music/ScaleManager.h>
#include <music/StrumPatterns.h>
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

    // Debug: Log lever settings updates to see stepSize
    if (_prefKey == "lever1" || _prefKey == "lever2") {
        LeverSettings* s = static_cast<LeverSettings*>(_dest);
        SERIAL_PRINT("✓ "); SERIAL_PRINT(_prefKey.c_str()); 
        SERIAL_PRINT(" settings updated - CC:"); SERIAL_PRINT(s->ccNumber);
        SERIAL_PRINT(", stepSize:"); SERIAL_PRINT(s->stepSize);
        SERIAL_PRINT(", mode:"); SERIAL_PRINTLN((int)s->functionMode);
    }

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
        const auto rxString = String(rxValue.c_str());
        const int commaIndex = rxString.indexOf(',');

        if (commaIndex != -1) {
            const int ccNumber = rxString.substring(0, commaIndex).toInt();
            const int ccValue = rxString.substring(commaIndex + 1).toInt();

            // Throttle serial output: only print every 10th message
            static uint8_t printCounter = 0;
            if (printCounter == 0) {
                SERIAL_PRINT("CC");
                SERIAL_PRINT(ccNumber);
                SERIAL_PRINT("=");
                SERIAL_PRINTLN(ccValue);
            }
            printCounter = (printCounter + 1) % 10;

            if (ccNumber >= 0 && ccNumber <= 127 && ccValue >= 0 && ccValue <= 127) {
                MIDI.sendControlChange(ccNumber, ccValue, 1);
            }
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

StrumIntervalsCallback::StrumIntervalsCallback(BluetoothController* controller, Preferences& preferences)
    : _controller(controller), _preferences(preferences)
{
}

void StrumIntervalsCallback::onWrite(BLECharacteristic *pCharacteristic) {
    const std::string rxValue = pCharacteristic->getValue();

    if (_controller) {
        _controller->updateLastActivity();
    }

    // Expect data format: length byte + interval bytes (int8_t array)
    // Max 16 intervals, so max 17 bytes total
    if (rxValue.length() < 1 || rxValue.length() > 17) {
        SERIAL_PRINTLN("Invalid strum intervals data length");
        return;
    }

    uint8_t length = static_cast<uint8_t>(rxValue[0]);
    if (length > MAX_PATTERN_LENGTH) {
        length = MAX_PATTERN_LENGTH;
    }

    // Extract intervals from remaining bytes
    int8_t intervals[MAX_PATTERN_LENGTH];
    for (uint8_t i = 0; i < length && i + 1 < rxValue.length(); i++) {
        intervals[i] = static_cast<int8_t>(rxValue[i + 1]);
    }

    // Set custom pattern
    setCustomPattern(intervals, length);

    // Persist to preferences
    _preferences.putBytes("customStrum", &customPattern, sizeof(CustomPattern));

    SERIAL_PRINT("Custom strum pattern updated: ");
    SERIAL_PRINT(length);
    SERIAL_PRINTLN(" intervals");
}
