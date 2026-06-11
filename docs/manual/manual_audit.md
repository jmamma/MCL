# Manual Audit

This file tracks the documentation migration and the MCL 5.00 content refresh.

## Migration Findings

Legacy `main.tex` referenced these files, but they were missing during migration. They have since been reconstructed as Markdown concept pages:

- `./TeX_files/chain_settings.tex` -> `sections/chain-settings.md`
- `./TeX_files/grid_slot_system.tex` -> `sections/grid-slot-system.md`
- `./TeX_files/grid_positions.tex` -> `sections/grid-positions.md`

Referenced legacy images not present during migration:

- `page_setup.png`

## MCL 5.00 Refresh Checklist

- [x] Resolve canonical MCL 5.00 release date between top-level and source changelogs. The manual follows the top-level changelog date: June 1, 2026.
- [x] Rewrite key concepts around Grid X/Y devices, TBD/SPSX, desktop/WASM, and project format changes.
- [x] Update MIDI, controller, sync, route, MD, and system configuration labels from `resource/menu_layouts.cpp` and `resource/menu_options.cpp`.
- [x] Update project load/save documentation for folders, cloning, moving, versions, and project config.
- [x] Update grid and slot menu documentation, including the `SOUND` slot option.
- [x] Update sequencer documentation for swing, mute masks, fill conditions, current condition labels, signed microtiming, and SPSX behavior.
- [x] Update LFO and arp documentation for per-track storage and new destinations/options.
- [x] Update polyphony/chromatic documentation for `POLY MODE` and multi-timbral behavior.
- [ ] Update mixer/performance documentation for mute/fill modes and performance-state fill storage.
- [ ] Update piano roll documentation for selection, zoom, copy/paste, and expanded MIDI locks.
- [ ] Update sample browser, sound browser, WAV designer, RAM, and MD samplebank-linking sections.
- [ ] Replace stale screenshots with SPS/MCL headless captures where possible.

## Organisation Notes

- Foundation pages were rewritten for MCL 5.00 and no longer depend on legacy PDF screenshots.
- The `GUI` section is long and still reads like a converted reference dump; it should probably be split into controller basics, Machinedrum enhanced mode, and shortcuts.
- The grid workflow has been split into Grid Slot System, Grid Positions, Grid Page, Slot Menu, Save Page, Load Page, and Chains And Queues.
- The sequencer workflow now has a high-level concept page, shared Sequencer Pages reference, and Step Editor page. PianoRoll, Chromatic, Polyphony, Arp and LFO still need their own MCL 5.00 refresh passes.

## Code Truth Sources

- `Changelog`
- `src/mcl/Changelog`
- `resource/menu_layouts.cpp`
- `resource/menu_options.cpp`
- `src/mcl/MCL/PageIndex.h`
- `src/mcl/MCL/MCLDefines.h`
- `src/mcl/MCL/MCLSysConfig.h`
- `src/mcl/MCL/Sequencer`
