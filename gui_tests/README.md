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
- `test_save_roundtrip.py` - loads `worker_rich` (+ connected dynamotest), drives
  **File > Save**, and asserts the written ICF byte-matches
  `golden_save/worker_rich.save.txt`. Proves the GUI Save path end to end.

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
