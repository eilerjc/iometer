# collect_coverage_all.ps1 - Unified coverage across ALL Iometer binaries using
# OpenCppCoverage (PDB-based, works on any MSVC build - core, Qt GUI, Dynamo,
# and the MFC GUI), so every source file is represented, not just the Qt core.
#
# Unlike collect_coverage.ps1 (clang-cl + llvm-cov, Qt-only), this runs the
# real MSVC binaries under OpenCppCoverage and merges all runs into one report.
#
# Scenarios covered:
#   - every Qt unit-test exe                       -> core + Qt engine + widgets
#   - IometerQt --validate-icf over the fixtures   -> main + DemoEngine
#   - dynamotest <-> IometerQt (batch)             -> Dynamo + DynamoEngine socket
#   - (-IncludeMfc) IOmetertest <-> dynamotest      -> MFC Iometer GUI. Uses the
#      AsInvoker IOmetertest.exe + a non-local-manager ICF so the GUI does NOT
#      auto-spawn the elevated local Dynamo (no UAC). Launches a window; off by
#      default. Covers GUI startup/cmdline/doc/login paths.
#
# Requires: OpenCppCoverage, a Qt build WITH PDBs (RelWithDebInfo), and the
# MSVC Dynamotest build. Run from anywhere; paths are derived from $PSScriptRoot.

[CmdletBinding()]
param(
    [string]$QtConfig  = "RelWithDebInfo",   # Qt config that carries PDBs
    [string]$QtPrefix  = "C:\Qt\6.8.3\msvc2022_64",
    [switch]$IncludeMfc,                      # also cover the MFC IOmeter GUI (launches a window)
    [switch]$Open
)

$ErrorActionPreference = "Continue"
Set-StrictMode -Version Latest

$occ = "C:\Program Files\OpenCppCoverage\OpenCppCoverage.exe"
if (-not (Test-Path $occ)) { Write-Host "ERROR: OpenCppCoverage not found" -ForegroundColor Red; exit 1 }

$root     = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path     # repo root
$srcFilt  = "iometer\github\iometer\src"                              # --sources substring
$qtBin    = Join-Path $PSScriptRoot "build\$QtConfig"
$dynTest  = Join-Path $PSScriptRoot "..\msvs11\Release\x64\dynamotest.exe"
$qtBatch  = Join-Path $qtBin "IometerQt.exe"
$fixtures = Join-Path $root "test"
$covDir   = Join-Path $PSScriptRoot "build_cov_occ"
$covRaw   = Join-Path $covDir "raw"

if (-not (Test-Path $qtBatch)) { Write-Host "ERROR: $qtBatch missing - build Qt $QtConfig first" -ForegroundColor Red; exit 1 }
if (-not (Test-Path $dynTest)) { Write-Host "ERROR: dynamotest.exe missing - build Dynamotest.vcxproj first" -ForegroundColor Red; exit 1 }

Remove-Item $covDir -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $covRaw -Force | Out-Null

$env:PATH = "$QtPrefix\bin;$env:PATH"
$env:QT_QPA_PLATFORM = "offscreen"

$covFiles = @()
function Cover([string]$tag, [string]$exe, [string[]]$exeArgs, [int]$timeoutSec = 60) {
    $out = Join-Path $covRaw "$tag.cov"
    $occArgs = @("--quiet", "--sources", $srcFilt,
                 "--excluded_sources", "tests", "--excluded_sources", "moc_",
                 "--excluded_sources", "qrc_", "--excluded_sources", "build_cov",
                 "--export_type", "binary:$out", "--")
    $all = $occArgs + @($exe) + $exeArgs
    $p = Start-Process $occ -ArgumentList $all -PassThru -NoNewWindow
    if (-not $p.WaitForExit($timeoutSec * 1000)) { try { $p.Kill() } catch {} }
    if (Test-Path $out) { $script:covFiles += $out; Write-Host "  [cov] $tag" -ForegroundColor Green }
    else { Write-Host "  [miss] $tag (no cov produced)" -ForegroundColor Yellow }
}

# --- 1. Qt unit tests --------------------------------------------------------
Write-Host "[1] Qt unit tests..." -ForegroundColor Cyan
Get-ChildItem (Join-Path $qtBin "tst_*.exe") | ForEach-Object {
    Cover $_.BaseName $_.FullName @() 40
}

