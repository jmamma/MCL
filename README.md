# MegaCommand Live (MCL)

A high-performance MIDI sequencer and live performance tool written in C/C++ that enhances the capabilities of the MachineDrum and other electronic instruments.

Visit the [releases page](https://github.com/jmamma/MCL/releases) for:
- Latest firmware binaries
- User documentation
- Installation instructions

## Platform Support

MCL can now be run across different hardware platforms using PlatformIO.

**Current platforms:**
- **AVR** - MegaCommand DIY and MegaCMD devices
- **RP2350** - TBD
- **RP2040**

## Upgrade MCL

1. **Install PlatformIO Core**
   ```bash
   pip install platformio
   ```

2.  **Clone the Repository**

    First, you need a local copy of the MCL repository. If you haven't already, open your terminal, navigate to a directory of your choice, and run the following commands:
    ```bash
    git clone https://github.com/jmamma/MCL.git
    cd MCL
    ```
3.  **Place the MegaCommand in to OS UPGRADE mode**

    Hold down the < Page > button when powering-on the MegaCommand to enter the boot menu.

    Select "OS UPGRADE" to place the MegaCommand in to serial mode.

4.  **Run the Upload Command**

    Choose the command that corresponds to your hardware:

    **MegaCommand DIY:**
    ```bash
    platformio run -t nobuild -t upload -e megacommand_latest
    ```

    **MegaCMD:**
    ```bash
    platformio run -t nobuild -t upload -e megacmd_latest
    ```

    When you execute one of these commands, a script automatically performs the following steps:
    *   Fetches the latest MCL release manifest from this respostiory.
    *   Downloads the latest firmware file for your selected device.
    *   Flashes the downloaded firmware onto your device.

## Developers

Building and uploading MCL from Source:

   **MegaCommand DIY:**
   ```bash
   platformio run -e megacommand -t upload
   ```

   **MegaCMD:**
   ```bash
   platformio run -e megacmd -t upload
   ```

   **TBD:**
   ```bash
   platformio run -e tbd -t upload
   ```

## Architecture

```
.
├── art              Pixel-art and animations
├── build            Release manifest and compiled firmwares. 
├── include          Header files required for building specific libraries
├── resource         Compressable C++ data structures used by MCL
│
├── src
│   ├── mcl          MCL source code
│   ├── platform     Platform specific code
│   └── resources    Compressed C++ data structures
│
└── tools            Various tools for building the firmware
```

The AVR version is built on top of the MegaCore Arduino core, and is extended for relevant platforms in `src/platform/avr`.

The RP2040/RP2350 version is built on top of the arduino-pico core, and is extended for supported hardware in `src/platform/rp2040`.

## Libraries

MCL builds upon proven open-source libraries:
- [ArduinoPico](https://github.com/earlephilhower/arduino-pico) by Earle F. Philhower, III
- [MegaCore](https://github.com/MCUdude/MegaCore) by MCUdude
- [MIDICtrl Framework](https://github.com/wesen/mididuino) by Manuel Odendahl
- [SdFat Library](https://github.com/greiman/SdFat) by Bill Greiman
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library) by Adafruit


















