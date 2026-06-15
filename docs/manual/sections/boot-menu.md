# Boot Menu

The Boot Menu is accessed by holding the MC's **`Page`** button while powering on the MegaCommand or MegaCMD MIDI controller.

![boot menu](../assets/images/boot_menu.png)

| Entry | Function |
| --- | --- |
| OS UPGRADE | Places the MegaCommand in Serial Mode for MCL firmware upgrade. |
| USB DISK | Shown in the Boot Menu. On MegaCMD and compatible RP2040 builds, including TBD, this enters USB Mass Storage mode for SD-card access. On AVR MegaCommand, the mode is reported as unavailable. |
| EXIT | Exit the menu and boot normally. |

AVR MegaCMD builds also include **`DFU MODE`**, which places the USB microcontroller into DFU mode for USB firmware update.
