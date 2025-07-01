#ifndef SECURITY_CALLBACKS_H
#define SECURITY_CALLBACKS_H

#include <BLESecurity.h> // Or esp_gap_ble_api.h for esp_ble_auth_cmpl_t
#include <objects/Globals.h> // For SERIAL_PRINT, SERIAL_PRINTLN

class SecurityCallbacks : public BLESecurityCallbacks {
    void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) override;
    bool onConfirmPIN(uint32_t pin) override;
    uint32_t onPassKeyRequest() override;
    void onPassKeyNotify(uint32_t pass_key) override;
    bool onSecurityRequest() override;

    static void onSSPRequest();
};

#endif // SECURITY_CALLBACKS_H