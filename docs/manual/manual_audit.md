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
- [x] Rewrite key concepts around Grid X/Y devices, TBD and project format changes.
- [x] Update MIDI, controller, sync, route, MD, and system configuration labels from `resource/menu_layouts.cpp` and `resource/menu_options.cpp`.
- [x] Update project load/save documentation for folders, cloning, moving, versions, and project config.
- [x] Update grid and slot menu documentation, including the `SOUND` slot option.
- [x] Update sequencer documentation for swing, mute masks, fill conditions, current condition labels and standardized microtiming.
- [x] Update LFO and arp documentation for per-track storage and new destinations/options.
- [x] Update polyphony/chromatic documentation for `POLY MODE` and multi-timbral behavior.
- [x] Update mixer/performance documentation for mute/fill modes and performance-state fill storage.
- [x] Update piano roll documentation for selection, zoom, copy/paste, and expanded MIDI locks.
- [x] Update sample browser, sound browser, WAV designer, RAM, and MD samplebank-linking sections.
- [x] Add a focused TBD section for the user-facing behavior exposed by the code.
- [x] Restore original manual screenshot references into refreshed Markdown sections.
- [ ] Replace stale screenshots with headless captures where possible.

## Organisation Notes

- Original manual screenshots have been restored where the corresponding image assets exist. Some high-priority menu screenshots have now been replaced with headless captures; the remaining legacy screenshots stay in place until their pages are recaptured.
- The `GUI` section is long and still reads like a converted reference dump; it should probably be split into controller basics, Machinedrum enhanced mode, and shortcuts.
- The grid workflow has been split into Grid Slot System, Grid Positions, Grid Page, Slot Menu, Save Page, Load Page, and Chains And Queues.
- The sequencer workflow now has a high-level concept page, shared Sequencer Pages reference, Step Editor page, and PianoRoll page.

## Screenshot Replacement Pass

Legacy screenshots are back in the manual, but several now show old menu labels or old fixed-layout assumptions. Replace these before treating the screenshot set as current.

| Priority | Section | Legacy image(s) | Status |
| --- | --- | --- | --- |
| High | MIDI Configuration | `midi_menu.png`, `midi_devices_menu.png`, `midi_ports_menu.png`, `midi_sync_menu.png`, `midi_route_menu.png`, `midi_prog_menu.png`, `chromatic_menu.png`, `midi_controller_output.png` | Recaptured top MIDI, Devices, Turbo, Sync, Routing, Program, Controller Input and Controller Output screens. Removed stale `MD MIDI`/`GEN MIDI` example references from the current manual page. |
| High | Project and Configuration Menu | `config_menu_1.png`, `project_menu.png`, `project_file_menu.png`, `project_versions.png`, `new_project.png` | Recaptured current top-level config, project browser, project file menu, version browser and new-project entry screens. |
| High | Page Select and Boot Menu | `page_select_page.png`, `boot_menu.png` | Recaptured Page Select and Boot Menu. Boot capture uses the helper's startup button hold path. |
| High | Grid, Slot, Save and Load | `grid_init_annot.png`, `slot_menu.png`, `range_copy.png`, `save_to_a.png`, `load_from_a.png`, `group_select_page.png`, `load_destination.png` | Kept the annotated grid layout diagram. Recaptured Slot Menu, range selection, Save, Load, Group Selector and Load Destination. |
| High | Sequencer pages | `step.png`, `step_action.png`, `utiming1.png`, `track_menu.png`, `seq_ptc_quant.png` | Recaptured current Step Editor, added-trig state, microtiming overlay, Track Menu and `QUANT` menu entry. |
| High | LFO, Arpeggiator, Chromatic and Polyphony | `lfo*.png`, `arp_page.png`, `chro_menu.png`, `chroma.png`, `chromat_action.png`, `chromatic_menu.png`, `voice_select_page.png` | Recaptured LFO subpages, ARP page, Chromatic page, Chromatic Track Menu, Controller Input `POLY MODE` screen and Polyphony voice-select page. |
| High | Mixer and Performance | `mixer_page_*.png`, `perf*.png` | Recaptured Mixer selector/base/performance-state preview and Performance selector/base/learn/menu/scene-lock screens. Kept legacy `mixer_page_perf_lock.png` because it shows the visible lock marker; headless attempts can reach the page but do not yet reproduce a non-empty lock marker reliably. |
| Medium | Machinedrum configuration | `machinedrum_menu.png`, `machinedrum_import.png` | Recaptured current Machinedrum device menu and import page. |
| Medium | Sound, Sample, RAM and WAV tools | `sound*.png`, `sample*.png`, `ram*.png`, `wav_designer*.png` | Recaptured Sample/Sound browsers, shared file menu, WAV Designer oscillator/waveform/mixer screens and RAM queue screens. Kept legacy `rom_select.png`; a new headless capture needs a UW sample-slot response fixture or real MD/UW state. |
| Medium | Route, Delay and Reverb | `route*.png`, `delay*.png`, `reverb*.png` | Recaptured current Route, route-toggle, Delay A/B and Reverb A/B screens. |
| Keep | Hardware/control diagrams | `megacommand_gui.png`, `machinedrum_gui.png`, `enhanced_mode.png`, `midi_machines*.png` | These remain useful unless the physical control labels change. |

