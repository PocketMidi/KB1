# KB1 Firmware v1.0 Code Summary

## INTRODUCTION
#### This code uses the ESP32 platform, which supports FreeRTOS and allows for multicore programming. 


The **buttonReadTask** function is running on **Core 1**, and the loop function (which contains the main MIDI processing logic) is running on **Core 0**.
The code initializes the MCP23X17 GPIO expanders, and MIDI objects. 
Then, it configures the buttons and starts the **buttonReadTask** on **Core 1** using FreeRTOS. 
The **buttonReadTask** continuously reads the state of buttons connected to the MCP23X17 GPIO expanders. 
It checks for button presses and releases, triggering MIDI values or other tasks accordingly. 
The button processing and MIDI functions are separated, allowing the MIDI processing to continue in the background on **Core 0** while the button reading occurs on Core 1. This approach ensures that time-sensitive MIDI processing is not affected by potential delays caused by button reading or other tasks running on **Core 1**. 
The loop function runs on **Core 0** handling notes, velocities, and octaves. *OCT_DN* and *OCT_UP* buttons modulate octaves, signifying transitions through rhythmic pulsations of their LEDs. Two Levers adjust parameters such as "finetune" (midi cc#3), "sustain", and note velocities during prolonged activation, visually represented by by pink and blue LEDs.

## Libraries

This firmware utilizes the Arduino framework for the [Xiao ESP32S3 board](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/). 

The board must be set up in the Arduino IDE per the Getting Started Guide above.

#### Additional Libraries:
Adafruit MCP23017 Library - https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library
Arduino Midi Library - https://github.com/FortySevenEffects/arduino_midi_library
FreeRTOS Library - https://github.com/feilipu/Arduino_FreeRTOS_Library

These are installable in the Arduino IDE, or as external packages.


## Input Mapping:
Reference - https://github.com/smallgram/KB1/blob/main/Keyboard/Firmware/KB1_Connection_Map_FW_v1.0%20-%20Sheet1.csv
#### MIDI Notes (Keys):
Various buttons on U1 and U2 pins are mapped to MIDI notes and control changes.

#### MIDI Control Changes (Lever 1):
*SWD1_Right* sends MIDI control change number 3 on channel 1 with values 127 when pressed and 64 when released. 

*SWD1_Center* triggers MIDI "sustain" mode. *Pink LED* pulses while active.

*SWD1_Left* sends MIDI control change number 3, value 0 when pressed, and 64 when released.

#### MIDI Note Velocity Changes “Lever 2”:
*SWD2_Right* increments note velocity by 8 upon button press. *Blue LED* indicates increase.

*SWD2_Center* sets note velocity to 64 when pressed. *Blue LED* indicates reset.

*SWD2_Left* decrements note velocity by 8 on button press. *Blue LED* indicates decrease.

#### MIDI Octave Changes:
*OCT_DN* blinks at different rates based on the octave level decrease.

*OCT_UP* blinks at varying speeds with octave level increase.

Simultaneous pressing of both octave buttons resets the octave level.



