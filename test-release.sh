#!/bin/bash
# KB1 Integration Test Script
# Run this before every release to catch regressions

set -e  # Exit on first error

echo "🧪 KB1 v1.7 Integration Test Suite"
echo "===================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Test counters
TESTS_PASSED=0
TESTS_FAILED=0

# Helper functions
pass() {
    echo -e "${GREEN}✓${NC} $1"
    TESTS_PASSED=$((TESTS_PASSED + 1))
}

fail() {
    echo -e "${RED}✗${NC} $1"
    TESTS_FAILED=$((TESTS_FAILED + 1))
}

warn() {
    echo -e "${YELLOW}⚠${NC} $1"
}

# ===================
# AUTOMATED TESTS
# ===================

echo "📦 Automated Build Tests"
echo "------------------------"

# Test 1: Firmware builds cleanly
echo -n "Building firmware... "
cd firmware
if pio run --environment seeed_xiao_esp32s3 > /dev/null 2>&1; then
    pass "Firmware builds successfully"
else
    fail "Firmware build failed"
    echo "  Run: cd firmware && pio run --environment seeed_xiao_esp32s3"
fi

# Test 2: Web app builds cleanly
echo -n "Building web app... "
cd ../KB1-config
if npm run build > /dev/null 2>&1; then
    pass "Web app builds successfully"
else
    fail "Web app build failed"
    echo "  Run: cd KB1-config && npm run build"
fi

# Test 3: TypeScript type checking
echo -n "Checking TypeScript types... "
if npm run vue-tsc -- --noEmit > /dev/null 2>&1; then
    pass "No TypeScript errors"
else
    fail "TypeScript errors found"
    echo "  Run: cd KB1-config && npm run vue-tsc"
fi

cd ..

echo ""
echo "📋 Manual Test Checklist"
echo "------------------------"
echo ""
echo "The following tests require physical hardware:"
echo ""

# Manual test checklist
MANUAL_TESTS=(
    "Upload firmware to ESP32-S3 device"
    "Connect via USB and verify serial output shows:"
    "  - Firmware version v1.7"
    "  - BLE advertising starts"
    "  - No error messages on boot"
    "Connect via BLE from web app"
    "Send settings update and verify no errors"
    "Test SCALE mode with Chromatic:"
    "  - All 12 keys should be active"
    "  - Root note should display as C"
    "  - Can set any root note without errors"
    "Test SCALE mode with Major scale:"
    "  - Root note must be C-B (60-71)"
    "  - Setting root note outside range shows error"
    "Test CHORD mode with strum:"
    "  - Drag dots adjusts speed smoothly"
    "  - REV/FWD toggle changes direction"
    "  - Speed displays as positive (5-360)"
    "Test lever assigned to CC 200 (Strum Speed):"
    "  - Full left = slow reverse (-360ms)"
    "  - Center = fast (±5ms)"
    "  - Full right = slow forward (+360ms)"
    "  - Lever syncs with current BLE strum speed"
    "Test voicing control:"
    "  - Click lines to set 1, 2, or 3 octaves"
    "  - Lines fill bottom-to-top"
    "  - Chord notes span selected octaves"
    "Verify performance:"
    "  - Dragging dots feels smooth (no lag)"
    "  - No console errors during drag"
    "  - Battery usage is reasonable"
    "Load v1.6 preset and verify compatibility:"
    "  - Preset loads without errors"
    "  - All settings display correctly"
    "  - Can modify and save preset"
)

for i in "${!MANUAL_TESTS[@]}"; do
    echo "  [ ] $((i+1)). ${MANUAL_TESTS[$i]}"
done

echo ""
echo "📊 Test Summary"
echo "==============="
echo -e "Automated Tests: ${GREEN}${TESTS_PASSED} passed${NC}, ${RED}${TESTS_FAILED} failed${NC}"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All automated tests passed!${NC}"
    echo ""
    echo "Next steps:"
    echo "1. Complete manual tests with physical hardware"
    echo "2. If all tests pass, update version in platformio.ini and package.json"
    echo "3. Commit changes with message: 'Release v1.7'"
    echo "4. Tag release: git tag v1.7.0"
    echo "5. Push: git push && git push --tags"
    exit 0
else
    echo -e "${RED}✗ Some automated tests failed${NC}"
    echo "Fix errors before proceeding to manual tests"
    exit 1
fi
