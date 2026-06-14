# Compare GUI Implementations (Layer 3: Behavioral Equivalence)
# Runs same config on Original (MFC) and Qt engines, compares CSV results

param(
    [string]$ConfigFile = "$PSScriptRoot\smoke_test.icf",
    [string]$ResultsDir = $PSScriptRoot,
    [string]$OriginalBin = "$PSScriptRoot\..\src\msvs11\Release\x64",
    [string]$QtBin = "$PSScriptRoot\..\src\qt\build\Release",
    [string]$DynamoBin = "$PSScriptRoot\..\src\msvs11\Release\x64",
    [int]$LoginTimeout = 60,
    [switch]$KeepResults
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"

function Require-File([string]$Path, [string]$Desc) {
    if (-not (Test-Path $Path)) {
        Write-Error "$Desc not found: $Path"; exit 1
    }
}

function Check-Elevation {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    $p = New-Object Security.Principal.WindowsPrincipal($id)
    if (-not $p.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        Write-Warning "Relaunching elevated..."
        $args = "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`""
        foreach ($kv in $PSBoundParameters.GetEnumerator()) {
            if ($kv.Key -notmatch "^(PipelinePosition|CommandRuntime)$") {
                $args += " -$($kv.Key) `"$($kv.Value)`""
            }
        }
        Start-Process powershell.exe -Verb RunAs -ArgumentList $args -Wait; exit
    }
}

function Get-CSVAllRow([string]$File) {
    if (-not (Test-Path $File)) { return $null }
    $inResults = $false
    foreach ($line in Get-Content $File) {
        if ($line -match "^'Results") { $inResults = $true; continue }
        if (-not $inResults -or $line -match "^'") { continue }
        $fields = $line -split ","
        if ($fields.Count -gt 27 -and $fields[0].Trim() -eq "ALL") { return $fields }
    }
    return $null
}

function Run-Engine([string]$Eng, [string]$Cfg, [string]$Out, [int]$TimeOut) {
    Write-Host "Running $Eng..." -ForegroundColor Yellow
    
    if ($Eng -eq "Original") {
        $exe = Join-Path $OriginalBin "IOmeter.exe"
        $dyn = Join-Path $DynamoBin "Dynamo.exe"
        Require-File $exe "IOmeter.exe"
        Require-File $dyn "Dynamo.exe"

        Get-Process IOmeter,Dynamo -ErrorAction SilentlyContinue | Stop-Process -Force
        Start-Sleep 1

        $p = Start-Process -FilePath $exe -ArgumentList "/c `"$Cfg`" /r `"$Out`" /t $TimeOut" -PassThru
        Start-Sleep 3
        $dp = Start-Process -FilePath $dyn -ArgumentList "-i 127.0.0.1 -m $env:COMPUTERNAME" -PassThru

        $waited = 0
        while (-not $p.HasExited -and $waited -lt $TimeOut) {
            Start-Sleep 2; $waited += 2; Write-Host "  ...${waited}s" -NoNewline
        }
        Write-Host ""
        
        if (-not $dp.HasExited) { Stop-Process -Id $dp.Id -Force -ErrorAction SilentlyContinue }
        if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue; return $false }
        return $true
    }
    else {
        $exe = Join-Path $QtBin "IometerQt.exe"
        $dyn = Join-Path $DynamoBin "Dynamo.exe"
        Require-File $exe "IometerQt.exe"
        Require-File $dyn "Dynamo.exe"

        Get-Process IometerQt,Dynamo -ErrorAction SilentlyContinue | Stop-Process -Force
        Start-Sleep 1

        $p = Start-Process -FilePath $exe -ArgumentList "/c `"$Cfg`" /r `"$Out`" /t $TimeOut" -PassThru
        Start-Sleep 3
        $dp = Start-Process -FilePath $dyn -ArgumentList "-i 127.0.0.1 -m $env:COMPUTERNAME" -PassThru

        $waited = 0
        while (-not $p.HasExited -and $waited -lt $TimeOut) {
            Start-Sleep 2; $waited += 2; Write-Host "  ...${waited}s" -NoNewline
        }
        Write-Host ""
        
        if (-not $dp.HasExited) { Stop-Process -Id $dp.Id -Force -ErrorAction SilentlyContinue }
        if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue; return $false }
        return $p.ExitCode -eq 0
    }
}

function Compare-Field($orig, $qt, [int]$Col) {
    try {
        $ov = [double]$orig[$Col].Trim()
        $qv = [double]$qt[$Col].Trim()
        if ($ov -eq 0 -and $qv -eq 0) { return $true, "Both=0" }
        if ($ov -ne 0) {
            $diff = [Math]::Abs(($qv - $ov) / $ov)
            return ($diff -le 0.01), "O=$ov Q=$qv D=$([Math]::Round($diff*100))%"
        }
        return ([Math]::Abs($qv - $ov) -lt 0.001), "O=$ov Q=$qv"
    } catch {
        return $false, "Parse error"
    }
}

Write-Host ""
Write-Host "=== GUI Behavioral Equivalence Comparison ===" -ForegroundColor Cyan
Write-Host ""

Require-File $ConfigFile "Config file"
Check-Elevation

$origFile = Join-Path $ResultsDir "results_${timestamp}_original.csv"
$qtFile = Join-Path $ResultsDir "results_${timestamp}_qt.csv"

Write-Host "Config  : $ConfigFile"
Write-Host ""
Write-Host "Step 1: Running engines..." -ForegroundColor Cyan
Write-Host ""

$origOK = Run-Engine "Original" $ConfigFile $origFile ($LoginTimeout + 30)
Write-Host ""
$qtOK = Run-Engine "Qt" $ConfigFile $qtFile ($LoginTimeout + 30)
Write-Host ""

if (-not ($origOK -and $qtOK)) { Write-Host "FAIL: Engines failed" -ForegroundColor Red; exit 1 }

Write-Host "Step 2: Comparing results..." -ForegroundColor Cyan
Write-Host ""

$origRow = Get-CSVAllRow $origFile
$qtRow = Get-CSVAllRow $qtFile
if (-not ($origRow -and $qtRow)) { Write-Host "FAIL: Missing rows" -ForegroundColor Red; exit 1 }

$passed = 0
$tests = @(
    @{ Name = "IOps"; Col = 6 }
    @{ Name = "MBps"; Col = 12 }
    @{ Name = "Errors"; Col = 27 }
)

foreach ($t in $tests) {
    $ok, $msg = Compare-Field $origRow $qtRow $t.Col
    if ($ok) {
        Write-Host "  [PASS] $($t.Name): $msg" -ForegroundColor Green; $passed++
    } else {
        Write-Host "  [FAIL] $($t.Name): $msg" -ForegroundColor Red
    }
}

Write-Host ""
if ($passed -eq $tests.Count) {
    if (-not $KeepResults) {
        Remove-Item -Path $origFile -Force -ErrorAction SilentlyContinue
        Remove-Item -Path $qtFile -Force -ErrorAction SilentlyContinue
    }
    Write-Host "PASS: Engines equivalent ($passed/$($tests.Count))" -ForegroundColor Green
    exit 0
} else {
    Write-Host "FAIL: Engines differ ($passed/$($tests.Count))" -ForegroundColor Red
    Write-Host "  Original: $origFile"
    Write-Host "  Qt      : $qtFile"
    exit 1
}
