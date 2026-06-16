# make_coverage_report.ps1 - produce the committed, browsable coverage report
# under <repo>/coverage. This is the single entry point; run it on demand and
# commit the /coverage tree.
#
# It runs BOTH coverage engines and assembles their HTML into one site:
#   - OpenCppCoverage (collect_coverage_all.ps1): LINE coverage over every real
#     MSVC binary - core, Qt GUI, Dynamo, AND (default) the MFC IOmeter GUI ->
#     coverage/full/   (the "browse every source file incl. MFC" view)
#   - llvm-cov (collect_coverage.ps1): LINE + BRANCH for the clang-cl-buildable
#     subset (core + Qt) -> coverage/branch/   (the "partial-branch" view)
# then writes coverage/index.html (headline metrics + links) and copies the
# llvm-cov text summary.
#
# Why two engines: only clang-cl/llvm-cov produces branch coverage, but it can't
# build the MFC GUI; only OpenCppCoverage (PDB-based) reaches the MFC binary, but
# it is line-only. The committed report carries both so every file is browsable
# (line) and core/Qt also shows branch detail.
#
# Flags:
#   -SkipMfc     OpenCppCoverage pass without the MFC GUI window (faster; full/
#                then has no MFC line data).
#   -IncludeGuiTests  First run the foreground PyAutoGUI suite under coverage
#                (gui_tests\collect_gui_coverage.ps1) so the interactive MFC
#                handlers are folded into full/. Requires an interactive desktop;
#                do not touch the mouse/keyboard during the run.
#   -NoCollect   Skip running the engines; just (re)assemble /coverage from the
#                existing build_cov/ and build_cov_occ/ outputs.
#   -Open        Open the landing page when done.

[CmdletBinding()]
param(
    [switch]$SkipMfc,
    [switch]$IncludeGuiTests,
    [switch]$NoCollect,
    [switch]$Open,
    [switch]$Publish,                  # push the assembled report to the gh-pages branch
    [string]$Branch = "gh-pages",
    [string]$QtPrefix = "C:\Qt\6.8.3\msvc2022_64"
)

$ErrorActionPreference = "Continue"
$here = $PSScriptRoot
$repo = (Resolve-Path (Join-Path $here "..\..")).Path
$out  = Join-Path $repo "coverage"

$llvmHtml = Join-Path $here "build_cov\coverage\html"
$llvmSum  = Join-Path $here "build_cov\coverage\summary.txt"
$occHtml  = Join-Path $here "build_cov_occ\html"
$occXml   = Join-Path $here "build_cov_occ\coverage.xml"

function Sect([string]$m) { Write-Host "" ; Write-Host $m -ForegroundColor Cyan }

if (-not $NoCollect) {
    # OpenCppCoverage needs a PDB-carrying Qt build (RelWithDebInfo). Build it if absent.
    $rwdiExe = Join-Path $here "build\RelWithDebInfo\IometerQt.exe"
    if (-not (Test-Path $rwdiExe)) {
        Sect "[prep] Building Qt RelWithDebInfo (PDBs for OpenCppCoverage)..."
        $env:PATH = "$QtPrefix\bin;$env:PATH"
        & cmake --build (Join-Path $here "build") --config RelWithDebInfo 2>&1 |
            Select-String -Pattern "error|-> " | Select-Object -Last 3
    }

    if ($IncludeGuiTests) {
        Sect "[prep] Foreground GUI-interaction coverage (do not touch mouse/keyboard)..."
        & (Join-Path $repo "gui_tests\collect_gui_coverage.ps1")
    }

    Sect "[1/3] llvm-cov: line + branch (core + Qt)..."
    & (Join-Path $here "collect_coverage.ps1")

    Sect "[2/3] OpenCppCoverage: line, all binaries$(if(-not $SkipMfc){' incl. MFC GUI'})..."
    if ($SkipMfc) {
        & (Join-Path $here "collect_coverage_all.ps1") -QtPrefix $QtPrefix
    } else {
        & (Join-Path $here "collect_coverage_all.ps1") -IncludeMfc -QtPrefix $QtPrefix
    }
}

Sect "[3/3] Assembling committed report -> $out"
Remove-Item $out -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $out -Force | Out-Null

if (Test-Path $occHtml)  { Copy-Item $occHtml  (Join-Path $out "full")   -Recurse -Force; Write-Host "  full/   <- OpenCppCoverage (line, all files)" }
else { Write-Host "  WARNING: OpenCppCoverage html missing ($occHtml)" -ForegroundColor Yellow }
if (Test-Path $llvmHtml) { Copy-Item $llvmHtml (Join-Path $out "branch") -Recurse -Force; Write-Host "  branch/ <- llvm-cov (line + branch, core+Qt)" }
else { Write-Host "  WARNING: llvm-cov html missing ($llvmHtml)" -ForegroundColor Yellow }
if (Test-Path $llvmSum)  { Copy-Item $llvmSum  (Join-Path $out "summary.txt") -Force }

# ---- metrics -----------------------------------------------------------------
$occLine = "n/a"
if (Test-Path $occXml) {
    try {
        [xml]$x = Get-Content $occXml
        $occLine = "{0:N1}%" -f ([double]$x.coverage.'line-rate' * 100)
    } catch {}
}
$llvmLine = "n/a"; $llvmBranch = "n/a"
if (Test-Path $llvmSum) {
    $totLine = (Get-Content $llvmSum | Select-String '^TOTAL')
    if ($totLine) {
        $f = ($totLine.Line -replace '%','') -split '\s+'
        # TOTAL regions missed cover funcs missed cover lines missed cover branches missed cover
        if ($f.Count -ge 13) { $llvmLine = "$($f[9])%"; $llvmBranch = "$($f[12])%" }
    }
}

