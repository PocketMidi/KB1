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

// Uncomment following line to enable /////
#define SERIAL_PRINT_ENABLED 1

#include <Adafruit_MCP23X17.h>
#include <HardwareSerial.h>
#include <MIDI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>

// Pin definitions for U1
#define SWD1_LEFT_PIN 5   // SWD1 Left button pin
#define SWD1_CENTER_PIN 6 // SWD1 Center button pin

// Pin definitions for U2
#define SWD1_RIGHT_PIN 0  // SWD1 Right button pin
#define SWD2_LEFT_PIN 1   // SWD2 Left button pin
#define SWD2_CENTER_PIN 2 // SWD2 Center button pin
#define SWD2_RIGHT_PIN 3  // SWD2 Right button pin

// Enable/disable serial printing /////
#ifdef SERIAL_PRINT_ENABLED
#define SERIAL_PRINT(msg) Serial.print(msg)
#define SERIAL_PRINTLN(msg) Serial.println(msg)
#else
#define SERIAL_PRINT(msg)
#define SERIAL_PRINTLN(msg)
#endif

struct key
{
    uint8_t midi;
    uint8_t pin;
    bool state;
    bool prevState;
    Adafruit_MCP23X17 *mcp;
    char name[12];
};

Adafruit_MCP23X17 mcp_U1; // MCP1, named U1
Adafruit_MCP23X17 mcp_U2; // MCP2, named U2

#define MAX_KEYS 19 // define keys
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
    {77, 8, true, true, &mcp_U2, "SW12 (F)"}};

// Define MIDI object
MIDI_CREATE_INSTANCE(HardwareSerial, Serial0, MIDI);

// Forward declarations
void playMidiNote(byte note);
void stopMidiNote(byte note);
void shiftOctave(int shift);
void buttonReadTask(void *pvParameters);
void gotTouch1();

// Touch Shift Mode ///
bool shiftModeEnabled = false;
volatile bool shiftModeActive = false;
int threshold = 1500; // Adjust this value according to your setup
bool touch1detected = false;


// Previous state variables for OCT buttons
volatile bool prevS3State = true;
volatile bool prevS4State = true;

// Previous state variables for Lever 1
bool prevSWD1LeftState = false;
bool prevSWD1CenterState = false;
bool prevSWD1RightState = false;

// Previous state variables for Lever 2
volatile bool prevSWD2LeftState = true;
volatile bool prevSWD2CenterState = true;
volatile bool prevSWD2RightState = true;
bool isSwd2LeftPressed = false;
bool isSwd2RightPressed = false;
bool isSwd2CenterPressed = false;
const int swd2Interval = 200;      // Interval in milliseconds (0.2 seconds)
volatile int currentVelocity = 127; // state variable for velocity value
int minVelocity = 8;

// LED Blue // Velocity // Lever 2 ////////
const int bluePin = 7;
int blueBrightness = 0;
unsigned long previousTime = 0;         // variables for non-blocking timers
const unsigned long ledOffDelay = 3000; // xx seconds in milliseconds

void updateVelocity(int delta)
{
    int previousVelocity = currentVelocity; // Store previous velocity for comparison
    currentVelocity += delta;
    if (currentVelocity < minVelocity) // Ensure velocity is within bounds
        currentVelocity = minVelocity;
    else if (currentVelocity > 127)
        currentVelocity = 127;
    if (delta > 0 || (delta < 0 && blueBrightness < 255 - 25))
    {                                               // Update blue LED brightness based on velocity change
        if (delta > 0 && blueBrightness < 255 - 25) // Increase or decrease brightness xx%
        {
            blueBrightness += 25;
        }
        else if (delta < 0)
        {
            if (currentVelocity <= minVelocity || previousVelocity > minVelocity)
            {
                blueBrightness = 25; // Set initial brightness to xx%
            }
            else if (blueBrightness > 25)
            {
                blueBrightness -= 25;
            }
        }
        analogWrite(bluePin, blueBrightness);
        previousTime = millis(); // Update time when brightness changes
    }
    if (delta > 0 && currentVelocity > previousVelocity)
    { // Report current velocity when it increases or decreases
        SERIAL_PRINT("Current Velocity Increased to: ");
        SERIAL_PRINTLN(currentVelocity);
    }
    else if (delta < 0 && currentVelocity < previousVelocity)
    {
        SERIAL_PRINT("Current Velocity Decreased to: ");
        SERIAL_PRINTLN(currentVelocity);
    }
}
void resetVelocity()
{
    currentVelocity = 127; // Reset velocity to 127
    SERIAL_PRINTLN("Velocity Reset to 127");
    analogWrite(bluePin, 51); // Set LED to xx% brightness
    delay(250);               // Delay for 0.5 seconds
    analogWrite(bluePin, 0);  // Turn off LED
}

