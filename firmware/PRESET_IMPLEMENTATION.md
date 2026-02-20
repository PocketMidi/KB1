# KB1 Firmware Preset System Implementation Guide

This guide describes the changes needed to add 8 preset slots to the KB1 firmware.

## Overview

- **8 preset slots** stored in ESP32 NVS flash memory
- **4 new BLE characteristics** for preset operations
- **Atomic save/load** - entire preset in one operation
- **Metadata support** - list presets without loading data

## Files to Modify

1. `src/objects/Settings.h` - Add preset data structures
2. `src/bt/BluetoothController.h` - Add preset characteristics
3. `src/bt/BluetoothController.cpp` - Initialize characteristics
4. `src/bt/PresetCallbacks.h` - NEW: Preset characteristic handlers
5. `src/bt/PresetCallbacks.cpp` - NEW: Preset callback implementations

## Implementation Steps

### Step 1: Update Settings.h

Add these structures after existing settings:

```cpp
// Add at the end of Settings.h, before #endif

#define MAX_PRESET_SLOTS 8
#define PRESET_NAME_MAX_LEN 32

/**
 * Preset metadata stored in NVS
 * Allows listing presets without loading full data
 */
struct PresetMetadata {
    char name[PRESET_NAME_MAX_LEN];
    uint32_t timestamp;      // Unix timestamp
    uint8_t isValid;         // 0 = empty slot, 1 = valid preset
    uint8_t padding[3];      // Align to 4-byte boundary
} __attribute__((packed));

/**
 * Complete preset data bundle
 * All device settings in one atomic structure
 */
struct PresetData {
    LeverSettings lever1;
    LeverPushSettings leverPush1;
    LeverSettings lever2;
    LeverPushSettings leverPush2;
    TouchSettings touch;
    ScaleSettings scale;
    SystemSettings system;
} __attribute__((packed));
```

### Step 2: Update BluetoothController.h

Add to private members:

```cpp
// Add after existing characteristics
BLECharacteristic* _pPresetSaveCharacteristic;
BLECharacteristic* _pPresetLoadCharacteristic;
BLECharacteristic* _pPresetListCharacteristic;
BLECharacteristic* _pPresetDeleteCharacteristic;
```

Add public getters:

```cpp
// Add after existing getters
BLECharacteristic* getPresetSaveCharacteristic() { return _pPresetSaveCharacteristic; }
BLECharacteristic* getPresetLoadCharacteristic() { return _pPresetLoadCharacteristic; }
BLECharacteristic* getPresetListCharacteristic() { return _pPresetListCharacteristic; }
BLECharacteristic* getPresetDeleteCharacteristic() { return _pPresetDeleteCharacteristic; }
```

### Step 3: Update BluetoothController.cpp

Add UUIDs (after existing UUIDs in the constructor):

```cpp
#define PRESET_SAVE_CHARACTERISTIC_UUID    "d3a7b321-0001-4000-8000-000000000009"
#define PRESET_LOAD_CHARACTERISTIC_UUID    "d3a7b321-0001-4000-8000-00000000000a"
#define PRESET_LIST_CHARACTERISTIC_UUID    "d3a7b321-0001-4000-8000-00000000000b"
#define PRESET_DELETE_CHARACTERISTIC_UUID  "d3a7b321-0001-4000-8000-00000000000c"
```

Add characteristic initialization (in constructor after existing characteristics):

```cpp
// Preset Save Characteristic (Write)
_pPresetSaveCharacteristic = _pService->createCharacteristic(
    PRESET_SAVE_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE
);
_pPresetSaveCharacteristic->setCallbacks(new PresetSaveCallback(this, _preferences, 
    _lever1Settings, _leverPush1Settings, _lever2Settings, _leverPush2Settings, 
    _touchSettings, _scaleSettings, _systemSettings));

// Preset Load Characteristic (Write)
_pPresetLoadCharacteristic = _pService->createCharacteristic(
    PRESET_LOAD_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE
);
_pPresetLoadCharacteristic->setCallbacks(new PresetLoadCallback(this, _preferences, 
    _scaleManager, _lever1Settings, _leverPush1Settings, _lever2Settings, 
    _leverPush2Settings, _touchSettings, _scaleSettings, _systemSettings));

// Preset List Characteristic (Read)
_pPresetListCharacteristic = _pService->createCharacteristic(
    PRESET_LIST_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ
);
_pPresetListCharacteristic->setCallbacks(new PresetListCallback(this, _preferences));

// Preset Delete Characteristic (Write)
_pPresetDeleteCharacteristic = _pService->createCharacteristic(
    PRESET_DELETE_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE
);
_pPresetDeleteCharacteristic->setCallbacks(new PresetDeleteCallback(this, _preferences));
```

### Step 4: Create PresetCallbacks.h

```cpp
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
```

### Step 5: Create PresetCallbacks.cpp

```cpp
#include <bt/PresetCallbacks.h>
#include <bt/BluetoothController.h>
#include <music/ScaleManager.h>
#include <objects/Globals.h>
#include <cstring>

// Helper function to generate NVS keys
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
    
    if (_controller) {
        _controller->updateLastActivity();
    }
    
    // Expected format: [slot#(1)][name(32)]
    if (rxValue.length() != 33) {
        SERIAL_PRINTLN("Invalid preset save format");
        return;
    }
    
    uint8_t slot = rxValue[0];
    
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
```

### Step 6: Update BluetoothController.cpp includes

Add to the includes at the top:

```cpp
#include <bt/PresetCallbacks.h>
```

## Testing Checklist

- [ ] Firmware compiles without errors
- [ ] Can save a preset to slot 0-7
- [ ] Can load a preset from slot 0-7
- [ ] Can list all presets (shows valid/empty)
- [ ] Can delete a preset
- [ ] Loaded preset applies to hardware correctly
- [ ] Settings survive power cycle

## Next Steps

After firmware is working, update the web app to:
1. Detect and list device presets
2. Add UI for device preset management
3. Add export/import for community sharing

