# icf_compat_golden.ps1 - Phase 0 golden harness for the ICF parser unification
# (see docs/dev/ICF_PARSER_UNIFICATION_PLAN.md).
#
# Runs the MFC IOmeter.exe in batch mode over every fixture in
# test/fixtures/icf_compat/ (non-elevated: TESTMGR @ localhost + external
# dynamotest), NORMALIZES the result CSV (strips run-to-run noise: timestamps,
# performance numbers) and either:
#   -Capture : stores the normalized output as the golden under
#              test/fixtures/icf_compat/golden/<fixture>.golden.txt
#   (default): re-runs and DIFFS against the stored goldens. Any difference
#              after a parser change is a CORE BUG (MFC behavior is the oracle).
#
# Usage:
#   .\icf_compat_golden.ps1 -Capture          # establish goldens (current binary)
#   .\icf_compat_golden.ps1                   # compare against goldens
#   .\icf_compat_golden.ps1 -Only oldformat_specs

[CmdletBinding()]
param(
    [switch]$Capture,
    [string]$Only,
    [string]$BinDir = "",
    [int]$MaxWaitSeconds = 90
)

$ErrorActionPreference = "Continue"

# Resolve paths from the script location ($PSScriptRoot proved unreliable in
# some hosts; MyInvocation always works for -File scripts).
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $BinDir) { $BinDir = (Resolve-Path (Join-Path $here "..\..\src\msvs11\Release\x64")).Path }

$iometer   = Join-Path $BinDir "IOmeter.exe"
$dynatest  = Join-Path $BinDir "dynamotest.exe"
$fixDir    = (Resolve-Path (Join-Path $here "..\fixtures\icf_compat")).Path
$goldDir   = Join-Path $fixDir "golden"
if (-not (Test-Path $iometer))  { Write-Host "ERROR: missing $iometer"  -ForegroundColor Red; exit 2 }
if (-not (Test-Path $dynatest)) { Write-Host "ERROR: missing $dynatest" -ForegroundColor Red; exit 2 }
New-Item -ItemType Directory -Force -Path $goldDir | Out-Null

function Kill-Stale {
    Get-Process IOmeter,Iometer,dynamotest,dynamo,Dynamo -ErrorAction SilentlyContinue |
        Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep 1
}

# Start a process via System.Diagnostics.Process (reliable HasExited/ExitCode,
# unlike Start-Process -PassThru).
function Start-Proc([string]$exe, [string]$exeArgs) {
    $p = New-Object System.Diagnostics.Process
    $p.StartInfo.FileName  = $exe
    $p.StartInfo.Arguments = $exeArgs
    $p.StartInfo.UseShellExecute = $true
    $null = $p.Start()
    return $p
}

# Run one fixture through batch IOmeter + its dynamotest worker(s); returns the
# exit tag. By default ONE worker is launched as manager TESTMGR. A fixture
# needing different/multiple workers provides a sidecar file "<fixture>.workers"
# with one dynamotest argument line per worker (blank lines and ' comments
# ignored), e.g. for two_managers_run.icf:
#   -n M1 -m localhost --rdelay 50 -i 127.0.0.1
#   -n M2 -m localhost --rdelay 50 -i 127.0.0.1
function Run-Fixture([string]$icf, [string]$outCsv) {
    Remove-Item $outCsv -Force -ErrorAction SilentlyContinue
    Kill-Stale

    $workerArgs = @()
    $sidecar = "$icf.workers"
    if (Test-Path $sidecar) {
        foreach ($line in Get-Content $sidecar) {
            $t = $line.Trim()
            if ($t -and -not $t.StartsWith("'")) { $workerArgs += $t }
        }
    }
    if ($workerArgs.Count -eq 0) {
        $workerArgs = @("-n TESTMGR -m localhost --rdelay 50 -i 127.0.0.1")
    }

    $ctrl = Start-Proc $iometer "/c `"$icf`" /r `"$outCsv`" /t 40"
    for ($i = 0; $i -lt 25; $i++) {
        Start-Sleep 1
        if (Get-NetTCPConnection -OwningProcess $ctrl.Id -LocalPort 1066 -State Listen -ErrorAction SilentlyContinue) { break }
    }

    $workers = @()
    foreach ($a in $workerArgs) { $workers += Start-Proc $dynatest $a }

    $w = 0
    while (-not $ctrl.HasExited -and $w -lt $MaxWaitSeconds) { Start-Sleep 2; $w += 2 }
    $tag = if ($ctrl.HasExited) { "EXIT=$($ctrl.ExitCode)" } else { "TIMEOUT" }
    foreach ($d in $workers) { if (-not $d.HasExited) { try { $d.Kill() } catch {} } }
    if (-not $ctrl.HasExited) { try { $ctrl.Kill() } catch {} }
    return $tag
}

