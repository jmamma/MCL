# Concepts and Platforms

## Hardware and Runtimes

MCL runs on more than one target. The exact menus and device options depend on the build.

| Platform | Notes |
| --- | --- |
| MegaCommand DIY | AVR-based DIY controller. |
| MegaCMD | Pre-built MegaCommand hardware with USB disk support. |
| TBD | RP2350-compatible CTAG TBD platform with integrated TBD sound/device control. |
| Desktop / WASM | Development, simulation and SPS-hosted runtimes. |

In this manual, **MC** means the MegaCommand-style controller surface unless a section is specifically describing TBD or desktop behavior.

## Naming Conventions

| Term | Meaning |
| --- | --- |
| MCL | MegaCommand Live firmware/software. |
| MD | Elektron Machinedrum. |
| MDX / SPS-X | Enhanced Machinedrum firmware variants used with MCL features. |
| MNM | Elektron Monomachine. |
| A4 | Elektron Analog Four. |
| TBD | CTAG TBD platform supported by MCL 5.00. |
| **`Button`** | A MegaCommand function button or encoder action. |
| **[Key]** | A key on the Machinedrum or connected instrument. |

## Pages

MCL is organised into pages. Each page focuses on a task such as grid navigation, step editing, chromatic play, mixer control, project loading or device configuration.

The Page Select menu is the fastest way to move between pages. Some devices also expose shortcuts from their own keys or panel controls.

## Projects

A project is the top-level saved work area on the SD card. MCL 5.00 projects can be placed in folders and can have multiple saved versions.

Important project behavior:

- Projects are upgraded to the MCL 5.00 format on first load.
- Converted projects are not backward-compatible with older MCL versions.
- Project versions let you keep snapshots without duplicating the whole project manually.
- The `PROJ CFG` system option controls whether configuration is global or follows the project.

## Grid Model

MCL stores musical material in a grid of rows and slots.

| Concept | Meaning |
| --- | --- |
| Grid | A logical storage area for device tracks and related state. |
| Grid X | The primary grid axis. Commonly the Machinedrum or TBD, depending on build and configuration. |
| Grid Y | The secondary grid axis. Commonly a generic MIDI device, Elektron device, TBD, or disabled. |
| Row | A horizontal grid position. Rows are commonly treated like patterns. |
| Bank | A group of 16 rows. The 128 rows form eight banks. |
| Slot | A position where one track or state item can be stored. |

MCL 5.00 no longer treats the grid as a fixed "16 MD tracks plus six external tracks" layout in every build. Instead, Grid X and Grid Y are configured from `CONFIG > MIDI > DEVICES`, and each grid chooses its device and port.

## Devices and Ports

A device is an instrument or driver assigned to a grid. A port is the MIDI or internal connection used by that device.

Examples:

- Grid X can be assigned to the Machinedrum on MIDI Port 1.
- Grid Y can be assigned to an Elektron device or generic MIDI target on MIDI Port 2.
- On TBD builds, a grid can target the internal TBD device.
- Some builds allow USB as a device port.

Device assignment is configured before working on projects so saved grid slots map to the intended instrument.

## Tracks

A track contains sequence data and, where supported, sound or device state.

Common track types include:

| Track type | Use |
| --- | --- |
| Machinedrum track | MD/SPS-X sequencing, sound data, parameter locks and performance integration. |
| External MIDI track | Notes, automation and MIDI control data for external instruments. |
| Elektron secondary track | Sequencing and stored state for supported Elektron devices. |
| TBD track | TBD sound triggering, parameter locks and internal device control on TBD builds. |
| Auxiliary/state track | Saved state for pages such as performance, routing, FX or tempo where applicable. |

## Sound vs Sequence Loading

The grid slot menu has a `SOUND` option. When `SOUND` is off, a slot can be loaded as sequence-only so the destination sound is not overwritten. This is useful when reusing rhythms or automation with a different sound.
