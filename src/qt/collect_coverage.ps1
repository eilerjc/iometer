# collect_coverage.ps1 — Collect code coverage from test suite
# This is a convenience wrapper around clang-cl + llvm-cov
#
# For most reliable results, see README_COVERAGE.md for manual steps

param([string]$BuildDir = "build_coverage", [string]$SourceDir = $PSScriptRoot)
$ErrorActionPreference = "Stop"

$llvmBin = "C:\Program Files\LLVM\bin"
$llvmProfdata = "$llvmBin\llvm-profdata.exe"
$llvmCov = "$llvmBin\llvm-cov.exe"

# Check prerequisites
if (-not (Test-Path $llvmProfdata)) {
    Write-Host "ERROR: llvm-profdata not found at $llvmProfdata" -ForegroundColor Red
    Write-Host "Install LLVM: winget install LLVM.LLVM" -ForegroundColor Yellow
    exit 1
}
if (-not (Test-Path $llvmCov)) {
    Write-Host "ERROR: llvm-cov not found at $llvmCov" -ForegroundColor Red
    exit 1
}

# Check if build dir exists and has test binaries
if (-not (Test-Path "$BuildDir\Release\tst_types.exe")) {
    Write-Host "ERROR: Coverage build not found or incomplete" -ForegroundColor Red
    Write-Host "Run: cd $BuildDir && cmake .. -DIOMETER_COVERAGE=ON && cmake --build . --config Release"  -ForegroundColor Yellow
    exit 1
}

Write-Host "Collecting coverage..." -ForegroundColor Cyan

# Create coverage dir
$coverageDir = (Resolve-Path $BuildDir).Path + "\coverage"
New-Item -ItemType Directory -Path $coverageDir -Force | Out-Null

# Run tests with profiling
$env:LLVM_PROFILE_FILE = "$coverageDir\%p.profraw"
$testDir = (Resolve-Path $BuildDir).Path + "\Release"
$tests = "tst_types", "tst_accessspecs", "tst_icf", "tst_results", "tst_demo", "tst_protocol"
$passCount = 0

Write-Host "Running tests..." -ForegroundColor Yellow
foreach ($test in $tests) {
    $exe = "$testDir\$test.exe"
    if (Test-Path $exe) {
        & $exe -v2 > $null 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Host "  ✓ $test" -ForegroundColor Green
            $passCount++
        } else {
            Write-Host "  ✗ $test" -ForegroundColor Red
        }
    }
}
Write-Host "Passed: $passCount / $($tests.Count) tests"

# Merge profdata
Write-Host "Merging coverage data..." -ForegroundColor Yellow
$profrawFiles = @(Get-ChildItem "$coverageDir\*.profraw" -ErrorAction SilentlyContinue)
if ($profrawFiles.Count -gt 0) {
    $mergedProf = "$coverageDir\merged.profdata"
    & $llvmProfdata merge -sparse $profrawFiles -o $mergedProf 2>&1 | Out-Null
    Write-Host "  Merged $($profrawFiles.Count) profraw files"
} else {
    Write-Host "  No profraw files — tests may not have run" -ForegroundColor Yellow
    exit 1
}

# Generate report
Write-Host "Generating HTML report..." -ForegroundColor Yellow
$reportDir = (Resolve-Path $BuildDir).Path + "\coverage_report"
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null

$binaries = @()
foreach ($test in $tests) {
    $exe = "$testDir\$test.exe"
    if (Test-Path $exe) { $binaries += (Resolve-Path $exe).Path }
}

if ($binaries.Count -gt 0) {
    & $llvmCov show -format=html -output-dir=$reportDir `
        -instr-profile=$mergedProf `
        -sources $SourceDir `
        $binaries 2>&1 | Out-Null
    Write-Host "  Generated report with $($binaries.Count) binary(ies)"
} else {
    Write-Host "  No test binaries found" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Coverage report: $reportDir\index.html" -ForegroundColor Cyan
Write-Host ""

if (Test-Path "$reportDir\index.html") {
    $ans = Read-Host "Open in browser? (y/n)"
    if ($ans -eq "y") { Start-Process "$reportDir\index.html" }
}
