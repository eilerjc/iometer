# icf_save_golden.ps1 - save-side golden harness for the ICF parser unification
# (Phase 6; counterpart of icf_compat_golden.ps1).
#
# There is no batch "save" in Iometer's CLI, so the MFC GUI build now honours an
# optional /s <file> switch: in batch mode, after the run completes, it re-saves
# the (loaded, run-unchanged) config via the normal SaveConfigFile path, then
# exits. This harness runs the MFC IOmeter over every fixture in
# test/fixtures/icf_compat/ (non-elevated: TESTMGR @ localhost + external
# dynamotest), captures the re-saved ICF, NORMALIZES the noise (the Version
# header/trailer lines) and either:
#   -Capture : stores the normalized save as the golden under
#              test/fixtures/icf_compat/golden_save/<fixture>.save.txt
#   (default): re-runs and DIFFS against the stored goldens. Any difference after
#              a save-side change is a CORE bug (MFC SaveConfig is the oracle).
#
# Usage:
#   .\icf_save_golden.ps1 -Capture          # establish goldens (current binary)
#   .\icf_save_golden.ps1                    # compare against goldens
#   .\icf_save_golden.ps1 -Only worker_rich

[CmdletBinding()]
param(
    [switch]$Capture,
    [string]$Only,
    [string]$BinDir = "",
    [int]$MaxWaitSeconds = 90
)

$ErrorActionPreference = "Continue"

$here = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $BinDir) { $BinDir = (Resolve-Path (Join-Path $here "..\..\src\msvs11\Release\x64")).Path }

$iometer   = Join-Path $BinDir "IOmeter.exe"
$dynatest  = Join-Path $BinDir "dynamotest.exe"
$fixDir    = (Resolve-Path (Join-Path $here "..\fixtures\icf_compat")).Path
$goldDir   = Join-Path $fixDir "golden_save"
if (-not (Test-Path $iometer))  { Write-Host "ERROR: missing $iometer"  -ForegroundColor Red; exit 2 }
if (-not (Test-Path $dynatest)) { Write-Host "ERROR: missing $dynatest" -ForegroundColor Red; exit 2 }
New-Item -ItemType Directory -Force -Path $goldDir | Out-Null

function Kill-Stale {
    Get-Process IOmeter,Iometer,dynamotest,dynamo,Dynamo -ErrorAction SilentlyContinue |
        Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep 1
}

function Start-Proc([string]$exe, [string]$exeArgs) {
    $p = New-Object System.Diagnostics.Process
    $p.StartInfo.FileName  = $exe
    $p.StartInfo.Arguments = $exeArgs
    $p.StartInfo.UseShellExecute = $true
    $null = $p.Start()
    return $p
}

# Run one fixture: IOmeter loads it (/c), runs, then re-saves it (/s), and exits.
# Returns the exit tag; the re-saved ICF is left at $saveOut.
function Run-Fixture([string]$icf, [string]$outCsv, [string]$saveOut) {
    Remove-Item $outCsv,$saveOut -Force -ErrorAction SilentlyContinue
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

    $ctrl = Start-Proc $iometer "/c `"$icf`" /r `"$outCsv`" /s `"$saveOut`" /t 40"
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

# Normalize the re-saved ICF: keep every line, but collapse the build-specific
# Version header/trailer to a stable token.
function Normalize([string]$savePath, [string]$exitTag) {
    $out = New-Object System.Collections.Generic.List[string]
    $out.Add("RUN $exitTag")
    if (-not (Test-Path $savePath)) { $out.Add("NO-SAVE"); return $out }
    foreach ($line in Get-Content $savePath) {
        if ($line -match "^Version ") { $out.Add("VERSION"); continue }
        $out.Add($line)
    }
    return $out
}

# Fixtures that intentionally FAIL to load (error-path load-goldens). They never
# complete a batch run, so there is no config to re-save - excluded here.
$NoSaveFixtures = @("nospec_error")

# ---- main ---------------------------------------------------------------------
$fixtures = Get-ChildItem (Join-Path $fixDir "*.icf") | Sort-Object Name
if ($Only) { $fixtures = $fixtures | Where-Object { $_.BaseName -eq $Only } }
else       { $fixtures = $fixtures | Where-Object { $NoSaveFixtures -notcontains $_.BaseName } }
if (-not $fixtures) { Write-Host "no fixtures selected" -ForegroundColor Red; exit 2 }

$mode = if ($Capture) { "CAPTURE" } else { "COMPARE" }
Write-Host ""
Write-Host "=== ICF save-side goldens ($mode) - $($fixtures.Count) fixtures ===" -ForegroundColor Cyan

$fail = 0
foreach ($fx in $fixtures) {
    $name    = $fx.BaseName
    $outCsv  = Join-Path $env:TEMP "icfsave_$name.csv"
    $saveOut = Join-Path $env:TEMP "icfsave_$name.icf"
    Write-Host ("[{0}] {1} ..." -f $mode.Substring(0,3), $name) -NoNewline
    $tag  = Run-Fixture $fx.FullName $outCsv $saveOut
    $norm = Normalize $saveOut $tag
    $goldFile = Join-Path $goldDir "$name.save.txt"

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
            $diff | Select-Object -First 10 | ForEach-Object {
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
if ($Capture) { Write-Host "Save-goldens stored in $goldDir" -ForegroundColor Cyan; exit 0 }
if ($fail -eq 0) { Write-Host "PASS: all fixtures match save-goldens" -ForegroundColor Green; exit 0 }
Write-Host "FAIL: $fail fixture(s) differ" -ForegroundColor Red
exit 1