$ts = Get-Date -Format "yyyy-MM-dd HH:mm"
$index = Join-Path $out "index.html"
$html = @"
<!DOCTYPE html><html><head><meta charset="utf-8"><title>Iometer coverage</title>
<style>
 body{font-family:Segoe UI,Arial,sans-serif;margin:2em;color:#222}
 h1{margin-bottom:0}.sub{color:#666;margin-top:.2em}
 table{border-collapse:collapse;margin:1.5em 0}td,th{border:1px solid #ccc;padding:.5em .9em;text-align:left}
 th{background:#f4f4f4}.big{font-size:1.4em;font-weight:bold}
 a.card{display:inline-block;border:1px solid #ccc;border-radius:6px;padding:1em 1.4em;margin:.4em 1em .4em 0;text-decoration:none;color:#06c}
 a.card:hover{background:#f0f7ff}.note{color:#555;font-size:.92em;max-width:60em}
</style></head><body>
<h1>Iometer &mdash; code coverage</h1>
<p class="sub">Generated $ts &middot; measured (not estimated) from the unit-test + smoke runs.</p>

<table>
 <tr><th>Report</th><th>Engine</th><th>Scope</th><th>Line</th><th>Branch</th></tr>
 <tr><td><a href="full/index.html">Full (all files)</a></td><td>OpenCppCoverage</td>
     <td>core + Qt + Dynamo + MFC GUI</td><td class="big">$occLine</td><td>&mdash;</td></tr>
 <tr><td><a href="branch/index.html">Branch detail</a></td><td>llvm-cov (clang-cl)</td>
     <td>core + Qt</td><td>$llvmLine</td><td class="big">$llvmBranch</td></tr>
</table>

<p>
 <a class="card" href="full/index.html">&#128193; Browse all source files (line coverage, incl. MFC)</a>
 <a class="card" href="branch/index.html">&#127807; Branch coverage (core + Qt)</a>
</p>

<p class="note">
 <b>Two engines, by necessity.</b> Only clang-cl/<code>llvm-cov</code> produces branch
 coverage, but it cannot build the MFC GUI; only OpenCppCoverage (PDB-based) reaches
 the MFC binary, but is line-only. So <b>Full</b> covers every file incl. MFC at line
 granularity, and <b>Branch detail</b> adds partial-branch highlighting for the
 clang-cl-buildable core + Qt. The MFC line data covers the batch run paths plus,
 when the foreground PyAutoGUI suite is collected (<code>make_coverage_report.ps1
 -IncludeGuiTests</code>), the interactive GUI handlers (tab edits, the Save
 dialog, access-spec assign/remove/reorder). Regenerate with
 <code>src/qt/make_coverage_report.ps1</code>. See <code>coverage/summary.txt</code>
 for the per-file llvm-cov table.
</p>
</body></html>
"@
Set-Content -Path $index -Value $html -Encoding UTF8
# Disable Jekyll on GitHub Pages (it skips files/dirs starting with _).
Set-Content -Path (Join-Path $out ".nojekyll") -Value "" -Encoding ASCII

# ---- publish to gh-pages (optional) ------------------------------------------
# The report is NOT committed to the source tree; it is force-pushed to an orphan
# gh-pages branch (fresh single-commit history each run, so it never accumulates).
# Done in a throwaway temp repo so the source working tree / index are untouched.
if ($Publish) {
    Sect "[publish] -> $Branch branch (GitHub Pages)"
    $originUrl = (& git -C $repo config --get remote.origin.url).Trim()
    if (-not $originUrl) { Write-Host "  ERROR: no origin remote" -ForegroundColor Red; return }
    $tmp = Join-Path $env:TEMP ("iometer-ghp-" + [System.Guid]::NewGuid().ToString('N').Substring(0,8))
    New-Item -ItemType Directory -Path $tmp -Force | Out-Null
    Copy-Item (Join-Path $out '*') $tmp -Recurse -Force
    Push-Location $tmp
    & git init -q
    & git checkout -q -b $Branch
    & git add -A
    & git -c user.name="coverage-bot" -c user.email="noreply@eiler.net" commit -q -m "Coverage report (generated $ts)"
    & git push -q --force $originUrl "$($Branch):$Branch"
    $pushOk = $LASTEXITCODE -eq 0
    Pop-Location
    Remove-Item $tmp -Recurse -Force -ErrorAction SilentlyContinue
    if ($pushOk) {
        $slug = ($originUrl -replace '.*github\.com[:/]','' -replace '\.git$','')
        $owner = ($slug -split '/')[0]; $name = ($slug -split '/')[1]
        Write-Host "  pushed $Branch. Pages URL (once enabled): https://$owner.github.io/$name/" -ForegroundColor Green
    } else {
        Write-Host "  push FAILED" -ForegroundColor Red
    }
}

Sect "Done."
Write-Host "  Full (line, all incl MFC) : $occLine"
Write-Host "  Branch (core+Qt)          : line $llvmLine / branch $llvmBranch"
Write-Host "  Landing: $index"
if ($Open -and (Test-Path $index)) { Start-Process $index }
