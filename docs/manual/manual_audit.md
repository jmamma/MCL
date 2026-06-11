# Manual Audit

This file tracks the documentation migration and the MCL 5.00 content refresh.

## Migration Findings

Missing legacy include files from `main.tex`:

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
- [ ] Update grid and slot menu documentation, including the `SOUND` slot option.
- [ ] Update sequencer documentation for swing, mute masks, fill conditions, current condition labels, signed microtiming, and SPSX behavior.
- [ ] Update LFO and arp documentation for per-track storage and new destinations/options.
- [ ] Update polyphony/chromatic documentation for `POLY MODE` and multi-timbral behavior.
- [ ] Update mixer/performance documentation for mute/fill modes and performance-state fill storage.
- [ ] Update piano roll documentation for selection, zoom, copy/paste, and expanded MIDI locks.
- [ ] Update sample browser, sound browser, WAV designer, RAM, and MD samplebank-linking sections.
- [ ] Replace stale screenshots with SPS/MCL headless captures where possible.

## Organisation Notes

- Foundation pages were rewritten for MCL 5.00 and no longer depend on legacy PDF screenshots.
- The `GUI` section is long and still reads like a converted reference dump; it should probably be split into controller basics, Machinedrum enhanced mode, and shortcuts.
- The `Load Page` / chain material is also long enough to split after the project/grid sections are updated.

## Code Truth Sources

- `Changelog`
- `src/mcl/Changelog`
- `resource/menu_layouts.cpp`
- `resource/menu_options.cpp`
- `src/mcl/MCL/PageIndex.h`
- `src/mcl/MCL/MCLDefines.h`
- `src/mcl/MCL/MCLSysConfig.h`
- `src/mcl/MCL/Sequencer`
