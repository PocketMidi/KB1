---
description: Quick reference for common KB1 development tasks and workflows
---

# KB1 Quick Reference & Common Tasks

Fast lookup guide for frequent KB1 development operations.

## Quick Commands

### Firmware (PlatformIO)

```bash
# Build complete image for distribution
./build_complete.sh

# Development build (fast iteration)
pio run -e seeed_xiao_esp32s3

# Upload to device
pio run -t upload -e seeed_xiao_esp32s3

# Serial monitor
pio device monitor

# Clean build
pio run -t clean -e seeed_xiao_esp32s3

# Full clean (re-download dependencies)
pio run -t fullclean -e seeed_xiao_esp32s3

# Check binary size
pio run -t size -e seeed_xiao_esp32s3
```

### Web App (npm)

```bash
# Development server
npm run dev

# Build for production
npm run build

# Preview production build
npm run preview

# Lint code
npm run lint

# Type check
npm run type-check

# Install dependencies
npm install
```

## Common File Locations

### Firmware

```
firmware/src/objects/Constants.h    # Version, UUIDs, config macros
firmware/src/main.cpp               # Main loop, init, sleep logic
firmware/src/bt/BluetoothController.cpp  # BLE stack
firmware/build_complete.sh          # Release build script
firmware/platformio.ini             # Build configuration
firmware/RELEASE_NOTES_v*.md        # Version history
```

### Web App

```
KB1-config/src/ble/kb1Protocol.ts          # BLE protocol logic
KB1-config/src/ble/bleClient.ts            # BLE connection
KB1-config/src/styles/themes/kb1.css       # Theme variables
KB1-config/src/assets/ui/                  # Visual assets
KB1-config/src/composables/useDeviceState.ts  # Global state
KB1-config/package.json                    # App version
```

### Documentation

```
CONTRIBUTING.md                           # Project conventions
firmware/BUILD_INSTRUCTIONS.md            # Build guide
KB1-config/DESIGNER_WORKFLOW.md          # Theme workflow
KB1-config/DEVICE_PRESET_IMPLEMENTATION.md  # Preset system
KB1-config/FIRMWARE_PRESET_UUIDS.md      # BLE UUIDs
```

## Version Bump Workflow

**Scenario:** Bumping from v1.2.5 → v1.2.6

1. Update firmware version:
```cpp
// firmware/src/objects/Constants.h
#define FIRMWARE_VERSION_MAJOR 1
#define FIRMWARE_VERSION_MINOR 2
#define FIRMWARE_VERSION_PATCH 6
```

2. Update build script:
```bash
# firmware/build_complete.sh (line ~10)
FIRMWARE_VERSION="v1.2.6"
```

3. Update README:
```markdown
# firmware/README.md (line ~5)
**Latest Release:** v1.2.6
```

4. Update web app:
```json
// KB1-config/package.json
{ "version": "1.2.6" }
```

5. Create release notes:
```bash
touch firmware/RELEASE_NOTES_v1.2.6.md
```

6. Build and test:
```bash
cd firmware && ./build_complete.sh
# Flash to device and verify!
```

## Adding a New BLE Characteristic

**Scenario:** Adding a new setting that needs BLE communication

### 1. Define UUID (Firmware)

```cpp
// firmware/src/objects/Constants.h
#define NEW_SETTING_UUID "d3a7b321-0001-4000-8000-000000000099"
```

### 2. Add Characteristic (Firmware)

```cpp
// firmware/src/bt/BluetoothController.cpp
pCharacteristic = pService->createCharacteristic(
  NEW_SETTING_UUID,
  BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
);
pCharacteristic->setCallbacks(new CharacteristicCallbacks());
```

### 3. Handle Read/Write (Firmware)

```cpp
// firmware/src/bt/CharacteristicCallbacks.cpp
void onWrite(BLECharacteristic* pCharacteristic) {
  if (strcmp(pCharacteristic->getUUID().toString().c_str(), NEW_SETTING_UUID) == 0) {
    std::string value = pCharacteristic->getValue();
    uint8_t newValue = static_cast<uint8_t>(value[0]);
    // Update setting...
    globalSettings.newSetting = newValue;
  }
}
```

### 4. Add to Protocol (Web App)

```typescript
// KB1-config/src/ble/kb1Protocol.ts
export const NEW_SETTING_UUID = 'd3a7b321-0001-4000-8000-000000000099';

export interface DeviceSettings {
  // ... existing settings
  newSetting: number;
}

export const DEFAULT_SETTINGS: DeviceSettings = {
  // ... existing defaults
  newSetting: 50,  // default value
};

export function validateNewSetting(value: number): boolean {
  return value >= 0 && value <= 100;
}
```

### 5. Add to BLE Client (Web App)

```typescript
// KB1-config/src/ble/bleClient.ts
private newSettingCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;

// In connect():
this.newSettingCharacteristic = await service.getCharacteristic(NEW_SETTING_UUID);

// Read method:
async readNewSetting(): Promise<number> {
  if (!this.newSettingCharacteristic) throw new Error('Not connected');
  const dataView = await this.newSettingCharacteristic.readValue();
  return dataView.getUint8(0);
}

// Write method:
async writeNewSetting(value: number): Promise<void> {
  if (!this.newSettingCharacteristic) throw new Error('Not connected');
  const data = new Uint8Array([value]);
  await this.newSettingCharacteristic.writeValue(data);
}
```

### 6. Add UI Component (Web App)

