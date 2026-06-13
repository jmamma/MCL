# Polyphony Page

The Polyphony Page assigns primary tracks to MIDI channel groups. Tracks in the same group act as voices for one multi-timbral polyphonic part.

Open it from the Chromatic Page Track Menu:

```text
[Global] > POLYPHONY
```

![voice select page](../assets/images/voice_select_page.png)

On hardware with panel buttons, the Chromatic Page can also open Polyphony with the record/load button chord.

## Core Idea

Each primary track can be assigned to:

- `--` for no poly group
- MIDI channel group `1..16`

Tracks sharing the same channel group form a voice pool. Different channel groups can exist at the same time, so the Machinedrum or other supported primary device can behave as multiple independent polyphonic instruments.

## Controls

| Control | Assignment |
| --- | --- |
| Encoder 1 | Select the channel group for held tracks. |
| **[Trig]** keys | Hold one or more tracks to assign. |
| **[Clear]** | Assign held tracks to `--` / off. |
| **[Yes/Enter]** or **[No/Exit]** | Save group assignments and return. |
| Panel buttons 2/3 | Cycle the selected group down/up where available. |

The page shows all 16 primary tracks. Each cell displays either `--` or the assigned channel group.

## Assigning Voices

1. Open the Polyphony Page.
2. Hold the **[Trig]** keys for the tracks that should act as voices.
3. Turn Encoder 1 to choose a channel group from `1..16`.
4. Release the tracks.
5. Press **[Yes/Enter]** or **[No/Exit]** to save and return.

To remove tracks from polyphonic play, hold them and choose `--`, or press **[Clear]**.

## Playing A Group

When the active Chromatic track belongs to a group, the Chromatic Page shows `PLY` and uses the group's voice pool.

| Input | Behavior |
| --- | --- |
| Internal Chromatic keyboard | Plays the selected track's group. |
| External controller with `POLY MODE = INT+EXT` | Plays the group whose channel matches the incoming MIDI channel. |
| External MIDI track routed to `MD1`-`MD16` | Plays the selected Machinedrum track or its polyphonic voice group from the PianoRoll sequencer. |
| MIDI CC 16-39 on a poly channel | Controls Machinedrum parameters 1-24 across compatible tracks in that group. |

If `POLY MODE` is `INT`, external input does not trigger poly groups.

## Voice Allocation

MCL chooses an available voice from the active group for each new note. If all voices are busy, the oldest active voice can be reused.

Only tracks that can behave as pitched voices are used. On Machinedrum, this means melodic voice-capable tracks or MIDI machines. On TBD, availability depends on the note/pitch behavior exposed by the active sound.

For the most predictable sound, assign tracks with the same or compatible machine/sound type to the same group.

## Parameter Linking

When a track belongs to a poly group, compatible parameter changes are linked across the other tracks in the group.

| Parameter type | Link behavior |
| --- | --- |
| Shared synthesis parameters | Linked when the destination voice uses a compatible machine/sound type. |
| Later Machinedrum parameter pages | Linked where the parameter can safely apply across voices. |
| Mute parameters | Not poly-linked. |

MCL shows a `POLY LINK` popup when a linked parameter update is sent outside the Mixer Page.

## Storage

Poly group assignments are stored with project configuration and with route/poly state where applicable. Save the route/auxiliary group when the current voice grouping should be captured in the grid along with a row.

Older projects using the legacy poly mask/channel format are migrated into channel groups on load.

## Relationship To `POLY MODE`

`POLY MODE` is configured from:

```text
CONFIG > MIDI > CONTROLLER > INPUT
```

| Value | Behavior |
| --- | --- |
| `INT` | Poly groups respond to internal Chromatic playing. |
| `INT+EXT` | Poly groups also respond to external controller input on matching group channels. |

The old `POLY CHAN` option is gone. Use Polyphony Page groups to decide which MIDI channel controls each group.

External MIDI tracks can also use these voice groups internally. Set the PianoRoll Track Menu `CHANNEL` value past `16` to choose an `MD1`-`MD16` route target.
