$pattern = "#pragma message:" 

Write-Host "============> Build started."

$buildOutput = &{
    arduino compile --warnings default -b MIDICtrl20_MegaCommand:avr:mega .\sketch\
} 2>&1 | ForEach-Object { Write-Host $_; $_ } | Select-String $pattern

# pass 1
$buildOutput | ForEach-Object {
    $lines = $_.ToString()
    $equation = $lines.Substring($lines.IndexOf($pattern) + $pattern.Length).Split([System.Environment]::NewLine)[0].Trim()
    $evaluate = $equation.Replace("sizeof(", '$($').Insert(0, '$')
    Invoke-Expression $evaluate
} -ErrorAction SilentlyContinue

Write-Host "============> Build complete."

# pass 2
$bank1 = @{}

$buildOutput | ForEach-Object {
    $lines = $_.ToString()
    $equation = $lines.Substring($lines.IndexOf($pattern) + $pattern.Length).Split([System.Environment]::NewLine)[0].Trim()
    $evaluate = $equation.Replace("sizeof(", '$($').Insert(0, '$')
    $variable = $evaluate.Split("=")[0]
    Invoke-Expression $evaluate
    $value = Invoke-Expression $variable
    $bank1[$variable] = $value
} -ErrorAction SilentlyContinue

$bank1.GetEnumerator() | Sort-Object -Property Value | Format-Table
#arduino upload -b MIDICtrl20_MegaCommand:avr:mega .\sketch\ -pCOM4
