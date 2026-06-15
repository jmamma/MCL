# Platforms

MCL started on the MegaCommand/MegaCMD hardware controller and now supports multiple platforms.

| Platform | Use |
| --- | --- |
| MegaCommand DIY / MegaCMD | Classic MCL hardware for Machinedrum-centered MIDI setups. |
| TBD | Integrated hardware platform with internal TBD device control, panel control and MCL grid sequencing. |
| Desktop / browser | Development and compatibility testing without dedicated MCL hardware. |

MegaCMD also supports USB disk mode for SD card file transfer. MiniCommand is legacy hardware and is not supported by current MCL releases.

# Terminology and Conventions


- MCL - the project and app/firmware name.
- MD - Machinedrum
- MDX - Machinedrum X OS.
- MC - MegaCommand or MegaCMD MIDI controller.
- **`Button`** Enclosed arrows will reference the use of a MegaCommand function button or encoders to perform an action.
- **[Key]** Enclosed square brackets will reference the use of a Machinedrum key to perform an action.


# Key Concepts


- **Page**


MCL consists of pages accessible through the active platform controls and, in the classic Machinedrum setup, through the MD's GUI. Each page contains distinct functionality and is described in this manual.
- **Project**


A project stored on the Micro SD-Card.
The maximum number of projects is only limited by the SD Card capacity.

- **Grid**


The MCL Firmware uses a Grid & Slot system to store Tracks.

Each project contains two grids, X and Y. Grid dimensions are 16 slots x 128 rows.

Grid X and Grid Y are assigned in `CONFIG > MIDI > DEVICES`. The selected device determines what each slot stores and which editor pages are used. In the classic Machinedrum setup, Grid X stores the 16 Machinedrum tracks and Grid Y stores external MIDI tracks plus auxiliary state such as performance, route, FX and tempo data.
- **Bank:**

The rows of the Grid X are divided in to groups of 16, forming 8 banks A,B,C,D,E,F,G,H.
- **Row/Pattern**


A row of the Grid.

- **Slot**


A position in the Grid where a Track can be stored. (Either occupied or unoccupied).
- **Track**


An MCL sequencer track that may contain both sound and MIDI sequencer data.

Common track and slot data includes:

- **Machinedrum track**: sound settings and sequencer data for one Machinedrum track.
- **External MIDI track**: polyphonic notes, MIDI automation and routing for a MIDI target or supported secondary device.
- **TBD track**: TBD sequence data, sound state and parameter locks when TBD is assigned to a grid.
- **Auxiliary slot data**: performance states, route state, Machinedrum master FX and tempo data where supported by the active device layout.


- **Device**

An attached or internal device selected from `CONFIG > MIDI > DEVICES`. Devices may use MIDI 1, MIDI 2, USB or the internal TBD port, depending on the platform and selected device.
- **Group**


Slots are categorised into groups based on the device they belong to, or a shared characteristic.
