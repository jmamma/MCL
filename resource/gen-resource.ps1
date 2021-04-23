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
  ../compress/bin/Release/netcoreapp3.1/compress.exe "$n.hex" "$n.ez"
  Remove-Item -ErrorAction Ignore patterns.txt
}

function gen_cpp($f) {
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

function parse_symbol($line) {
  $segs = -split $line
  if ($segs.Length -lt 2) {
    return
  }
  $loc = $line.Substring(0, 8)
  $flags = $line.Substring(9, 7).Trim()
  $seg = $line.Substring(17,5).Trim()
  try {
    $size = [int64]("0x"+$line.Substring(23,8).Trim())
    $name = $line.Substring(31)
    $hidden =  $name -match ".hidden"
    $name = $name.Replace(".hidden ", "").Trim()
    if ($Size -eq "00000000") {
      return
    }
    return [PSCustomObject]@{
      Location = $loc
      Flags = $flags
      Segment = $seg
      Size = $size
      Name = $name
      Hidden = $hidden
    }
  } catch {}
}


$Script:h = "#pragma once"
$Script:h += "`n#include <avr/pgmspace.h>"
$Script:h += "`n#include `"MCL.h`""
$Script:h += "`n#include `"MCL_impl.h`""
$Script:resman = ""

function gen_h($f) {
  $n = [System.IO.Path]::GetFileNameWithoutExtension($f)
  $Script:h += "`nextern const unsigned char __R_$n[] PROGMEM;"

  $path = [System.IO.Path]::GetDirectoryName($f)
  $skip = $true
  $syms = @()
  $typs = @{}
  foreach($line in $(avr-objdump -x "$path\$n.o")) {
    if ($line -eq "SYMBOL TABLE:") {
      $skip = $false
    } elseif (-not $skip) {
      $sym = parse_symbol($line)
      if ($sym.Segment -eq ".data" -and $sym.Name -ne ".data") {
        $syms += $sym
      }
    }
  }
  foreach($line in $(Get-Content $f)) {
    if ($line -match ".*=.*") {
      [System.Text.RegularExpressions.Match]$m = [regex]::Match($line, "([^=\[\]]*)(?:\[.*\])?\s* = ")
      $split = -split $m.Groups[1].Value
      $len = $split.Length
      $type = $split[0..($len-2)] -join " "
      $name = $split[$len-1]
      echo "name = $name, type = $type"
      if ($name -eq "") {
        continue
      }
      $typs[$name]=$type
    }
  }

  $Script:h += "`nstruct __T_$n {"
  $total_sz = 0
  foreach($sym in $syms) {
    $name = $sym.Name
    $type = $typs[$name]
    if ($type -eq "") {
      continue
    }
    $size = $sym.Size
    $Script:h += "`n  union {"
    $Script:h += "`n    $type $name[0];"
    $Script:h += "`n    char zz__$name[$size];"
    $Script:h += "`n  };"

    $Script:h += "`n  static constexpr size_t countof_$name = $size / sizeof($type);"
    $Script:h += "`n  static constexpr size_t sizeofof_$name = $size;"
    $total_sz += $size
  }

  $Script:h += "`n  static constexpr size_t __total_size = $total_sz;"

  $Script:h += "`n};`n"
  $Script:resman += "  `n__T_$n *$n;"
  $Script:resman += "  `nvoid use_$n() { $n = (__T_$n*) __use_resource(__R_$n); }"
}

function clean($f) {
  $n = [System.IO.Path]::GetFileNameWithoutExtension($f)
  rm "$n.o", "$n.hex", "$n.ez"
}

Get-ChildItem *.cpp | ForEach-Object {
  compile $_ 
  gen_cpp $_
  gen_h $_
  clean $_
}

$Script:h | Out-File -Encoding utf8 -FilePath "../avr/cores/megacommand/resources/R.h"
$Script:resman | Out-File -Encoding utf8 -FilePath "../avr/cores/megacommand/resources/ResMan.h"
