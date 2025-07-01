#include <bt/ServerCallbacks.h>
#include <BLEDevice.h>

void ServerCallbacks::onConnect(BLEServer* pServer) {
    deviceConnected = true;
    SERIAL_PRINTLN("BLE Client Connected!");
    BLEDevice::getAdvertising()->stop();
}

void ServerCallbacks::onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    SERIAL_PRINTLN("BLE Client Disconnected!");
    BLEDevice::startAdvertising();
}