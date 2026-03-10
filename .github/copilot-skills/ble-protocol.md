---
applyTo:
  - "firmware/src/bt/**/*.{cpp,h}"
  - "KB1-config/src/ble/**/*.ts"
  - "**/*protocol*.{ts,cpp,h}"
  - "**/*bluetooth*.{ts,cpp,h}"
---

# KB1 BLE Protocol Expert

Expert knowledge for KB1's Bluetooth Low Energy communication protocol between firmware and web app.

## Protocol Architecture

### BLE Stack Overview

```
Web App (Chrome/Edge)
    ↕ Web Bluetooth API
BLE GATT Protocol
    ↕ ESP32 BLE Stack
Firmware (ESP32-S3)
```

**Key concepts:**
- **GATT Service:** Container for related characteristics (KB1 uses one main service)
- **Characteristic:** Individual data endpoint (read, write, notify)
- **UUID:** 128-bit identifier for services and characteristics

## KB1 Service Structure

### Main Service

```cpp
// Service UUID
#define KB1_SERVICE_UUID "f22b99e8-81ab-4e46-abff-79a74a1f2ff3"
```

### Configuration Characteristics

Each setting has a dedicated characteristic:

```cpp
// Lever 1 Settings
#define LEVER1_CC_UUID          "d3a7b321-0001-4000-8000-000000000001"
#define LEVER1_MIN_UUID         "d3a7b321-0001-4000-8000-000000000002"
#define LEVER1_MAX_UUID         "d3a7b321-0001-4000-8000-000000000003"
// ... (continues for all lever parameters)

// Lever 2 Settings
#define LEVER2_CC_UUID          "d3a7b321-0001-4000-8000-000000000020"
// ...

// Touch Sensor Settings
#define TOUCH_CC_UUID           "d3a7b321-0001-4000-8000-000000000040"
#define TOUCH_THRESHOLD_UUID    "d3a7b321-0001-4000-8000-000000000041"
// ...

// System Settings
#define LIGHT_SLEEP_UUID        "d3a7b321-0001-4000-8000-000000000060"
#define BLE_TIMEOUT_UUID        "d3a7b321-0001-4000-8000-000000000061"
// ...

// Keyboard/Scale Settings
#define SCALE_ROOT_UUID         "d3a7b321-0001-4000-8000-000000000080"
#define SCALE_TYPE_UUID         "d3a7b321-0001-4000-8000-000000000081"
// ...
```

### Preset Management Characteristics

```cpp
// Device Preset Operations (8 slots)
#define PRESET_SAVE_UUID    "d3a7b321-0001-4000-8000-000000000009"
#define PRESET_LOAD_UUID    "d3a7b321-0001-4000-8000-00000000000a"
#define PRESET_LIST_UUID    "d3a7b321-0001-4000-8000-00000000000b"
#define PRESET_DELETE_UUID  "d3a7b321-0001-4000-8000-00000000000c"
```

### Keepalive Characteristic

```cpp
// BLE Connection Keepalive (prevents sleep)
#define KEEPALIVE_UUID      "d3a7b321-0001-4000-8000-000000000070"
```

## Data Formats

### Basic Types

All characteristics use **little-endian** byte order.

```typescript
// TypeScript encoding examples
function encodeUint8(value: number): Uint8Array {
  return new Uint8Array([value]);
}

function encodeUint16(value: number): Uint8Array {
  const buffer = new Uint8Array(2);
  buffer[0] = value & 0xFF;
  buffer[1] = (value >> 8) & 0xFF;
  return buffer;
}

function encodeUint32(value: number): Uint8Array {
  const buffer = new Uint8Array(4);
  buffer[0] = value & 0xFF;
  buffer[1] = (value >> 8) & 0xFF;
  buffer[2] = (value >> 16) & 0xFF;
  buffer[3] = (value >> 24) & 0xFF;
  return buffer;
}
```

```cpp
// C++ decoding examples
uint8_t value8 = data[0];

uint16_t value16 = data[0] | (data[1] << 8);

uint32_t value32 = data[0] | 
                   (data[1] << 8) | 
                   (data[2] << 16) | 
                   (data[3] << 24);
```

### Setting Value Ranges

```typescript
// Lever CC Number (0-127)
type CC_NUMBER = Uint8Array;  // 1 byte

// Lever Min/Max Values (0-127)
type CC_VALUE = Uint8Array;   // 1 byte

// Lever Step Size (0-127)
type STEP_SIZE = Uint8Array;  // 1 byte

// Function Mode (0-3)
// 0 = Unidirectional, 1 = Bidirectional, 2 = Momentary, 3 = Toggle
type FUNCTION_MODE = Uint8Array;  // 1 byte

// Value Mode (0-3)
// 0 = Jump, 1 = Hook, 2 = Pickup, 3 = Latch
type VALUE_MODE = Uint8Array;  // 1 byte

// Interpolation Time (0-5000 milliseconds)
type INTERP_TIME = Uint16Array;  // 2 bytes

// Interpolation Type (0-2)
// 0 = Linear, 1 = S-Curve, 2 = Logarithmic
type INTERP_TYPE = Uint8Array;  // 1 byte

// Touch Threshold (0-65535)
type THRESHOLD = Uint16Array;  // 2 bytes

// Sleep Timeout (180-600 seconds = 3-10 minutes)
type TIMEOUT = Uint16Array;  // 2 bytes

// BLE Timeout (300-1200 seconds = 5-20 minutes)
type BLE_TIMEOUT = Uint16Array;  // 2 bytes
```

