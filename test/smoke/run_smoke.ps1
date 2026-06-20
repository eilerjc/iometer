# run_smoke.ps1 - Windows smoke-test runner.
#
# Reads the shared, platform-neutral scenarios.json and executes the selected
# scenarios. The companion run_smoke.sh runs the SAME manifest on Linux/macOS
# (for the cross-platform scenarios). Keep behavior in sync between the two.
#
# Usage:
#   .\run_smoke.ps1                       # mode=test (non-elevated) scenarios
#   .\run_smoke.ps1 -Mode real            # real Dynamo (real disk; self-elevates)
#   .\run_smoke.ps1 -Mode both
#   .\run_smoke.ps1 -Only qt-dynamotest,unit-tests
#   .\run_smoke.ps1 -List                 # list scenarios, run nothing
#   .\run_smoke.ps1 -IncludePending       # also run scenarios marked status=pending
#   .\run_smoke.ps1 -Coverage             # also collect line coverage (opt-in)
#
# Coverage (-Coverage): re-runs the WHOLE selected suite under OpenCppCoverage with
# --cover_children, so every binary the scenarios launch (IometerQt, dynamotest,
# Dynamo, the MFC GUI, the ctest unit exes) contributes line coverage to a single
# .cov under cov_raw/. Opt-in because it needs PDB-carrying builds: it forces
# QtConfig=RelWithDebInfo (build that config first); the MSVC Release x64 binaries
# already ship PDBs. src/qt/collect_coverage_all.ps1 auto-folds cov_raw/*.cov into
# the published report. (Linux run_smoke.sh coverage is a TODO - OCC is Windows-only.)

