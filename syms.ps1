$env:PATH += ";C:\Users\Yatao\AppData\Local\Arduino15\packages\arduino\tools\avr-gcc\7.3.0-atmel3.6.1-arduino7\bin"

function parse-symbol($line) {
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

Set-Location sketch
$lines = $(avr-objdump -x build/MIDICtrl20_MegaCommand.avr.mega\sketch.ino.elf)
$header = $lines | Select-Object -First 37
$symbols = $lines | Select-Object -Skip 38 | %{ parse-symbol($_) }

$text = $symbols | Where-Object -Property Segment -eq ".text" | Sort-Object -Descending -Property Size
$bss = $symbols | Where-Object -Property Segment -eq ".bss" | Sort-Object -Descending -Property Size
$data = $symbols | Where-Object -Property Segment -eq ".data" | Sort-Object -Descending -Property Size

$lines | Out-File "sym_objdump.txt"
$header | Out-File "sym_header.txt"
echo "There are $($text.Count) .text objects"
echo "There are $($bss.Count) .bss objects"
echo "There are $($data.Count) .data objects"

$comp_target = $text | Where-Object -Property Flags -Match "O"
$comp_total_size = 0
foreach ($obj in $comp_target) {
  $comp_total_size += $obj.Size
}

echo "Total compressable object size: $comp_total_size"

$data | Out-GridView
$bss | Out-GridView
Set-Location ..
