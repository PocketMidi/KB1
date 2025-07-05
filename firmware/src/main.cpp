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

//----------------------------------
// Includes
//----------------------------------
#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include <Preferences.h>
#include <MIDI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
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
#include <led/LEDController.h>
#include <controls/LeverControls.h>
#include <controls/TouchControl.h>
#include <controls/LeverPushControls.h>
#include "controls/OctaveControl.h"
#include "controls/KeyboardControl.h"
#include <music/ScaleManager.h>

//----------------------------------
// Declarations
//----------------------------------
Adafruit_MCP23X17 mcp_U1;
Adafruit_MCP23X17 mcp_U2;

LEDController ledController;

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

OctaveControl<Adafruit_MCP23X17, LEDController> octaveControl(mcp_U2, ledController);

ScaleManager scaleManager;
KeyboardControl<decltype(MIDI), decltype(octaveControl), LEDController> keyboardControl(MIDI, octaveControl, ledController, scaleManager);

Preferences preferences; // Define Preferences object
BLEServer* pServer = nullptr;
bool deviceConnected = false;

// NEW: Definitions of new BLE Characteristic pointers
BLECharacteristic* pSWD1LRCCCharacteristic = nullptr;
BLECharacteristic* pSWD1CenterCCCharacteristic = nullptr;
BLECharacteristic* pSWD2LRCCCharacteristic = nullptr;
BLECharacteristic* pSWD2CenterCCCharacteristic = nullptr;
BLECharacteristic* pMidiCcCharacteristic = nullptr;
BLECharacteristic* pRootNoteCharacteristic = nullptr;
BLECharacteristic* pScaleTypeCharacteristic = nullptr;

// Global state variables and their initial values
int ccNumberSWD1LeftRight = 3;  // Default for SWD1 Left/Right
int ccNumberSWD1Center = 24;    // Default for SWD1 Center (Sustain)
int ccNumberSWD2LeftRight = 7;  // Default for SWD2 Left/Right (e.g., Expression)
int ccNumberSWD2Center = 1;     // Default for SWD2 Center (e.g., Modulation)

void readInputs(void *pvParameters);

//---------------------------------------------------
//
//  Setup
//
//---------------------------------------------------
void setup() {
    SERIAL_BEGIN();
    SERIAL_PRINTLN("Serial monitor started.");

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

    //////// U1 //////////////////////////////////////////////////////
    // Configure SWD buttons on U1 as inputs with pull-up
    mcp_U1.pinMode(SWD1_LEFT_PIN, INPUT_PULLUP);
    mcp_U1.pinMode(SWD1_CENTER_PIN, INPUT_PULLUP);

    // Configure SWD buttons on U2 as inputs with pull-up
    mcp_U2.pinMode(SWD1_RIGHT_PIN, INPUT_PULLUP);
    mcp_U2.pinMode(SWD2_LEFT_PIN, INPUT_PULLUP);
    mcp_U2.pinMode(SWD2_CENTER_PIN, INPUT_PULLUP);
    mcp_U2.pinMode(SWD2_RIGHT_PIN, INPUT_PULLUP);

    xTaskCreatePinnedToCore(readInputs, "readInputs", 4096, nullptr, 1, nullptr, 1);

    MIDI.begin(1);

    octaveControl.begin();
    keyboardControl.begin();

    scaleManager.setScale(ScaleType::MINOR);
    scaleManager.enableWhiteKeyQuantization(true);


    ledController.begin(LedColor::OCTAVE_UP, 7, &mcp_U2);
    ledController.begin(LedColor::OCTAVE_DOWN, 5, &mcp_U2);

    ledController.begin(LedColor::BLUE, 7);
    ledController.begin(LedColor::PINK, 8);

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

    pRootNoteCharacteristic = pService->createCharacteristic(
        ROOT_NOTE_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pRootNoteCharacteristic->addDescriptor(new BLE2902());
    pRootNoteCharacteristic->setCallbacks(new CharacteristicCallbacks());
    auto initialRootNote = static_cast<uint8_t>(scaleManager.getRootNote());
    pRootNoteCharacteristic->setValue(&initialRootNote, 1);

    pScaleTypeCharacteristic = pService->createCharacteristic(
        SCALE_TYPE_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pScaleTypeCharacteristic->addDescriptor(new BLE2902());
    pScaleTypeCharacteristic->setCallbacks(new CharacteristicCallbacks());
    auto initialScaleType = static_cast<uint8_t>(scaleManager.getScaleType());
    pScaleTypeCharacteristic->setValue(&initialScaleType, 1);

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

//---------------------------------------------------
//
//  Loops
//
//---------------------------------------------------
void loop() {
    ledController.update();
    // Small delay to avoid watchdog timer issues
    vTaskDelay(1 / portTICK_PERIOD_MS);
}

[[noreturn]] void readInputs(void *pvParameters) {
    while (true) {
        touch1.update();
        lever1.update();
        lever2.update();
        leverPush1.update();
        leverPush2.update();
        octaveControl.update();
        keyboardControl.updateKeyboardState();

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
