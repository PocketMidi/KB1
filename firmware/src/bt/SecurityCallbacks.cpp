#include <bt/SecurityCallbacks.h>
#include "objects/Globals.h"

void SecurityCallbacks::onAuthenticationComplete(const esp_ble_auth_cmpl_t cmpl) {
    if (cmpl.success) {
        SERIAL_PRINTLN("Authentication Complete - Success!");
    } else {
        SERIAL_PRINT("Authentication Complete - Failed, reason: ");
        SERIAL_PRINTLN(cmpl.fail_reason);
    }
}
bool SecurityCallbacks::onConfirmPIN(uint32_t pin) {
    return true;
}
uint32_t SecurityCallbacks::onPassKeyRequest() {
    SERIAL_PRINTLN("PassKey Request received, returning 0 (Just Works).");
    return 0;
}
void SecurityCallbacks::onPassKeyNotify(const uint32_t pass_key) {
    SERIAL_PRINT("The passkey is: ");
    SERIAL_PRINTLN(pass_key);
}
bool SecurityCallbacks::onSecurityRequest() {
    SERIAL_PRINTLN("Security request received.");
    return true;
}
void SecurityCallbacks::onSSPRequest() {
}