# PianoRoll Editor Page

The PianoRoll page is used to edit External MIDI sequencer tracks 1-6.

The editor features two modes: Note editing and Automation editing.


![proll](../assets/images/proll.png)


_Press **[Bank Group] + [Trig 7]** to open the Pianoroll Editor page_

| Control | Assignment |
| --- | --- |
| Encoder 1 | Cursor X |
| Encoder 2 | Cursor Y / Note Value |
| Encoder 3 | Cursor Width / Note Width |
| Encoder 4 | Zoom |
| Save / No | Record |
| Page | PageSelect |
| Load / Yes | Add or Remove Note |
| Shift | Pianoroll Menu |


## GUI


![proll edit](../assets/images/proll_edit.png)


The PianoRoll editor operates in continuous time, with the finest resolution being 1/192nd of a quarter note. Dots are drawn to illustrate each quarter note. Vertical lines denote the commencement of a new beat.


- The cursor can be moved to a specific time offset by rotating **`Encoder 1`**.
- **`Encoder 4`** adjusts the zoom along time the time axis.
- The note value is chosen using **`Encoder 2`**
- The note width controlled using **`Encoder 3`**.
- Notes can be both entered and deleted by pressing the **`Load`** button.


Alternatively, the MD's GUI can be used to navigate the piano roll, and edit the sequence.


- **[Enter/Yes]** add or remove notes.
- **[Left/Right]** move cursor along time axis.
- **[Enter/Yes] + [Left/Right]** nudge cursor along time axis (fine control).
- **[Exit/No] + [Left/Right]** adjust cursor width.
- **[Enter/Yes] + [Exit/No] + [Left/Right]** nudge cursor width (fine control).
- **[Up/Down]** move cursor along note axis.
- **[Function] + [Up/Down/Left/Right]** cursor fast travel.
- **[Exit/No] + [Up/Down]** zoom in and out.
- **[Clear/Copy/Paste]** clear/copy/paste for track.
- **[Scale]** Toggle sequencer page.
- **[Function] + [Scale]** The MD's scale menu can be used to configure the length and speed of the current track.
- The MD's **[Trig]** keys can be used to position the cursor at step intervals relative to the current page.


## External MIDI Control

An external MIDI device connected on Port 2 or USB MIDI can be used to position the cursor's vertical position.


- Ensure that the **Config-->MIDI-->CTRL PORT** is set to the MIDI port your external keyboard/sequencer is connected.


_When using an external MIDI keyboard, the octave, and scale mapping can be changed from the Chromatic Page as discussed in the next chapter._
Holding a note on an external MIDI keyboard, and then pressing the MD's **[Enter/Yes]** or a **[Trig]** key, allows individual notes to be added at the cursor position.


Similarly, using a simultaneous combination of the MD's **[Trig]** keys and an external MIDI Keyboard, chords can be entered into the Note Editor. First play a chord on the Keyboard, then press **[Enter/Yes]** or a **[Trig]** key to store the chord at the cursor location.

## Live Record

Live Record mode can be activated using the MD's **[Rec]** function. Both note and automation data can be recorded simultaneously. Automation data includes all ControlChannel, Pitchbend and channel pressure messages received on MIDI port 2.

All 6 tracks can be recorded to simultaneously.


Tracks will only record incoming data that is on the same MIDI Channel. _(see section MIDI Channel Selection)_


To disable Automation Recording use the CC Rec option in the Track Menu.

## PianoRoll Editor Track Menu


![proll menu](../assets/images/proll_menu.png)


Holding **[Global]** opens the Track menu. For each track you can adjust the MIDI Channel, track length and playback speed. Cursor editing options are also included here including note velocity and note conditional settings.


| Entry | Function |
| --- | --- |
| Track Select | Change Track |
| Edit | Note or Automation editing modes |
| VEL | Note Velocity |
| Cond | Trig Condition |
| Channel | MIDI Channel |
| Key | MIDI Channel |
| CC Rec | Enable/Disable Automation recording. |


## Ext MIDI Track Selection & Mutes

When opening the Track Menu **[Global]** from within in either the PianoRoll Editor or Chromatic Page, the MD's **[Trig]** keys can be used to switch between Ext MIDI Tracks and/or mute/unmute them.


- MD **[Trig]** keys 1-6 correspond to Ext MIDI Track selection 1-6.
- MD **[Trig]** keys 9-14 correspond to Ext MIDI Track mutes 1-6.


## MIDI Channel Selection

Each Ext MIDI track listens and transmits on a set MIDI Channel. The channel defaults to the track number. This can be easily changed by modifying the Track menu CHAN option.


## Change Edit Mode

From the PianoRoll menu, the "Edit" parameter changes the editing mode. Switch between either Note editing, or editing automation parameters 1-8.


## Automation Editing


![proll aut](../assets/images/proll_aut.png)


_The PianoRoll Editor page allows for automation editing. From the Track menu, use the "Edit" menu option to select one of eight Automation Parameters._


Each External MIDI track features 8 automation parameters.


## Automation Editor Track Menu

Hold **[Global]** to open the Track menu.


| Entry | Function |
| --- | --- |
| Edit | Note or Automation editing modes |
| Slide | Linear slide between automation values |
| CC | CC destination, Program Change, Pitch Bend, Channel Pressure, MIDI learn |
| CC Rec | Enable/Disable Automation recording. |


Slide: enables/disable interpolation between automation events.


CC: Allows a specific MIDI Control Channel number to be a chosen parameter.


- When set to LEARN, to Automatically learn the next received CC on the same MIDI channel.
- When set to PRG, the track will send Program Change messages.
- When set to PB, the track will send pitch bend messages.
- When set to CHP, the track will send channel pressure messages.


## Automation Step Locks

It's possible to enter CC automation data at specific steps, by holding down the corresponding **[Trig]** and rotating the external MIDI CC control knob. This feature is available from both the Piano Roll and Automation Editor.
