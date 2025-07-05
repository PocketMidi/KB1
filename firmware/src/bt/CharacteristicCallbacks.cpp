#include <bt/CharacteristicCallbacks.h>
#include <objects/Globals.h>
#include <music/ScaleManager.h>

extern ScaleManager scaleManager;

void CharacteristicCallbacks::onWrite(BLECharacteristic *pCharacteristic) {
    const std::string rxValue = pCharacteristic->getValue();

    if (pCharacteristic == pSWD1LRCCCharacteristic) {
        if (!rxValue.empty()) {
            // Read the raw byte value sent from the web app
            const int newCC = (int)rxValue[0];
            if (newCC >= 0 && newCC <= 127) { // Validate MIDI CC range
                if (ccNumberSWD1LeftRight != newCC) {
                    ccNumberSWD1LeftRight = newCC;
                    preferences.putInt("ccSWD1LR", ccNumberSWD1LeftRight); // Save to NVS
                    SERIAL_PRINT("SWD1 LR CC set (BLE) and saved: ");
                    SERIAL_PRINTLN(ccNumberSWD1LeftRight);
                }
            }
        }
    } else if (pCharacteristic == pSWD1CenterCCCharacteristic) {
        if (!rxValue.empty()) {
            // Read the raw byte value sent from the web app
            const int newCC = (int)rxValue[0];
            if (newCC >= 0 && newCC <= 127) {
                if (ccNumberSWD1Center != newCC) {
                    ccNumberSWD1Center = newCC;
                    preferences.putInt("ccSWD1Center", ccNumberSWD1Center); // Save to NVS
                    SERIAL_PRINT("SWD1 Center CC set (BLE) and saved: ");
                    SERIAL_PRINTLN(ccNumberSWD1Center);
                }
            }
        }
    } else if (pCharacteristic == pSWD2LRCCCharacteristic) {
        if (!rxValue.empty()) {
            // Read the raw byte value sent from the web app
            const int newCC = (int)rxValue[0];
            if (newCC >= 0 && newCC <= 127) {
                if (ccNumberSWD2LeftRight != newCC) {
                    ccNumberSWD2LeftRight = newCC;
                    preferences.putInt("ccSWD2LR", ccNumberSWD2LeftRight); // Save to NVS
                    SERIAL_PRINT("SWD2 LR CC set (BLE) and saved: ");
                    SERIAL_PRINTLN(ccNumberSWD2LeftRight);
                }
            }
        }
    } else if (pCharacteristic == pSWD2CenterCCCharacteristic) {
        if (!rxValue.empty()) {
            // Read the raw byte value sent from the web app
            const int newCC = (int)rxValue[0];
            if (newCC >= 0 && newCC <= 127) {
                if (ccNumberSWD2Center != newCC) {
                    ccNumberSWD2Center = newCC;
                    preferences.putInt("ccSWD2Center", ccNumberSWD2Center); // Save to NVS
                    SERIAL_PRINT("SWD2 Center CC set (BLE) and saved: ");
                    SERIAL_PRINTLN(ccNumberSWD2Center);
                }
            }
        }
    } else if (pCharacteristic == pMidiCcCharacteristic) { // Handle incoming MIDI CCs from faders
        if (!rxValue.empty()) {
            SERIAL_PRINT("Received BLE MIDI CC: ");
            SERIAL_PRINTLN(rxValue.c_str());

            // Parse the incoming string "CC_NUMBER,VALUE"
            const auto rxString = String(rxValue.c_str());
            const int commaIndex = rxString.indexOf(',');

            if (commaIndex != -1) {
                const int ccNumber = rxString.substring(0, commaIndex).toInt();
                const int ccValue = rxString.substring(commaIndex + 1).toInt();

                // Validate MIDI CC range
                if (ccNumber >= 0 && ccNumber <= 127 && ccValue >= 0 && ccValue <= 127) {
                    // Relay the MIDI CC command via HardwareSerial MIDI directly
                    MIDI.sendControlChange(ccNumber, ccValue, 1); // Channel 1, or make configurable
                    SERIAL_PRINT("Relayed MIDI CC#");
                    SERIAL_PRINT(ccNumber);
                    SERIAL_PRINT(", Value: ");
                    SERIAL_PRINTLN(ccValue);
                } else {
                    SERIAL_PRINTLN("Invalid MIDI CC number or value received.");
                }
            } else {
                SERIAL_PRINTLN("Invalid MIDI CC string format. Expected 'CC_NUMBER,VALUE'.");
            }
        }
    } else if (pCharacteristic == pScaleTypeCharacteristic) {
        if (!rxValue.empty()) {
            const int newScaleType = (int)rxValue[0];
            if (newScaleType >= 0 && newScaleType <= static_cast<int>(ScaleType::BLUES)) { // Validate against enum range
                scaleManager.setScale(static_cast<ScaleType>(newScaleType));
                preferences.putUChar("scaleType", newScaleType); // Save to NVS
                SERIAL_PRINT("Scale Type set (BLE) and saved: ");
                SERIAL_PRINTLN(newScaleType);
            }
        }
    } else if (pCharacteristic == pRootNoteCharacteristic) {
        if (!rxValue.empty()) {
            const int newRootNote = (int)rxValue[0];
            if (newRootNote >= 0 && newRootNote <= 127) { // Validate MIDI note range
                scaleManager.setRootNote(newRootNote);
                preferences.putUChar("rootNote", newRootNote); // Save to NVS
                SERIAL_PRINT("Root Note set (BLE) and saved: ");
                SERIAL_PRINTLN(newRootNote);
            }
        }
    }
}