// Octave Timers & Callbacks //
volatile int currentOctave = 0;
TimerHandle_t upTimer;
TimerHandle_t downTimer;
void upTimerCallback(TimerHandle_t xTimer);
void downTimerCallback(TimerHandle_t xTimer);
void resetVelocity();

// MIDI notes
bool isNoteOn[128];
void playMidiNote(byte note)
{ // Play MIDI note with octave shift & velocity
    MIDI.sendNoteOn(note + currentOctave * 12, currentVelocity, 1);
    SERIAL_PRINT("Note On: ");
    SERIAL_PRINT(note + currentOctave * 12);
    SERIAL_PRINT(", Velocity: ");
    SERIAL_PRINTLN(currentVelocity);
    isNoteOn[note] = true; // Set note-on flag to true
}
void stopMidiNote(byte note)
{ // Check if note-on flag is true before sending note-off signal
    if (isNoteOn[note])
    { // Stop MIDI note with octave shift & velocity 0
        MIDI.sendNoteOff(note + currentOctave * 12, 0, 1);
        SERIAL_PRINT("Note Off: ");
        SERIAL_PRINT(note + currentOctave * 12);
        SERIAL_PRINTLN(", Velocity: 0");
        isNoteOn[note] = false; // Reset note-on flag to false
    }
}

// LED Pink // Sustain // Lever 1 /////
const int pinkPin = 8;
void controlPinkBreathingLED(bool state) // Control LED with breathing effect
{
    static unsigned long previousMillis = 0;
    static int brightness = 0;
    static int fadeAmount = 1;               // breathing speed
    static unsigned long pulseInterval = 84; // breathing interval (in milliseconds)
    const int minBrightness = 4;
    const int maxBrightness = 18;
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= pulseInterval)
    {
        previousMillis = currentMillis;
        if (brightness <= minBrightness)
        {
            fadeAmount = abs(fadeAmount); // Ensure fadeAmount is positive
        }
        else if (brightness >= maxBrightness)
        {
            fadeAmount = -abs(fadeAmount); // Ensure fadeAmount is negative
        }
        brightness += fadeAmount;
        analogWrite(pinkPin, brightness);
    }
    if (state)
    {
        analogWrite(pinkPin, 0); // Ensure LED is completely off when state is true
    }
}

// Toggle shift mode state
    void gotTouch1() {
    shiftModeActive = !shiftModeActive; 
    if (shiftModeActive) {
        SERIAL_PRINTLN("SHIFT Mode Enabled");
    } else {
        SERIAL_PRINTLN("SHIFT Mode Disabled");
    }
}



/////////////////////////////////////////////////////////////////
////////////////////////  SETUP  ////////////////////////////////
/////////////////////////////////////////////////////////////////

