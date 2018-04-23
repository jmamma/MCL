## MIDI-CTRL20

The following repository contains a functional Arduino Mega 2560 core and the MegaCommandLive firmware.

It is compatibile with the MegaCommand Arduino Shield and the Arduino IDE framework.

This is a fork of Manuel Odendahl's MIDICtrl Framework for the Ruin&Wesen MiniCommand
MIDI controller:
https://github.com/wesen/mididuino

The updated repository contains numerous enhancements and fixes.

- In 2016 the core was adapted to compile with the ArduinoIDE and MegaCommand hardware design.
- In 2017 the core was modified to work alongside standard Arduino Code and Libraries.
- In 2018 the MegaCommandLive firmware was refactored in to c++ libraries and is now
part of the MIDICtrl framework.

### Installing the MIDICtrl core.

(Instructions for OSX, should be similar for Windows)

1) Download the Arduino IDE https://www.arduino.cc/en/Main/Software (1.8.5 tested)

2) Get the MIDICtrl library and MegaCommand Core (same repo):
```
   cd /Applications/Arduino.app/Contents/Java/hardware/
   git clone https://github.com/jmamma/MIDICtrl20_MegaCommand
```
### Selecting the Core

1) Open the Arduino IDE, Under the Tools menu, select the core you wish to use from the "Board:" menu

The default Arduino core is named "Arduion/Genuino Mega or Mega 2560"

The MegaCommand core will be listed at the bottom.

### Compiling MegaCommandLive Firmware

All the source code for MegaCommand Live is contained in the following repository.

1) Create a new Arduino Sketch

2) Insert the following code

```
#include "MCL.h"

void setup() {
  mcl.setup();
}
```
3) Compile sketch and upload to your MegaCommand
