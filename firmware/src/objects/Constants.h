#ifndef CONSTANTS_H
#define CONSTANTS_H

// Uncomment following line to enable Serial Printing
#define SERIAL_PRINT_ENABLED 1

// Pin definitions for U1
#define SWD1_LEFT_PIN 5   // SWD1 Left button pin
#define SWD1_CENTER_PIN 6 // SWD1 Center button pin

// Pin definitions for U2
#define SWD1_RIGHT_PIN 0  // SWD1 Right button pin
#define SWD2_LEFT_PIN 1   // SWD2 Left button pin
#define SWD2_CENTER_PIN 2 // SWD2 Center button pin
#define SWD2_RIGHT_PIN 3  // SWD2 Right button pin

#define SERVICE_UUID             "f22b99e8-81ab-4e46-abff-79a74a1f2ff3"
#define LEVER1_SETTINGS_UUID     "6bae0d4d-a0a4-4bc6-9802-a5d27fb15680"
#define LEVERPUSH1_SETTINGS_UUID "1de84ff3-36c0-4cf6-912b-208600cf94f4"
#define LEVER2_SETTINGS_UUID     "13ffbea4-793f-40f5-82da-ac9eca5f0e09"
#define LEVERPUSH2_SETTINGS_UUID "52629808-3d14-4ae8-a826-40bcec6467d5"
#define TOUCH_SETTINGS_UUID      "5612b54d-8bfe-4217-a079-c9c95ab32c41"
#define SCALE_SETTINGS_UUID      "297bd635-c3e8-4fb4-b5e0-93586da8f14c"
#define MIDI_UUID                "eb58b31b-d963-4c7d-9a11-e8aabec2fe32"
#define KEEPALIVE_UUID           "a8f3d5e2-9c4b-11ef-8e7a-325096b39f47"

// Keep-alive timing constants (in milliseconds)
#define KEEPALIVE_GRACE_PERIOD_MS 600000  // 10 minutes

#endif
