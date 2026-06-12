# Platforms

MCL can run on several supported platforms. The original platform is the MegaCommand MIDI controller, available as the MegaCommand DIY (2017) and MegaCMD (2021). Both were designed by Justin Mammarella.


The MegaCommand DIY is built upon the Arduino Mega 2560 development board and requires soldering and self assembly. The MegaCMD is a pre-built version based on an SMD design, requiring no user assembly.


The MegaCMD benefits from some additional circuitry that allows USB disk access to the MicroSD card for file transfer between a host computer.


The MegaCommand DIY can be powered via a standard DC power jack whilst the MegaCMD only accepts power via USB.


The MiniCommand (2010) is an older controller developed by Ruin & Wesen that paved the way for the MegaCommand. It is not compatible with the current version of MCL.

# Terminology and Conventions


- MCL - the project and firmware/runtime name. Historically expanded as MegaCommand Live.
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
A sequencer track for the Elektron MD. Each MD Track contains the Machine's Sound Settings and Sequencer Data.

- **External MIDI Track** (Grid Y: Slots 1-6):
A polyphonic sequencer track used to control a sound module connected via MIDI. Each External MIDI track contains Sequencer Data, and for supported Elektron devices sound data is retained.

- **AUX Track** (Grid Y: Slots: 12-16)

An auxiliary track. Used to store/recall one of: Performance Controllers + Performance States, legacy LFO settings, audio Routing, the Machinedrum's master FX settings and Tempo settings.


- **Device**

An attached MIDI device on MIDI port 1 or 2.
- **Group**


Slots are categorised into groups based on the device they belong to, or a shared characteristic.

# MCL 5.00 Updates

The key concepts above describe the classic Machinedrum layout. MCL 5.00 keeps the same grid workflow, but `Grid X` and `Grid Y` can now be assigned to different devices from `CONFIG > MIDI > DEVICES`.

The selected device decides what its grid slots store. For example, Grid X can store Machinedrum or TBD primary tracks, while Grid Y can store external MIDI tracks, supported Elektron tracks, TBD secondary tracks or auxiliary state.

When this manual describes the fixed 16 MD + 6 external track layout, read it as the classic Machinedrum setup.

See [TBD](tbd.md) for TBD setup, panel controls and sequencing behavior.
