# Introduction


## Preface


This document is the user manual for MCL. MCL began as MegaCommand Live for the MegaCommand MIDI controller, but the project now runs on multiple supported platforms.

To build a DIY MegaCommand MIDI controller, see the hardware documentation at <https://github.com/jmamma/MegaCommand_Design>.


## Hello

MCL is a live MIDI sequencing, control and project-management system for the Elektron Machinedrum, TBD and other supported devices.


MCL is developed alongside the Machinedrum X OS. Together they create the modern sequencer enhancement for the Elektron Machinedrum. In the classic Machinedrum setup, the MD provides the GUI and synthesis components, while MCL handles sequencing, grid storage and live control.


The classic Machinedrum layout uses 22 tracks, each with individual length, parameter locks, micro-timing, conditional trigs, slides, mutes and an arpeggiator. Sixteen tracks are dedicated to the MD, and the remaining six tracks are polyphonic tracks for an additional MIDI device.


Projects are stored on the platform's project storage. Each project uses a grid and slot system to save and load tracks. Unlike the MD, tracks are not bound to a specific pattern or Kit, but instead can be loaded freely between different rows (patterns). Track loading can be automated and slots chained together to create dynamic musical phrases or songs.


Other MCL features include: Chromatic + Polyphonic Mode, Level Mixer, Performance Controllers, Audio Mute + Routing System, Sample + Sound Management, Single Cycle Waveform Designer, per-track LFOs, FX Pages, Automated RAM recording, USB MIDI, and much more.


A great deal of care has gone in to this project. Both MCL and MDX have been optimised to provide outstanding MIDI and sequencing performance, at TURBO speeds.


We have intended to make the firmware as intuitive as possible, particularly for users already familiar with the MD. Please take your time to read each section of this manual carefully.


Let's get started.

## MCL 5.00 Update

The classic 16 MD tracks + 6 external MIDI tracks layout still works. In MCL 5.00, `Grid X` and `Grid Y` can also be assigned to different devices from `CONFIG > MIDI > DEVICES`.

The selected device decides what each grid slot stores.
