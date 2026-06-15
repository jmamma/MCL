# Machinedrum Configuration

The Machinedrum configuration page controls Machinedrum-specific project and transfer behavior.

![machinedrum menu](../assets/images/machinedrum_menu.png)

Open it from the dynamic device entry in the Configuration menu. When the Machinedrum is assigned as a grid device, the entry is named for the Machinedrum instead of the old fixed `DRIVER 1` or `DRIVER 2` label.

## Requirements

Use Machinedrum OS X.13 with MCL 5.00.

For Machinedrum MK1 units, keep Turbo MIDI at `4x` or lower from the MIDI port/turbo settings.

## Menu Entries

| Entry | Function |
| --- | --- |
| `IMPORT` | Import Machinedrum patterns into the MCL grid. |
| `RAM LINK` | Select `MONO` or `STEREO` RAM page behavior. |
| `NORMALIZE` | Enable or disable automatic MD level normalization when saving tracks. |
| `SAMPLEBANK` | Link the project to a fixed Machinedrum +Drive sample bank, or disable linking with `OFF`. |

## Import

`IMPORT` opens the Machinedrum pattern import page.

![machinedrum import](../assets/images/machinedrum_import.png)

| Field | Function |
| --- | --- |
| `SRC` | Starting source pattern on the Machinedrum. |
| `DEST` | Destination row on the MCL grid. |
| `COUNT` | Number of patterns to import. |
| `RUN` | Start the import. |

Imports can be used to bring existing Machinedrum material into the MCL project grid. MCL 5.00 also upgrades older project data on load where supported.

## RAM Link

`RAM LINK` controls whether the RAM pages act as `MONO` or `STEREO` during recording and playback.

This option moved from the old Page Setup area into the Machinedrum configuration page.

## Normalize

When `NORMALIZE` is set to `AUTO`, saved MD tracks have their track `LEV` raised to 127 while related volume parameters and locks are adjusted to preserve the perceived loudness.

This makes the track level control a predictable maximum-level control during performance. While the sequencer is running, MCL does not transmit the track `LEV` parameter continuously, so the performer can still fade tracks manually from the Machinedrum.

Use `OFF` if saved MD track levels should remain untouched.

## Samplebank

`SAMPLEBANK` links the current project to a Machinedrum +Drive sample bank.

| Value | Behavior |
| --- | --- |
| `OFF` | Do not load a fixed sample bank with the project. |
| `1..128` | Load the selected +Drive sample bank when the project is loaded. |

The linked sample bank is stored with the project configuration.

MCL sends the sample-bank load when the project is loaded and Machinedrum is the primary grid device. The connected Machinedrum must support +Drive sample-bank switching.

## Machinedrum MIDI Input

Machinedrum chromatic, trig and polyphonic input behavior is configured from:

```text
CONFIG > MIDI > CONTROLLER > INPUT
```

The old `POLY CHAN` setting has been replaced by `POLY MODE`.
