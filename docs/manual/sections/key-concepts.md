# Platforms

MCL started on the MegaCommand/MegaCMD hardware controller and now supports multiple platforms.

| Platform | Use |
| --- | --- |
| MegaCommand DIY / MegaCMD | Classic MCL hardware for Machinedrum-centered MIDI setups. |
| TBD | Integrated hardware platform with internal TBD device control, panel control and MCL grid sequencing. |
| Desktop builds | Development and compatibility testing without dedicated MCL hardware. |

MegaCMD also supports USB disk mode for SD card file transfer. MiniCommand is legacy hardware and is not supported by current MCL releases.

# Terminology and Conventions


- MCL - the project and firmware/runtime name.
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

Each project contains two grids, X and Y. Grid dimensions are 16 Slots x 128 Rows.


Grid X is used to store 16 MD tracks.
Grid Y is used to store 6 External MIDI tracks + 5 AUX tracks.
- **Bank:**

The rows of the Grid X are divided in to groups of 16, forming 8 banks A,B,C,D,E,F,G,H.
- **Row/Pattern**


A row of the Grid.

- **Slot**


A position in the Grid where a Track can be stored. (Either occupied or unoccupied).
- **Track**


An MCL sequencer track that may contain both sound and MIDI sequencer data.

There are 3 types of tracks.


- **MachineDrum Track** (Grid X: Slots 1-16):
A sequencer track for the Machinedrum. Each MD Track contains the Machine's Sound Settings and Sequencer Data.

- **External MIDI Track** (Grid Y: Slots 1-6):
A polyphonic sequencer track used to control a sound module connected via MIDI. Each External MIDI track contains Sequencer Data, and for supported secondary devices sound data is retained.

- **AUX Track** (Grid Y: Slots: 12-16)

An auxiliary track. Used to store/recall one of: Performance Controllers + Performance States, legacy LFO settings, audio Routing, the Machinedrum's master FX settings and Tempo settings.


- **Device**

An attached MIDI device on MIDI port 1 or 2.
- **Group**


Slots are categorised into groups based on the device they belong to, or a shared characteristic.
