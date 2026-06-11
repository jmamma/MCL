# Introduction

## About MCL

MegaCommand Live (MCL) is a sequencing, project-management and live-performance system for the MegaCommand family of controllers. It was originally built around the Elektron Machinedrum and now also supports additional devices and runtimes.

MCL 5.00 can target several device classes depending on the build and hardware:

| Target | Role |
| --- | --- |
| Machinedrum / MDX / SPS-X | Primary drum-machine sequencing, sound management, performance control and extended sequencing features. |
| Monomachine and Analog Four | Secondary Elektron device sequencing and grid storage where supported. |
| Generic MIDI devices | External MIDI sequencing, chromatic play, piano roll editing and controller forwarding. |
| TBD | Integrated CTAG TBD device control and sequencing on supported builds. |
| Desktop / WASM runtime | Simulation, development and SPS-hosted use cases. |

## Before Upgrading

MCL 5.00 introduces a new project format. Existing projects are upgraded automatically on first load and cannot be opened again by older MCL versions after conversion.

Before installing MCL 5.00:

- Back up every project on the SD card.
- Upgrade the Machinedrum to OS X.13.
- Upgrade the Monomachine to OS X.01 if you use one with MCL.
- Expect saved configuration to be reinitialised on first boot after upgrading.

## What MCL Adds

MCL extends connected instruments with:

- Grid-based project storage for tracks, rows and device state.
- Independent Grid X and Grid Y device assignments.
- Step sequencing with parameter locks, microtiming, slides, mutes, swing masks and conditional trigs.
- Per-track LFO and arpeggiator settings.
- Polyphonic and chromatic play modes.
- Project folders, project cloning/moving and project versions.
- Mixer and performance pages for mutes, fills, levels and performance controller states.
- Sample, sound, RAM and WAV tools for the Machinedrum workflow.
- USB MIDI, MIDI routing, controller input and program-change driven loading.

## Manual Status

The manual is now maintained as Markdown inside the MCL source repository. Some screenshots still come from the legacy PDF manual and will be replaced after the SPS/MCL headless screenshot workflow is added.
