# Chromatic Page

The Chromatic Page, enables the user to play tracks of the Machinedrum or an External MIDI device chromatically. Each MD track can be used as a voice of a monophonic/polyphonic synthesizer. Melodic compositions can be recorded in real-time.


![chroma](../assets/images/chroma.png)


_The Chromatic page can be accessed by pressing **[BANK GROUP]** and **[Trig 8]**._


## Using the Chromatic Page


![chromat action](../assets/images/chromat_action.png)


| Control | Assignment |
| --- | --- |
| Encoder 1 | Octave (OC) |
| Encoder 2 | Fine Tune (F) |
| Encoder 3 | Track Length |
| Encoder 4 | Scale Type (S) |
| Save / No | Record |
| Page | PageSelect |
| Load / Yes | Apply All |
| Shift | Track/Trig Menu |


## Device Selection

The top left of the screen shows the active device tab and indicates whether the Chromatic Page is targeting the MD or an Ext MIDI device. The active device can be toggled by pressing **[SCALE]**.


## Parameters

The Octave (OCT) parameter allows for adjusting the relative octave of the track's tuning. The Detune (DET) parameter can be used for offsetting the absolute pitch by small increments (MD only). Length (LEN) controls the length of the associated sequencer track. Scale (SCA) maps MIDI notes to a musical scale type.
A keyboard at the bottom of the screen is displayed showing notes as they are played.

## Track Speed and Length

Track speed and length can also be set when in Step Edit Mode using the MD's Scale menu by pressing **[Func] + [Scale]**.

## Machinedrum Setup for Chromatic mode

There are three important MIDI configurations options for the Chromatic Page:


- **Config-->MIDI-->PORT CONFIG-->CTRL PORT**: specifies which Port the MCL should receive note input from.
- **Config-->MIDI-->MD MIDI-->CHRO CHAN**: specifies whether the MD should receive note input from a MIDI channel on an External MIDI Device. _See Chapter: Global Settings_
- **Config-->MIDI-->MD MIDI-->POLY CHAN**: specifies which MIDI channel to use when one or more MD tracks are dedicated synth voices. _See Chapter: Polyphonic Mode_


## Tuning

For MD tracks that are assigned a melodic machine, the machines PTC parameter is mapped to notes of a selected scale. The track can then be played chromatically using the MD's **[Trig]** keys or an attached MIDI keyboard.


_If the machine's tuning setting is TONAL then the new quarter tone, equal temperament tuning table will be used. If the machine's tuning setting is DEFAULT the legacy microtonal tuning table is used._


## Chromatic Page Track Menu


![chromatic menu](../assets/images/chromatic_menu.png)


Holding **[ GLOBAL ]** opens the Track menu with specific options for the Chromatic Page.


| Entry | Function |
| --- | --- |
| Device | Toggle between MD or External MIDI device |
| Arpeggiator | Opens the Arpeggiator Page |
| Key | Scale is shifted up in semi-tones |
| Polyphony | Opens the PolyPage, for voice selection |
| Quant | Toggle quantiztion for both live record and the Arpeggiator. |


## Ext MIDI Tracks

The Chromatic Page is also used to record/play the External MIDI tracks.
Octave and Scale settings from the Chromatic Page are also applied to incoming note data when using the PianoRoll editor.

## Ext MIDI Track Selection & Mutes

When opening the Track Menu **[Global]** from within either the PianoRoll Editor or Chromatic Page, the MD's **[Trig]** keys can be used to switch between Ext MIDI Tracks and/or mute/unmute them.


- MD **[Trig]** keys 1-6 correspond to Ext MIDI Track selection 1-6.
- MD **[Trig]** keys 9-14 correspond to Ext MIDI Track mutes 1-6.


## Recording a Sequence

Use the MD's **_[Rec]_** + **[**Play**]** function to enable live record mode.


Play notes on either the MD or External Midi to record a melody. The Sequencer menu's QUANT option can be toggled to enable/disable live record quantization.


## Clearing Recorded Sequence


- A recorded track can be quickly cleared using the MD's **[Clear]** function (Repeat to UNDO).
