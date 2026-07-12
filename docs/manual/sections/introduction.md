![MCL logo](../assets/images/mcl_logo_black_short.png)

# Introduction

MCL is a live MIDI sequencing, grid storage and performance-control system for hardware instruments.

MCL began on the MegaCommand hardware platform, with the Machinedrum as the original focus. The project now runs on multiple supported platforms and can control several device types, but the core idea is the same: keep the immediacy of hardware while adding deeper sequencing, recall and live arrangement.

With the Machinedrum, MCL acts as an external brain for sequencing, sound management and performance control. The Machinedrum remains the synthesis engine and front-panel instrument, while MCL adds a larger project structure around it: tracks can be saved, loaded, chained, copied, transformed and performed without being tied permanently to one pattern or kit.

The center of MCL is the grid. A project is organized into rows and slots. A slot can hold track data, sound state or auxiliary performance state, depending on the device and track type. This makes it possible to build songs and live sets from reusable pieces: one row can be a pattern, a transition, a fill, a variation or a complete scene.

MCL's sequencer supports independent track length and speed, parameter locks, microtiming, trig conditions, mutes, slides, fill behavior, per-track LFOs and per-track arpeggiators. Step editing, PianoRoll editing, Chromatic mode and Polyphonic mode provide different ways to program rhythm, automation and melody.

For performance, MCL provides mixer pages, performance controllers, scene-style parameter morphing, audio mute and routing tools, FX pages, RAM recording tools, sample and sound management, a single-cycle waveform designer and USB MIDI support where the platform provides it.

MCL can interact with several kinds of devices:

- **Machinedrum / MDX** for primary drum-machine sequencing, sound management, routing, FX, RAM tools and performance control.
- **External MIDI instruments** for polyphonic sequencing, chromatic play, automation and program changes.
- **Monomachine and Analog Four** as supported secondary devices.
- **TBD-16** hardware platform, including external MIDI device control and `INT` mode for the internal TBD sound engine.

This manual explains those concepts first, then moves into the page-by-page reference.
