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
  #F:\git\vcpkg\installed\x64-windows\tools\brotli\brotli.exe "$n.hex" -o "$n.br"
  ../compress/bin/Release/netcoreapp3.1/compress.exe "$n.hex" "$n.ez"
  rm -ErrorAction Ignore patterns.txt
  rm "$n.o"
}

function gen($f) {
  Write-Host "Generating $f"
  $n = [System.IO.Path]::GetFileNameWithoutExtension($f)
  $path = [System.IO.Path]::GetDirectoryName($f)
  $cpp = "#include `"R.h`""
  $cpp += "`nconst unsigned char __R_$n[] PROGMEM = {";
  $data = [System.IO.File]::ReadAllBytes("$path\$n.ez")
  foreach($b in $data) {
    $cpp += "`n  $b,"
  }
  $cpp += "`n};"
  $cpp | Out-File -Encoding utf8 -FilePath "../avr/cores/megacommand/resources/R_$n.cpp"
}

$h = "#pragma once"
$h += "`n#include <avr/pgmspace.h>"

Get-ChildItem *.cpp | ForEach-Object {
  compile $_ 
  gen $_
  $n = [System.IO.Path]::GetFileNameWithoutExtension($_)
  $h += "`nextern const unsigned char __R_$n[] PROGMEM;"
}

$h | Out-File -Encoding utf8 -FilePath "../avr/cores/megacommand/resources/R.h"
