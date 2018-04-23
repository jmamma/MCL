The following repository contains a functional Arduino Mega 2560 core.
It is compatibile with the MegaCommand Arduino Shield and the Arduino IDE framework.

The repository is maintained by Justin Mammarella <jmamma@gmail.com>

This is a fork of Manuel Odendahl's MIDICtrl Framework for the Ruin&Wesen MiniCommand
MIDI controller:
https://github.com/wesen/mididuino

The updated repository contains numerous enhancements and fixes.

In 2016 the core was adapted to compile with the ArduinoIDE and MegaCommand hardware design.
In 2017 the core was modified to work alongside standard Arduino Code and Libraries.
In 2018 the MegaCommandLive firmware was refactored in to usable libraries and is now
part of the MIDICtrl framework.

It contains code to:

- control the hardware (LCD/OLED, encoders, buttons, MIDI interface, SD card)
- handle the MIDI inputs and the MIDI output
- general data structures and algorithms (stack, callbacks, vectors,
  ring buffers, etc...)
- create GUIs (using sketches, pages, modal pages, event handlers)
- handle SDCard storage
- interface with the Elektron MachineDrum, MonoMachine, Analog4