[CmdletBinding()]
param(
    [ValidateSet("test", "real", "both")]
    [string]$Mode = "test",
    [string[]]$Only,
    [switch]$IncludePending,
    [switch]$List,
    [switch]$Coverage,
    [string]$CovDir,
    [string]$OccExe = "C:\Program Files\OpenCppCoverage\OpenCppCoverage.exe",
    [string]$QtPrefix = "C:\Qt\6.8.3\msvc2022_64",
    [string]$QtConfig = "Release"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$here     = $PSScriptRoot
$testDir  = (Resolve-Path (Join-Path $here "..")).Path
$repo     = (Resolve-Path (Join-Path $here "..\..")).Path     # repo root (src/ lives here)
$manifest = Get-Content (Join-Path $here "scenarios.json") -Raw | ConvertFrom-Json
$cfg      = $manifest.config
$host4    = $env:COMPUTERNAME

# --- Coverage: re-run the whole suite under OpenCppCoverage (opt-in) ----------
# --cover_children records every binary the scenarios spawn (controllers, workers,
# unit-test exes) into one .cov - even ones the runners kill, since OCC flushes per
# child exit. We just re-invoke this script under OCC and let it run normally; the
# IOMETER_SMOKE_OCC sentinel (inherited by the child) prevents an infinite relaunch.
if (-not $List -and $Coverage -and -not $env:IOMETER_SMOKE_OCC) {
    if (-not (Test-Path $OccExe)) { Write-Host "ERROR: OpenCppCoverage not found at $OccExe" -ForegroundColor Red; exit 2 }
    if (-not $CovDir) { $CovDir = Join-Path $here "cov_raw" }
    New-Item -ItemType Directory -Path $CovDir -Force | Out-Null
    # Qt binaries need PDBs; the Release Qt build has none, so use RelWithDebInfo.
    if ($QtConfig -eq "Release") {
        $QtConfig = "RelWithDebInfo"
        Write-Host "NOTE: coverage needs PDBs - using QtConfig=RelWithDebInfo (build that config first)." -ForegroundColor Yellow
    }
    $cov     = Join-Path $CovDir ("smoke_{0}.cov" -f (Get-Date -Format "yyyyMMdd_HHmmss"))
    $srcFilt = "iometer\github\iometer\src"
    # One verbatim command string so the quoted .ps1 / .cov paths (with spaces) survive.
    $inner = "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`" -Mode $Mode -QtConfig $QtConfig"
    if ($Only)           { $inner += " -Only $($Only -join ',')" }
    if ($IncludePending) { $inner += " -IncludePending" }
    $occ = "--quiet --cover_children --sources $srcFilt " +
           "--excluded_sources tests --excluded_sources moc_ --excluded_sources qrc_ " +
           "--export_type binary:`"$cov`" -- powershell.exe $inner"
    $env:IOMETER_SMOKE_OCC = "1"
    Write-Host "=== Smoke suite UNDER OpenCppCoverage -> $cov ===" -ForegroundColor Cyan
    $p = Start-Process $OccExe -ArgumentList $occ -NoNewWindow -PassThru -Wait
    $env:IOMETER_SMOKE_OCC = $null
    if (Test-Path $cov) { Write-Host "[cov] wrote $cov ($([math]::Round((Get-Item $cov).Length/1kb))kb)" -ForegroundColor Green }
    else { Write-Host "WARNING: no .cov produced (built the RelWithDebInfo/PDB binaries?)" -ForegroundColor Yellow }
    exit $p.ExitCode
}

# --- Resolve logical binary names -> real Windows paths ----------------------
$relBin = Join-Path $repo "src\msvs11\Release\x64"
$qtBin  = Join-Path $repo "src\qt\build\$QtConfig"
function Resolve-Bin([string]$logical) {
    switch ($logical) {
        "IometerQt"   { return (Join-Path $qtBin  "IometerQt.exe") }
        "dynamotest"  { return (Join-Path $relBin "dynamotest.exe") }
        "Dynamo"      { return (Join-Path $relBin "Dynamo.exe") }
        "Iometertest" { return (Join-Path $relBin "Iometertest.exe") }
        "IOmeter"     { return (Join-Path $relBin "IOmeter.exe") }
        default       { throw "Unknown logical binary '$logical'" }
    }
}

if (Test-Path "$QtPrefix\bin") { $env:PATH = "$QtPrefix\bin;$env:PATH" }

# --- Helpers -----------------------------------------------------------------
function Port-Listening([int]$p = 1066) {
    [bool](Get-NetTCPConnection -LocalPort $p -State Listen -ErrorAction SilentlyContinue)
}
function Wait-Port([int]$p = 1066, [int]$timeoutSec = 50) {
    for ($i = 0; $i -lt $timeoutSec; $i++) {
        if (Port-Listening $p) { return $true }
        Start-Sleep 1
    }
    return $false
}
function Get-AllRow([string]$file) {
    if (-not (Test-Path $file)) { return $null }
    $inR = $false
    foreach ($line in Get-Content $file) {
        if ($line -match "^'Results") { $inR = $true; continue }
        if (-not $inR -or $line -match "^'") { continue }
        $f = $line -split ","
        if ($f.Count -gt 27 -and $f[0].Trim() -eq "ALL") { return $f }
    }
    return $null
}
function Verify-Csv($verify, [string]$file) {
    $row = Get-AllRow $file
    if ($null -eq $row) { return $false, "no ALL row in $file" }
    $iops = [double]$row[6].Trim(); $mbps = [double]$row[12].Trim(); $err = [double]$row[27].Trim()
    $detail = "IOps=$iops MBps=$mbps Errors=$err"
    if ($verify.PSObject.Properties.Name -contains "iopsGt"   -and -not ($iops -gt $verify.iopsGt))   { return $false, "$detail (IOps not > $($verify.iopsGt))" }
    if ($verify.PSObject.Properties.Name -contains "errorsEq" -and -not ($err  -eq $verify.errorsEq)) { return $false, "$detail (Errors != $($verify.errorsEq))" }
    return $true, $detail
}
function Kill-Stale {
    Get-Process IometerQt,Iometertest,IOmeter,dynamotest,Dynamo -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep 1
}

# --- Scenario kinds ----------------------------------------------------------
function Find-Ctest {
    $c = Get-Command ctest -ErrorAction SilentlyContinue
    if ($c) { return $c.Source }
    $cands = @(
        "C:\Program Files\CMake\bin\ctest.exe",
        "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe"
    )
    foreach ($p in $cands) { if (Test-Path $p) { return $p } }
    # Last resort: search next to any cmake.exe we can find.
    $cm = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cm) { $alt = Join-Path (Split-Path $cm.Source) "ctest.exe"; if (Test-Path $alt) { return $alt } }
    return $null
}
function Run-Ctest($s) {
    $build = Join-Path $repo "src\qt\build"
    if (-not (Test-Path $build)) { return $false, "qt build dir missing: $build" }
    $ctest = Find-Ctest
    if (-not $ctest) { return $false, "ctest.exe not found (install CMake or add it to PATH)" }
    Push-Location $build
    try {
        & $ctest -C $QtConfig --output-on-failure | Out-Null
        $code = $LASTEXITCODE
    } finally { Pop-Location }
    return ($code -eq 0), "ctest exit $code"
}
function Run-ValidateIcf($s) {
    $exe = Resolve-Bin $s.controller
    if (-not (Test-Path $exe)) { return $false, "missing $exe" }
    $fail = 0; $n = 0
    foreach ($icf in $s.icfs) {
        $full = Join-Path $testDir $icf
        if (-not (Test-Path $full)) { Write-Host "      [missing] $icf" -ForegroundColor Yellow; $fail++; continue }
        $p = Start-Process $exe -ArgumentList "--validate-icf `"$full`"" -PassThru -Wait
        $n++
        if ($p.ExitCode -ne 0) { Write-Host "      [fail] $icf (exit $($p.ExitCode))" -ForegroundColor Red; $fail++ }
        else { Write-Host "      [ok]   $icf" -ForegroundColor DarkGray }
    }
    return ($fail -eq 0), "$($n - $fail)/$n fixtures validated"
}
function Run-Demo($s) {
    $exe = Resolve-Bin $s.controller
    if (-not (Test-Path $exe)) { return $false, "missing $exe" }
    Kill-Stale
    $p = Start-Process $exe -ArgumentList "--demo" -PassThru
    Start-Sleep 5
    if ($p.HasExited) { return $false, "exited early (code $($p.ExitCode))" }
    $w = 0; while (-not $p.HasExited -and $w -lt $cfg.qtRunSeconds) { Start-Sleep 2; $w += 2 }
    $crashed = $p.HasExited
    if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue }
    if ($crashed) { return $false, "crashed during demo run" }
    return $true, "ran ${w}s without crashing"
}
function Run-BatchWorker($s) {
    $exe = Resolve-Bin $s.controller
    $wkr = Resolve-Bin $s.worker
    if (-not (Test-Path $exe)) { return $false, "missing controller $exe" }
    if (-not (Test-Path $wkr)) { return $false, "missing worker $wkr" }
    $icf = Join-Path $testDir $s.icf
    $out = Join-Path $env:TEMP ("smoke_{0}.csv" -f $s.name)
    Remove-Item $out -Force -ErrorAction SilentlyContinue
    Kill-Stale
    $ctrl = Start-Process $exe -ArgumentList "/c `"$icf`" /r `"$out`" /t $($cfg.loginTimeout)" -PassThru
    if (-not (Wait-Port 1066 50)) { Stop-Process -Id $ctrl.Id -Force -EA SilentlyContinue; return $false, "controller never listened on 1066" }
    $wargs = $s.workerArgs -replace "\{host\}", $host4 -replace "\{rdelay\}", "$($cfg.rdelay)"
    $dyn = Start-Process $wkr -ArgumentList $wargs -PassThru
    $w = 0; while (-not $ctrl.HasExited -and $w -lt $cfg.batchMaxWaitSeconds) { Start-Sleep 2; $w += 2 }
    $done = $ctrl.HasExited
    if (-not $dyn.HasExited)  { Stop-Process -Id $dyn.Id  -Force -EA SilentlyContinue }
    if (-not $ctrl.HasExited) { Stop-Process -Id $ctrl.Id -Force -EA SilentlyContinue }
    if (-not $done) { return $false, "controller did not finish within $($cfg.batchMaxWaitSeconds)s" }
    return (Verify-Csv $s.verify $out)
}
function Run-BatchSelfSpawn($s) {
    $exe = Resolve-Bin $s.controller
    if (-not (Test-Path $exe)) { return $false, "missing controller $exe" }
    $icf = Join-Path $testDir $s.icf
    $out = Join-Path $env:TEMP ("smoke_{0}.csv" -f $s.name)
    Remove-Item $out -Force -ErrorAction SilentlyContinue
    Kill-Stale
    $ctrl = Start-Process $exe -ArgumentList "/c `"$icf`" /r `"$out`" /t $($cfg.loginTimeout)" -WorkingDirectory $relBin -PassThru
    $w = 0; while (-not $ctrl.HasExited -and $w -lt $cfg.batchMaxWaitSeconds) { Start-Sleep 2; $w += 2 }
    $done = $ctrl.HasExited
    Get-Process dynamotest,Dynamo -EA SilentlyContinue | Stop-Process -Force -EA SilentlyContinue
    if (-not $ctrl.HasExited) { Stop-Process -Id $ctrl.Id -Force -EA SilentlyContinue }
    if (-not $done) { return $false, "controller did not finish within $($cfg.batchMaxWaitSeconds)s" }
    return (Verify-Csv $s.verify $out)
}

