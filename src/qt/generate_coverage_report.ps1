# generate_coverage_report.ps1 - DEPRECATED.
#
# The previous version of this script emitted a hand-authored HTML page with
# HARD-CODED, fabricated coverage numbers (invented per-line maps, "102 tests",
# "100%" everywhere). Those figures were not measured from anything and were
# misleading.
#
# Real, measured coverage now comes from collect_coverage.ps1, which builds an
# instrumented (clang-cl) tree, runs the test suite, and renders genuine
# llvm-cov line-by-line HTML.
#
# This stub forwards to it so nothing silently produces fake data.

Write-Host "generate_coverage_report.ps1 is deprecated - delegating to collect_coverage.ps1" -ForegroundColor Yellow
& "$PSScriptRoot\collect_coverage.ps1" @args
exit $LASTEXITCODE
