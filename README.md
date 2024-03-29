## MegaCommand Live (MCL)

The following repository contains a functional Arduino Mega 2560 core and the MegaCommand Live firmware.

It is compatibile with the MegaCommand Arduino Shield and the Arduino IDE framework.

Parts of this project are built upon the work of:
   - Manuel Odendahl's MIDICtrl Framework: https://github.com/wesen/mididuino
   - Bill Greiman's SdFat library: https://github.com/greiman/SdFat
   - Adafruit's GFX Library: https://github.com/adafruit/Adafruit-GFX-Library
 
The updated repository contains numerous enhancements and fixes.

- In 2016 the core was adapted to compile with the ArduinoIDE and MegaCommand hardware design.
- In 2017 the core was modified to work alongside standard Arduino Code and Libraries.
- In 2018 the MegaCommandLive firmware was refactored in to c++ libraries.
- In 2021 the repository was renamed to MCL, to coincide with the MCL 4.0 release.

### Firmware Download.

See https://github.com/jmamma/MCL/releases for firmware binaries, user documentation and upload instructions.

### Installing the MCL core.

*The documentation below is for advanced users*

MacOS / Linux: 

1) Download the Arduino Legacy IDE [https://www.arduino.cc/en/Main/Software](https://www.arduino.cc/en/software/OldSoftwareReleases) (1.8.5 tested)

2) Copy Arduino.app to your /Applications folder and launch it.
   (Must be opened first before performing step below)

2) Get the MIDICtrl library and MegaCommand Core (same repo) and copy it to /Applications/Arduino.app/Contents/Java/hardware/ :
```
   cd /Applications/Arduino.app/Contents/Java/hardware/
   git clone https://github.com/jmamma/MCL
```
Windows:

The Arduino compiler (avr-gcc) does not like spaces within the full path name when compiling the custom core.

1) Download the Windows ZIP file for non-admin install
https://www.arduino.cc/en/software/OldSoftwareReleases

2) Extract zip file to desktop.

3) Download MIDICtrl20_MegaCommand and extract to the `arduino-1.8.5\hardware\` folder. 

4) If you have an admin-installed Arduino IDE, extract MCL to the arduino user directory `%USERPROFILE%\Documents\Arduino\hardware\` (e.g. `C:\Users\your_name\Documents\Arduino\hardware\`)

### Selecting the Core

1) Open the Arduino IDE Application

2) Select: Tools -> Board -> MegaCommand

### Compiling MegaCommandLive Firmware

All the source code for MegaCommand Live is contained within this repository.

To compile the MegaCommandLive firmware do the following:

1) Create a new Arduino Sketch

2) Insert the following code

```
#include "MCL.h"

void setup() {
  mcl.setup();
}
```
3) Compile sketch and upload to your MegaCommand
