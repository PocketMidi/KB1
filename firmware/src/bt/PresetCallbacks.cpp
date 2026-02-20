#include <bt/PresetCallbacks.h>
#include <bt/BluetoothController.h>
#include <music/ScaleManager.h>
#include <objects/Globals.h>
#include <cstring>

// Helper functions to generate NVS keys
static String getPresetMetaKey(uint8_t slot) {
    return "preset_" + String(slot) + "_meta";
}

static String getPresetDataKey(uint8_t slot) {
    return "preset_" + String(slot) + "_data";
}

//==============================================================================
// PresetSaveCallback
//==============================================================================

PresetSaveCallback::PresetSaveCallback(
    BluetoothController* controller,
    Preferences& preferences,
    LeverSettings& lever1,
    LeverPushSettings& leverPush1,
    LeverSettings& lever2,
    LeverPushSettings& leverPush2,
    TouchSettings& touch,
    ScaleSettings& scale,
    SystemSettings& system
) : _controller(controller), 
    _preferences(preferences),
    _lever1(lever1),
    _leverPush1(leverPush1),
    _lever2(lever2),
    _leverPush2(leverPush2),
    _touch(touch),
    _scale(scale),
    _system(system)
{}

void PresetSaveCallback::onWrite(BLECharacteristic *pCharacteristic) {
    const std::string rxValue = pCharacteristic->getValue();
    
    SERIAL_PRINTLN("=== PRESET SAVE CALLBACK TRIGGERED ===");
    SERIAL_PRINT("Received data length: ");
    SERIAL_PRINTLN(rxValue.length());
    
    if (_controller) {
        _controller->updateLastActivity();
    }
    
    // Expected format: [slot#(1)][name(32)]
    if (rxValue.length() != 33) {
        SERIAL_PRINT("Invalid preset save format - expected 33 bytes, got ");
        SERIAL_PRINTLN(rxValue.length());
        return;
    }
    
    uint8_t slot = rxValue[0];
    
    SERIAL_PRINT("Saving to slot: ");
    SERIAL_PRINTLN(slot);
    
    if (slot >= MAX_PRESET_SLOTS) {
        SERIAL_PRINT("Invalid preset slot: ");
        SERIAL_PRINTLN(slot);
        return;
    }
    
    // Extract name (32 bytes)
    char name[PRESET_NAME_MAX_LEN];
    memcpy(name, rxValue.data() + 1, PRESET_NAME_MAX_LEN);
    
    // Create metadata
    PresetMetadata meta;
    memcpy(meta.name, name, PRESET_NAME_MAX_LEN);
    meta.timestamp = millis() / 1000; // Convert to seconds
    meta.isValid = 1;
    memset(meta.padding, 0, sizeof(meta.padding));
    
    // Bundle current settings
    PresetData data;
    memcpy(&data.lever1, &_lever1, sizeof(LeverSettings));
    memcpy(&data.leverPush1, &_leverPush1, sizeof(LeverPushSettings));
    memcpy(&data.lever2, &_lever2, sizeof(LeverSettings));
    memcpy(&data.leverPush2, &_leverPush2, sizeof(LeverPushSettings));
    memcpy(&data.touch, &_touch, sizeof(TouchSettings));
    memcpy(&data.scale, &_scale, sizeof(ScaleSettings));
    memcpy(&data.system, &_system, sizeof(SystemSettings));
    
    // Save to NVS
    String metaKey = getPresetMetaKey(slot);
    String dataKey = getPresetDataKey(slot);
    
    _preferences.putBytes(metaKey.c_str(), &meta, sizeof(PresetMetadata));
    _preferences.putBytes(dataKey.c_str(), &data, sizeof(PresetData));
    
    SERIAL_PRINT("Preset saved to slot ");
    SERIAL_PRINT(slot);
    SERIAL_PRINT(": ");
    SERIAL_PRINTLN(name);
}

//==============================================================================
// PresetLoadCallback
//==============================================================================

PresetLoadCallback::PresetLoadCallback(
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
) : _controller(controller),
    _preferences(preferences),
    _scaleManager(scaleManager),
    _lever1(lever1),
    _leverPush1(leverPush1),
    _lever2(lever2),
    _leverPush2(leverPush2),
    _touch(touch),
    _scale(scale),
    _system(system)
{}

