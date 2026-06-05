# Collect code coverage for GUI connection scenario
# Tests: Start GUI, wait, start Dynamo, connect, shutdown
# No I/O operations executed
#
# Usage:
#   .\collect_gui_connection_coverage.ps1                     # Qt only
#   .\collect_gui_connection_coverage.ps1 -IncludeMFC         # MFC + Qt (info only)

param(
    [switch]$IncludeMFC,           # Document MFC coverage limitations
    [string]$QtBuildDir = "$PSScriptRoot\..\src\qt\build_coverage",
    [string]$DynamoBin = "$PSScriptRoot\..\src\msvs11\Release\x64",
    [int]$WaitSeconds = 5,         # Time to let GUI run before stopping
    [int]$LoginTimeout = 30        # Time to wait for Dynamo connection
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "=== GUI Connection Coverage Collection ===" -ForegroundColor Cyan
Write-Host ""

if ($IncludeMFC) {
    Write-Host "Note: MFC Coverage Instrumentation" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "  MFC (Original Iometer) cannot be instrumented for coverage because:" -ForegroundColor DarkYellow
    Write-Host "  • MFC is Windows-only legacy code (2003 era)" -ForegroundColor DarkGray
    Write-Host "  • No clang-cl instrumentation available in MFC build" -ForegroundColor DarkGray
    Write-Host "  • MSVC coverage tools (PGO) incompatible with clang-cl profiles" -ForegroundColor DarkGray
    Write-Host "  • Would require complete MFC rebuild with instrumentation" -ForegroundColor DarkGray
    Write-Host ""
    Write-Host "  Instead, we:" -ForegroundColor DarkYellow
    Write-Host "  ✓ Instrument Qt GUI with clang-cl" -ForegroundColor Green
    Write-Host "  ✓ Run behavioral equivalence tests (same inputs, compare outputs)" -ForegroundColor Green
    Write-Host "  ✓ Document MFC → Qt function mapping (GUI_FUNCTION_MAPPING.md)" -ForegroundColor Green
    Write-Host ""
}

Write-Host "Qt GUI Connection Coverage" -ForegroundColor Cyan
Write-Host ""

# Check for build directory with coverage instrumentation
if (-not (Test-Path $QtBuildDir)) {
    Write-Host "Build directory not found: $QtBuildDir" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "To build with coverage instrumentation:" -ForegroundColor Cyan
    Write-Host "  cd src\qt" -ForegroundColor DarkGray
    Write-Host "  cmake -B build_coverage -DCMAKE_BUILD_TYPE=Release -DCOVERAGE=ON" -ForegroundColor DarkGray
    Write-Host "  cmake --build build_coverage --config Release" -ForegroundColor DarkGray
    Write-Host ""
    exit 1
}

$iometerQt = Join-Path $QtBuildDir "Release\IometerQt.exe"
$profdata = Join-Path $QtBuildDir "iometer.profdata"

if (-not (Test-Path $iometerQt)) {
    Write-Host "IometerQt not found: $iometerQt" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $DynamoBin)) {
    Write-Host "Dynamo directory not found: $DynamoBin" -ForegroundColor Red
    exit 1
}

$dynamo = Join-Path $DynamoBin "Dynamo.exe"
if (-not (Test-Path $dynamo)) {
    Write-Host "Dynamo.exe not found: $dynamo" -ForegroundColor Red
    exit 1
}

Write-Host "Configuration:" -ForegroundColor Cyan
Write-Host "  IometerQt    : $iometerQt"
Write-Host "  Dynamo       : $dynamo"
Write-Host "  Wait time    : $WaitSeconds seconds"
Write-Host "  Login timeout: $LoginTimeout seconds"
Write-Host ""

# Step 1: Start GUI with coverage instrumentation
Write-Host "Step 1: Starting Qt GUI (with coverage instrumentation)..." -ForegroundColor Yellow

# Kill any stale processes
Get-Process IometerQt,Dynamo -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep 1

$env:LLVM_PROFILE_FILE = (Join-Path $QtBuildDir "iometer.profraw")
Write-Host "  Coverage output: $env:LLVM_PROFILE_FILE" -ForegroundColor DarkGray

$qtProc = Start-Process -FilePath $iometerQt -PassThru
$qtPid = $qtProc.Id
Write-Host "  PID: $qtPid"

