# MCL Migration: ATmega2560 to RP2040

This is a WIP migration of [MCL](https://github.com/jmamma/mcl) from the ATmega2560 to the RP2040. Given MCL became a modified Arduino core, the port will initially be based around arduino-pico, enabling us to leverage the SdFat and Adafruit graphics libraries that are core components of MCL.

## Current Progress

- [X] Platform.txt -> Makefile translation
 - Code can be compiled independently of the Arduino IDE
 - Correct linking of arduino-pico, pico-sdk and related libraries

- [X] VSCode + gdb + openocd SW debugger integration
 - For realtime hardware debugging

- [X] Implementation of low level ISRs for UART + timers, as per MCL
  - The atmega2560 does not support nested interrupts. I've re-implemented ISR locking as per MCL, with the ability to unlock when entering Sequencer and MIDI processing routines.
  - Eventually this will be re-architectured to leverage the 2nd core.
- [X] MIDI stack
 - Low level MIDI stack responsible for initialising the UARTs
 - Processing rx/tx of MIDI data

- [X] Validate MIDI stack at various turbo speeds (1x, 2x, 4x, 8x, 10x)
 - [X] Sysex Tx/Rx
 - [X] Midi Note Tx/Rx
 - [X] MIDI CC Tx/Rx

- [X] Verify SDFat is functional with the arduino-pico core

## Todo

- [ ] Oled display + Adafruit GFX
- [ ] Page system
- [ ] GUI
- [ ] Menus
- [ ] MCL project/grid initialisation
- [ ] Object serialisation to/from SD Card
- [ ] MIDI Device Drivers
- [ ] Sequencer
- [ ] Compressed Assets
- [ ] Stack Size. Currently limited to 4KB per core. Could we use all 8KB for a single core?
      MCL in current implementation requires a stack size of 8KB.
- [ ] ... and more
