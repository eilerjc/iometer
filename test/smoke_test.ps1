# Iometer Smoke Test
#
# Supports two engines:
#
#   -Engine Original  (default)
#       Runs the classic MFC IOmeter.exe in batch mode against an .icf file,
#       verifies the CSV result file contains IOps > 0 and Errors = 0.
#       Requires elevation (Dynamo needs it for raw-disk access).
#
#   -Engine Qt
#       Tests the Qt GUI replacement (IometerQt.exe).
#       Two sub-modes:
#
#       -Demo   Launch IometerQt.exe --demo (no Dynamo required).
#               Verifies the app starts, stays alive for 15 s, and exits cleanly.
#
#       (default)  Launch IometerQt.exe + Dynamo.exe, verify the TCP login
#               handshake completes (Dynamo stays alive for $LoginTimeout s
#               without early exit), then tear down cleanly.
#               Requires elevation.
#
# Usage examples:
#   .\smoke_test.ps1                                      # original, default dirs
#   .\smoke_test.ps1 -Engine Qt -Demo                     # Qt demo mode
#   .\smoke_test.ps1 -Engine Qt                           # Qt + real Dynamo
#   .\smoke_test.ps1 -Engine Qt -QtBinDir C:\myBuild\Release
#   .\smoke_test.ps1 -BinDir C:\custom\Release\x64

