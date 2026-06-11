# MIDI Settings Menu

The MIDI menu provides access to seven MIDI configuration sub-menus.


![midi menu](../assets/images/midi_menu.png)


_Changes made in each MIDI configuration are applied upon menu exit._


## Port Config

The Port Config menu is used to configure the MIDI port settings.


![midi ports menu](../assets/images/midi_ports_menu.png)


| Entry | Function |
| --- | --- |
| TURBO 1: [ 1x, 2x, 4x, 8x ] | Port 1's Turbo MIDI speed |
| TURBO 2: [ 1x, 2x, 4x, 8x ] | Port 2's Turbo MIDI speed |
| TURBO USB: [ 1x, 2x, 4x, 8x ] | USB Port's Turbo MIDI speed |
| Driver 1: [ Gener, MD ] | Port 1's MIDI driver, either Generic or MD |
| Driver 2: [ Gener, Elekt ] | Port 2's MIDI driver, either Generic or Elektron |
| CTRL Port: [ 2, USB, 2 + USB) ] | Which MIDI port provides control input (Note + CC) for Sequencer + Chromatic pages. |


_If using the Machinedrum MK1, set Turbo Speed no greater than 4x._


If you are intending to pair your Megacommand with a supported Elektron device
(MNM, A4) set **Driver 2** to Elektron. For all other MIDI devices (including unsupported Elektron machines), set **Driver 2** to General MIDI (default setting).


_If you are intending to use the paired Elektron device's MIDI thru to control further external equipment, you must set Turbo 2 speed to 1x._


## Sync

The Sync Config menu is used to configure MIDI clock + transport receive and destination settings.


![midi sync menu](../assets/images/midi_sync_menu.png)


| Entry | Function |
| --- | --- |
| CLOCK RECV: [1, 2, USB] | Receive MIDI Clock from port. |
| TRANS RECV: [1, 2, USB] | Receive MIDI Transport from port. |
| CLOCK SEND: [OFF, 2, 2 + USB] | Forward MIDI Clock to selected port(s) |
| TRANS SEND: [OFF, 2, 2 + USB] | Forward MIDI Transport to selected port(s) |


_Note: MCL does not generate its own MIDI Clock, instead it relies on a clock signal from either Port 1, Port 2 or USB._


_The MD's internal clock settings are controlled by MCL and will be automatically configured based on the options selected above._


## Routing

The MIDI Routing configuration menu can be used to set MIDI traffic forwarding between ports.


![midi route menu](../assets/images/midi_route_menu.png)


| Entry | Function |
| --- | --- |
| MIDI1 FWD: [OFF, 2, USB, 2 + USB] | Forward non-realtime MIDI data from Port 1 input to the selected MIDI output port(s). |
| MIDI2 FWD: [OFF, 1, USB, 1 + USB] | Forward non-realtime MIDI data from Port 2 input to the selected MIDI output port(s). |
| USB FWD: [OFF, 1, 2, 1 + 2] | Forward non-realtime MIDI data from USB-MIDI input to the selected MIDI output port(s). |
| CC LOOP: [OFF, 2->2] | Loopback MIDI CC messages on same port. |


## Program

The Program menu is used to enable the sending and receiving of MIDI Program Change messages.


![midi prog menu](../assets/images/midi_prog_menu.png)


| Entry | Function |
| --- | --- |
| PROG Mode: [BASIC, ADV] | Basic or Advanced. |
| PRG IN: [--, 1 .. 16, OMNI] | Program change receive channel. |
| PRG OUT: [--, 1 .. 16] | Program change transmit channel. |


**PROG Mode:** MIDI Program Change IN has two modes of operation, BASIC or ADV (Advanced).


**BASIC:** Program Change Receive will "Group Load" an entire row according to Load Page mode and Group Select settings.
**ADV:** Program change receive will set the row for slots to be loaded from.

MIDI notes from C3 upwards on Port 2 can be used to select slots to be loaded.

In this way, you can load any slot (or multiple slots) using a combination of Program Change + MIDI note.


If Program Change is not received before MIDI note on/off, MCL will load from the current selected GUI row.


MCL cannot load instantaneously, there is a minimum 1 bar delay.


**PRG In:**
Set the receive channel for Program Change and Note messages on Port 2.


When set to -- (default) Program Change In is disabled.
**PRG Out:**
Set the send channel for Program Change messages on Port 2, which are sent when loading slots or rows.


When set to -- (default) Program Change Out is is disabled.


## MD MIDI

The MD MIDI configuration menu is used to configure Machinedrum driver's MIDI settings.


![midi md menu](../assets/images/midi_md_menu.png)


| Entry | Function |
| --- | --- |
| CHRO CHAN: [--, 1..16] | MIDI Channel for MD Chromatic mode. |
| POLY CHAN: [--, 1..16] | MIDI Channel for MD Polyphonic mode. |
| TRIG CHAN: [--, 1..16] | MIDI Channel for MD Trig mode. |


**CHRO CHAN:** The CHRO CHAN setting is used to control which input channel the MD should receive note data from when in Chromatic mode. When set to INT (default) the MD will be controlled by the MD's **[Trig]** keys in chromatic mode.

_For this setting to work correctly, CTRL PORT in PORT CONFIG must also be set to the desired port._

**POLY CHAN:** Dedicated MIDI Channel for playing designated Polyphonic tracks chromatically.
**TRIG CHAN:** Dedicated MIDI channel for triggering MD tracks via Note On message. Useful for triggering sounds using a MIDI drum pad. Track Mapping starts from note C2. Note Velocity is mapped to volume.


## General MIDI

The General MIDI configuration menu is used to configure the Generic MIDI driver's settings.


![general md menu](../assets/images/general_md_menu.png)


| Entry | Function |
| --- | --- |
| MUTE CC: [ 0..127,--] | CC parameter for sending/receving Ext MIDI track mute state on Mixer Page. |
