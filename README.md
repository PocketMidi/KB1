# KB1 - Pocket MIDI Keyboard Controller

A pocket-sized, feature-rich MIDI keyboard controller designed for the [Polyend Tracker Mini](https://polyend.com/tracker-mini/). This first release delivers professional-grade performance controls, wireless configuration, and extensive musical capabilities in an ultra-portable package.

![KB1 banner](assets/banner_1.jpg)

## System Overview

The KB1 system consists of three integrated components working together to provide a complete MIDI performance solution:

**Hardware**
- 19-key velocity-sensitive keyboard with dual operating modes (Scale/Chord)
- 2 analog levers with push buttons for expressive control
- Capacitive touch sensor for additional expression
- Built-in mini speakers for audio monitoring
- ESP32-based with Bluetooth Low Energy connectivity
- Rechargeable battery with intelligent power management

**Firmware** ([details](firmware/README.md))
- Dual keyboard modes: Scale (quantized to musical scales) and Chord (10 chord types with chord/strum options)
- Fully configurable controls with advanced interpolation curves and function modes
- 12 real-time performance sliders (CC 51-62) with bipolar/unipolar and momentary/latched modes
- 8 on-device preset slots for storing complete configurations
- Wireless BLE configuration and standard MIDI output

**Web Configuration App** ([details](https://github.com/PocketMidi/KB1-config))
- Browser-based configuration tool (no installation required)
- Real-time parameter editing over Bluetooth Low Energy
- 12-slider performance interface with mobile live mode
- Preset management with unlimited browser-stored configurations
- Works on desktop and mobile (Chrome, Edge, Opera)

## Quick Start

### 1. Flash Firmware to Your Device

**Download the firmware:** Get the latest firmware from [Releases](https://github.com/PocketMidi/KB1/releases) (e.g., `KB1-firmware-v1.1.1.bin`)

**Flash using ESPConnect (Recommended - No Tools Required):**

1. **Connect KB1** to your computer via USB-C
2. **Open ESPConnect** in Chrome/Edge: [https://thelastoutpostworkshop.github.io/ESPConnect/](https://thelastoutpostworkshop.github.io/ESPConnect/)
3. **Click CONNECT** and select your KB1 device from the serial port dialog

4. **Backup Your Flash (Important!):**
   - Click **Flash Tools** in the left sidebar
   - Click **DOWNLOAD FLASH BACKUP**
   - Save the backup file somewhere safe
   
   ![Flash Tools and Backup](assets/installation/espconnect-step-1.png)
   ![Download Flash Backup](assets/installation/espconnect-step-2.png)

5. **Load Firmware File:**
   - Click the firmware file upload field
   - Select the `KB1-firmware-v1.1.1.bin` file you downloaded
   - Flash offset should remain at `0x0`
   
   ![Load Firmware](assets/installation/espconnect-step-3.png)

6. **Flash the Firmware:**
   - Check the box for **"Erase entire flash before writing"**
   - Click the **FLASH FIRMWARE** button
   - Wait for the flashing process to complete (~30-60 seconds)
   
   ![Flash Firmware](assets/installation/espconnect-step-4.png)
   ![Flash in Progress](assets/installation/espconnect-step-5.png)

7. **Verify (Optional):**
   - Click **Serial Monitor** in the left sidebar
   - Click **START** to view real-time output
   - Press keys on your KB1 to see MIDI note events
   
   ![Serial Monitor](assets/installation/espconnect-step-6.png)

**Video Tutorial:** [ESPConnect guide](https://youtu.be/-nhDKzBxHiI)

### 2. Enable Bluetooth on Your KB1

**IMPORTANT**: Before connecting, ensure Bluetooth is enabled on your device.

**To enable Bluetooth:**
1. Hold both octave buttons (up + down) simultaneously for 3 seconds
2. Watch for LED confirmation:
   - **Fast blinking** (pink + blue LEDs) = Bluetooth enabled ✓
   - **Slow blinking** = Bluetooth disabled

Repeat the same process to toggle Bluetooth on/off at any time.

### 3. Configure Over Bluetooth

**Launch the web app**: [https://pocketmidi.github.io/KB1-config](https://pocketmidi.github.io/KB1-config)

1. Click **DISCONNECTED** (top-right) and pair with "KB1"
2. Settings load automatically from device
3. Configure keyboard modes, controls, scales, and performance sliders
4. Click **Save to Device** to persist changes to flash memory

**Performance Sliders**: Switch to the SLIDERS tab for 12 real-time CC controllers. On mobile, rotate to landscape for fullscreen live mode.

**iOS Users**: Safari does not support Web Bluetooth. Download the [Bluefy app](https://apps.apple.com/app/bluefy-web-ble-browser/id1492822055) from the App Store to access the web configuration app on iOS devices. Note: When using Bluefy, live mode automatically compensates for touch offset, and you may see one slider grayed out (11 remain active) - this is normal behavior for optimal touch accuracy.

## Documentation

- [Configuration App Guide](https://github.com/PocketMidi/KB1-config) - Complete web app documentation with usage examples
- [Firmware Documentation](firmware/README.md) - Technical details, features, and build instructions
- [Hardware Design](hardware/) - Schematics and PCB files

## Building from Source

**Firmware** (PlatformIO):
```bash
cd firmware
pio run --target upload
```

**Web App** (Vite + Vue 3):
```bash
cd KB1-config
npm install
npm run build
```

Detailed build instructions are available in the respective README files.

## Gallery

![inner top](assets/inner_1.png)
![inner bottom](assets/inner_2.png)

## License

- **Software & Firmware**: MIT License (see LICENSE)
- **Hardware Designs**: CERN Open Hardware Licence v2 – Strongly Reciprocal (see hardware/LICENSE-CERN-OHL-S.txt)

---

Built for the Polyend Tracker Mini community