## Preset Protocol

### Save Preset Command

**Characteristic:** `PRESET_SAVE_UUID`  
**Operation:** Write

```
Format: [slot#(1 byte)][name(32 bytes)]
Total: 33 bytes

Byte 0:      Slot number (0-7)
Bytes 1-32:  Preset name (UTF-8, null-terminated)
```

**TypeScript encoding:**
```typescript
function encodePresetSave(slot: number, name: string): Uint8Array {
  if (slot < 0 || slot >= 8) {
    throw new Error(`Invalid slot: ${slot}`);
  }
  
  const buffer = new Uint8Array(33);
  buffer[0] = slot;
  
  // Encode name (truncate/pad to 32 bytes)
  const encoder = new TextEncoder();
  const nameBytes = encoder.encode(name.slice(0, 32));
  buffer.set(nameBytes, 1);
  
  return buffer;
}
```

**C++ decoding:**
```cpp
void handlePresetSave(const uint8_t* data, size_t length) {
  if (length < 33) return;
  
  uint8_t slot = data[0];
  if (slot >= 8) return;
  
  // Extract name (ensure null-termination)
  char name[33];
  memcpy(name, &data[1], 32);
  name[32] = '\0';
  
  // Save current settings to slot
  savePreset(slot, name);
}
```

### Load Preset Command

**Characteristic:** `PRESET_LOAD_UUID`  
**Operation:** Write

```
Format: [slot#(1 byte)]
Total: 1 byte

Byte 0: Slot number (0-7)
```

**TypeScript:**
```typescript
function encodePresetLoad(slot: number): Uint8Array {
  if (slot < 0 || slot >= 8) {
    throw new Error(`Invalid slot: ${slot}`);
  }
  return new Uint8Array([slot]);
}
```

**C++:**
```cpp
void handlePresetLoad(const uint8_t* data, size_t length) {
  if (length < 1) return;
  
  uint8_t slot = data[0];
  if (slot >= 8) return;
  
  loadPreset(slot);
}
```

### List Presets Response

**Characteristic:** `PRESET_LIST_UUID`  
**Operation:** Read

```
Format: 8 metadata entries × 40 bytes each = 320 bytes

For each slot (0-7):
  Bytes 0-31:  Name (UTF-8, null-terminated, 32 bytes)
  Bytes 32-35: Timestamp (uint32_t, little-endian, Unix seconds)
  Byte 36:     isValid (1 = has data, 0 = empty)
  Bytes 37-39: Padding (reserved for future use)
```

**TypeScript decoding:**
```typescript
interface DevicePresetMetadata {
  slot: number;
  name: string;
  timestamp: number;
  isValid: boolean;
}

function decodePresetList(dataView: DataView): DevicePresetMetadata[] {
  const METADATA_SIZE = 40;
  const presets: DevicePresetMetadata[] = [];
  
  for (let slot = 0; slot < 8; slot++) {
    const offset = slot * METADATA_SIZE;
    
    // Extract name (32 bytes, null-terminated)
    const nameBytes = new Uint8Array(dataView.buffer, offset, 32);
    const nullIndex = nameBytes.indexOf(0);
    const decoder = new TextDecoder();
    const name = decoder.decode(
      nullIndex >= 0 ? nameBytes.slice(0, nullIndex) : nameBytes
    );
    
    // Extract timestamp (4 bytes, little-endian)
    const timestamp = dataView.getUint32(offset + 32, true);
    
    // Extract isValid (1 byte)
    const isValid = dataView.getUint8(offset + 36) === 1;
    
    presets.push({
      slot,
      name: name || '[Empty]',
      timestamp,
      isValid,
    });
  }
  
  return presets;
}
```

**C++ encoding:**
```cpp
struct PresetMetadata {
  char name[32];
  uint32_t timestamp;
  uint8_t isValid;
  uint8_t padding[3];
} __attribute__((packed));

void handlePresetListRead(uint8_t* buffer, size_t bufferSize) {
  if (bufferSize < 320) return;
  
  PresetMetadata* metadata = (PresetMetadata*)buffer;
  
  for (uint8_t slot = 0; slot < 8; slot++) {
    if (presets[slot].isValid) {
      strncpy(metadata[slot].name, presets[slot].name, 32);
      metadata[slot].timestamp = presets[slot].timestamp;
      metadata[slot].isValid = 1;
    } else {
      strcpy(metadata[slot].name, "[Empty]");
      metadata[slot].timestamp = 0;
      metadata[slot].isValid = 0;
    }
    memset(metadata[slot].padding, 0, 3);
  }
}
```

### Delete Preset Command

