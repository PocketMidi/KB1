#ifndef SERVER_CALLBACKS_H
#define SERVER_CALLBACKS_H

#include <BLEServer.h>

class BluetoothController;

class ServerCallbacks final : public BLEServerCallbacks {
public:
    explicit ServerCallbacks(BluetoothController* controller);
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;

private:
    BluetoothController* _controller;
};

#endif