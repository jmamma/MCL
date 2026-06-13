# Grid Slot System

The grid is the main storage and performance model in MCL. Tracks can be saved, loaded, chained and rearranged without treating a whole row as a fixed, monolithic pattern.

## Grid Model

The grid consists of two 16-column storage grids stacked side by side:

| Grid | Width | Typical role |
| --- | --- | --- |
| Grid X | 16 slots per row | Primary device tracks, commonly Machinedrum or TBD. |
| Grid Y | 16 slots per row | Secondary device tracks and auxiliary state, depending on device configuration. |

Each project has 128 rows. Rows are grouped into eight banks of 16 rows.

| Term | Meaning |
| --- | --- |
| Slot | One storage position for a track or state item. |
| Row | A set of slots at the same vertical position. Often used like a pattern. |
| Bank | A 16-row group. Banks A-H cover all 128 rows. |
| Active grid | The grid currently shown or edited, either X or Y. |
| Group | A logical set of slots selected by device or state type. |

## Slot Contents

A slot can store sequence data and, where the device supports it, sound or device state.

Examples:

| Slot content | Stored data |
| --- | --- |
| Machinedrum track | Sequence data, sound/machine state, parameter locks and track link settings. |
| External MIDI track | Notes, automation, MIDI locks, length, loop and link settings. |
| TBD track | TBD sequence data, sound state and parameter locks. |
| Performance / route / tempo state | Page-specific state used by group save/load. |

The exact slot type depends on `CONFIG > MIDI > DEVICES`, the active grid, and the connected driver.

## Grid X And Grid Y

MCL 5.00 uses the configured Grid X / Grid Y device model.

| Grid setup example | Result |
| --- | --- |
| Grid X = `MD`, Grid Y = `GENER` | Machinedrum on the primary grid, generic MIDI tracks on the secondary grid. |
| Grid X = `MD`, Grid Y = `ELEKT` | Machinedrum plus a supported secondary device, such as Monomachine or Analog Four. |
| Grid X = `TBD`, Grid Y = `OFF` | TBD as the primary grid device. |
| Grid X = `MD`, Grid Y = `TBD` | Machinedrum primary with TBD as the secondary grid device. |

Older MCL manuals described Grid X as always 16 Machinedrum tracks and Grid Y as six external tracks plus fixed auxiliary slots. That is still a useful reference for classic Machinedrum projects, but MCL 5.00 resolves the actual meaning of each slot from the active device configuration.

## Row Names

Rows can be named from the Slot Menu. MCL 5.00 treats row names as row-level information shared across grids.

When row data is saved, the row name is taken from a device that participated in the save, and the grid info row displays the name when one exists.

## Sound And Sequence Separation

Grid loading can optionally separate sound data from sequence data.

The Slot Menu has a `SOUND` option:

| Value | Load behavior |
| --- | --- |
| `ON` | Load both sequence and sound/device data where available. |
| `OFF` | Load sequence-only and leave the destination sound unchanged. |

Use sequence-only loading to reuse rhythm, notes, locks or automation without replacing the current sound.

## Group Save And Load

Save and Load pages can act on selected slots or on groups.

Group selection is based on the current configured devices and state groups:

| Group slot | Meaning |
| --- | --- |
| 1 | Primary device group. |
| 2 | Secondary device group. |
| 3 | Performance state group. |
| 4 | Auxiliary or route state group. |
| 5 | Tempo/clock state group. |

The icons shown in the group selector follow the connected devices, so group labels may differ from older screenshots.

## Practical Use

The grid supports layered project building:

- Save a full row for a pattern-like snapshot.
- Save only a few slots for reusable track ideas.
- Load sequence-only when the sound should stay in place.
- Use groups when performing with whole device or state sets.
- Use chains and queues when arranging row changes over time.