```vue
<!-- KB1-config/src/components/NewSettingControl.vue -->
<template>
  <div class="setting-control">
    <label>New Setting</label>
    <input 
      type="range" 
      v-model.number="value"
      min="0" 
      max="100"
      @change="handleChange"
    />
    <span>{{ value }}</span>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue';
import { useDeviceState } from '@/composables/useDeviceState';

const value = ref(50);
const { sendNewSetting } = useDeviceState();

async function handleChange() {
  await sendNewSetting(value.value);
}
</script>
```

## Adding a New Component

**Scenario:** Creating a new settings panel component

### 1. Create Component File

```bash
touch KB1-config/src/components/NewSettings.vue
```

### 2. Component Template

```vue
<template>
  <div class="new-settings">
    <h2>New Settings</h2>
    <div class="settings-grid">
      <div class="setting-item">
        <label>Setting 1</label>
        <input v-model="settings.setting1" type="number" />
      </div>
      <!-- More settings... -->
    </div>
    
    <div class="actions">
      <button @click="handleApply" class="btn-primary">Apply</button>
      <button @click="handleReset" class="btn-secondary">Reset</button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue';
import { useDeviceState } from '@/composables/useDeviceState';

interface Settings {
  setting1: number;
  setting2: number;
}

const settings = ref<Settings>({
  setting1: 0,
  setting2: 0,
});

const { sendSettings } = useDeviceState();

async function handleApply() {
  await sendSettings(settings.value);
}

function handleReset() {
  settings.value = {
    setting1: 0,
    setting2: 0,
  };
}
</script>

<style scoped>
.new-settings {
  background: var(--kb1-bg-secondary);
  padding: var(--kb1-spacing-lg);
  border-radius: var(--kb1-border-radius);
}

.settings-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: var(--kb1-spacing-md);
}

.btn-primary {
  background: var(--btn-primary-bg);
  color: var(--btn-primary-color);
}
</style>
```

### 3. Add to Router (if needed)

```typescript
// KB1-config/src/router/index.ts
import NewSettings from '@/components/NewSettings.vue';

const routes = [
  // ... existing routes
  {
    path: '/new-settings',
    component: NewSettings,
  },
];
```

## Debugging Workflows

### Firmware Serial Debugging

```cpp
// Enable in Constants.h
#define SERIAL_PRINT_ENABLED true

// Use macros in code
SERIAL_PRINT("Value: ");
SERIAL_PRINTLN(myValue);
SERIAL_PRINTF("CC: %d, Value: %d\n", ccNumber, ccValue);
```

```bash
# Monitor output
pio device monitor

# Or with baud rate
pio device monitor -b 115200
```

### Web App Console Debugging

```typescript
// BLE debugging
console.log('Connected:', device.name);
console.log('Characteristic:', uuid);
console.log('Data:', Array.from(new Uint8Array(dataView.buffer)));

// State debugging
console.log('Settings:', JSON.stringify(settings, null, 2));

// Error handling
try {
  await bleClient.connect();
} catch (error) {
  console.error('Connection failed:', error);
}
```

### Browser DevTools

1. **Network Tab:** Monitor BLE traffic (limited visibility)
2. **Console Tab:** View logs and errors
3. **Application Tab:** Inspect localStorage presets
4. **Elements Tab:** Inspect computed CSS variables

## Testing Workflows

### Firmware Testing

```bash
# Build and flash
./build_complete.sh
pio run -t upload -e seeed_xiao_esp32s3

# Monitor serial output
pio device monitor

# Test checklist:
# - Device boots successfully
# - BLE advertises
# - Settings read/write work
# - Sleep timers behave correctly
# - Levers/buttons respond
# - MIDI output correct
```

### Web App Testing

```bash
# Run dev server
npm run dev

# Open in Chrome/Edge (Web Bluetooth required)
# Test checklist:
# - BLE connection works
# - Settings load from device
# - Settings save to device
# - UI updates correctly
# - Presets save/load
# - Responsive at different sizes
```

### Integration Testing

1. Build latest firmware
2. Flash to device
3. Build latest web app
4. Test all BLE characteristics
5. Verify settings persist
6. Test preset system
7. Test sleep behavior
8. Test keepalive mechanism

## Git Workflow

### Feature Branch

```bash
# Create feature branch
git checkout -b feature/new-setting

# Make changes...
git add .
git commit -m "Add new setting for XYZ"

# Push to remote
git push origin feature/new-setting

# Create pull request on GitHub
```

### Release Workflow

```bash
# Tag release
git tag -a v1.2.6 -m "Release v1.2.6 - New features"
git push origin v1.2.6

# GitHub Actions (if configured) or manual:
# - Build firmware
# - Build web app
# - Create GitHub release
# - Attach binaries
```

## Troubleshooting Quick Fixes

### Can't upload to ESP32-S3

```bash
# Hold BOOT button while connecting USB
# Or reset PlatformIO USB detection
pio device list

# Check for driver issues (macOS)
ls /dev/cu.*
```

### BLE connection fails

1. Check Bluetooth enabled on hardware (cross-lever gesture)
2. Verify Chrome/Edge browser (Web Bluetooth support)
3. Check browser permissions (allow Bluetooth)
4. Try disconnecting and reconnecting

### Build fails

```bash
# Clean and rebuild
pio run -t fullclean -e seeed_xiao_esp32s3
pio run -e seeed_xiao_esp32s3
```

### Settings don't persist

1. Check `settings.save()` called in firmware
2. Verify NVS (non-volatile storage) not corrupted
3. Try erasing flash and reflashing

---

**Pro tip:** Keep this file bookmarked for quick reference during development!
