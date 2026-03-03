#include <BLEDevice.h>
#include <bt/ServerCallbacks.h>
#include <bt/BluetoothController.h>
#include "objects/Globals.h"

ServerCallbacks::ServerCallbacks(BluetoothController* controller) : _controller(controller) {}

void ServerCallbacks::onConnect(BLEServer* pServer) {
    _controller->setDeviceConnected(true);
    SERIAL_PRINTLN("BLE Client Connected!");
    _controller->getAdvertising()->stop();
    
    // Double-blink pink LED to confirm connection
    _controller->_ledController.pulse(LedColor::PINK, 200, 600);
}

void ServerCallbacks::onDisconnect(BLEServer* pServer) {
    _controller->setDeviceConnected(false);
    SERIAL_PRINTLN("BLE Client Disconnected!");
    _controller->getAdvertising()->start();
    
    // Single blink blue LED to confirm disconnection
    _controller->_ledController.pulse(LedColor::BLUE, 100, 200);
}