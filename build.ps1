param(
  [Switch]
  $ShowStats,
  [Switch]
  $Quiet,
  [Switch]
  $Upload,
  [Switch]
  $Clean,
  [Switch]
  $Debug
)

$pattern = "#pragma message:" 

Write-Host "============> Build started."

$DEBUG_FLAG = ""
$DEBUG_OPT = ""

if ($Debug) {
    $DEBUG_FLAG = "--build-properties"
    $DEBUG_OPT = "compiler.cpp.extra_flags=-DDEBUGMODE"
}

if ($Clean) {
    Remove-Item -ErrorAction SilentlyContinue -Recurse -Force bin
    Remove-Item -ErrorAction SilentlyContinue -Recurse -Force obj
}

$BIN_PATH = [System.IO.Path]::GetFullPath("$PSScriptRoot/bin")
$OBJ_PATH = [System.IO.Path]::GetFullPath("$PSScriptRoot/obj")
$SKETCH_PATH = [System.IO.Path]::GetFullPath("$PSScriptRoot/sketch")

Write-Output $SKETCH_PATH

$buildOutput = & {
  arduino-cli compile `
      --warnings default `
      --build-path $BIN_PATH `
      --build-cache-path $OBJ_PATH `
      -b MIDICtrl20_MegaCommand:avr:mega `
      $DEBUG_FLAG $DEBUG_OPT `
      $SKETCH_PATH
  $Script:compileStatus = $LASTEXITCODE
} 2>&1 | ForEach-Object { 
  $content = $_.ToString()
  if ($content.Contains("pragma message")) {
    if (-not $Quiet) {
      Write-Host $_ -ForegroundColor Green
    }
  } elseif ($content.Contains("overflow") -or $content.Contains("error:")){
      Write-Host $_ -ForegroundColor Red
  } elseif ($content.Contains("invalid conversion") -or $content.Contains("warning:") -or $content.Contains("note:")) {
    if (-not $Quiet) {
      Write-Host $_ -ForegroundColor DarkGray
    }
  }else{
      Write-Host $_
  }
  $_ 
} | Select-String $pattern

if ($Script:compileStatus -eq 0) {
  Write-Host "============> Build complete." -ForegroundColor Green
} else {
  Write-Host "============> Build failed, exit code = $compileStatus." -ForegroundColor Red
  return
}

if ($ShowStats) {
  $bank1 = @{}
  $buildOutput | ForEach-Object {
      $lines = $_.ToString()
      $equation = $lines.Substring($lines.IndexOf($pattern) + $pattern.Length).Split([System.Environment]::NewLine)[0].Trim()
      $equation = $equation.Replace("sizeof(MDTrackLight)", "501").Replace("sizeof(A4Track)", "1742")
      $evaluate = $equation.Replace("sizeof(", '$($').Replace("UL", "").Insert(0, '$')
      
      $variable = $evaluate.Split("=")[0]
      . Invoke-Expression $evaluate
      $value = Invoke-Expression $variable
      $bank1[$variable] = $value
  } -ErrorAction SilentlyContinue
  $bank1.GetEnumerator() | Sort-Object -Property Value | Format-Table
}

if ($Upload) {
  Write-Host "==============> Uploading..." -ForegroundColor Yellow
  arduino-cli upload -b MIDICtrl20_MegaCommand:avr:mega -pCOM4 -i bin/sketch.ino.hex
  Write-Host "==============> Finished." -ForegroundColor Green
}
