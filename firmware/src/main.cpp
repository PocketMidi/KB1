/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * It is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details: <http://www.gnu.org/licenses/>.
 *
 */

#include <Arduino.h>
#include <Preferences.h>
#include <Adafruit_MCP23X17.h>
#include <MIDI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLESecurity.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_gatts_api.h>
#include <esp_bt_defs.h>
#include <objects/Constants.h>
#include <objects/Globals.h>
#include <bt/ServerCallbacks.h>
#include <bt/SecurityCallbacks.h>
#include <bt/CharacteristicCallbacks.h>
#include <controls/LeverControls.h>
#include <controls/TouchControl.h>
#include "controls/LeverPushControls.h"
#include <led/LEDController.h>

Adafruit_MCP23X17 mcp_U1;
Adafruit_MCP23X17 mcp_U2;

MIDI_CREATE_INSTANCE(HardwareSerial, Serial0, MIDI);

// Lever Objects
LeverControls<decltype(MIDI)> lever1(
    &mcp_U1,
    &mcp_U2,
    SWD1_LEFT_PIN,
    SWD1_RIGHT_PIN,
    3,
    0,
    127,
    1,
    LeverFunctionMode::INTERPOLATED,
    LeverMode::BIPOLAR,
    1000,
    1000,
    InterpolationType::DECELERATING,
    InterpolationType::ACCELERATING,
    MIDI);
LeverControls<decltype(MIDI)> lever2(
    &mcp_U2,
    &mcp_U2,
    SWD2_LEFT_PIN,
    SWD2_RIGHT_PIN,
    7,
    0,
    127,
    4,
    LeverFunctionMode::INCREMENTAL,
    LeverMode::BIPOLAR,
    200,
    200,
    InterpolationType::LINEAR,
    InterpolationType::LINEAR,
    MIDI);

LeverPushControls<decltype(MIDI)> leverPush1(
        &mcp_U1,
        SWD1_CENTER_PIN,
        24,
        0,
        127,
        LeverPushFunctionMode::INTERPOLATED,
        200,
        200,
        InterpolationType::LINEAR,
        InterpolationType::LINEAR,
        MIDI);

LeverPushControls<decltype(MIDI)> leverPush2(
        &mcp_U2,
        SWD2_CENTER_PIN,
        1,
        0,
        127,
        LeverPushFunctionMode::JUMP_AND_INTERPOLATE,
        200,
        200,
        InterpolationType::LINEAR,
        InterpolationType::LINEAR,
        MIDI);

TouchControl<decltype(MIDI)> touch1(
    T1, 
    51,
    0,
    127,
    26000,
    155000, 
    26000,
    TouchFunctionMode::CONTINUOUS,
    MIDI
);


key keys[MAX_KEYS] = {
    {59, 4, true, true, &mcp_U1, "SW1 (B)"},
    {60, 14, true, true, &mcp_U1, "SW2 (C)"},
    {61, 1, true, true, &mcp_U1, "SB1 (C#)"},
    {62, 13, true, true, &mcp_U1, "SW3 (D)"},
    {63, 2, true, true, &mcp_U1, "SB2 (D#)"},
    {64, 12, true, true, &mcp_U1, "SW4 (E)"},
    {65, 11, true, true, &mcp_U1, "SW5 (F)"},
    {66, 0, true, true, &mcp_U1, "SB3 (F#)"},
    {67, 10, true, true, &mcp_U1, "SW6 (G)"},
    {68, 3, true, true, &mcp_U1, "SB4 (G#)"},
    {69, 9, true, true, &mcp_U1, "SW7 (A)"},
    {70, 14, true, true, &mcp_U2, "SB5 (A#)"},
    {71, 8, true, true, &mcp_U1, "SW8 (B)"},
    {72, 11, true, true, &mcp_U2, "SW9 (C)"},
    {73, 13, true, true, &mcp_U2, "SB6 (C#)"},
    {74, 10, true, true, &mcp_U2, "SW10 (D)"},
    {75, 12, true, true, &mcp_U2, "SB7 (D#)"},
    {76, 9, true, true, &mcp_U2, "SW11 (E)"},
    {77, 8, true, true, &mcp_U2, "SW12 (F)"}
};