void setup()
{
    Serial.begin(115200); // Start serial monitor
    SERIAL_PRINTLN("Serial monitor started.");

    memset(isNoteOn, false, sizeof(isNoteOn)); // Initialize all note-on flags to false

    pinMode(LED_BUILTIN, OUTPUT); // initialize digital pin LED_BUILTIN as an output.

    digitalWrite(LED_BUILTIN, LOW); // Keep built-in LED on continuously

    // Start MCP_U1
    if (!mcp_U1.begin_I2C(0x20)) // MCP1 is at address 0x20
    {
        SERIAL_PRINTLN("Error initializing U1.");
        while (1)
            ;
    }
    // Start MCP_U2
    if (!mcp_U2.begin_I2C(0x21)) // MCP2 is at address 0x21
    {
        SERIAL_PRINTLN("Error initializing U2.");
        while (1)
            ;
    }
    // Configure Keys as input with pull-up
    for (int i = 0; i < MAX_KEYS; i++)
    {
        keys[i].mcp->pinMode(keys[i].pin, INPUT_PULLUP);

        touchAttachInterrupt(T1, gotTouch1, threshold); 
        
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
    xTaskCreatePinnedToCore(buttonReadTask, "ButtonReadTask", 4096, NULL, 1, NULL, 1);

    //////// MIDI //////////////////////////////////////////////////////
    // Configure Serial0 (default serial port) on pins TX=1 & RX=0 (default)
    Serial0.begin(31250);
    Serial0.print("Serial0");
    MIDI.begin(1); // Initialize MIDI on channel 1
    SERIAL_PRINTLN("MIDI library initialized on channel 1.");

    //////// Octave Button LEDs ////////////////////////////////////////////
    // Configure LEDs on U2 // Set LEDs to initial state (both off)
    mcp_U2.pinMode(5, OUTPUT);
    mcp_U2.pinMode(7, OUTPUT);
    mcp_U2.digitalWrite(5, HIGH); // DN LED (active low)
    mcp_U2.digitalWrite(7, HIGH); // UP LED (active low)

    //////// Lever Indicator LEDs ////////////////////////////////////////////
    upTimer = xTimerCreate("UpTimer", pdMS_TO_TICKS(1500), pdTRUE, (void *)0, upTimerCallback);
    downTimer = xTimerCreate("DownTimer", pdMS_TO_TICKS(1500), pdTRUE, (void *)0, downTimerCallback);
    if (upTimer == NULL || downTimer == NULL)
    {
        SERIAL_PRINTLN("Error creating timers!");
        while (1)
            ;
    }
    pinMode(pinkPin, OUTPUT);
    pinMode(bluePin, OUTPUT);
    analogWrite(bluePin, blueBrightness); // Set initial brightness
}

////////////////////////////////////////////////////////////////////////
////////////////////////  LOOP  ////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void loop()
{ // MIDI processing continues on Core 0
  // add additional logic here if needed
}
void upTimerCallback(TimerHandle_t xTimer)
{ // Toggle UP LED state
    mcp_U2.digitalWrite(7, !mcp_U2.digitalRead(7));
}
void downTimerCallback(TimerHandle_t xTimer)
{ // Toggle DN LED state
    mcp_U2.digitalWrite(5, !mcp_U2.digitalRead(5));
}
void shiftOctave(int shift)
{
    currentOctave += shift;
    if (currentOctave < -3)
        currentOctave = -3;
    else if (currentOctave > 3)
        currentOctave = 3;
    xTimerStop(upTimer, 0); // Stop timers before starting new ones
    xTimerStop(downTimer, 0);
    if (currentOctave == 0) // Control Octave LEDs based on current octave
    {
        mcp_U2.digitalWrite(5, HIGH); // DN LED off (active low)
        mcp_U2.digitalWrite(7, HIGH); // UP LED off (active low)
    }
    else if (currentOctave > 0)
    {
        mcp_U2.digitalWrite(5, HIGH);                           // DN LED off (active low)
        mcp_U2.digitalWrite(7, LOW);                            // UP LED on (active low)
        if (currentOctave == 1)                                 // Start timer for flashing Octave UP LED
            xTimerChangePeriod(upTimer, pdMS_TO_TICKS(750), 0); // Reduced from 1500 to 750
        else if (currentOctave == 2)
            xTimerChangePeriod(upTimer, pdMS_TO_TICKS(375), 0); // Reduced from 750 to 375
        else if (currentOctave == 3)
            xTimerChangePeriod(upTimer, pdMS_TO_TICKS(175), 0); // Reduced from 350 to 175
        xTimerStart(upTimer, 0);
    }
    else
    {
        mcp_U2.digitalWrite(7, HIGH);                             // UP LED off (active low)
        mcp_U2.digitalWrite(5, LOW);                              // DN LED on (active low)
        if (currentOctave == -1)                                  // Start timer for flashing Octave DN LED
            xTimerChangePeriod(downTimer, pdMS_TO_TICKS(750), 0); // Reduced from 1500 to 750
        else if (currentOctave == -2)
            xTimerChangePeriod(downTimer, pdMS_TO_TICKS(375), 0); // Reduced from 750 to 375
        else if (currentOctave == -3)
            xTimerChangePeriod(downTimer, pdMS_TO_TICKS(175), 0); // Reduced from 350 to 175
        xTimerStart(downTimer, 0);
    }
    delay(80);
}

////////////////////////  Button Read  /////////////////////////////////
////////////////////////////////////////////////////////////////////////

void buttonReadTask(void *pvParameters)
{
    // Track S3 & S4 OCT buttons press state
    bool isS3Pressed = false;
    bool isS4Pressed = false;
    bool areS3S4Pressed = false;

    while (1)
    { // Read key states & button states on U2
        for (int i = 0; i < MAX_KEYS; i++)
        {
            keys[i].state = !keys[i].mcp->digitalRead(keys[i].pin);
        }
        bool SWD1LeftState = !mcp_U1.digitalRead(SWD1_LEFT_PIN);
        bool SWD1CenterState = !mcp_U1.digitalRead(SWD1_CENTER_PIN);
        bool SWD1RightState = !mcp_U2.digitalRead(SWD1_RIGHT_PIN);
        bool S3State = !mcp_U2.digitalRead(4);                       // U2 pin 4, S3 button
        bool S4State = !mcp_U2.digitalRead(6);                       // U2 pin 6, S4 button
        bool SWD2LeftState = !mcp_U2.digitalRead(SWD2_LEFT_PIN);     // SWD2_Left
        bool SWD2CenterState = !mcp_U2.digitalRead(SWD2_CENTER_PIN); // SWD2_Center
        bool SWD2RightState = !mcp_U2.digitalRead(SWD2_RIGHT_PIN);   // SWD2_Right

        ////////// LEVER 1 LEFT  /////////////////////////////////////////////////
        if (SWD1LeftState && !prevSWD1LeftState)
        { // Send MIDI control change message: CC#3, value 0
            MIDI.sendControlChange(3, 0, 1);
            SERIAL_PRINTLN("SWD1_Left Pressed! MIDI CC#3, Value 0");
        }
        else if (!SWD1LeftState && prevSWD1LeftState)
        { // Send MIDI control change message: CC#3, value 64
            MIDI.sendControlChange(3, 64, 1);
            SERIAL_PRINTLN("SWD1_Left Released! MIDI CC#3, Value 64");
        }

        ////////// LEVER 1 RIGHT  /////////////////////////////////////////////////
        if (SWD1RightState && !prevSWD1RightState)
        { // Send MIDI control change message: CC#3, value 127
            MIDI.sendControlChange(3, 127, 1);
            SERIAL_PRINTLN("SWD1_Right Pressed! MIDI CC#3, Value 127");
        }
        else if (!SWD1RightState && prevSWD1RightState)
        { // Send MIDI control change message: CC#3, value 64
            MIDI.sendControlChange(3, 64, 1);
            SERIAL_PRINTLN("SWD1_Right Released! MIDI CC#3, Value 64");
        }

        ////////// LEVER 1 CENTER  /////////////////////////////////////////////////
        static bool sustain = true; // Set initial state of sustain to off
        if (SWD1CenterState && !prevSWD1CenterState)
        {
            if (sustain)
            { // Send MIDI control change message: CC#24, value 96, channel 1 = 5.5 sec
                MIDI.sendControlChange(24, 96, 1);
                SERIAL_PRINTLN("MIDI CC#24, Value 96, Channel 1");
            }
            else
            { // Send MIDI control change message: CC#24, value 48, channel 1 = .75 sec
                MIDI.sendControlChange(24, 48, 1);
                SERIAL_PRINTLN("MIDI CC#24, Value 48, Channel 1");
            }
            sustain = !sustain;               // Toggle state
            controlPinkBreathingLED(sustain); // Control LED based on sustain state
        }
        controlPinkBreathingLED(sustain); // Call controlBreathingLED function in loop

        ////////// LEVER 2 LEFT ///////////////////////////////////////////////
        if (SWD2LeftState && !prevSWD2LeftState && !isSwd2LeftPressed)
        {
            updateVelocity(-8); // Subtract 8 from current velocity
            SERIAL_PRINTLN("SWD2_Left Pressed! Decrease Velocity");
            isSwd2LeftPressed = true; // Start a new task for continuous change
        }
        else if (!SWD2LeftState && prevSWD2LeftState)
        {
            isSwd2LeftPressed = false;
        }

        ////////// LEVER 2 RIGHT ///////////////////////////////////////////////
        if (SWD2RightState && !prevSWD2RightState && !isSwd2RightPressed)
        {
            updateVelocity(8); // Add 8 to current velocity
            SERIAL_PRINTLN("SWD2_Right Pressed! Increase Velocity");
            isSwd2RightPressed = true; // Start a new task for continuous change
        }
        else if (!SWD2RightState && prevSWD2RightState)
        {
            isSwd2RightPressed = false;
        }

        ////////// LEVER 2 CENTER ///////////////////////////////////////////////
        if (SWD2CenterState && !prevSWD2CenterState && !isSwd2CenterPressed)
        { // Reset velocity to 127
            resetVelocity();
            currentVelocity = 127;
            SERIAL_PRINTLN("SWD2_Center Pressed! Reset Velocity");
            isSwd2CenterPressed = true;
        }
        else if (!SWD2CenterState && prevSWD2CenterState)
        {
            isSwd2CenterPressed = false;
        }
        // Update Blue LED brightness
        unsigned long currentTime = millis();
        unsigned long elapsedTime = currentTime - previousTime;
        if (elapsedTime < ledOffDelay)
        {
            int fadeValue = map(elapsedTime, 0, ledOffDelay, blueBrightness, 0);
            analogWrite(bluePin, fadeValue);
        }
        else
        {
            analogWrite(bluePin, 0); // Turn off LED when fade is complete
        }

        // Save current button states for Levers
        prevSWD1LeftState = SWD1LeftState;
        prevSWD1CenterState = SWD1CenterState;
        prevSWD1RightState = SWD1RightState;
        prevSWD2LeftState = SWD2LeftState;
        prevSWD2CenterState = SWD2CenterState;
        prevSWD2RightState = SWD2RightState;

        //////////////////  OCTAVE Buttons ////////////////////////////////////////
        if (S3State && S4State) // Check if both S3 & S4 are pressed simultaneously
        {
            if (!areS3S4Pressed)
            { // Reset octave to normal
                currentOctave = 0;
                SERIAL_PRINTLN("S3 & S4 Pressed Simultaneously! Octave Reset");
                areS3S4Pressed = true;                // Set button press state & apply debounce
                vTaskDelay(100 / portTICK_PERIOD_MS); // Debounce delay for 100 milliseconds
            }
        }
        else
        { // Reset button press state when S3 & S4 are released
            areS3S4Pressed = false;
            if (!S3State && isS3Pressed)
            {                    // Check U2 button states for individual actions
                shiftOctave(-1); // Decrease current octave by 1 when S3 is released
                SERIAL_PRINTLN("S3 Released! Octave Down by 1");
                isS3Pressed = false;
                vTaskDelay(50 / portTICK_PERIOD_MS); // Debounce delay for 50 milliseconds
            }
            else if (S3State)
            { // Set button press state when S3 is pressed
                isS3Pressed = true;
            }
            if (!S4State && isS4Pressed)
            { // Increase current octave by 1 when S4 is released
                shiftOctave(1);
                SERIAL_PRINTLN("S4 Released! Octave Up by 1");
                isS4Pressed = false;
                vTaskDelay(50 / portTICK_PERIOD_MS); // Debounce delay for 50 milliseconds
            }
            else if (S4State)
            { // Set button press state when S4 is pressed
                isS4Pressed = true;
            }
        }
        prevS3State = S3State;
        prevS4State = S4State;


        //////////////////  KEYS /////////////////////////////////////////////
        // check key states & initiate MIDI actions
        for (int i = 0; i < MAX_KEYS; i++)
        {
            if (keys[i].state != keys[i].prevState)
            {
                if (keys[i].state)
                { // press
                    playMidiNote(keys[i].midi);
                    SERIAL_PRINT(keys[i].name);
                    SERIAL_PRINTLN(" Pressed!");
                }
                else
                { // release
                    stopMidiNote(keys[i].midi);
                }
            }
            keys[i].prevState = keys[i].state;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS); // delay to avoid bouncing & reduce CPU usage
    }
}
