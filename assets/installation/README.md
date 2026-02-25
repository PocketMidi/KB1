# Installation Tutorial Images

This directory contains step-by-step screenshots for the ESPConnect firmware installation tutorial.

## Required Images

Place the following 6 images in this directory:

### espconnect-step-1.png
**Flash Tools Selection**
- Shows ESPConnect interface with "Flash Tools" selected in left sidebar
- View of Flash Backup & Erase panel

### espconnect-step-2.png
**Download Flash Backup**
- Shows "DOWNLOAD FLASH BACKUP" button highlighted
- Displays flash download in progress with progress bar and byte count

### espconnect-step-3.png
**Load Firmware File**
- Shows firmware binary file selector with "KB1-firmware-v1.1.1.bin" loaded
- Flash offset field showing "0x0"
- "Recommended offset" dropdown visible

### espconnect-step-4.png
**Erase and Flash**
- Shows "Erase entire flash before writing" checkbox checked
- "FLASH FIRMWARE" button visible and ready to click
- Confirmation dialog: "Confirm Flash - Flash KB1-firmware-v1.1.1.bin at 0x0?"

### espconnect-step-5.png
**Flash in Progress**
- Shows "Flash in progress" modal
- Progress bar with "Flashing KB1-firmware-v1.1.1.bin - X of Y bytes @ 921,600 bps"
- STOP button visible

### espconnect-step-6.png
**Serial Monitor**
- Shows Serial Monitor interface (left sidebar item selected)
- START/STOP/PAUSE/CLEAR/RESET controls visible
- Console output showing MIDI note events:
  - "Note On: XX, Velocity: XX"
  - "Note Off: XX, Velocity: 0"
  - Key press confirmations (e.g., "SW2 (C) Pressed!")

## Image Specifications

- **Format:** PNG (preferred) or JPG
- **Recommended width:** 1200-1600px
- **Keep legible:** Ensure text and UI elements are clearly readable
- **Crop appropriately:** Focus on relevant UI sections, remove unnecessary browser chrome if possible

## Usage

These images are referenced in:
- `/README.md` (Quick Start section)
- `/firmware/RELEASE_NOTES_v1.1.1.md` (Installation section)
