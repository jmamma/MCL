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
    $name = $line.Substring(31).Replace(".hidden ", "").Trim()
    if ($Size -eq "00000000") {
      return
    }
    return [PSCustomObject]@{
      Location = $loc
      Flags = $flags
      Segment = $seg
      Size = $size
      Name = $name
    }
  } catch {}
}

Set-Location sketch
$lines = $(avr-objdump -x sketch.MIDICtrl20_MegaCommand.avr.mega.elf)
$header = $lines | Select-Object -First 37
$symbols = $lines | Select-Object -Skip 38 | %{ parse-symbol($_) }

$text = $symbols | Where-Object -Property Segment -eq ".text" | Sort-Object -Descending -Property Size
$bss = $symbols | Where-Object -Property Segment -eq ".bss" | Sort-Object -Descending -Property Size
$data = $symbols | Where-Object -Property Segment -eq ".data" | Sort-Object -Descending -Property Size

$lines | Out-File "sym_objdump.txt"
$header | Out-File "sym_header.txt"
echo "There are $($text.Count) .text objects"
$text | Out-GridView 
echo "There are $($bss.Count) .bss objects"
$bss | Out-GridView 
echo "There are $($data.Count) .data objects"
$data | Out-GridView

Set-Location ..
