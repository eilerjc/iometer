# collect_gui_coverage.ps1 - Run the PyAutoGUI GUI tests with the MFC IOmeter.exe
# launched under OpenCppCoverage, then merge the per-test runs into one HTML +
# Cobertura report. This captures the INTERACTIVE MFC paths (tab switches, field
# edits, the Save dialog, the access-spec assign/remove/reorder buttons) that the
# batch /c /r /t scenario in src\qt\collect_coverage_all.ps1 never reaches.
#
# MUST run on an interactive desktop, FOREGROUND (the tests drive the real mouse
# and keyboard). The GUI is slower under the debugger; the tests' own timeouts
# absorb it. Each test launches IOmeter via iometer_gui.launch(), which wraps it
# in OpenCppCoverage when IOCOV_DIR is set.
#
#   .\collect_gui_coverage.ps1            # collect + build report
#   .\collect_gui_coverage.ps1 -Open      # also open the HTML report

[CmdletBinding()]
param([switch]$Open)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Continue"

$occ = "C:\Program Files\OpenCppCoverage\OpenCppCoverage.exe"
if (-not (Test-Path $occ)) { Write-Host "ERROR: OpenCppCoverage not found" -ForegroundColor Red; exit 1 }

$rawDir  = Join-Path $PSScriptRoot "cov_raw"
$htmlDir = Join-Path $PSScriptRoot "cov_html"
Remove-Item $rawDir, $htmlDir -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $rawDir | Out-Null

$tests = @(
    "test_save_roundtrip.py",
    "test_edit_testsetup.py",
    "test_edit_worker.py",
    "test_assign_spec.py",
    "test_remove_spec.py",
    "test_reorder_spec.py"
)

$env:IOCOV_DIR = $rawDir
foreach ($t in $tests) {
    $tag = [System.IO.Path]::GetFileNameWithoutExtension($t)
    $env:IOCOV_TAG = $tag
    Write-Host "=== $t (covering) ===" -ForegroundColor Cyan
    & python (Join-Path $PSScriptRoot $t)
    $cov = Join-Path $rawDir "$tag.cov"
    if (Test-Path $cov) { Write-Host "  [cov] $tag ($([math]::Round((Get-Item $cov).Length/1kb))kb)" -ForegroundColor Green }
    else                { Write-Host "  [miss] $tag (no .cov produced)" -ForegroundColor Yellow }
}
Remove-Item Env:\IOCOV_DIR, Env:\IOCOV_TAG -ErrorAction SilentlyContinue

$covFiles = Get-ChildItem $rawDir -Filter *.cov -ErrorAction SilentlyContinue
if (-not $covFiles) { Write-Host "ERROR: no coverage captured" -ForegroundColor Red; exit 1 }

Write-Host "[merge] $($covFiles.Count) runs -> report..." -ForegroundColor Cyan
$merge = @()
foreach ($c in $covFiles) { $merge += @("--input_coverage", $c.FullName) }
# Drop code unreachable on Windows x64 so the report reflects reachable lines
# (same exclusions as src\qt\collect_coverage_all.ps1).
foreach ($dead in @("IOTargetVI","IOCQVI","IOVIPL","NetVI","VINic","ByteOrder")) {
    $merge += @("--excluded_sources", $dead)
}
$merge += @("--export_type", "html:$htmlDir",
            "--export_type", "cobertura:$(Join-Path $PSScriptRoot 'cov_coverage.xml')")
$m = Start-Process $occ -ArgumentList $merge -PassThru -NoNewWindow
$m.WaitForExit(120000) | Out-Null

$index = Join-Path $htmlDir "index.html"
Write-Host ""
Write-Host "GUI-interaction coverage report: $index" -ForegroundColor Cyan
if ($Open -and (Test-Path $index)) { Start-Process $index }
