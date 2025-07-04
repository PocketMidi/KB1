#ifndef SECURITY_CALLBACKS_H
#define SECURITY_CALLBACKS_H

#include <BLESecurity.h>
#include <objects/Globals.h>

class SecurityCallbacks : public BLESecurityCallbacks {
    void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) override;
    bool onConfirmPIN(uint32_t pin) override;
    uint32_t onPassKeyRequest() override;
    void onPassKeyNotify(uint32_t pass_key) override;
    bool onSecurityRequest() override;

    static void onSSPRequest();
};

#endif