param(
    [ValidateSet("Original", "Qt")]
    [string]$Engine        = "Original",

    # Original engine paths
    [string]$BinDir        = "$PSScriptRoot\..\src\msvs11\Release\x64",
    [string]$IcfFile       = "$PSScriptRoot\smoke_test.icf",

    # Qt engine path
    [string]$QtBinDir      = "$PSScriptRoot\..\src\qt\build\Release",

    # Qt-only flags
    [switch]$Demo,   # use --demo mode (no Dynamo needed)
    [switch]$Batch,  # use batch mode (/c /r /t) — mirrors Original mode exactly

    [string]$TestDir       = $PSScriptRoot,
    [int]   $LoginTimeout  = 60,

    # Qt+Dynamo (connection-only) mode: seconds to keep both alive
    [int]   $QtRunSeconds  = 15
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# --- Helpers ------------------------------------------------------------------

function Require-File([string]$Path) {
    if (-not (Test-Path $Path)) {
        Write-Error "Required file not found: $Path"
        exit 1
    }
}

function Check-Elevation {
    $identity  = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    $isAdmin   = $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
    if (-not $isAdmin) {
        Write-Warning "Not running as Administrator -- relaunching elevated..."
        $argList = "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`""
        foreach ($kv in $PSBoundParameters.GetEnumerator()) {
            $argList += " -$($kv.Key) `"$($kv.Value)`""
        }
        Start-Process powershell.exe -Verb RunAs -ArgumentList $argList
        exit
    }
}

function Wait-PortListen([int]$Port, [int]$TimeoutSec = 20) {
    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    while ((Get-Date) -lt $deadline) {
        $listening = Get-NetTCPConnection -LocalPort $Port -State Listen `
                         -ErrorAction SilentlyContinue
        if ($listening) { return $true }
        Start-Sleep -Milliseconds 500
    }
    return $false
}

function Wait-PortEstablished([int]$Port, [int]$TimeoutSec = 30) {
    # Wait until something connects TO the local port (ESTABLISHED incoming)
    $deadline = (Get-Date).AddSeconds($TimeoutSec)
    while ((Get-Date) -lt $deadline) {
        $conn = Get-NetTCPConnection -LocalPort $Port -State Established `
                    -ErrorAction SilentlyContinue
        if ($conn) { return $true }
        Start-Sleep -Milliseconds 500
    }
    return $false
}

# --- Banner -------------------------------------------------------------------

Write-Host ""
Write-Host "=== Iometer Smoke Test ===" -ForegroundColor Cyan
$modeStr = if ($Engine -eq 'Qt' -and $Demo) { ' (--demo)' } elseif ($Engine -eq 'Qt' -and $Batch) { ' (--batch)' } else { '' }
Write-Host "  Engine : $Engine$modeStr"
Write-Host ""

# --- ORIGINAL ENGINE ----------------------------------------------------------
if ($Engine -eq "Original") {

    Check-Elevation

    $iometer = Join-Path $BinDir "IOmeter.exe"
    $dynamo  = Join-Path $BinDir "Dynamo.exe"

    Require-File $iometer
    Require-File $dynamo
    Require-File $IcfFile

    $startTime  = Get-Date
    $stamp      = $startTime | Get-Date -Format "yyyyMMdd_HHmmss"
    $resultFile = Join-Path $TestDir "results_$stamp.csv"

    Write-Host "  Config : $IcfFile"
    Write-Host "  Results: $resultFile"
    Write-Host ""

    # Kill stale instances
    Get-Process -Name "IOmeter","Dynamo" -ErrorAction SilentlyContinue |
        Stop-Process -Force
    Start-Sleep -Seconds 1

    # [1/4] Start IOmeter in batch mode
    Write-Host "[1/4] Starting IOmeter (batch mode, ${LoginTimeout}s login timeout)..." `
               -ForegroundColor Yellow
    $iometerProc = Start-Process -FilePath $iometer `
        -ArgumentList "/c `"$IcfFile`" /r `"$resultFile`" /t $LoginTimeout" `
        -PassThru
    Start-Sleep -Seconds 3

    # [2/4] Start Dynamo
    Write-Host "[2/4] Starting Dynamo (connecting to 127.0.0.1)..." -ForegroundColor Yellow
    $hostname   = $env:COMPUTERNAME.ToLower()
    $dynamoProc = Start-Process -FilePath $dynamo `
        -ArgumentList "-i 127.0.0.1 -m $hostname" `
        -PassThru

    # [3/4] Wait for IOmeter to finish
    Write-Host "[3/4] Waiting for test (10s run + login timeout + overhead)..." `
               -ForegroundColor Yellow
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
        Write-Host "TIMEOUT: IOmeter did not finish within ${maxWait}s. Killing." `
                   -ForegroundColor Red
        Stop-Process -Id $iometerProc.Id -Force -ErrorAction SilentlyContinue
        exit 1
    }

    # [4/4] Verify results
    Write-Host "[4/4] Verifying results..." -ForegroundColor Yellow

    $found = Get-ChildItem -Path $TestDir -Filter "results_*.csv" `
                 -ErrorAction SilentlyContinue |
             Where-Object { $_.LastWriteTime -ge $startTime } |
             Sort-Object LastWriteTime -Descending |
             Select-Object -First 1

    if ($null -eq $found) {
        Write-Host "FAIL: No results file created after $($startTime.ToString('HH:mm:ss')) in $TestDir" `
                   -ForegroundColor Red
        exit 1
    }

    $resultFile = $found.FullName
    Write-Host "  Found  : $resultFile"

    # CSV column indices (0-based) from ManagerList::SaveResults:
    #   6  = IOps   12 = MBps (Decimal)   27 = Errors
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
        Write-Host "FAIL: No 'ALL' aggregate row in results file." -ForegroundColor Red
        Write-Host "      Check $resultFile"                       -ForegroundColor Red
        exit 1
    }

    $mbps   = [double]$allRow[12].Trim()
    $iops   = [double]$allRow[6].Trim()
    $errors = [double]$allRow[27].Trim()

    Write-Host ""
    Write-Host "  MBps (Dec) : $mbps"
    Write-Host "  IOps       : $iops"
    Write-Host "  Errors     : $errors"
    Write-Host ""

    $pass = $true
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
        Write-Host "FAIL: One or more checks failed. See $resultFile for details." `
                   -ForegroundColor Red
        exit 1
    }
}

# --- QT ENGINE ----------------------------------------------------------------
if ($Engine -eq "Qt") {

    $iometerQt = Join-Path $QtBinDir "IometerQt.exe"
    Require-File $iometerQt

    # Kill any stale IometerQt instance
    Get-Process -Name "IometerQt" -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Seconds 1

    # -- Qt + --demo mode -------------------------------------------------------
    if ($Demo) {
        Write-Host "  Binary : $iometerQt"
        Write-Host "  Mode   : Demo (simulated I/O, no Dynamo)"
        Write-Host ""

        # [1/3] Launch
        Write-Host "[1/3] Launching IometerQt.exe --demo..." -ForegroundColor Yellow
        $qtProc = Start-Process -FilePath $iometerQt `
            -ArgumentList "--demo" `
            -PassThru

        # [2/3] Verify alive after 5 s (catches DLL-not-found and startup crashes)
        Start-Sleep -Seconds 5
        if ($qtProc.HasExited) {
            Write-Host "FAIL: IometerQt exited within 5 s (exit code $($qtProc.ExitCode))." `
                       -ForegroundColor Red
            Write-Host "      Check for missing Qt DLLs or a startup crash."
            exit 1
        }
        Write-Host "  OK: process alive after 5 s (demo engine running)."

        # [2/3] Let the demo engine produce a few update cycles (500 ms each)
        Write-Host "[2/3] Running for ${QtRunSeconds}s..." -ForegroundColor Yellow
        $waited = 0
        while (-not $qtProc.HasExited -and $waited -lt $QtRunSeconds) {
            Start-Sleep -Seconds 2
            $waited += 2
            Write-Host "  ...${waited}s" -NoNewline
        }
        Write-Host ""

        if ($qtProc.HasExited) {
            Write-Host "FAIL: IometerQt crashed during demo run (exit code $($qtProc.ExitCode))." `
                       -ForegroundColor Red
            exit 1
        }

        # [3/3] Tear down cleanly
        Write-Host "[3/3] Terminating IometerQt..." -ForegroundColor Yellow
        Stop-Process -Id $qtProc.Id -Force -ErrorAction SilentlyContinue
        $qtProc.WaitForExit(5000) | Out-Null

        Write-Host ""
        Write-Host "PASS: IometerQt --demo ran for ${QtRunSeconds}s without crashing." `
                   -ForegroundColor Green
        exit 0
    }

    # -- Qt + Batch mode --------------------------------------------------------
    # Mirrors the Original engine path exactly: /c /r /t flags, same CSV parsing.
    # Requires elevation (Dynamo needs raw-disk access).
    if ($Batch) {
        Check-Elevation

        $dynamo = Join-Path $BinDir "Dynamo.exe"
        Require-File $dynamo

        $startTime  = Get-Date
        $stamp      = $startTime | Get-Date -Format "yyyyMMdd_HHmmss"
        $resultFile = Join-Path $TestDir "results_${stamp}_qt.csv"

        Write-Host "  Binary : $iometerQt"
        Write-Host "  Dynamo : $dynamo"
        Write-Host "  Config : $IcfFile"
        Write-Host "  Results: $resultFile"
        Write-Host ""

        # Kill stale instances
        Get-Process -Name "IometerQt","Dynamo" -ErrorAction SilentlyContinue |
            Stop-Process -Force
        Start-Sleep -Seconds 1

        # [1/4] Start IometerQt in batch mode (same flags as original IOmeter.exe)
        Write-Host "[1/4] Starting IometerQt (batch mode, ${LoginTimeout}s login timeout)..." `
                   -ForegroundColor Yellow
        $qtProc = Start-Process -FilePath $iometerQt `
            -ArgumentList "/c `"$IcfFile`" /r `"$resultFile`" /t $LoginTimeout" `
            -PassThru
        Start-Sleep -Seconds 3

        if ($qtProc.HasExited) {
            Write-Host "FAIL: IometerQt exited immediately (code $($qtProc.ExitCode))." `
                       -ForegroundColor Red
            exit 1
        }

        # [2/4] Start Dynamo
        Write-Host "[2/4] Starting Dynamo (connecting to 127.0.0.1)..." -ForegroundColor Yellow
        $hostname   = $env:COMPUTERNAME.ToLower()
        $dynamoProc = Start-Process -FilePath $dynamo `
            -ArgumentList "-i 127.0.0.1 -m $hostname" `
            -PassThru

        # [3/4] Wait for IometerQt to finish (it exits when the test completes)
        Write-Host "[3/4] Waiting for test to complete (run time + login timeout + overhead)..." `
                   -ForegroundColor Yellow
        $maxWait = $LoginTimeout + 60   # ICF run time (10s) + login window + overhead
        $waited  = 0
        while (-not $qtProc.HasExited -and $waited -lt $maxWait) {
            Start-Sleep -Seconds 2
            $waited += 2
            Write-Host "  ...${waited}s" -NoNewline
        }
        Write-Host ""

        if (-not $dynamoProc.HasExited) {
            Stop-Process -Id $dynamoProc.Id -Force -ErrorAction SilentlyContinue
        }

        if (-not $qtProc.HasExited) {
            Write-Host "TIMEOUT: IometerQt did not finish within ${maxWait}s. Killing." `
                       -ForegroundColor Red
            Stop-Process -Id $qtProc.Id -Force -ErrorAction SilentlyContinue
            exit 1
        }

        if ($qtProc.ExitCode -ne 0) {
            Write-Host "FAIL: IometerQt exited with code $($qtProc.ExitCode)." `
                       -ForegroundColor Red
            exit 1
        }

        # [4/4] Verify results (identical logic to Original engine)
        Write-Host "[4/4] Verifying results..." -ForegroundColor Yellow

        $found = Get-ChildItem -Path $TestDir -Filter "results_*_qt.csv" `
                     -ErrorAction SilentlyContinue |
                 Where-Object { $_.LastWriteTime -ge $startTime } |
                 Sort-Object LastWriteTime -Descending |
                 Select-Object -First 1

        if ($null -eq $found) {
            Write-Host "FAIL: No Qt results file created." -ForegroundColor Red
            exit 1
        }

        $resultFile = $found.FullName
        Write-Host "  Found  : $resultFile"

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
            Write-Host "FAIL: No 'ALL' row in results file." -ForegroundColor Red
            exit 1
        }

        $mbps   = [double]$allRow[12].Trim()
        $iops   = [double]$allRow[6].Trim()
        $errors = [double]$allRow[27].Trim()

        Write-Host ""
        Write-Host "  MBps (Dec) : $mbps"
        Write-Host "  IOps       : $iops"
        Write-Host "  Errors     : $errors"
        Write-Host ""

        $pass = $true
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
            Write-Host "FAIL: One or more checks failed. See $resultFile" -ForegroundColor Red
            exit 1
        }
    }

    # -- Qt + real Dynamo mode --------------------------------------------------
    # Requires elevation for raw-disk access by Dynamo
    Check-Elevation

    $dynamo = Join-Path $BinDir "Dynamo.exe"
    Require-File $dynamo

    Write-Host "  Binary : $iometerQt"
    Write-Host "  Dynamo : $dynamo"
    Write-Host "  Mode   : Real Dynamo (TCP port 1066 login handshake)"
    Write-Host ""

    Get-Process -Name "Dynamo" -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Seconds 1

    # [1/4] Launch IometerQt (it will open port 1066 immediately)
    Write-Host "[1/4] Launching IometerQt.exe (DynamoEngine)..." -ForegroundColor Yellow
    $qtProc = Start-Process -FilePath $iometerQt `
        -PassThru

    # Verify it survives initial startup
    Start-Sleep -Seconds 3
    if ($qtProc.HasExited) {
        Write-Host "FAIL: IometerQt exited at startup (code $($qtProc.ExitCode))." `
                   -ForegroundColor Red
        exit 1
    }

    # [2/4] Wait for port 1066 to be in LISTEN state
    Write-Host "[2/4] Waiting for port 1066 (LISTEN)..." -ForegroundColor Yellow
    if (-not (Wait-PortListen -Port 1066 -TimeoutSec 20)) {
        Write-Host "FAIL: Port 1066 never entered LISTEN state within 20 s." `
                   -ForegroundColor Red
        Stop-Process -Id $qtProc.Id -Force -ErrorAction SilentlyContinue
        exit 1
    }
    Write-Host "  OK: port 1066 is listening."

    # [3/4] Launch Dynamo and wait for it to connect
    Write-Host "[3/4] Starting Dynamo (connecting to 127.0.0.1)..." -ForegroundColor Yellow
    $hostname   = $env:COMPUTERNAME.ToLower()
    $dynamoProc = Start-Process -FilePath $dynamo `
        -ArgumentList "-i 127.0.0.1 -m $hostname" `
        -PassThru

    # Wait for the login ESTABLISHED connection on port 1066
    Write-Host "  Waiting for login connection on port 1066..." -NoNewline
    $connected = Wait-PortEstablished -Port 1066 -TimeoutSec $LoginTimeout
    Write-Host ""

    if (-not $connected) {
        Write-Host "FAIL: Dynamo did not connect to port 1066 within ${LoginTimeout}s." `
                   -ForegroundColor Red
        Stop-Process -Id $dynamoProc.Id -Force -ErrorAction SilentlyContinue
        Stop-Process -Id $qtProc.Id     -Force -ErrorAction SilentlyContinue
        exit 1
    }
    Write-Host "  OK: login connection established."

    # [4/4] Hold for QtRunSeconds and verify both stay alive
    # After the login handshake the login socket closes; Dynamo then
    # stays alive while connected to IometerQt's DynamoEngine.
    Write-Host "[4/4] Verifying stability for ${QtRunSeconds}s..." -ForegroundColor Yellow
    $waited = 0
    $earlyExit = $false
    while ($waited -lt $QtRunSeconds) {
        Start-Sleep -Seconds 2
        $waited += 2
        Write-Host "  ...${waited}s" -NoNewline
        if ($dynamoProc.HasExited) {
            Write-Host ""
            Write-Host "FAIL: Dynamo exited early (code $($dynamoProc.ExitCode)) after ${waited}s." `
                       -ForegroundColor Red
            $earlyExit = $true
            break
        }
        if ($qtProc.HasExited) {
            Write-Host ""
            Write-Host "FAIL: IometerQt exited early (code $($qtProc.ExitCode)) after ${waited}s." `
                       -ForegroundColor Red
            $earlyExit = $true
            break
        }
    }
    Write-Host ""

    # Tear down
    Stop-Process -Id $dynamoProc.Id -Force -ErrorAction SilentlyContinue
    Stop-Process -Id $qtProc.Id     -Force -ErrorAction SilentlyContinue

    if ($earlyExit) {
        exit 1
    }

    Write-Host ""
    Write-Host "PASS: IometerQt + Dynamo connected and stayed alive for ${QtRunSeconds}s." `
               -ForegroundColor Green
    exit 0
}
