# MIDI Setup


## Connectivity

**Machinedrum**


- Connect the MIDI-Out of the Machinedrum to the MIDI-In (1) of the MegaCommand.
- Connect the MIDI-Out (1) of the MegaCommand to MIDI-In of the Machinedrum.


**Elektron Device (Analog4/MNM) (Optional)**


- Connect the MIDI-Out of the Elektron to the MIDI-In (2) of the MegaCommand.
- Connect the MIDI-Out (2) of the MegaCommand to MIDI-In of the Elektron.


![midi machines](../assets/images/midi_machines.png)


**External Clock (Optional)**


- A MIDI Keyboard, or sequencer can be connected to MIDI-In (2) , or USB MIDI.
- Attached sequencers can be used as external clock source.


**External MIDI (Optional)**


- A MIDI Keyboard connected to MIDI-In (2).
- MIDI-Out (2) connected to synth a module's MIDI-In.
- Attached MIDI Keyboards may be used to play notes in chromatic modes or via the PianoRoll editor.
- External synth can be sequenced from the External Sequencer Tracks.


![midi machines2](../assets/images/midi_machines2.png)


## MachineDrum Settings

Upgrade your MD to the latest MDX OS.


MCL communicates with the MachineDrum using SYSEX messages, and will configure your MD's current global settings automatically.


The Machinedrum's base channel can be chosen from the MD's Global menu.


For MK1 models, Turbo Speed should be set no higher than 4x.
(See Configuration Menu -> MIDI -> Port Config)

## MonoMachine Settings

Upgrade your MNM to the latest MNMX OS.


MCL communicates with the MonoMachine using SYSEX messages, and will configure your MNM's current global settings automatically.

## AnalogFour Settings


The following configuration must be manually applied in the Analog 4's Global Settings menu:


MIDI Port Config:


Output to MIDI
Input to MIDI
Keyboard CFG = EXT
Receive Notes = True
Receive CC/NPRN = True


MIDI Channels:

Tracks 1-6 channels need to be set to MIDI Channels 1-6 respectively.