# --- 2. IometerQt --validate-icf over the fixtures ---------------------------
Write-Host "[2] IometerQt --validate-icf (DemoEngine/main)..." -ForegroundColor Cyan
$icfList = @("smoke_test.icf","fixtures\minimal.icf","fixtures\multispec.icf",
             "fixtures\two_managers.icf","fixtures\multi_target.icf",
             "fixtures\multispec_worker.icf","fixtures\default_assign.icf")
$i = 0
foreach ($icf in $icfList) {
    $full = Join-Path $fixtures $icf
    if (Test-Path $full) { Cover ("validate_{0}" -f $i++) $qtBatch @("--validate-icf", $full) 30 }
}

# --- 3. dynamotest <-> IometerQt: cover the DYNAMO side -----------------------
Write-Host "[3] dynamotest <-> IometerQt (covering Dynamo)..." -ForegroundColor Cyan
$smoke = Join-Path $fixtures "smoke_test.icf"
Get-Process IometerQt,dynamotest -ErrorAction SilentlyContinue | Stop-Process -Force
# Controller (plain, not covered here); then dynamotest UNDER coverage.
$ctrl = Start-Process $qtBatch -ArgumentList "/c `"$smoke`" /r C:\tmp\occ_dyn.csv /t 30" -PassThru
Start-Sleep 3
Cover "dynamo" $dynTest @("--rdelay","50","-i","127.0.0.1","-m",$env:COMPUTERNAME) 60
if (-not $ctrl.HasExited) { Stop-Process -Id $ctrl.Id -Force -ErrorAction SilentlyContinue }

# --- 3b. Rich disk scenario: "All in one" spec exercises read/write/random/sizes
#         across 2 workers, covering Dynamo's write + random + multi-worker paths.
Write-Host "[3b] dynamotest <-> IometerQt (rich All-in-one disk patterns)..." -ForegroundColor Cyan
$richIcf = Join-Path $fixtures "fixtures\cov_rich_disk.icf"
if (Test-Path $richIcf) {
    Get-Process IometerQt,dynamotest -ErrorAction SilentlyContinue | Stop-Process -Force
    $ctrlR = Start-Process $qtBatch -ArgumentList "/c `"$richIcf`" /r $env:TEMP\occ_rich.csv /t 25" -PassThru
    for ($i=0; $i -lt 15 -and -not (Get-NetTCPConnection -LocalPort 1066 -State Listen -ErrorAction SilentlyContinue); $i++) { Start-Sleep 1 }
    Cover "dynamo_rich" $dynTest @("--rdelay","30","-i","127.0.0.1","-m","4KMONSTER") 180
    if (-not $ctrlR.HasExited) { Stop-Process -Id $ctrlR.Id -Force -ErrorAction SilentlyContinue }
}

# --- 4. dynamotest <-> IometerQt: cover the CONTROLLER (DynamoEngine) ---------
Write-Host "[4] dynamotest <-> IometerQt (covering IometerQt/DynamoEngine)..." -ForegroundColor Cyan
Get-Process IometerQt,dynamotest -ErrorAction SilentlyContinue | Stop-Process -Force
# Dynamo (plain) started after the covered controller is listening.
$covCtrlOut = Join-Path $covRaw "iometerqt_run.cov"
$occArgs = @("--quiet","--sources",$srcFilt,"--excluded_sources","tests",
             "--excluded_sources","moc_","--excluded_sources","qrc_",
             "--export_type","binary:$covCtrlOut","--",
             $qtBatch,"/c","$smoke","/r","C:\tmp\occ_ctrl.csv","/t","30")
$covCtrl = Start-Process $occ -ArgumentList $occArgs -PassThru -NoNewWindow
Start-Sleep 4
$dyn2 = Start-Process $dynTest -ArgumentList "--rdelay 50 -i 127.0.0.1 -m $env:COMPUTERNAME" -PassThru
if (-not $covCtrl.WaitForExit(90 * 1000)) { try { $covCtrl.Kill() } catch {} }
if (-not $dyn2.HasExited) { Stop-Process -Id $dyn2.Id -Force -ErrorAction SilentlyContinue }
if (Test-Path $covCtrlOut) { $covFiles += $covCtrlOut; Write-Host "  [cov] iometerqt_run" -ForegroundColor Green }
else { Write-Host "  [miss] iometerqt_run" -ForegroundColor Yellow }