**Characteristic:** `PRESET_DELETE_UUID`  
**Operation:** Write

```
Format: [slot#(1 byte)]
Total: 1 byte

Byte 0: Slot number (0-7)
```

**Implementation:** Similar to Load Preset, but marks slot as invalid.

## Keepalive Protocol

### Purpose

Prevents device from entering sleep mode while web app is actively connected.

### Mechanism

**Web app:** Sends periodic ping (every 30-60 seconds)  
**Firmware:** Resets sleep timer on each ping received

### Implementation

**TypeScript (Web App):**
```typescript
class BLEClient {
  private keepaliveInterval: number | null = null;
  
  startKeepalive() {
    // Ping every 30 seconds
    this.keepaliveInterval = setInterval(() => {
      this.sendKeepalive();
    }, 30000);
  }
  
  stopKeepalive() {
    if (this.keepaliveInterval) {
      clearInterval(this.keepaliveInterval);
      this.keepaliveInterval = null;
    }
  }
  
  async sendKeepalive() {
    if (!this.keepaliveCharacteristic) return;
    
    const ping = new Uint8Array([1]);
    await this.keepaliveCharacteristic.writeValue(ping);
    console.log('Keepalive ping sent');
  }
}
```

**C++ (Firmware):**
```cpp
// In CharacteristicCallbacks.cpp
void onWrite(BLECharacteristic* pCharacteristic) {
  if (strcmp(pCharacteristic->getUUID().toString().c_str(), KEEPALIVE_UUID) == 0) {
    // Reset sleep timers
    lastActivityTime = millis();
    SERIAL_PRINTLN("Keepalive received - sleep timer reset");
  }
}
```

### Timing Considerations

**Firmware grace period:** 10 minutes (default, configurable)
- If no keepalive received for 10 minutes, sleep timers proceed normally
- This allows web app to gracefully disconnect without immediate sleep

**Web app ping interval:** 30 seconds
- Ensures multiple pings within grace period
- Tolerates network hiccups

## BLE Connection Lifecycle

### Connection Flow

```
1. User clicks "Connect" in web app
2. Browser shows device picker dialog
3. User selects KB1 device
4. Web Bluetooth API connects to device
5. Discover KB1 service (by UUID)
6. Discover all characteristics
7. Enable notifications on relevant characteristics
8. Start keepalive interval
9. Load settings from device
```

### Disconnection Flow

```
1. Stop keepalive interval
2. Close GATT connection
3. Update UI to "disconnected" state
4. (Firmware) Sleep timers now proceed normally
5. (Firmware) After BLE timeout, device may enter sleep
```

### Error Handling

**Web app should handle:**
- Device not found (Bluetooth disabled on hardware)
- Connection timeout (device out of range)
- GATT errors (characteristic not found)
- Disconnection events (device powered off, out of range)

**Firmware should handle:**
- Invalid data length
- Out-of-range values
- Malformed requests
- Disconnection during write

## Validation & Constraints

### Input Validation (Web App)

Always validate before sending:

```typescript
// Example: Lever CC number
function validateCC(value: number): boolean {
  return value >= 0 && value <= 127;
}

// Example: Sleep timeout
function validateSleepTimeout(seconds: number): boolean {
  return seconds >= 180 && seconds <= 600;  // 3-10 minutes
}

// Example: Threshold
function validateThreshold(value: number): boolean {
  return value >= 0 && value <= 65535;
}
```

### Range Enforcement (Firmware)

Always clamp incoming values:

```cpp
// Example: Clamp CC value
uint8_t clampCC(uint8_t value) {
  if (value > 127) return 127;
  return value;
}

// Example: Clamp sleep timeout
uint16_t clampSleepTimeout(uint16_t seconds) {
  if (seconds < 180) return 180;   // 3 min minimum
  if (seconds > 600) return 600;   // 10 min maximum
  return seconds;
}
```

## Debug Logging

### Web App (TypeScript)

```typescript
console.log('BLE connected to:', device.name);
console.log('Writing CC:', ccNumber, 'to characteristic:', uuid);
console.log('Read value:', dataView.getUint8(0));
console.error('BLE error:', error);
```

### Firmware (C++)

```cpp
SERIAL_PRINT("BLE characteristic written: ");
SERIAL_PRINTLN(pCharacteristic->getUUID().toString().c_str());

SERIAL_PRINT("Preset saved to slot ");
SERIAL_PRINTLN(slot);
```

## Best Practices

1. **Always validate input** on both sides (web app + firmware)
2. **Use little-endian** byte order consistently
3. **Handle disconnections gracefully** (especially during writes)
4. **Test with real devices** (not just simulators)
5. **Document UUID mappings** clearly
6. **Use typed interfaces** for type safety (TypeScript)
7. **Keep characteristic writes atomic** (complete data in one write)
8. **Implement proper error handling** on both ends
9. **Log important events** for debugging
10. **Test edge cases** (min/max values, disconnections, timeouts)

---

**Remember:** BLE has limited bandwidth and latency. Batch updates when possible and avoid rapid successive writes.
