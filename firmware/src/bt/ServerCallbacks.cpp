#include <BLEDevice.h>
#include <bt/ServerCallbacks.h>
#include <bt/BluetoothController.h>
#include "objects/Globals.h"

ServerCallbacks::ServerCallbacks(BluetoothController* controller) : _controller(controller) {}

void ServerCallbacks::onConnect(BLEServer* pServer) {
    _controller->setDeviceConnected(true);
    SERIAL_PRINTLN("BLE+");
    _controller->getAdvertising()->stop();
    
    // Note: BLE remote address will be extracted during first connection parameter update
    // We cannot easily get it from ESP32 BLE Arduino library at connection time,
   // so we'll use the connection handle when needed in updateConnectionParams()
    
    // Start in CONFIGURATION mode for responsive initial setup
    _controller->setActivityMode(CONFIGURATION);
    
    // Double-blink pink LED to confirm connection
    _controller->_ledController.pulse(LedColor::PINK, 200, 600);
}

void ServerCallbacks::onDisconnect(BLEServer* pServer) {
    // Update battery tracking for time spent in current BLE mode before disconnecting
    _controller->updateBatteryModeTracking();
    
    // Mark as manual disconnect (not eligible for auto-reconnect)
    // Note: If this was a keepalive timeout, main loop will override this
    _controller->setReconnectEligible(false);
    
    _controller->setDeviceConnected(false);
    SERIAL_PRINTLN("BLE-");
    _controller->getAdvertising()->start();
    
    // Single blink blue LED to confirm disconnection
    _controller->_ledController.pulse(LedColor::BLUE, 100, 200);
}