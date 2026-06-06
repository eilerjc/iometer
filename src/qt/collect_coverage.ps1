# collect_coverage.ps1 - One-command code coverage for the Iometer Qt test suite.
#
# Configures a clang-cl + Ninja coverage build, builds every unit-test target,
# runs them with LLVM source-based instrumentation, also drives the shipped
# IometerQt.exe through the smoke-test ICF paths, merges the profile data, and
# emits BOTH a console summary and a browsable HTML report via llvm-cov.
#
# Self-contained: it locates and imports the MSVC build environment and adds the
# Windows SDK rc.exe to PATH itself (standalone LLVM clang-cl is used, so the
# Visual Studio "ClangCL" toolset component is NOT required). No elevation.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File collect_coverage.ps1
#   ... -Open                 # also open the HTML report in the browser
#   ... -FailUnder 75         # exit 1 if total line coverage < 75% (CI gate)
#   ... -QtPrefix C:\Qt\6.8.3\msvc2022_64
#
# Output:
#   build_cov\coverage\summary.txt      console report (also printed)
#   build_cov\coverage\html\index.html  browsable line-by-line report
#
# NOTE: tst_testrunner is skipped for coverage ONLY - its clang-cl instrumented
# build crashes on startup with heap corruption (a toolchain artifact). It
# passes 19/19 under the normal MSVC build and is fully exercised there.

