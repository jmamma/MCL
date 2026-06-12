# Introduction


## Preface


**_The following document is intended to be read as a user manual for the MegaCommand Live OS. To learn how to build a DIY MegaCommand MIDI controller please see the link below _(Note: DIY MegaCommand hardware documentation: <https://github.com/jmamma/MegaCommand_Design>.)_._**


## Hello

MegaCommand Live (MCL) is a firmware designed for the MegaCommand (MC) MIDI controller that expands the Elektron Machinedrum's (MD) sequencing, sound design and live performance capabilities.


MCL is developed alongside the Machinedrum X OS. Together they create the modern sequencer enhancement for the Elektron Machinedrum. The MD provides the GUI and synthesis components, whilst the sequencing is now performed within the MegaCommand.


The MC's sequencer consists of 22 tracks each with individual length, parameter locks, micro-timing, conditional trigs, slides, mutes and an arpeggiator. Sixteen of these tracks are dedicated to the MD, the remaining six tracks are polyphonic and can be used to drive an additional MIDI device.


Projects are stored on the MC's SD Card. Each project utilises a grid and slot system to save and load tracks. Unlike the MD, tracks are not bound to a specific pattern or Kit, but instead can be loaded freely between different rows (patterns). Track loading can be automated and slots chained together to create dynamic musical phrases or songs.


Other features of the MCL firmware include: Chromatic + Polyphonic Mode, Level Mixer, Performance Controllers, Audio Mute + Routing System, Sample + Sound Management, Single Cycle Waveform Designer, per-track LFOs, FX Pages, Automated RAM recording, USB MIDI, and much more.


A great deal of care has gone in to this project. Both MCL and MDX have been optimised to provide outstanding MIDI and sequencing performance, at TURBO speeds.


We have intended to make the firmware as intuitive as possible, particularly for users already familiar with the MD. Please take your time to read each section of this manual carefully.


Let's get started.

## MCL 5.00 Update

The classic 16 MD tracks + 6 external MIDI tracks layout still works. In MCL 5.00, `Grid X` and `Grid Y` can also be assigned to different devices from `CONFIG > MIDI > DEVICES`.

The selected device decides what each grid slot stores.
