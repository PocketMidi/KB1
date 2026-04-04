#!/bin/bash
#
# KB1 Firmware Flash Script - Preserves Battery Calibration & Settings
# 
# This script:
# 1. Backs up NVS (Non-Volatile Storage) partition containing battery calibration
# 2. Flashes new firmware binary
# 3. Restores NVS partition so settings survive the update
#
# Usage: ./flash_preserve_settings.sh [firmware.bin] [port]
#   firmware.bin - Path to firmware binary (default: KB1-firmware-v1.5.0.bin)
#   port         - Serial port (default: auto-detect)
#

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
FIRMWARE_BIN="${1:-KB1-firmware-v1.5.0.bin}"
PORT="${2:-auto}"
NVS_OFFSET="0x9000"
NVS_SIZE="0x5000"  # 20KB
NVS_BACKUP="nvs_backup_$(date +%Y%m%d_%H%M%S).bin"
ESPTOOL="python3 ~/.platformio/packages/tool-esptoolpy/esptool.py"

echo -e "${BLUE}╔══════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  KB1 Firmware Flash - Preserve Battery & Settings       ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════╝${NC}"
echo ""

# Check firmware binary exists
if [ ! -f "$FIRMWARE_BIN" ]; then
    echo -e "${RED}✗ Error: Firmware file not found: $FIRMWARE_BIN${NC}"
    echo "  Usage: $0 [firmware.bin] [port]"
    exit 1
fi

# Auto-detect port if needed
if [ "$PORT" == "auto" ]; then
    echo -e "${YELLOW}→ Auto-detecting serial port...${NC}"
    PORT=$(ls /dev/cu.usbmodem* 2>/dev/null | head -1)
    if [ -z "$PORT" ]; then
        PORT=$(ls /dev/tty.usbserial* 2>/dev/null | head -1)
    fi
    if [ -z "$PORT" ]; then
        echo -e "${RED}✗ No ESP32 device found${NC}"
        echo "  Please connect your KB1 device via USB"
        exit 1
    fi
    echo -e "${GREEN}✓ Found device: $PORT${NC}"
else
    echo -e "${GREEN}✓ Using port: $PORT${NC}"
fi

echo ""
echo -e "${BLUE}Firmware: ${NC}$FIRMWARE_BIN"
echo -e "${BLUE}Port:     ${NC}$PORT"
echo ""

# Step 1: Backup NVS
echo -e "${YELLOW}[1/3] Backing up NVS partition (battery calibration & settings)...${NC}"
$ESPTOOL --chip esp32s3 --port "$PORT" read_flash $NVS_OFFSET $NVS_SIZE "$NVS_BACKUP"

if [ -f "$NVS_BACKUP" ]; then
    echo -e "${GREEN}✓ NVS backed up to: $NVS_BACKUP${NC}"
else
    echo -e "${RED}✗ NVS backup failed${NC}"
    exit 1
fi

echo ""

# Step 2: Flash firmware
echo -e "${YELLOW}[2/3] Flashing firmware...${NC}"
echo -e "${BLUE}  This will install the new firmware while preserving your settings${NC}"
$ESPTOOL --chip esp32s3 --port "$PORT" --baud 921600 write_flash --flash_mode dio --flash_freq 80m 0x0 "$FIRMWARE_BIN"

echo -e "${GREEN}✓ Firmware flashed successfully${NC}"
echo ""

# Step 3: Restore NVS
echo -e "${YELLOW}[3/3] Restoring NVS partition (battery calibration & settings)...${NC}"
$ESPTOOL --chip esp32s3 --port "$PORT" write_flash $NVS_OFFSET "$NVS_BACKUP"

echo -e "${GREEN}✓ NVS restored - your battery calibration is preserved!${NC}"
echo ""

# Cleanup
echo -e "${YELLOW}→ Cleaning up backup file...${NC}"
rm "$NVS_BACKUP"
echo -e "${GREEN}✓ Backup file removed${NC}"

echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✓ Flash Complete - Settings Preserved!                 ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${BLUE}Your KB1 is ready to use with:${NC}"
echo -e "  • Updated firmware: $(basename $FIRMWARE_BIN)"
echo -e "  • Preserved battery calibration"
echo -e "  • All settings intact"
echo ""
echo -e "${YELLOW}Press the RESET button on your KB1 to start using it.${NC}"