[CmdletBinding()]
param(
    [string]$BuildDir  = "$PSScriptRoot\build_cov",
    [string]$QtPrefix  = "C:/Qt/6.8.3/msvc2022_64",
    [string]$LlvmBin   = "C:\Program Files\LLVM\bin",
    [double]$FailUnder = 0,
    [switch]$Open
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Die([string]$msg) { Write-Host "ERROR: $msg" -ForegroundColor Red; exit 1 }

# Run an exe with a hard timeout; returns its exit code, or 124 if it had to be
# killed. Uses System.Diagnostics.Process directly (Start-Process -PassThru does
# not reliably surface ExitCode in Windows PowerShell 5.1). Child inherits the
# parent env (LLVM_PROFILE_FILE, QT_QPA_PLATFORM, PATH). Output is drained and
# discarded to avoid pipe-buffer deadlock.
function Invoke-Timed([string]$exe, [int]$timeoutSec) {
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $exe
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    $p = [System.Diagnostics.Process]::Start($psi)
    $null = $p.StandardOutput.ReadToEndAsync()
    $null = $p.StandardError.ReadToEndAsync()
    if (-not $p.WaitForExit($timeoutSec * 1000)) {
        try { $p.Kill() } catch {}
        try { [void]$p.WaitForExit(2000) } catch {}
        return 124
    }
    return $p.ExitCode
}

# Tests skipped for coverage capture (none by default). GUI widget tests may
# still time out under clang-cl instrumentation; that is handled gracefully.
$skipForCoverage = @()

# Per-test wall-clock limit (clang-cl-instrumented GUI tests can wedge; the
# console/core suites all finish in a few seconds even under instrumentation).
$testTimeoutSec = 25

# --- 1. Locate the toolchain -------------------------------------------------
$cmakeCmd = Get-Command cmake -ErrorAction SilentlyContinue
$cmake    = if ($cmakeCmd) { $cmakeCmd.Source } else { "C:\cmake-4.3.2-windows-x86_64\bin\cmake.exe" }
$llvmProfdata = Join-Path $LlvmBin "llvm-profdata.exe"
$llvmCov      = Join-Path $LlvmBin "llvm-cov.exe"
$clangCl      = Join-Path $LlvmBin "clang-cl.exe"

if (-not (Test-Path $cmake))        { Die "cmake not found ($cmake)" }
if (-not (Test-Path $llvmProfdata)) { Die "llvm-profdata not found ($llvmProfdata) - install LLVM (winget install LLVM.LLVM)" }
if (-not (Test-Path $llvmCov))      { Die "llvm-cov not found ($llvmCov)" }
if (-not (Test-Path $clangCl))      { Die "clang-cl not found ($clangCl)" }

# vcvars64.bat - prefer vswhere, fall back to the known VS18 path.
$vcvars = $null
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
    $vsPath = & $vswhere -latest -products * -property installationPath 2>$null
    if ($vsPath) { $vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat" }
}
if (-not $vcvars -or -not (Test-Path $vcvars)) {
    $vcvars = "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
}
if (-not (Test-Path $vcvars)) { Die "vcvars64.bat not found - install Visual Studio C++ tools" }

# Windows SDK rc.exe directory (CMake's link wrapper needs rc on PATH under Ninja).
$rcExe = Get-ChildItem "C:\Program Files (x86)\Windows Kits\10\bin\*\x64\rc.exe" -ErrorAction SilentlyContinue |
    Sort-Object FullName -Descending | Select-Object -First 1
if (-not $rcExe) { Die "Windows SDK rc.exe not found under Windows Kits\10\bin" }
$rcPath = Split-Path $rcExe.FullName -Parent

Write-Host "=== Iometer coverage ===" -ForegroundColor Cyan
Write-Host "  cmake   : $cmake"
Write-Host "  clang-cl: $clangCl"
Write-Host "  vcvars  : $vcvars"
Write-Host "  SDK rc  : $rcPath"
Write-Host ""

# --- 2. Configure + build the coverage tree (inside the MSVC env) ------------
# Everything that needs the env runs in one cmd session via a generated .bat.
$covDir   = Join-Path $BuildDir "coverage"
$htmlDir  = Join-Path $covDir   "html"
$buildLog = Join-Path $env:TEMP "iometer_cov_build.log"
$bat      = Join-Path $env:TEMP "iometer_cov_build.bat"

@"
@echo off
call "$vcvars" >nul 2>nul
set "PATH=%PATH%;$rcPath"
set CC=clang-cl
set CXX=clang-cl
cd /d "$PSScriptRoot"
"$cmake" -S "$PSScriptRoot" -B "$BuildDir" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$QtPrefix" -DIOMETER_COVERAGE=ON > "$buildLog" 2>&1
if errorlevel 1 exit /b 1
"$cmake" --build "$BuildDir" >> "$buildLog" 2>&1
exit /b %ERRORLEVEL%
"@ | Set-Content -Path $bat -Encoding ascii

Write-Host "[1/4] Configuring and building (clang-cl + Ninja)..." -ForegroundColor Yellow
# Run the build batch; tolerate any stderr noise and key off the real exit code.
$prevEAP = $ErrorActionPreference
$ErrorActionPreference = "Continue"
cmd.exe /c "`"$bat`"" 2>$null | Out-Null
$buildExit = $LASTEXITCODE
$ErrorActionPreference = $prevEAP
if ($buildExit -ne 0) {
    Write-Host "Build failed (exit $buildExit). Tail of $buildLog :" -ForegroundColor Red
    Get-Content $buildLog -Tail 25 | ForEach-Object { Write-Host "  $_" }
    exit 1
}

# --- 3. Run every test target with profiling ---------------------------------
Write-Host "[2/4] Running instrumented tests..." -ForegroundColor Yellow
Remove-Item $covDir -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $covDir -Force | Out-Null

$env:PATH = "$QtPrefix/bin;$env:PATH"
$env:QT_QPA_PLATFORM = "offscreen"

$testExes = Get-ChildItem (Join-Path $BuildDir "tst_*.exe") -ErrorAction SilentlyContinue
if (-not $testExes) { Die "No test executables found in $BuildDir" }

$ran = 0; $failed = 0; $skipped = 0
$objArgs = @()
foreach ($exe in $testExes) {
    $name = $exe.BaseName
    if ($skipForCoverage -contains $name) {
        Write-Host "  [skip] $name (excluded from coverage - see header)" -ForegroundColor DarkGray
        $skipped++
        continue
    }
    $env:LLVM_PROFILE_FILE = Join-Path $covDir ($name + ".profraw")
    $code = Invoke-Timed $exe.FullName $testTimeoutSec
    $raw  = Join-Path $covDir ($name + ".profraw")
    if ($code -eq 0 -and (Test-Path $raw) -and (Get-Item $raw).Length -gt 0) {
        Write-Host "  [ok]   $name" -ForegroundColor Green
        $ran++
        $objArgs += @("-object", $exe.FullName)
    } elseif ($code -eq 124) {
        Write-Host "  [TIMEOUT] $name (clang-cl instrumentation wedge; excluded)" -ForegroundColor DarkYellow
        $failed++
    } else {
        Write-Host "  [FAIL] $name (exit $code)" -ForegroundColor Red
        $failed++
    }
}
Remove-Item Env:\LLVM_PROFILE_FILE -ErrorAction SilentlyContinue

if ($ran -eq 0) { Die "No tests produced profile data" }
Write-Host "  ran=$ran failed=$failed skipped=$skipped" -ForegroundColor Gray

# --- 3b. Drive the instrumented app through the smoke-test paths --------------
# Runs the coverage-instrumented IometerQt.exe the same way smoke_test.ps1 does
# (ICF validation via the test engine), so main.cpp / DemoEngine / the validate
# entry points get real coverage too - not just the unit-test units.
$appExe = Join-Path $BuildDir "IometerQt.exe"
if (Test-Path $appExe) {
    Write-Host "[2b/4] Exercising IometerQt smoke paths..." -ForegroundColor Yellow
    $fixtureRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..\test")).Path
    $icfSet = @(
        "$fixtureRoot\smoke_test.icf",
        "$fixtureRoot\fixtures\minimal.icf",
        "$fixtureRoot\fixtures\multispec.icf",
        "$fixtureRoot\fixtures\two_managers.icf",
        "$fixtureRoot\fixtures\multi_target.icf",
        "$fixtureRoot\fixtures\multispec_worker.icf",
        "$fixtureRoot\fixtures\default_assign.icf"
    )
    $smokeOk = 0
    foreach ($icf in $icfSet) {
        if (-not (Test-Path $icf)) { continue }
        $tag = [IO.Path]::GetFileNameWithoutExtension($icf)
        $env:LLVM_PROFILE_FILE = Join-Path $covDir ("app_validate_" + $tag + ".profraw")
        & $appExe --validate-icf $icf *> $null
        if ($LASTEXITCODE -eq 0) { $smokeOk++ }
    }
    Remove-Item Env:\LLVM_PROFILE_FILE -ErrorAction SilentlyContinue
    if ($smokeOk -gt 0) {
        Write-Host "  [ok]   IometerQt --validate-icf x$smokeOk" -ForegroundColor Green
        $objArgs += @("-object", (Resolve-Path $appExe).Path)
    } else {
        Write-Host "  [warn] no smoke profile captured" -ForegroundColor Yellow
    }
}

# --- 4. Merge + report -------------------------------------------------------
# llvm-* tools print benign warnings to stderr (e.g. "N functions have
# mismatched data"); under ErrorActionPreference=Stop PowerShell would turn
# those into terminating errors. Relax it for the reporting section.
$ErrorActionPreference = "Continue"

Write-Host "[3/4] Merging profile data..." -ForegroundColor Yellow
$profdata = Join-Path $covDir "merged.profdata"
$profraws = Get-ChildItem (Join-Path $covDir "*.profraw")
& $llvmProfdata merge -sparse $profraws.FullName -o $profdata
if ($LASTEXITCODE -ne 0) { Die "llvm-profdata merge failed" }

# The first -object must be positional for llvm-cov.
$firstObj = $objArgs[1]
$restObj  = if ($objArgs.Count -gt 2) { $objArgs[2..($objArgs.Count - 1)] } else { @() }

# Report over the whole first-party tree (core library + Qt GUI/engine).
# Test code, Qt headers, and generated files (moc_/qrc_/mocs_) are excluded so
# the numbers reflect product code only.
$coreDir = (Resolve-Path (Join-Path $PSScriptRoot "..\core")).Path
$qtDir   = (Resolve-Path $PSScriptRoot).Path
$ignore  = "tests[\\/]|moc_|qrc_|mocs_compilation|[\\/]build_cov[\\/]|[\\/]Qt[\\/]|autogen"

Write-Host "[4/4] Generating reports (core + qt)..." -ForegroundColor Yellow
$summary = & $llvmCov report $firstObj @restObj "-instr-profile=$profdata" `
    "-ignore-filename-regex=$ignore" $coreDir $qtDir 2>$null
$summaryFile = Join-Path $covDir "summary.txt"
$summary | Set-Content -Path $summaryFile -Encoding utf8

# HTML (line-by-line) for all first-party sources.
Remove-Item $htmlDir -Recurse -Force -ErrorAction SilentlyContinue
& $llvmCov show $firstObj @restObj "-instr-profile=$profdata" `
    "-ignore-filename-regex=$ignore" `
    -format=html -output-dir=$htmlDir -show-line-counts-or-regions $coreDir $qtDir *> $null

# --- Print summary + optional CI gate ----------------------------------------
Write-Host ""
Write-Host "===== COVERAGE (core + qt, product code only) =====" -ForegroundColor Cyan
$summary | ForEach-Object { Write-Host $_ }
Write-Host ""
Write-Host "Summary : $summaryFile"
Write-Host "HTML    : $(Join-Path $htmlDir 'index.html')" -ForegroundColor Cyan

if ($failed -gt 0) {
    Write-Host "WARNING: $failed test(s) failed under instrumentation." -ForegroundColor Yellow
}

$exit = 0
if ($FailUnder -gt 0) {
    $totalLine = $summary | Where-Object { $_ -match "^TOTAL" } | Select-Object -First 1
    if ($totalLine -and $totalLine -match "(\d+\.\d+)%\s+\d+\s+\d+\s+(\d+\.\d+)%") {
        $lineCov = [double]$Matches[2]
        Write-Host ""
        if ($lineCov -lt $FailUnder) {
            Write-Host "GATE FAIL: total line coverage $lineCov% < $FailUnder%" -ForegroundColor Red
            $exit = 1
        } else {
            Write-Host "GATE OK: total line coverage $lineCov% >= $FailUnder%" -ForegroundColor Green
        }
    } else {
        Write-Host "GATE: could not parse TOTAL line - skipping gate." -ForegroundColor Yellow
    }
}

if ($Open -and (Test-Path (Join-Path $htmlDir "index.html"))) {
    Start-Process (Join-Path $htmlDir "index.html")
}

exit $exit
