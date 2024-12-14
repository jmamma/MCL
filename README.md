This is a WIP migration of MCL github.com/jmamma/mcl from the atmega2560 to the RP2040.

Given MCL was a modified Arduino core, the port will initially be based around arduino-pico.
Enabling us to leverage the SdFat and Adafruit graphics libraries that are core component of MCL.

Current progress:

- [X] platform.txt -> Makefile translation. Code can be compiled independently of the Arduino IDE,
      with correct linking of arduino-pico, pico-sdk and related libraries.
- [X] VSCode + gdb + openocd SW debugger integration. For realtime hardware debugging.
- [X] Implementation of low level ISRs for UART + timers, as per MCL
- [X] MIDI stack. The low level MIDI stack responsible for initialising the UARTs and processing rx/tx of MIDI data.
- [X] Validate MIDI stack at various turbo speeds 1x, 2x, 4x, 8x, 10x
   -  [X] Sysex Tx/Rx
   -  [X] Midi Note Tx/Rx
   -  [X] MIDI CC Tx/Rx
- [X] Verify SDFat is functional with the arduino-pico core.

Todo:
- [ ] Oled display + Adafruit GFX
- [ ] Page system
- [ ] GUI
- [ ] Menus
- [ ] MCL project/grid initialisation.
- [ ] Object serialisation to/from SD Card.
- [ ] MIDI Device Drivers.
- [ ] Sequencer
- [ ] Compressed Assets.
- [ ] ... and more.
