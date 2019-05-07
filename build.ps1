$pattern = "#pragma message:" 

Write-Host "============> Build started."

$buildOutput = &{
    arduino compile --warnings default -b MIDICtrl20_MegaCommand:avr:mega .\sketch\
} 2>&1 | ForEach-Object { 
    $content = $_.ToString()
    if ($content.Contains("pragma message")) {
        Write-Host $_ -ForegroundColor Green
    } elseif ($content.Contains("overflow")){
        Write-Host $_ -ForegroundColor Red
    } elseif ($content.Contains("invalid conversion")) {
        Write-Host $_ -ForegroundColor DarkGray
    }else{
        Write-Host $_
    }
    $_ 
} | Select-String $pattern

Write-Host "============> Build complete."

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
#arduino upload -b MIDICtrl20_MegaCommand:avr:mega .\sketch\ -pCOM4