# --- 4b. MFC IOmeter GUI (opt-in; launches a window) -------------------------
# Uses the now-AsInvoker IOmeter.exe + a NON-LOCAL manager ICF so the GUI does
# NOT auto-spawn a local Dynamo (no UAC). dynamotest fills the manager
# externally. (The GUI's manifest is asInvoker and it only elevates Dynamo, via
# ShellExecuteEx(runas), when it spawns a LOCAL manager - which this ICF avoids.)
# The GUI is slow under the debugger, so we wait for the login port before
# connecting and rely on the batch login-timeout to exit.
if ($IncludeMfc) {
    Write-Host "[4b] MFC IOmeter GUI (non-local manager)..." -ForegroundColor Cyan
    $iomTest = Join-Path $PSScriptRoot "..\msvs11\Release\x64\IOmeter.exe"
    $nlIcf   = Join-Path $fixtures "fixtures\nonlocal_manager.icf"
    if ((Test-Path $iomTest) -and (Test-Path $nlIcf)) {
        Get-Process IOmeter,dynamotest,Dynamo -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
        $mfcOut = Join-Path $covRaw "mfc_iometer.cov"
        $occArgs = @("--quiet","--sources",$srcFilt,"--modules","IOmeter.exe",
                     "--excluded_sources","tests","--export_type","binary:$mfcOut","--",
                     $iomTest,"/c",$nlIcf,"/r","$env:TEMP\occ_mfc.csv","/t","30")
        $covMfc = Start-Process $occ -ArgumentList $occArgs -PassThru -NoNewWindow
        for ($i=0; $i -lt 60 -and -not $covMfc.HasExited; $i++) {
            Start-Sleep 2
            if (Get-NetTCPConnection -LocalPort 1066 -State Listen -ErrorAction SilentlyContinue) { break }
        }
        $dynM = Start-Process $dynTest -ArgumentList "-n TESTMGR -m localhost --rdelay 50 -i 127.0.0.1" -PassThru
        if (-not $covMfc.WaitForExit(300000)) { try { $covMfc.Kill() } catch {} }
        if (-not $dynM.HasExited) { Stop-Process -Id $dynM.Id -Force -ErrorAction SilentlyContinue }
        if (Test-Path $mfcOut) { $covFiles += $mfcOut; Write-Host "  [cov] mfc_iometer" -ForegroundColor Green }
        else { Write-Host "  [miss] mfc_iometer" -ForegroundColor Yellow }
    } else {
        Write-Host "  [skip] IOmetertest.exe or nonlocal_manager.icf missing" -ForegroundColor Yellow
    }
}

# --- 5. Merge all runs into one report ---------------------------------------
Write-Host "[5] Merging $($covFiles.Count) runs -> unified report..." -ForegroundColor Cyan
if ($covFiles.Count -eq 0) { Write-Host "ERROR: no coverage captured" -ForegroundColor Red; exit 1 }
$htmlDir = Join-Path $covDir "html"
$merge = @()
foreach ($c in $covFiles) { $merge += @("--input_coverage", $c) }
# Exclude code that cannot execute on Windows x64, so the report reflects
# reachable code: the Virtual Interface (VI) target stack (no VI hardware) and
# ByteOrder.cpp (big-endian byte-swapping only).
foreach ($dead in @("IOTargetVI","IOCQVI","IOVIPL","NetVI","VINic","ByteOrder")) {
    $merge += @("--excluded_sources", $dead)
}
$merge += @("--export_type", "html:$htmlDir", "--export_type", "cobertura:$(Join-Path $covDir 'coverage.xml')")
$m = Start-Process $occ -ArgumentList $merge -PassThru -NoNewWindow
$m.WaitForExit(120 * 1000) | Out-Null

Write-Host ""
Write-Host "Unified report: $(Join-Path $htmlDir 'index.html')" -ForegroundColor Cyan
Write-Host "Cobertura     : $(Join-Path $covDir 'coverage.xml')"
if ($Open -and (Test-Path (Join-Path $htmlDir 'index.html'))) { Start-Process (Join-Path $htmlDir 'index.html') }
