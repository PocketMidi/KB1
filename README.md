# ***KB1*** - Pocket MIDI Keyboard Controller

***KB1*** is a compact MIDI keyboard controller built specifically for the Polyend Tracker Mini. It is configured via a browser-based web app connected via Bluetooth Low Energy.

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

## User Guide and suite of tools

### (https://pocketmidi.github.io/KB1-studio/)





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

