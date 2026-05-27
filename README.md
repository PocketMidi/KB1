# KB1 - Pocket MIDI Keyboard Controller

KB1 is a compact MIDI keyboard controller built specifically for the Polyend Tracker Mini. It is configured via a browser-based web app connected via Bluetooth Low Energy.

![KB1 banner](assets/banner_1.jpg)

## System Overview

**Hardware**
- 19-key keyboard
- 2 analog levers with integrated push buttons
- 1 capacitive touch input
- 2× 8Ω 1W speakers
- XIAO ESP32-S3 MCU with BLE and USB-C
- 420mAh Li-ion battery, charged via USB-C 

**Firmware** ([details](firmware/README.md))
- Lever and Press and Touch controls: configurable CC output with interpolation curves
- Scale mode: note output quantized to a selectable musical scale
- Chord mode: 10 chord types with strum, voicing (1–3 octave expansion), and swing
- Arp Mode: User defined arpeggiator
- 12 performance sliders: bipolar/unipolar, momentary/latched
- 8 on-device preset slots
- Serial MIDI output

**Web Configuration App** ([details](https://github.com/PocketMidi/KB1-config)) [![Traffic](https://img.shields.io/badge/analytics-umami-blue)](https://cloud.umami.is/analytics/us/share/X00Oso9T1qydknsS)
- Mobile first design
- Runs in Chromium based browsers (**iOS**: Safari does not support Web Bluetooth. [V Browser ](https://vbrowser.co) recommended for iOS)
- Connects over Web Bluetooth
- Settings load from and save to device over BLE
- 12-slider live performance interface (landscape fullscreen on mobile)
- Preset management stored in browser

## Quick Start

### 1. Flash Firmware

1. **Connect KB1** to your computer via USB-C
2. **Open the flash tool** in browser: [https://pocketmidi.github.io/KB1-flash/](https://pocketmidi.github.io/KB1-flash/)
3. **Click CONNECT** and select your KB1 from the serial port dialog

   ![KB1 Flash Tool — Connected](assets/installation/connected.png)

4. **Select a firmware version** from the list (latest is pre-selected) and click **Flash Selected Version**
   - NVS settings are backed up before flashing and restored after
   - You can also drag and drop a local `.bin` file

   ![KB1 Flash Tool — Flashing in progress](assets/installation/progress.png)

5. **Wait for all four steps to complete**: Connect → Backup NVS → Flash → Restore NVS

   ![KB1 Flash Tool — Update complete](assets/installation/complete.png)

The flash tool also includes:
- **Device Info** — firmware version, BLE name, battery status, and NVS values
- **Serial Monitor** — live serial output from the device

### 2. Enable Bluetooth

Bluetooth is off by default. To toggle it:

1. Squeeze both levers toward each other (left lever right, right lever left) and hold for 3 seconds
2. LED feedback during hold:
   - Octave arrow LEDs turn on immediately
   - Pink + blue LEDs pulse with increasing speed
   - All LEDs turn off = toggle complete, release levers

The gesture is cancelled if any key is pressed during the hold.

### 3. Configure

Open [https://pocketmidi.github.io/KB1-config](https://pocketmidi.github.io/KB1-config)

1. Click **KB1 CONFIGURATOR** (logo) and pair with "KB1"
2. Settings load automatically from the device
3. Edit settings and click bouncing yellow arrow to send to KB1

**Sliders tab**: 12 CC controllers (CC 51–62). On mobile, rotate to landscape for fullscreen mode.


## Documentation

- [Configuration App Guide](https://github.com/PocketMidi/KB1-config)
- [Firmware Documentation](firmware/README.md)
- [Hardware Design](hardware/)

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

## Gallery

![inner top](assets/inner_1.png)
![inner bottom](assets/inner_2.png)

## License

- **Software & Firmware**: MIT License (see LICENSE)
- **Hardware Designs**: CERN Open Hardware Licence v2 – Strongly Reciprocal (see hardware/LICENSE-CERN-OHL-S.txt)

