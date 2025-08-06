# MegaCommand Live (MCL)

A high-performance MIDI sequencer and live performance tool written in C/C++ that enhances the capabilities of the MachineDrum and other electronic instruments.

## Features

- Real-time MIDI sequencing with low latency and no jitter
- Advanced performance controls for live electronic music
- Seamless integration with Elektron MachineDrum
- Cross-platform firmware support

## Platform Support

MCL was restructured in 2025 to support multiple hardware platforms through the PlatformIO build system. This allows the same codebase to run on different microcontroller architectures.

**Current platforms:**
- **AVR** - MegaCommand DIY and MegaCMD devices
- **RP2350** - TBD
- **RP2040**

## Quick Start

### Download Pre-built Firmware
Visit the [releases page](https://github.com/jmamma/MCL/releases) for:
- Latest firmware binaries
- User documentation
- Installation instructions

### Building from Source and Uploading

1. **Install PlatformIO**
   ```bash
   pip install platformio
   ```

2. **Compile and upload for your device:**

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
- [ArduinoPico](https://github.com/earlephilhower/arduino-pico)
- [MegaCore](https://github.com/MCUdude/MegaCore)
- [MIDICtrl Framework](https://github.com/wesen/mididuino) by Manuel Odendahl
- [SdFat Library](https://github.com/greiman/SdFat) by Bill Greiman
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library) by Adafruit
