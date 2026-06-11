# Load Page

The Load Page is used to load tracks from slots in the current row of the active Grid. When a slot is loaded, Machine/Sound data is sent to the corresponding MIDI device and the sequencer data is loaded into the matching sequencer track.


![load from a](../assets/images/load_from_a.png)


_The Load Page is accessible from the GridPage by pressing the MD's **[Enter/Yes]** key._

| Control | Assignment |
| --- | --- |
| Encoder 1 | Load MODE: [ MAN, AUT, QUE ] |
| Encoder 2 | Length Override: [ -, N ] |
| Encoder 3 | -- |
| Encoder 4 | Quantization |
| Save / No | Cancel Load |
| Page | -- |
| Load / Yes | -- |
| Shift | Group Select |


_The Length and Quantization encoders can be toggled between exponential and incremental value changes by holding down the encoder button when rotating._


The MD's **[Bank A/B/C]** keys can be used to quickly change the load MODE setting between [ MAN, AUT, QUE ].


## Loading Individual Tracks

The Load Page utilises the MD's **[Trig]** keys to specify which slots are to be loaded. Pressing and then releasing multiple **[Trig]** keys will load the corresponding slots from the current row of the visible Grid.

## Grid Toggle

The **[Scale]** button can be used to toggle loading between Grid X, the 16 MD tracks, or Grid Y, the EXT (1-6) and AUX (12-16) tracks.
**[Trig]** keys.

## Simultaneous Load from Grid X and Grid Y

It is possible to simultaneously load a collection of tracks from both Grids X and Y.


- First select the tracks from Grid X pressing and holding the corresponding **[Trig]** keys.
- Tap **[Global]** to switch grids
- Release the **[Trig]** selection
- Select tracks from the alternate Grid Y pressing and holding the corresponding **[Trig]** keys.
- Finally release the second grid **[Trig]** selection to confirm the action.


## Load Track Groups

When in the Save or Load Page, holding the MD's **[Enter/Yes]** key opens the Group Select menu, allowing you to load or save all tracks corresponding to a group. An entire row (pattern) including tracks across both Grids X + Y can be loaded or saved this way.


![group select page](../assets/images/group_select_page.png)


There are five groups:


- MACHINEDRUM _(Grid X tracks 1-16) + MDFX (**FX** = Grid Y track 13)_
- EXT MIDI DEVICE (A4/MNM/Generic MIDI) _(Grid Y tracks 1-6)_
- PERF _(**PF** = Grid Y track 12) + LFOTrack (**LF**= Grid Y track 15)_
- RouteTrack _(**RT** = Grid Y track 14)_
- TEMPO _(**TP** = Grid Y track 16)_


From the Group Select Menu each group can be enabled/disabled using the MD's **[Trig]** keys 1-5.


Releasing **[Enter/Yes]** will load tracks corresponding to the active groups.


## Playback Queue

Each column of the grid has a dedicated Playback Queue.

Up to 8 slots can be queued in each column. The slots in the queue form a repeating chain, each slot will be loaded sequentially. When the end of the queue is reached the first slot is loaded again.

Queued slots are drawn with inverse colours on the Grid Page.

The Playback Queue for any column can be activated by setting the load MODE to QUE and then loading a slot in that column.


## LOOP & JUMP

Each slot contains a LOOP & JUMP setting which can be adjusted via the Grid page's Slot Menu.

The LOOPS value specifies how many times a track should repeat before it transitions to the slot located at the the bank & row specified by JUMP.


The LOOP & JUMP settings only applies when a slot is loaded with load MODE set to: Auto (AUT). Auto mode is a useful way to create preset arrangement or songs.


## Load MODE setting

Each column of the grid has a dedicated MODE setting. The MODE setting is applied to the corresponding column on load, and can be set to one of the following values:


- Manual (MAN): The selected slots will be loaded at the next transition interval. Existing playback queues in matching columns will be discarded.
- Auto (AUT): The selected slots will be loaded at the next transition interval. If the slots LOOP setting is greater than 0, the column will begin loading slots automatically based on their LOOP/JUMP settings. Existing playback queues in matching columns will be discarded.
- Queue (QUE): Each slot will be added to the corresponding column's playback queue. The slots in the queue will be loaded sequentially and repeat in a loop. A maximum of 8 slots can be queued per column.


As each column has an independent mode setting, it is possible to have some slots loading automatically (AUT) according to their LOOP & JUMP setting, other slots can be looping in an improvised queue (QUE) the remaining slots could be left static via manual load (MAN).


## Slot Length Override

When load mode is set to QUE, the LEN parameter allows overriding the loaded slot's length. This can be useful if you have a short track, of say 4 steps, and instead want it to play for 16, looping throughout. Alternatively if you are loading a group of slots, each with different length, it is possible to force them to all play to the same length.


## Quantization Rules

The quantization rules specify the transition interval for the selected slots.

The minimum quantization value is 2 steps. Quantization values can be changed by adjusting the "Q:" parameter, and increase in powers of 2. Holding down the encoder button will step increment the quantization value.


- 02: Toggle cue on next possible 2nd step
- 04: Toggle cue on next possible 4th step
- 08: Toggle cue on next possible 8th step
- 16: Toggle cue on next possible 16th step
- 32: Toggle cue on next possible 32th step
- 64: Toggle cue on next possible 64th step


## Load Destination


![load destination](../assets/images/load_destination.png)


In Manual mode, pressing the **[ Function ]** key from the Load Page will allow you to specify a DESTINATION track for the next load. The offset track is chosen by pressing a **[ Trig ]** key.

## Track Levels

_To provide consistent level mixing across slot loading, the LEV parameter of a loaded track is never transmitted when the sequencer is running._

This allows the performer to use the LEV parameter to fade the loaded track in and out of the mix. The MD's track's VOL parameter should therefor be used to control the relative track loudness.


_Please read the manual section describing the **Config-->Machinedrum-->Normalize** setting which automatically adjusts a saved track's gain staging for this purpose._


# Additional Loading Methods


## Loading via [Bank] + [Trig]

The Machinedrum's pattern changing functionality has been replicated in MCL. Each row of the Grid can be imagined as a pattern, compromising of slots for each of the MD's 16 tracks, plus Auxiliary slots containing the Master FX settings.


- A combination of **[Bank]** + **[Trig]** will load the corresponding row of the grid.

_Slots are loaded from the chosen row, according to the Load Page's Group Selection settings. If the MD groups is active, this mimics the behaviour of pattern loading on the MD._
- A combination of **[Bank]** + multiple **[Trig]** will chain multiple rows together.

_This is achieved by automatically setting each column's load MODE setting to QUE and then adding slots to each Playback Queue._


## Loading via the Grid Slot Menu

Slots can be loaded directly from the Grid Page's Slot Menu.


- From the Grid Page, pressing and holding the **[Exit/No]** button will open the Slot Menu. The MD's **[Up/Down/Left/Right]** keys can then be used to highlight a selection of slots
- When the Slot Menu is active, selected slots can be loaded by pressing the MD's **[Enter/Yes]** key.
- Slots are loaded according to the current load MODE setting. If more than one row is selected, the load MODE is temporarily set to QUE, and the loaded slots are added to each column's Playback Queue.
- The MD's **[Bank A/B/C]** keys can be used to quickly change the load MODE setting between [ MAN, AUT, QUE ].
