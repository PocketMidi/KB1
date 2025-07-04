#ifndef SERVER_CALLBACKS_H
#define SERVER_CALLBACKS_H

#include <BLEServer.h>
#include <objects/Globals.h>

class ServerCallbacks final : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;
};

#endif