# Iometer GUI interaction tests (PyAutoGUI)

Image/coordinate-driven GUI tests for the **MFC** Iometer GUI (and later the Qt
GUI), built with [PyAutoGUI]. They cover the interactive behavior the byte-golden
harnesses (`test/smoke/icf_*_golden.ps1`) can't reach - drag-assign, default
spec assignment, validation, cycling - by **driving the GUI and asserting through
the existing ICF / results goldens** (perform actions -> Save -> byte-compare),
so the oracle stays deterministic rather than pixel-based.

Primary purpose: a safety net for the live data-model work (the migration that
would unlock the Tier 8 / cycling extractions), since that behavior has no other
automated coverage.

## Requirements
- An **interactive desktop session, run in the FOREGROUND.** These drive the real
  mouse/keyboard and screen-capture; a backgrounded run loses window focus and the
  clicks/keys land nowhere (the Save toolbar won't open its dialog, etc.).
- `python -m pip install pyautogui opencv-python pygetwindow`
- A built MFC `Release|x64` `IOmeter.exe`.

## Tests
Run them all:  `python gui_tests/run_gui_tests.py`  (foreground).
- `test_save_roundtrip.py` - loads `worker_rich` (+ connected dynamotest), drives
  **File > Save**, asserts the written ICF byte-matches
  `golden_save/worker_rich.save.txt`. Proves the GUI Save path end to end.
- `test_edit_testsetup.py` - the edit->save->verify pattern: changes the Test
  Description on the Test Setup tab, Saves, confirms the new text lands in the
  ICF. This is how interaction tests guard the data model.
- `test_edit_worker.py` - the same pattern against the **live Worker** model:
  changes "# of Outstanding I/Os" on the Disk Targets tab and verifies it reaches
  the worker in the saved ICF. The coverage most relevant to the data-model
  unification (which migrates exactly these per-worker target settings).
- `test_assign_spec.py` - assigns an additional access spec to a worker via the
  Access Specifications tab (`<< Add`) and verifies it lands in the worker's
  "Assigned access specs" block. Exercises the Test_Spec assignment model, which
  has no byte-golden coverage.
- `test_remove_spec.py` - the mirror: removes the worker's assigned spec via
  `Remove >>` and verifies the assigned block is empty. Together with the assign
  test, covers both directions of the assignment model.
- `test_reorder_spec.py` - assigns a 2nd spec, then `Move Up` on it, and verifies
  the new run order in the assigned block. Guards assigned-spec ordering.
- `test_edit_disk_targets.py` - edits Maximum Disk Size, Starting Sector and the
  Write IO Data Pattern on the Disk Targets tab; verifies the worker geometry line
  (`PageDisk.cpp`).
- `test_edit_results_display.py` - flips "Results Since" to Start of Test and bumps
  the Update Frequency; verifies the RESULTS DISPLAY section (`PageDisplay.cpp`).
- `test_edit_runtime_cycling.py` - sets Run Time / Ramp Up and changes the Cycling
  Options (test type); verifies the TEST SETUP section (`PageSetup.cpp`).
- `test_save_subset.py` - unchecks "Settings to save" boxes in the Save dialog and
  verifies the omitted sections are absent (`ICFSaveDialog.cpp`).
- `test_about_eula.py` - opens the About ("Iometer Information") dialog and, from it,
  the Intel Open Source License box, then closes both (`GalileoApp.cpp`,
  `LegalBox.cpp`). No ICF assertion - checks the modals open/close cleanly.
- `test_new_access_spec.py` - clicks "New" on the Access Specifications tab, names a
  spec and sets its Transfer Request Size in the "Edit Access Specification" dialog,
  and verifies the new global spec lands in the ICF (`AccessDialog.cpp`).
- `test_bigmeter.py` - opens the Presentation Meter from a Results Display ">" button,
  runs a short synthetic test so the needle updates, then closes it (`BigMeter.cpp`,
  `MeterCtrl.cpp`; also the start/stop + results path). The only test that runs a test.

## Coverage
Run `.\collect_gui_coverage.ps1` (foreground).
It runs each test with the MFC `IOmeter.exe` launched under **OpenCppCoverage**
(PDB-based; no rebuild) and merges the runs into `cov_html\index.html` +
`cov_coverage.xml`. `iometer_gui.launch()` does the wrapping when `IOCOV_DIR` is
set, so the tests orchestrate exactly as normal.

This reaches the **interactive** MFC handlers the batch `/c /r /t` scenario in
`src\qt\collect_coverage_all.ps1` can't: tab switches, field edits, the Save
dialog, and the access-spec assign/remove/reorder buttons - e.g. `PageAccess.cpp`
~59%, `PageSetup.cpp` ~71%, `ICFSaveDialog.cpp` ~70%, `AccessSpecList.cpp` ~87%.

## Gotchas learned (codified in the scripts)
- Main window title is `Iometer <version>  [Built: ...]`; match `^Iometer\s\d`
  so an editor window with "iometer" in its path isn't picked up.
- Loading a non-local ICF raises a modal **`Waiting for Managers`** dialog;
  dismiss it (Esc) for Test-Setup/Access-Spec-only tests, or connect a
  `dynamotest` for tests needing the full manager list.
- The Save dialog's **File name** field opens with "Iometer" pre-selected - type
  the path **directly** to replace it (clicking + Ctrl+A / triple-click / End+
  Shift+Home do NOT reliably select-all in that combo edit).
- All six "Settings to save" checkboxes default checked = the full ICF (same as
  the `/s` batch flag), so a plain Save matches the save-golden.

The MFC GUI is launched with a **non-local-manager ICF** so it loads a config
without auto-spawning the elevated local Dynamo (no UAC prompt to block
automation) - the same trick the coverage harness uses.

## Layout
- `iometer_gui.py` - helpers: launch, find/position the window, screenshot, close.
- `probe_mfc.py`   - viability probe (launch + capture; no assertions).
- `artifacts/`     - generated screenshots (gitignored).

[PyAutoGUI]: https://pyautogui.readthedocs.io/