# --- Elevation (only for real mode, which needs raw-disk access) -------------
function Ensure-Elevated {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    $pr = New-Object Security.Principal.WindowsPrincipal($id)
    if (-not $pr.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        Write-Warning "Real mode needs elevation; relaunching elevated (accept the UAC prompt)..."
        $a = "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`" -Mode $Mode"
        if ($Only)           { $a += " -Only $($Only -join ',')" }
        if ($IncludePending) { $a += " -IncludePending" }
        Start-Process powershell.exe -Verb RunAs -ArgumentList $a
        exit
    }
}

# --- Select + run ------------------------------------------------------------
$modes = if ($Mode -eq "both") { @("test", "real") } else { @($Mode) }
$selected = $manifest.scenarios | Where-Object {
    ($_.platforms -contains "windows") -and
    ($_.modes | Where-Object { $modes -contains $_ }) -and
    (-not $Only -or ($Only -contains $_.name))
}

if ($List) {
    Write-Host ""
    Write-Host "Scenarios (platform=windows):" -ForegroundColor Cyan
    foreach ($s in $manifest.scenarios | Where-Object { $_.platforms -contains "windows" }) {
        $st = if ($s.PSObject.Properties.Name -contains "status") { " [$($s.status)]" } else { "" }
        $el = if ($s.PSObject.Properties.Name -contains "elevated" -and $s.elevated) { " (elevated)" } else { "" }
        Write-Host ("  {0,-16} modes={1,-12}{2}{3}" -f $s.name, ($s.modes -join ','), $el, $st)
        Write-Host ("      {0}" -f $s.description) -ForegroundColor DarkGray
    }
    exit 0
}