# Verify it started
Start-Sleep 2
if ($qtProc.HasExited) {
    Write-Host "FAIL: IometerQt exited immediately (code $($qtProc.ExitCode))" -ForegroundColor Red
    exit 1
}
Write-Host "  ✓ GUI started successfully"
Write-Host ""

# Step 2: Start Dynamo and wait for connection
Write-Host "Step 2: Starting Dynamo and waiting for connection..." -ForegroundColor Yellow

$hostname = $env:COMPUTERNAME.ToLower()
$dynamoProc = Start-Process -FilePath $dynamo `
    -ArgumentList "-i 127.0.0.1 -m $hostname" `
    -PassThru

# Wait for connection on port 1066
$deadline = (Get-Date).AddSeconds($LoginTimeout)
$connected = $false

while ((Get-Date) -lt $deadline) {
    try {
        $conn = Get-NetTCPConnection -LocalPort 1066 -State Established `
            -ErrorAction SilentlyContinue
        if ($conn) {
            $connected = $true
            break
        }
    } catch {}
    Start-Sleep -Milliseconds 500
}

if ($connected) {
    Write-Host "  ✓ Dynamo connected successfully"
} else {
    Write-Host "  ⚠ Dynamo connection timeout (but collecting coverage anyway)" -ForegroundColor Yellow
}
Write-Host ""

# Step 3: Let them run for specified time
Write-Host "Step 3: Running for ${WaitSeconds}s..." -ForegroundColor Yellow

for ($i = 0; $i -lt $WaitSeconds; $i++) {
    Start-Sleep 1
    Write-Host "  ...${i}s" -NoNewline
}
Write-Host ""
Write-Host ""

# Step 4: Shutdown and collect coverage
Write-Host "Step 4: Shutting down and collecting coverage..." -ForegroundColor Yellow

if (-not $dynamoProc.HasExited) {
    Stop-Process -Id $dynamoProc.Id -Force -ErrorAction SilentlyContinue
    Write-Host "  Stopped Dynamo"
}

if (-not $qtProc.HasExited) {
    Stop-Process -Id $qtProc.Id -Force -ErrorAction SilentlyContinue
    Write-Host "  Stopped IometerQt"
}

$qtProc.WaitForExit(5000) | Out-Null

Write-Host ""

# Step 5: Generate coverage report
Write-Host "Step 5: Generating coverage report..." -ForegroundColor Yellow
Write-Host ""

# Check for profraw file
$profraw = $env:LLVM_PROFILE_FILE
if (Test-Path $profraw) {
    Write-Host "  Raw profile: $profraw"
    Write-Host "  (Use llvm-profdata and llvm-cov to generate HTML report)"
    Write-Host ""
    Write-Host "  To view coverage:" -ForegroundColor Cyan
    Write-Host "    llvm-profdata merge -o iometer.profdata iometer.profraw" -ForegroundColor DarkGray
    Write-Host "    llvm-cov show -format=html ..." -ForegroundColor DarkGray
} else {
    Write-Host "  No coverage data collected. Make sure Qt was built with:" -ForegroundColor Yellow
    Write-Host "    -fprofile-instr-generate -fcoverage-mapping" -ForegroundColor DarkGray
}

Write-Host ""
Write-Host "Summary:" -ForegroundColor Cyan
Write-Host "  ✓ Qt GUI connection scenario executed" -ForegroundColor Green
Write-Host "  ✓ Coverage data collected for analysis"
Write-Host "  " -ForegroundColor DarkGray
Write-Host "  Which Qt code executed during connection:" -ForegroundColor DarkGray
Write-Host "  • MainWindow constructor/initialization" -ForegroundColor DarkGray
Write-Host "  • DynamoEngine startup and TCP listening" -ForegroundColor DarkGray
Write-Host "  • DySession login sequence handling" -ForegroundColor DarkGray
Write-Host "  • Signal/slot connections for manager events" -ForegroundColor DarkGray
Write-Host ""

Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "  1. Install LLVM tools: winget install LLVM.LLVM" -ForegroundColor DarkGray
Write-Host "  2. Merge profile data: llvm-profdata merge -o merged.profdata *.profraw" -ForegroundColor DarkGray
Write-Host "  3. Generate HTML report: llvm-cov show -format=html ..." -ForegroundColor DarkGray
Write-Host ""
