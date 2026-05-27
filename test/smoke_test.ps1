# Iometer Smoke Test
# Runs a 10-second 64kB sequential read against W:\iobw.tst and verifies results.
# Must be run as Administrator (Dynamo needs elevation for disk enumeration).

param(
    [string]$BinDir      = "$PSScriptRoot\..\src\msvs11\Release\x64",
    [string]$TestDir     = $PSScriptRoot,
    [string]$IcfFile     = "$PSScriptRoot\smoke_test.icf",
    [int]   $LoginTimeout = 60
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# --- Elevation check ----------------------------------------------------------
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
               [Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Warning "Not running as Administrator. Relaunching elevated..."
    Start-Process powershell.exe -Verb RunAs -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`""
    exit
}

# --- Resolve paths ------------------------------------------------------------
$iometer = Join-Path $BinDir "IOmeter.exe"
$dynamo  = Join-Path $BinDir "Dynamo.exe"

foreach ($exe in $iometer, $dynamo, $IcfFile) {
    if (-not (Test-Path $exe)) {
        Write-Error "Required file not found: $exe"
        exit 1
    }
}

# --- Timestamped results file -------------------------------------------------
$stamp      = Get-Date -Format "yyyyMMdd_HHmmss"
$resultFile = Join-Path $TestDir "results_$stamp.csv"

Write-Host ""
Write-Host "=== Iometer Smoke Test ===" -ForegroundColor Cyan
Write-Host "  Config : $IcfFile"
Write-Host "  Results: $resultFile"
Write-Host ""

# --- Kill any stale instances -------------------------------------------------
Get-Process -Name "IOmeter","Dynamo" -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

# --- Launch IOmeter in batch mode ---------------------------------------------
Write-Host "[1/4] Starting IOmeter (batch mode, ${LoginTimeout}s login timeout)..." -ForegroundColor Yellow
$iometerProc = Start-Process -FilePath $iometer `
    -ArgumentList "/c `"$IcfFile`" /r `"$resultFile`" /t $LoginTimeout" `
    -PassThru

Start-Sleep -Seconds 3

# --- Launch Dynamo ------------------------------------------------------------
Write-Host "[2/4] Starting Dynamo (connecting to 127.0.0.1)..." -ForegroundColor Yellow
$hostname   = $env:COMPUTERNAME.ToLower()
$dynamoProc = Start-Process -FilePath $dynamo `
    -ArgumentList "-i 127.0.0.1 -m $hostname" `
    -PassThru

# --- Wait for IOmeter to finish -----------------------------------------------
Write-Host "[3/4] Waiting for test to complete (10s run + login timeout + overhead)..." -ForegroundColor Yellow
$maxWait = $LoginTimeout + 30
$waited  = 0
while (-not $iometerProc.HasExited -and $waited -lt $maxWait) {
    Start-Sleep -Seconds 2
    $waited += 2
    Write-Host "  ...${waited}s" -NoNewline
}
Write-Host ""

if (-not $dynamoProc.HasExited) {
    Stop-Process -Id $dynamoProc.Id -Force -ErrorAction SilentlyContinue
}

if (-not $iometerProc.HasExited) {
    Write-Host "TIMEOUT: IOmeter did not finish within ${maxWait}s. Killing." -ForegroundColor Red
    Stop-Process -Id $iometerProc.Id -Force -ErrorAction SilentlyContinue
    exit 1
}

# --- Verify results -----------------------------------------------------------
Write-Host "[4/4] Verifying results..." -ForegroundColor Yellow

if (-not (Test-Path $resultFile)) {
    Write-Host "FAIL: Results file was not created: $resultFile" -ForegroundColor Red
    exit 1
}

# CSV column indices (0-based) from ManagerList::SaveResults:
#   6  = IOps
#   12 = MBps (Decimal)
#   27 = Errors
$allRow    = $null
$inResults = $false
foreach ($line in Get-Content $resultFile) {
    if ($line -match "^'Results") { $inResults = $true; continue }
    if (-not $inResults)          { continue }
    if ($line -match "^'")        { continue }
    $fields = $line -split ","
    if ($fields.Count -gt 27 -and $fields[0].Trim() -eq "ALL") {
        $allRow = $fields
        break
    }
}

if ($null -eq $allRow) {
    Write-Host "FAIL: No 'ALL' aggregate row found in results file." -ForegroundColor Red
    Write-Host "      File may be incomplete - check $resultFile" -ForegroundColor Red
    exit 1
}

$iops   = [double]$allRow[6].Trim()
$mbps   = [double]$allRow[12].Trim()
$errors = [double]$allRow[27].Trim()

Write-Host ""
Write-Host "  IOps       : $iops"
Write-Host "  MBps (Dec) : $mbps"
Write-Host "  Errors     : $errors"
Write-Host ""

$pass = $true

if ($iops -le 0) {
    Write-Host "FAIL: IOps is $iops (expected > 0)" -ForegroundColor Red
    $pass = $false
}
if ($mbps -le 0) {
    Write-Host "FAIL: MBps is $mbps (expected > 0)" -ForegroundColor Red
    $pass = $false
}
if ($errors -ne 0) {
    Write-Host "FAIL: Error count is $errors (expected 0)" -ForegroundColor Red
    $pass = $false
}

if ($pass) {
    Write-Host "PASS: All checks passed." -ForegroundColor Green
    exit 0
} else {
    Write-Host "FAIL: One or more checks failed. See $resultFile for details." -ForegroundColor Red
    exit 1
}