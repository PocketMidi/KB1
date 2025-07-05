#include <BLEDevice.h>
#include <bt/ServerCallbacks.h>
#include <bt/BluetoothController.h>
#include "objects/Globals.h"

ServerCallbacks::ServerCallbacks(BluetoothController* controller) : _controller(controller) {}

void ServerCallbacks::onConnect(BLEServer* pServer) {
    _controller->setDeviceConnected(true);
    SERIAL_PRINTLN("BLE Client Connected!");
    _controller->getAdvertising()->stop();
}

void ServerCallbacks::onDisconnect(BLEServer* pServer) {
    _controller->setDeviceConnected(false);
    SERIAL_PRINTLN("BLE Client Disconnected!");
    _controller->getAdvertising()->start();
}