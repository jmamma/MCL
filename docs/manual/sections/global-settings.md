# Project and Configuration Menu

The Configuration menu is used for project management and global or per-project settings.

Open it from the grid page with:

```text
[Bank Group] + [Global]
```

Use **[Up]** and **[Down]** to move through entries, **[Yes/Enter]** to select, and **[No/Exit]** to leave a menu.

## Top-Level Entries

The top-level menu is named `CONFIG`.

| Entry | Function |
| --- | --- |
| `LOAD PROJECT` | Opens the project browser. |
| `MIDI` | Opens device, port, sync, routing, controller and program-change settings. |
| Device config entries | Dynamic entries for connected devices, such as Machinedrum, Elektron, generic MIDI or TBD. |
| `SYSTEM` | Opens display, project-configuration and grid-encoder settings. |

The device config entries are dynamic. In older documentation these appeared as fixed entries such as `DRIVER 1` and `DRIVER 2`; in MCL 5.00 they are named for the connected driver where possible.

## Project Browser

`LOAD PROJECT` opens the project browser. MCL 5.00 supports folders, so projects can be grouped and nested on the SD card.

The project browser can show:

| Entry type | Meaning |
| --- | --- |
| `[ NEW PROJECT ]` | Create a new project in the current folder. |
| `..` | Move to the parent folder. |
| Folder | Open the folder. |
| Project | Load the project. |

The currently loaded project is marked in the browser.

## File Menu

Hold **[Global]** from the project browser to open the file menu.

| Entry | Function |
| --- | --- |
| `NEW DIR` | Create a folder in the current location. |
| `RENAME` | Rename the selected project or folder where allowed. |
| `MOVE` | Move the selected project or folder to another folder. |
| `CLONE` | Duplicate the selected project or folder. |
| `VERS` | Open project versions for the selected project. |
| `DELETE` | Delete the selected project or folder where allowed. |
| `RECV ALL` / `SEND ALL` | Device/file transfer actions where supported by the current browser context. |

The available actions depend on the selected entry and build options.

## Project Versions

The `VERS` action opens the project version browser. Versions are snapshots of the same project, useful before major edits or before converting an older project.

Typical version actions:

| Action | Function |
| --- | --- |
| `BACKUP` | Create a new version snapshot. |
| Load version | Restore a previous project version. |
| Delete version | Remove a non-active version. |

## New Project

Creating a project opens a text-entry page. If a project or directory with the same name already exists, MCL reports an error instead of overwriting it.

## Project Files

Projects are stored on the SD card with a project master file and grid files. MCL 5.00 adds versioned project and grid headers so older projects can be upgraded safely.

Do not edit project files by hand. Use the project browser, file menu and version browser from MCL.

## Saving Projects

MCL writes project state as slots, grids and configuration are saved. You usually do not need a separate full-project save action after saving the material you are working on.

When `SYSTEM > PROJ CFG` is enabled, project-specific configuration is stored with the project and restored when the project is loaded.