Preferences preferences; // Define Preferences object
BLEServer* pServer = nullptr;
bool deviceConnected = false;

// NEW: Definitions of new BLE Characteristic pointers
BLECharacteristic* pSWD1LRCCCharacteristic = nullptr;
BLECharacteristic* pSWD1CenterCCCharacteristic = nullptr;
BLECharacteristic* pSWD2LRCCCharacteristic = nullptr;
BLECharacteristic* pSWD2CenterCCCharacteristic = nullptr;
BLECharacteristic* pMidiCcCharacteristic = nullptr;

// Global state variables and their initial values
volatile int currentOctave = 0;
volatile int currentVelocity = 80;
int minVelocity = 8;

int ccNumberSWD1LeftRight = 3;  // Default for SWD1 Left/Right
int ccNumberSWD1Center = 24;    // Default for SWD1 Center (Sustain)
int ccNumberSWD2LeftRight = 7;  // Default for SWD2 Left/Right (e.g., Expression)
int ccNumberSWD2Center = 1;     // Default for SWD2 Center (e.g., Modulation)

// Previous state variables for OCT buttons
volatile bool prevS3State = true;
volatile bool prevS4State = true;

// Octave Timers & Callbacks //
TimerHandle_t upTimer;
TimerHandle_t downTimer;

// MIDI notes
bool isNoteOn[128]; // Initialized in setup()

// LED Controller
LEDController ledController;


// Forward declarations for functions defined later in this file
void playMidiNote(byte note);
void stopMidiNote(byte note);
void shiftOctave(int shift);
void buttonReadTask(void *pvParameters);
void upTimerCallback(TimerHandle_t xTimer);
void downTimerCallback(TimerHandle_t xTimer);
void resetVelocity();
void updateVelocity(int delta);



/////////////////////////////////////////////////////////////////
////////////////////////  SETUP  ////////////////////////////////
/////////////////////////////////////////////////////////////////

