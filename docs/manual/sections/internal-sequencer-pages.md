# MCL Sequencer Pages


_The primary Sequencer Pages are accessed from the Page Select menu via the **[Bank Group]** key._ These include:


- **Step Editor** (MD) (PageSelect: 5)
- **PianoRoll Editor** (Ext MIDI) (PageSelect: 7)
- **Chromatic** (MD/Ext MIDI) (PageSelect: 8)


## Track selection

The current track in any sequencer page can be chosen by pressing **[Global]** to enter the Track Menu and modifying the TRACK SELECT option.


For the Step Editor page the current track selection is automatically synced to the Machinedrum. When you change track on the MD the track will change on the MegaCommand.


Alternatively, if a MIDI device is connected to port 2 and you are in either the Chromatic or PianoRoll pages, the track will automatically change to the first Ext MIDI track that is set to the same channel as incoming note data.


From the Chromatic or PianoRoll page, by pressing MD **[Trig]** keys 1-6 whilst the Track menu is open will change the focused Ext MIDI track.

## Track Expansion

When a track's length is extended and the added steps are clear from step or lock data, MCL fills these steps by repeating the initial sequence to meet the new length.

## Live Record

_Live Record mode can be activated from any Sequencer page by holding **[Rec]** and pressing **[Play]** on the MD to activate record mode._

Depending upon the page type, Live Record can be used to capture:


- Trig presses.
- Parameter changes, CC Automation.
- Notes played in Chromatic Mode.
- Notes played, Pitch Bend + Channel Pressure on the PianoRoll Editor page.


_The MD's **[Clear]** function can be used to clear the recorded sequence for the current track._


## Track menu


![track menu](../assets/images/track_menu.png)


From within a sequencer page, the track menu can be opened by holding **[Global]**.
The selected entry is activated upon release.


The track menu consists of the following entries that are common to all Sequencer Pages:


| Entry | Function |
| --- | --- |
| Edit | Change editor mode. |
| Track Select | Select active track. |
| Copy | TRK: copy track, ALL: copy pattern. |
| Clear | TRK: clear track, ALL: clear pattern. |
| Paste | TRK: paste track, ALL: paste pattern. |
| Speed | The current track's playback speed`br` 1x, 2x, 3/2x, 3/4x, 1/2x, 1/4x, 1/8x.`br` Hold `Load` and release `Shift` to change speed of all tracks. |
| Length | Track Length |
| Shift | L/R: shifts the track left/right.`br` L ALL/R ALL: shifts the pattern left/right. |
| Reverse | TRK: reverse the track, ALL: reverse the pattern. |
| Quant | Toggle Live Record and Arpeggiator Quantization. |


Most sequencer pages feature extra parameters in the Track Menu, which will be detailed in subsequent sections of this manual.