if ($modes -contains "real") { Ensure-Elevated }

Write-Host ""
Write-Host "=== Iometer Smoke Suite (mode=$Mode) ===" -ForegroundColor Cyan

$results = @()
foreach ($s in $selected) {
    $pending = ($s.PSObject.Properties.Name -contains "status" -and $s.status -eq "pending")
    if ($pending -and -not $IncludePending) {
        Write-Host ("[SKIP] {0} (pending - use -IncludePending)" -f $s.name) -ForegroundColor DarkYellow
        $results += [pscustomobject]@{ name = $s.name; ok = $null; detail = "skipped (pending)" }
        continue
    }
    Write-Host ""
    Write-Host ("[RUN ] {0} ({1})" -f $s.name, $s.kind) -ForegroundColor Yellow
    try {
        switch ($s.kind) {
            "ctest"           { $ok, $detail = Run-Ctest $s }
            "validate-icf"    { $ok, $detail = Run-ValidateIcf $s }
            "demo"            { $ok, $detail = Run-Demo $s }
            "batch-worker"    { $ok, $detail = Run-BatchWorker $s }
            "batch-selfspawn" { $ok, $detail = Run-BatchSelfSpawn $s }
            default           { $ok = $false; $detail = "unknown kind '$($s.kind)'" }
        }
    } catch {
        $ok = $false; $detail = "exception: $($_.Exception.Message)"
    }
    $tag = if ($ok) { "PASS" } else { "FAIL" }
    $col = if ($ok) { "Green" } else { "Red" }
    Write-Host ("[{0}] {1}: {2}" -f $tag, $s.name, $detail) -ForegroundColor $col
    $results += [pscustomobject]@{ name = $s.name; ok = $ok; detail = $detail }
}

Write-Host ""
Write-Host "==================== SUMMARY ====================" -ForegroundColor Cyan
$pass = 0; $fail = 0; $skip = 0
foreach ($r in $results) {
    if     ($null -eq $r.ok) { $mark = "SKIP"; $col = "DarkYellow"; $skip++ }
    elseif ($r.ok)           { $mark = "PASS"; $col = "Green"; $pass++ }
    else                     { $mark = "FAIL"; $col = "Red"; $fail++ }
    Write-Host ("  [{0}] {1,-16} {2}" -f $mark, $r.name, $r.detail) -ForegroundColor $col
}
Write-Host ""
Write-Host ("Result: $pass passed, $fail failed, $skip skipped") -ForegroundColor $(if ($fail -eq 0) { "Green" } else { "Red" })
exit $(if ($fail -eq 0) { 0 } else { 1 })
