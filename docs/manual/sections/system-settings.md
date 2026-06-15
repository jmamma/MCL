# System Settings

The System menu contains MCL-wide display and configuration-scope options.

Open it from:

```text
CONFIG > SYSTEM
```

## Menu Entries

| Entry | Values | Function |
| --- | --- | --- |
| `DISPLAY` | `INT`, `INT+EXT` | Enables the internal display only, or internal plus external display mirroring. |
| `PROJ CFG` | `OFF`, `ON` | Chooses whether configuration remains global or is stored and loaded with each project. |
| `GRID ENCOD` | `--`, `PERF` | Reassigns the Grid Page encoders to performance-controller behavior when enabled. |

## Display

`DISPLAY` controls whether MCL mirrors the display to an external display endpoint where available.

Use `INT` for normal hardware use. Use `INT+EXT` only for an external display mirror, because mirroring can reduce GUI performance on constrained hardware.

## Project Configuration

`PROJ CFG` controls how configuration is stored.

| Value | Behavior |
| --- | --- |
| `OFF` | Configuration is global. Loading a project does not replace the current system/device configuration. |
| `ON` | Supported configuration is stored with the project and restored when the project loads. |

Project configuration includes device and MIDI-related settings that affect how the grid maps to connected instruments. Use this when different projects require different device layouts.

After upgrading to MCL 5.00, stored configuration may be reset to defaults on first boot. Re-check device layout, MIDI ports, sync, project-configuration mode and any project samplebank link before continuing work.

## Grid Encoder Mode

`GRID ENCOD` changes the Grid Page encoder behavior.

| Value | Behavior |
| --- | --- |
| `--` | Encoders keep their normal grid-navigation behavior. |
| `PERF` | Encoders act as performance controllers from the Grid Page. |