void PresetLoadCallback::onWrite(BLECharacteristic *pCharacteristic) {
    const std::string rxValue = pCharacteristic->getValue();
    
    if (_controller) {
        _controller->updateLastActivity();
    }
    
    // Expected format: [slot#(1)]
    if (rxValue.length() != 1) {
        SERIAL_PRINTLN("Invalid preset load format");
        return;
    }
    
    uint8_t slot = rxValue[0];
    
    if (slot >= MAX_PRESET_SLOTS) {
        SERIAL_PRINT("Invalid preset slot: ");
        SERIAL_PRINTLN(slot);
        return;
    }
    
    // Check if slot is valid
    String metaKey = getPresetMetaKey(slot);
    PresetMetadata meta;
    size_t metaSize = _preferences.getBytes(metaKey.c_str(), &meta, sizeof(PresetMetadata));
    
    if (metaSize != sizeof(PresetMetadata) || meta.isValid != 1) {
        SERIAL_PRINT("Preset slot ");
        SERIAL_PRINT(slot);
        SERIAL_PRINTLN(" is empty");
        return;
    }
    
    // Load preset data
    String dataKey = getPresetDataKey(slot);
    PresetData data;
    size_t dataSize = _preferences.getBytes(dataKey.c_str(), &data, sizeof(PresetData));
    
    if (dataSize != sizeof(PresetData)) {
        SERIAL_PRINTLN("Failed to load preset data");
        return;
    }
    
    // Apply settings
    memcpy(&_lever1, &data.lever1, sizeof(LeverSettings));
    memcpy(&_leverPush1, &data.leverPush1, sizeof(LeverPushSettings));
    memcpy(&_lever2, &data.lever2, sizeof(LeverSettings));
    memcpy(&_leverPush2, &data.leverPush2, sizeof(LeverPushSettings));
    memcpy(&_touch, &data.touch, sizeof(TouchSettings));
    memcpy(&_scale, &data.scale, sizeof(ScaleSettings));
    memcpy(&_system, &data.system, sizeof(SystemSettings));
    
    // Update scale manager
    _scaleManager.setScale(data.scale.scaleType);
    _scaleManager.setRootNote(data.scale.rootNote);
    
    // Also save to current settings in NVS
    _preferences.putBytes("lever1", &_lever1, sizeof(LeverSettings));
    _preferences.putBytes("leverPush1", &_leverPush1, sizeof(LeverPushSettings));
    _preferences.putBytes("lever2", &_lever2, sizeof(LeverSettings));
    _preferences.putBytes("leverPush2", &_leverPush2, sizeof(LeverPushSettings));
    _preferences.putBytes("touch", &_touch, sizeof(TouchSettings));
    _preferences.putBytes("scale", &_scale, sizeof(ScaleSettings));
    _preferences.putBytes("system", &_system, sizeof(SystemSettings));
    
    SERIAL_PRINT("Preset loaded from slot ");
    SERIAL_PRINT(slot);
    SERIAL_PRINT(": ");
    SERIAL_PRINTLN(meta.name);
}

//==============================================================================
// PresetListCallback
//==============================================================================

PresetListCallback::PresetListCallback(BluetoothController* controller, Preferences& preferences)
    : _controller(controller), _preferences(preferences)
{}

void PresetListCallback::onRead(BLECharacteristic *pCharacteristic) {
    if (_controller) {
        _controller->updateLastActivity();
    }
    
    // Build list of all preset metadata
    // Format: [slot0_meta][slot1_meta]...[slot7_meta]
    uint8_t buffer[MAX_PRESET_SLOTS * sizeof(PresetMetadata)];
    memset(buffer, 0, sizeof(buffer));
    
    for (uint8_t slot = 0; slot < MAX_PRESET_SLOTS; slot++) {
        String metaKey = getPresetMetaKey(slot);
        PresetMetadata meta;
        
        size_t size = _preferences.getBytes(metaKey.c_str(), &meta, sizeof(PresetMetadata));
        
        if (size == sizeof(PresetMetadata) && meta.isValid == 1) {
            memcpy(buffer + (slot * sizeof(PresetMetadata)), &meta, sizeof(PresetMetadata));
        } else {
            // Empty slot - set isValid to 0
            PresetMetadata emptyMeta;
            memset(&emptyMeta, 0, sizeof(PresetMetadata));
            memcpy(buffer + (slot * sizeof(PresetMetadata)), &emptyMeta, sizeof(PresetMetadata));
        }
    }
    
    pCharacteristic->setValue(buffer, sizeof(buffer));
    SERIAL_PRINTLN("Preset list sent");
}

//==============================================================================
// PresetDeleteCallback
//==============================================================================

PresetDeleteCallback::PresetDeleteCallback(BluetoothController* controller, Preferences& preferences)
    : _controller(controller), _preferences(preferences)
{}

void PresetDeleteCallback::onWrite(BLECharacteristic *pCharacteristic) {
    const std::string rxValue = pCharacteristic->getValue();
    
    if (_controller) {
        _controller->updateLastActivity();
    }
    
    // Expected format: [slot#(1)]
    if (rxValue.length() != 1) {
        SERIAL_PRINTLN("Invalid preset delete format");
        return;
    }
    
    uint8_t slot = rxValue[0];
    
    if (slot >= MAX_PRESET_SLOTS) {
        SERIAL_PRINT("Invalid preset slot: ");
        SERIAL_PRINTLN(slot);
        return;
    }
    
    // Remove from NVS
    String metaKey = getPresetMetaKey(slot);
    String dataKey = getPresetDataKey(slot);
    
    _preferences.remove(metaKey.c_str());
    _preferences.remove(dataKey.c_str());
    
    SERIAL_PRINT("Preset deleted from slot ");
    SERIAL_PRINTLN(slot);
}
