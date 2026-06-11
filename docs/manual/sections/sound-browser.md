# Sound Manager Page

The Sound Manager saves and loads Machinedrum sound presets. It is separate from the Sample Manager: sounds store machine setup and parameters, while samples store audio data.

Open it from a Machinedrum step track:

**[Global] > SOUND**

## What A Sound Contains

A sound preset stores the active Machinedrum track's machine assignment and parameters. If the track uses a Trig Group to trigger another track, the linked track's machine settings are stored with it so the pair can be recalled together.

Sound files use the `.snd` extension and are stored under:

`/Sounds`

## Browser Controls

| Control | Action |
| --- | --- |
| **[Up]** / **[Down]** | Move through folders and `.snd` files. |
| **[Yes]** / **[Load/Yes]** | Enter a folder, save from `[SAVE]`, or load the selected sound. |
| **[No]** | Cancel or return from the browser. |
| Hold **[Global]** | Open the file menu. |

The Sound Manager filters the file list to `.snd` files and folders.

## Saving A Sound

1. Select the Machinedrum track you want to capture.
2. Open the Sound Manager from the Track Menu.
3. Select `[SAVE]`.
4. Enter a sound name.

MCL suggests a short name from the kit name and machine type. The sound is written to the current `/Sounds` folder.

## Loading A Sound

Select a `.snd` file and press **[Yes]**. MCL loads the sound into the current Machinedrum track.

If the sound contains a linked Trig Group pair, MCL also loads the linked track data so the paired sound is restored together.

## File Menu

Hold **[Global]** to create folders, rename sounds or delete entries. Bulk sample-transfer actions are intentionally not available here; use the Sample Manager for WAV/SYX transfer.
