"""GUI interaction test (#9 - Test Setup options): on the Test Setup tab set the
Run Time seconds and Ramp Up Time, and change the Cycling Options (test type),
then verify they land in the TEST SETUP section of the saved ICF.

Exercises PageSetup.cpp beyond the description edit (test_edit_testsetup.py):
the run-time / ramp-up edits and the cycling-options combo (Test Type handler).

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_edit_runtime_cycling.py
"""
import sys
import time
import subprocess
import pathlib

import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
OUT = pathlib.Path("C:/tmp/gui_runtime_cycling.icf")

TAB_TEST_SETUP = (583, 98)
FIELD_RUN_SECONDS = (215, 254)       # Run Time: Seconds, shows 3
FIELD_RAMP_SECONDS = (375, 204)      # Ramp Up Time (Seconds), shows 0
COMBO_CYCLING = (475, 327)           # "Cycling Options", shows "Normal -- ..."

NEW_SECONDS = "10"
NEW_RAMP = "5"


def run():
    g.kill_iometer(); time.sleep(1.0)
    g.launch(ICF)
    win = g.find_window(timeout=20.0)
    if not win:
        return None, "no main window"
    subprocess.Popen([str(DYNAMOTEST), "-n", "TESTMGR", "-m", "localhost",
                      "--rdelay", "50", "-i", "127.0.0.1"])
    time.sleep(7.0)
    g.position(win, 50, 30, 1500, 950); time.sleep(1.5)
    win = g.find_window() or win

    g.click_window(win, *TAB_TEST_SETUP); time.sleep(0.6)
    g.set_field(win, *FIELD_RUN_SECONDS, NEW_SECONDS)
    g.set_field(win, *FIELD_RAMP_SECONDS, NEW_RAMP)
    g.select_combo(win, *COMBO_CYCLING, key="down", presses=1)   # Normal -> next type
    time.sleep(0.3)

    if not g.gui_save(win, OUT):
        g.kill_iometer()
        return None, f"{OUT} not written"
    lines = OUT.read_text().splitlines()
    g.close_iometer(win)
    return lines, None


def _after(lines, marker):
    for i, ln in enumerate(lines):
        if ln.strip() == marker and i + 1 < len(lines):
            return lines[i + 1].strip()
    return None


def main():
    lines, err = run()
    if err:
        print(f"FAIL: {err}"); return 1

    # Run Time data line follows the "hours minutes seconds" comment.
    run_secs = None
    for i, ln in enumerate(lines):
        if ln.strip().startswith("'") and "hours" in ln and "seconds" in ln and i + 1 < len(lines):
            run_secs = lines[i + 1].split()[-1]
            break
    ramp = _after(lines, "'Ramp Up Time (s)")
    ttype = _after(lines, "'Test Type")
    print(f"run-seconds={run_secs!r} ramp={ramp!r} test-type={ttype!r}")

    ok = (run_secs == NEW_SECONDS and ramp == NEW_RAMP and ttype not in (None, "NORMAL"))
    if ok:
        print(f"PASS: Test Setup edits reached the ICF (run {NEW_SECONDS}s, ramp {NEW_RAMP}s, type {ttype})")
        return 0
    print(f"FAIL: expected run={NEW_SECONDS}, ramp={NEW_RAMP}, type!=NORMAL")
    return 1


if __name__ == "__main__":
    sys.exit(main())
