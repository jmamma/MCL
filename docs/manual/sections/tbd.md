# TBD

TBD uses the same Grid X / Grid Y setup as other MCL devices. Select `TBD` with the `INT` port to control the internal TBD sound engine from MCL. TBD tracks can then be sequenced, saved, loaded, mixed and parameter-locked from the normal grid and editor pages.

## Device Setup

Open the device menu from:

```text
CONFIG > MIDI > DEVICES
```

TBD adds the `TBD` device and the `INT` port to the normal grid setup:

| Setting | Use |
| --- | --- |
| `Grid X = TBD`, port `INT` | Use TBD as the primary step-sequenced device. |
| `Grid Y = TBD`, port `INT` | Use TBD as the secondary MIDI-style device. |
| `Grid X = MD`, port `MIDI 1` | Use a connected Machinedrum as the primary device. |
| `Grid Y = GENER` or `ELEKT` | Use generic MIDI, Monomachine or Analog Four style secondary tracks. |

The selected grid device determines the track types that are created in new projects and the editor pages used for those tracks.

## Grid Tracks

`Grid X = TBD` gives you 16 TBD step tracks in slots 1-16. These use the Step Editor, Mixer, Chromatic Page, Arpeggiator Page and LFO Page.

`Grid Y = TBD` gives you six TBD tracks for PianoRoll sequencing and automation.

TBD grid slots can store:

| Data | Meaning |
| --- | --- |
| Sequence data | Trigs, notes, lengths, loop settings, mutes, conditions and timing. |
| Sound state | The current TBD sound metadata and parameter values for the track. |
| Mixer state | Visible mixer parameters exposed by the current TBD sound. |
| Parameter locks | Step locks for visible TBD audio and mixer parameters, plus note locks for sounds that use per-step pitch. |

## Device UI, Presets And Parameter Strip

Short-tap the top-left button to open the primary TBD sound UI; use the top-right button for the secondary TBD UI when a secondary TBD device is configured. The full-screen view shows parameter windows or the preset browser. While a TBD UI is active, hold top-right to switch between the full-screen view and the bottom four-encoder strip; hold top-left to leave the UI and open Page Select.

In the strip and full-screen parameter views, the four encoders edit the visible TBD parameters for the active track. The strip shows each parameter label and briefly shows the edited value. Arrow keys browse parameter windows in the full-screen view.

The preset window browses machine/preset groups with Encoder 1 and presets with Encoder 2. Press Encoder 2 to load the selected preset into the current TBD track. While the preset window is active, a short top-right tap also loads the selected preset. If the active sequencer track has no TBD sound or no presets are available, the UI shows `NO TRACK` or `NO PRESETS`.

## Panel Controls

TBD panel input is mapped into MCL's page and device model.

| Control | Action |
| --- | --- |
| Top-left button | Open Page Select from normal pages. When the device UI is active or collapsed, short-tap to enter the focused primary device UI. In menus and browsers, use it as back/up. |
| Hold top-left while the device UI is active | Leave the device UI and open Page Select. |
| Top-left + top-right | Open the System menu. |
| Top-right button | Open or control the secondary device UI when a secondary UI device is configured. In local menus it acts as the confirm/enter key. |
| Encoder 1 tap | Open or close the Grid bank popup. |
| Arrow keys | Navigate pages, menus, steps and selected values. |
| Transport keys | Act as Record, Play and Stop. In copy mode they act as Copy, Clear and Paste. |
| Trig pads | Select rows, trigger tracks, edit steps, play notes or select TBD UI tracks depending on the active page. |
| B button | Acts as Scale/page toggle on Grid, Mixer, Save/Load selection views and sequencer pages; otherwise it acts as the legacy menu modifier. |
| Y button | Opens legacy MCL page menus on Grid, Mixer, Save/Load selection views and sequencer pages; otherwise it acts as Function. |

If an expanded device UI is active, TBD device controls get first chance to handle the buttons. If the device UI is collapsed, the active MCL page handles the same buttons.

## Grid Page Differences

On TBD, Save and Load keep the Grid Page visible instead of opening separate full-screen pages.

| Action | Result |
| --- | --- |
| Open Save | Select the current Grid Page slots and save them without leaving the grid. |
| Open Load | Select the current Grid Page slots and load them without leaving the grid. |
| Slot Menu arrows | Move through Slot Menu entries first. Use the normal modifier behavior when adjusting selection geometry. |
| Bank popup | Use the bank popup to jump to rows and keep row-bank feedback visible while selecting. |

The visible grid still follows the same row, slot, bank, chain and queue concepts described in the Grid sections.

## Clock And Tempo

TBD can use the internal clock source in addition to clock from MIDI 1, MIDI 2 or USB.

Configure clock and transport from:

```text
CONFIG > MIDI > SYNC
```

The TBD sync menu uses `CLOCK SRC` and `TRANS SRC`. Available source values are:

| Source | Meaning |
| --- | --- |
| `1` | MIDI 1. |
| `2` | MIDI 2. |
| `USB` | USB MIDI. |
| `INT` | Internal TBD/MCL clock. |

The tempo window edits the internal tempo when MCL is using its own clock source.

From the Grid Page, press **[Function]** to open the tempo window. Turn Encoder 1 to adjust tempo in 1 BPM steps, or press **[Up]** / **[Down]** for 0.1 BPM changes. Hold **[Function]** and tap **Y** to enter tap-tempo mode; after four taps MCL applies the averaged tempo. **[No]** closes the window. The window shows the current tempo and clock source label: `INT`, `EXT1`, `EXT2` or `USB`.
