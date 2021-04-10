$env:PATH += ";C:\Users\Yatao\AppData\Local\Arduino15\packages\arduino\tools\avr-gcc\7.3.0-atmel3.6.1-arduino7\bin"
function compile($f) {
  Write-Host "Compiling $f"
  $n = [System.IO.Path]::GetFileNameWithoutExtension($f)
  avr-g++ `
    -I../avr/variants/mega `
    -I../avr/cores/megacommand `
    -I../avr/cores/megacommand/Sequencer `
    -I../avr/cores/megacommand/SDCard `
    -I../avr/cores/megacommand/Elektron `
    -I../avr/cores/megacommand/A4 `
    -I../avr/cores/megacommand/Wire `
    -I../avr/cores/megacommand/Wire/utility `
    -I../avr/cores/megacommand/Midi `
    -I../avr/cores/megacommand/MidiTools `
    -I../avr/cores/megacommand/Adafruit-GFX-Library `
    -I../avr/cores/megacommand/Adafruit-GFX-Library/Fonts `
    -I../avr/cores/megacommand/SdFat `
    -I../avr/cores/megacommand/SdFat/SdCard `
    -I../avr/cores/megacommand/SdFat/FatLib `
    -I../avr/cores/megacommand/SdFat/SpiDriver `
    -I../avr/cores/megacommand/SdFat/SpiDriver/boards `
    -I../avr/cores/megacommand/MCL `
    -I../avr/cores/megacommand/SPI `
    -I../avr/cores/megacommand/Adafruit_SSD1305_Library `
    -I../avr/cores/megacommand/CommonTools `
    -I../avr/cores/megacommand/GUI `
    -I../avr/cores/megacommand/MD `
    -I../avr/cores/megacommand/MNM `
    -I../avr/cores/megacommand/uzlib `
    -DF_CPU=16000000L `
    -DARDUINO=10803 `
    -DARDUINO_AVR_MEGA2560 `
    -DARDUINO_ARCH_AVR `
    -D__AVR_ATmega2560__ `
    -DAVR `
    -std=gnu++1z `
    -Os $f -c -o "$n.o"
  avr-objcopy -O binary -j .data "$n.o" "$n.hex"
  ../uzlib-host/Release/compress.exe "$n.hex" "$n.z"
}

Get-ChildItem *.cpp | ForEach-Object { compile $_ }