### Capture Status

- Old hardware capture path: `tools/mc_display_mirror.py` receives OLED dumps from a real MegaCommand/MegaCMD over MIDI and saves screenshots manually.
- Headless capture path: the local capture runner writes the MCL framebuffer to a BMP without requiring the MD-connected readiness flag.
- Docs helper: `tools/docs/capture_mcl_screenshot.py` prepares a writable temporary data directory, runs the headless screenshot macro, supports tap/chord/hold/release/encoder/key navigation, supports startup-held buttons for boot-time pages, and writes PNG assets under `docs/manual/assets/images`.
- Capture size convention: manual LCD screenshots are saved as the top `128x32` window. Use `--full-height` or `--frame-height 64` only for TBD-specific or other full-height `128x64` captures.
- Captured in this pass: `project_menu.png`, `config_menu_1.png`, `project_file_menu.png`, `project_versions.png`, `new_project.png`, `page_select_page.png`, `boot_menu.png`, `slot_menu.png`, `range_copy.png`, `save_to_a.png`, `load_from_a.png`, `group_select_page.png`, `load_destination.png`, `midi_menu.png`, `midi_devices_menu.png`, `midi_ports_menu.png`, `midi_sync_menu.png`, `midi_route_menu.png`, `midi_prog_menu.png`, `midi_controller_output.png`, `machinedrum_menu.png`, `machinedrum_import.png`, `step.png`, `step_action.png`, `utiming1.png`, `track_menu.png`, `seq_ptc_quant.png`, `lfo.png`, `lfo_action.png`, `lfo_dest.png`, `lfo_learn.png`, `lfo_offset.png`, `chroma.png`, `chromat_action.png`, `chromatic_menu.png`, `chro_menu.png`, `arp_page.png`, `voice_select_page.png`, `proll.png`, `mixer_page_select.png`, `mixer_page_init.png`, `mixer_page_perfs.png`, `perf.png`, `perf_page.png`, `perf_page_learn.png`, `perf_controller_menu.png`, `perf_page_scene_locks.png`, `route.png`, `route_action.png`, `delay.png`, `delay_action.png`, `delay_action_2.png`, `reverb.png`, `reverb_action.png`, `reverb_action_2.png`, `sample_manager.png`, `sample_browser_page.png`, `file_menu.png`, `sound_manager.png`, `sound_browser_page.png`, `osc_menu.png`, `wav_designer_sine_init.png`, `wav_designer_sin.png`, `wav_designer_tri.png`, `wav_designer_pulse.png`, `wav_designer_saw.png`, `wav_designer_user.png`, `wav_designer_pulse_width.png`, `wav_designer_mixer.png`, `oscmixer_menu.png`, `ram1.png`, and `ram1_action.png`.

## Code Truth Sources

- `Changelog`
- `src/mcl/Changelog`
- `resource/menu_layouts.cpp`
- `resource/menu_options.cpp`
- `src/mcl/MCL/PageIndex.h`
- `src/mcl/MCL/MCLDefines.h`
- `src/mcl/MCL/MCLSysConfig.h`
- `src/mcl/MCL/Sequencer`
