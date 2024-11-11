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

### Compiling MCL Firmware

See https://github.com/jmamma/MCL/releases for pre-compiled firmware binaries

*It's no longer possible to compile the MCL firmware from within the Arduino IDE.*

Compressed assets for the graphics and menu structures must be generated first by running 
one of the following scripts using Powershell.
```
  MCL/resource/gen-resource-linux.ps1 (MAC/Linux)
  MCL/resource/gen-resource.ps1 (Windows)
```
The MCL firmware can then be compiled using the provided Makefile
```
  MCL/avr/cores/megacommand/Makefile
```
Only avr-gcc 7.30 should be used (included with the legacy IDE). 
Newer versions of avr-gcc result in a much larger binary and stability problems.

Example steps to compile the firmware on Mac: [README-BUILD-MAC.md](README-BUILD-MAC.md)