# Normalize a result CSV: keep structure + parse-sensitive fields, strip noise
# (timestamps, build strings, performance numbers).
function Normalize([string]$csvPath, [string]$exitTag) {
    $out = New-Object System.Collections.Generic.List[string]
    $out.Add("RUN $exitTag")
    if (-not (Test-Path $csvPath)) { $out.Add("NO-CSV"); return $out }
    $skipNextValue = $false
    foreach ($line in Get-Content $csvPath) {
        if ($skipNextValue) { $out.Add("<TS>"); $skipNextValue = $false; continue }
        if ($line -match "^'Time Stamp") { $out.Add($line); $skipNextValue = $true; continue }
        if ($line -match "^'")          { $out.Add($line); continue }
        if ($line -match "^Version")    { $out.Add("VERSION"); continue }
        $f = $line -split ","
        if ($f.Count -gt 27) {
            # Result data row: identity fields + error count + iops nonzero flag.
            $iopsVal = 0.0
            $iopsPos = ([double]::TryParse($f[6].Trim(), [ref]$iopsVal)) -and ($iopsVal -gt 0)
            $out.Add(("ROW {0} ERR={1} IOPS>0={2}" -f (($f[0..5] -join ",").Trim()), $f[27].Trim(), $iopsPos))
        } else {
            $out.Add($line)
        }
    }
    return $out
}

# ---- main ---------------------------------------------------------------------
$fixtures = Get-ChildItem (Join-Path $fixDir "*.icf") | Sort-Object Name
if ($Only) { $fixtures = $fixtures | Where-Object { $_.BaseName -eq $Only } }
if (-not $fixtures) { Write-Host "no fixtures selected" -ForegroundColor Red; exit 2 }

$mode = if ($Capture) { "CAPTURE" } else { "COMPARE" }
Write-Host ""
Write-Host "=== ICF parser compat goldens ($mode) - $($fixtures.Count) fixtures ===" -ForegroundColor Cyan

$fail = 0
foreach ($fx in $fixtures) {
    $name   = $fx.BaseName
    $outCsv = Join-Path $env:TEMP "icfgold_$name.csv"
    Write-Host ("[{0}] {1} ..." -f $mode.Substring(0,3), $name) -NoNewline
    $tag  = Run-Fixture $fx.FullName $outCsv
    $norm = Normalize $outCsv $tag
    $goldFile = Join-Path $goldDir "$name.golden.txt"

    if ($Capture) {
        $norm | Set-Content -Path $goldFile -Encoding ASCII
        Write-Host (" captured ({0}, {1} lines)" -f $tag, $norm.Count) -ForegroundColor Green
        if ($tag -ne "EXIT=0") { Write-Host "      WARNING: run did not exit 0 - inspect before trusting this golden" -ForegroundColor Yellow }
    } else {
        if (-not (Test-Path $goldFile)) { Write-Host " FAIL (no golden - run -Capture first)" -ForegroundColor Red; $fail++; continue }
        $gold = Get-Content $goldFile
        $diff = Compare-Object -ReferenceObject $gold -DifferenceObject $norm
        if ($diff) {
            Write-Host " FAIL (differs from golden)" -ForegroundColor Red
            $diff | Select-Object -First 8 | ForEach-Object {
                $side = if ($_.SideIndicator -eq "<=") { "golden " } else { "current" }
                Write-Host ("      {0}: {1}" -f $side, $_.InputObject) -ForegroundColor DarkYellow
            }
            $fail++
        } else {
            Write-Host " PASS" -ForegroundColor Green
        }
    }
}

Kill-Stale
Write-Host ""
if ($Capture) { Write-Host "Goldens stored in $goldDir" -ForegroundColor Cyan; exit 0 }
if ($fail -eq 0) { Write-Host "PASS: all fixtures match goldens" -ForegroundColor Green; exit 0 }
Write-Host "FAIL: $fail fixture(s) differ" -ForegroundColor Red
exit 1