void setup() {
    SERIAL_BEGIN();
    SERIAL_PRINTLN("Serial monitor started.");

    // Initialize all note-on flags to false
    memset(isNoteOn, false, sizeof(isNoteOn));

    // initialize digital pin LED_BUILTIN as an output.
    pinMode(LED_BUILTIN, OUTPUT);
    // Keep built-in LED on continuously (active low for some boards)
    digitalWrite(LED_BUILTIN, LOW);

    // Initialize Preferences
    if (!preferences.begin("kb1-settings", false)) {
        SERIAL_PRINTLN("Error initializing Preferences. Rebooting...");
        ESP.restart();
    } else {
        SERIAL_PRINTLN("Preferences initialized successfully.");
    }

    // NEW: Load configurable CC numbers from NVS
    ccNumberSWD1LeftRight = preferences.getInt("ccSWD1LR", 3);
    ccNumberSWD1Center = preferences.getInt("ccSWD1Center", 24);
    ccNumberSWD2LeftRight = preferences.getInt("ccSWD2LR", 7);
    ccNumberSWD2Center = preferences.getInt("ccSWD2Center", 1);

    // Set CC numbers for levers
    lever1.setCCNumber(ccNumberSWD1LeftRight);
    lever2.setCCNumber(ccNumberSWD2LeftRight);

    SERIAL_PRINT("Loaded SWD1 LR CC: "); SERIAL_PRINTLN(ccNumberSWD1LeftRight);
    SERIAL_PRINT("Loaded SWD1 Center CC: "); SERIAL_PRINTLN(ccNumberSWD1Center);
    SERIAL_PRINT("Loaded SWD2 LR CC: "); SERIAL_PRINTLN(ccNumberSWD2LeftRight);
    SERIAL_PRINT("Loaded SWD2 Center CC: "); SERIAL_PRINTLN(ccNumberSWD2Center);

    // Start MCP_U1
    if (!mcp_U1.begin_I2C(0x20)) {
        SERIAL_PRINTLN("Error initializing U1.");
        // ReSharper disable once CppDFAEndlessLoop
        while (true) {}
    }
    // Start MCP_U2
    if (!mcp_U2.begin_I2C(0x21)) {
        SERIAL_PRINTLN("Error initializing U2.");
        // ReSharper disable once CppDFAEndlessLoop
        while (true) {}
    }

    // Add a small delay to allow MCP2 to fully initialize
    delay(100);

    // Configure Keys as input with pull-up
    for (const auto & key : keys) {
        key.mcp->pinMode(key.pin, INPUT_PULLUP);
    }

    //////// U1 //////////////////////////////////////////////////////
    // Configure SWD buttons on U1 as inputs with pull-up
    mcp_U1.pinMode(SWD1_LEFT_PIN, INPUT_PULLUP);
    mcp_U1.pinMode(SWD1_CENTER_PIN, INPUT_PULLUP);

    //////// U2 //////////////////////////////////////////////////////
    // Configure S3 & S4 buttons on U2 as inputs with pull-up
    mcp_U2.pinMode(4, INPUT_PULLUP); // U2 pin 4, S3 button
    mcp_U2.pinMode(6, INPUT_PULLUP); // U2 pin 6, S4 button
    // Configure SWD buttons on U2 as inputs with pull-up
    mcp_U2.pinMode(SWD1_RIGHT_PIN, INPUT_PULLUP);
    mcp_U2.pinMode(SWD2_LEFT_PIN, INPUT_PULLUP);
    mcp_U2.pinMode(SWD2_CENTER_PIN, INPUT_PULLUP);
    mcp_U2.pinMode(SWD2_RIGHT_PIN, INPUT_PULLUP);

    //////// Button Read Task ////////////////////////////////////////////
    xTaskCreatePinnedToCore(buttonReadTask, "ButtonReadTask", 4096, nullptr, 1, nullptr, 1);

    //////// MIDI //////////////////////////////////////////////////////
    MIDI.begin(1); // Initialize MIDI on channel 1
    SERIAL_PRINTLN("MIDI library initialized on channel 1.");

    //////// Octave Button LEDs ////////////////////////////////////////////
    // Configure LEDs on U2 // Set LEDs to initial state (both off)
    mcp_U2.pinMode(5, OUTPUT);
    mcp_U2.pinMode(7, OUTPUT);
    mcp_U2.digitalWrite(5, HIGH); // DN LED (active low)
    mcp_U2.digitalWrite(7, HIGH); // UP LED (active low)

    //////// Lever Indicator LEDs ////////////////////////////////////////////
    upTimer = xTimerCreate("UpTimer", pdMS_TO_TICKS(1500), pdTRUE, (void *)nullptr, upTimerCallback);
    downTimer = xTimerCreate("DownTimer", pdMS_TO_TICKS(1500), pdTRUE, (void *)nullptr, downTimerCallback);
    if (upTimer == nullptr || downTimer == nullptr) {
        SERIAL_PRINTLN("Error creating timers!");
        // ReSharper disable once CppDFAEndlessLoop
        while (true) {}
    }

    ledController.begin(LedColor::BLUE, 7);
    ledController.set(LedColor::BLUE, 0);
    ledController.begin(LedColor::PINK, 8);
    ledController.set(LedColor::PINK, 0);

    // ***** BLE SETUP *****
    BLEDevice::init("KB1");
    BLEDevice::setSecurityCallbacks(new SecurityCallbacks());

    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));

    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);

    auto valSWD1LR = static_cast<uint8_t>(ccNumberSWD1LeftRight);
    pSWD1LRCCCharacteristic = pService->createCharacteristic(
        SWD1_LR_CC_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pSWD1LRCCCharacteristic->addDescriptor(new BLE2902());
    pSWD1LRCCCharacteristic->setCallbacks(new CharacteristicCallbacks());
    pSWD1LRCCCharacteristic->setValue(&valSWD1LR, 1);

    auto valSWD1Center = static_cast<uint8_t>(ccNumberSWD1Center);
    pSWD1CenterCCCharacteristic = pService->createCharacteristic(
        SWD1_CENTER_CC_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pSWD1CenterCCCharacteristic->addDescriptor(new BLE2902());
    pSWD1CenterCCCharacteristic->setCallbacks(new CharacteristicCallbacks());
    pSWD1CenterCCCharacteristic->setValue(&valSWD1Center, 1);

    auto valSWD2LR = static_cast<uint8_t>(ccNumberSWD2LeftRight);
    pSWD2LRCCCharacteristic = pService->createCharacteristic(
        SWD2_LR_CC_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pSWD2LRCCCharacteristic->addDescriptor(new BLE2902());
    pSWD2LRCCCharacteristic->setCallbacks(new CharacteristicCallbacks());
    pSWD2LRCCCharacteristic->setValue(&valSWD2LR, 1);

    auto valSWD2Center = static_cast<uint8_t>(ccNumberSWD2Center);
    pSWD2CenterCCCharacteristic = pService->createCharacteristic(
        SWD2_CENTER_CC_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pSWD2CenterCCCharacteristic->addDescriptor(new BLE2902());
    pSWD2CenterCCCharacteristic->setCallbacks(new CharacteristicCallbacks());
    pSWD2CenterCCCharacteristic->setValue(&valSWD2Center, 1);

    // Create MIDI CC Characteristic (Write Without Response for efficiency)
    pMidiCcCharacteristic = pService->createCharacteristic(
        MIDI_CC_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE_NR // WRITE_NR for "Write Without Response"
    );
    pMidiCcCharacteristic->setCallbacks(new CharacteristicCallbacks());

    // Start the service
    pService->start();

    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    SERIAL_PRINTLN("Waiting for a BLE client connection...");
}

////////////////////////////////////////////////////////////////////////
////////////////////////  LOOP  ////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void loop() {
    ledController.update();

    // Small delay to avoid watchdog timer issues
    vTaskDelay(1 / portTICK_PERIOD_MS);
}

// Helper functions (defined outside loop/setup)
void upTimerCallback(TimerHandle_t xTimer) {
    mcp_U2.digitalWrite(7, !mcp_U2.digitalRead(7));
}

void downTimerCallback(TimerHandle_t xTimer) {
    mcp_U2.digitalWrite(5, !mcp_U2.digitalRead(5));
}

void shiftOctave(const int shift) {
    currentOctave += shift;
    if (currentOctave < -3) {
        SERIAL_PRINTLN("Lowest Octave");
        currentOctave = -3;
    } else if (currentOctave > 3) {
        SERIAL_PRINTLN("Highest Octave");
        currentOctave = 3;
    }

    xTimerStop(upTimer, 0);
    xTimerStop(downTimer, 0);

    if (currentOctave == 0)
    {
        mcp_U2.digitalWrite(5, HIGH);
        mcp_U2.digitalWrite(7, HIGH);
    }
    else if (currentOctave > 0)
    {
        mcp_U2.digitalWrite(5, HIGH);
        mcp_U2.digitalWrite(7, LOW);
        if (currentOctave == 1)
            xTimerChangePeriod(upTimer, pdMS_TO_TICKS(750), 0);
        else if (currentOctave == 2)
            xTimerChangePeriod(upTimer, pdMS_TO_TICKS(375), 0);
        else if (currentOctave == 3)
            xTimerChangePeriod(upTimer, pdMS_TO_TICKS(175), 0);
        xTimerStart(upTimer, 0);
    }
    else // currentOctave < 0
    {
        mcp_U2.digitalWrite(7, HIGH);
        mcp_U2.digitalWrite(5, LOW);
        if (currentOctave == -1)
            xTimerChangePeriod(downTimer, pdMS_TO_TICKS(750), 0);
        else if (currentOctave == -2)
            xTimerChangePeriod(downTimer, pdMS_TO_TICKS(375), 0);
        else if (currentOctave == -3)
            xTimerChangePeriod(downTimer, pdMS_TO_TICKS(175), 0);
        xTimerStart(downTimer, 0);
    }
    delay(80);
}

void playMidiNote(const byte note) {
    constexpr byte channel = 1;
    MIDI.sendNoteOn(note + currentOctave * 12, currentVelocity, channel);
    SERIAL_PRINT("Note On: ");
    SERIAL_PRINT(note + currentOctave * 12);
    SERIAL_PRINT(", Velocity: ");
    SERIAL_PRINTLN(currentVelocity);
    isNoteOn[note] = true;
}

void stopMidiNote(const byte note) {
    if (isNoteOn[note])
    {
        constexpr byte channel = 1;
        MIDI.sendNoteOff(note + currentOctave * 12, 0, channel);
        SERIAL_PRINT("Note Off: ");
        SERIAL_PRINT(note + currentOctave * 12);
        SERIAL_PRINTLN(", Velocity: 0");
        isNoteOn[note] = false;
    }
}

void updateVelocity(const int delta) {
    const int previousVelocity = currentVelocity;
    currentVelocity += delta;
    if (currentVelocity < minVelocity)
        currentVelocity = minVelocity;
    else if (currentVelocity > 127)
        currentVelocity = 127;

    if (delta > 0) {
        ledController.set(LedColor::BLUE, 255, 500); // Fade to full brightness over 500ms
    } else if (delta < 0) {
        ledController.set(LedColor::BLUE, 0, 3000); // Fade to off over 3000ms
    }
    if (currentVelocity != previousVelocity) {
        if (delta > 0) {
            SERIAL_PRINT("Current Velocity Increased to: ");
        } else {
            SERIAL_PRINT("Current Velocity Decreased to: ");
        }
        SERIAL_PRINTLN(currentVelocity);
    }
}

void resetVelocity() {
    currentVelocity = 80;
    SERIAL_PRINTLN("Velocity Reset to 80");
    ledController.set(LedColor::BLUE, 51, 250);
    delay(250);
    ledController.set(LedColor::BLUE, 0, 0);
}

////////////////////////  Button Read  /////////////////////////////////
////////////////////////////////////////////////////////////////////////

[[noreturn]] void buttonReadTask(void *pvParameters) {
    bool isS3Pressed = false;
    bool isS4Pressed = false;
    bool areS3S4Pressed = false;

    while (true)
    {
        // Read key states & button states on U2
        for (auto & key : keys)
        {
            key.state = !key.mcp->digitalRead(key.pin);
        }

        touch1.update();

        lever1.update();
        lever2.update();

        leverPush1.update();
        leverPush2.update();

        //////////////////  OCTAVE Buttons ////////////////////////////////////////
        const bool S3State = !mcp_U2.digitalRead(4);
        const bool S4State = !mcp_U2.digitalRead(6);

        if (S3State && S4State) {
            if (!areS3S4Pressed) {
                shiftOctave(0 - currentOctave);
                SERIAL_PRINTLN("S3 & S4 Pressed Simultaneously! Octave Reset");
                areS3S4Pressed = true;
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
        }
        else
        {
            areS3S4Pressed = false;
            if (!S3State && isS3Pressed)
            {
                shiftOctave(-1);
                SERIAL_PRINTLN("S3 Released! Octave Down by 1");
                isS3Pressed = false;
                vTaskDelay(50 / portTICK_PERIOD_MS);
            }
            else if (S3State)
            {
                isS3Pressed = true;
            }
            if (!S4State && isS4Pressed)
            {
                shiftOctave(1);
                SERIAL_PRINTLN("S4 Released! Octave Up by 1");
                isS4Pressed = false;
                vTaskDelay(50 / portTICK_PERIOD_MS);
            }
            else if (S4State)
            {
                isS4Pressed = true;
            }
        }
        prevS3State = S3State;
        prevS4State = S4State;

        //////////////////  KEYS /////////////////////////////////////////////
        for (auto & key : keys)
        {
            if (key.state != key.prevState)
            {
                if (key.state)
                {
                    playMidiNote(key.midi);
                    SERIAL_PRINT(key.name);
                    SERIAL_PRINTLN(" Pressed!");
                }
                else
                {
                    stopMidiNote(key.midi);
                }
            }
            key.prevState = key.state;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
