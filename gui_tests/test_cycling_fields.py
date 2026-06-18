"""GUI interaction test (#5 - PageSetup cycling fields): select a cycling test
type (Cycle Targets), which enables the Targets cycling fields, edit its Start /
Step and change the step-type combo, then verify the Disk Cycling line in the ICF.

Exercises PageSetup.cpp's cycling enable/edit paths (OnSelchangeCTestType ->
EnableCyclingInfo + the cycling field/step-type controls) beyond the run-time and
description edits already covered.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_cycling_fields.py
"""
import sys
import time
import subprocess
import pathlib

import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
OUT = pathlib.Path("C:/tmp/gui_cycling_fields.icf")

TAB_TEST_SETUP = (583, 98)
COMBO_CYCLING = (475, 327)           # Cycling Options (index 1 == Cycle Targets)
TARGETS_START = (445, 369)
TARGETS_STEP = (445, 393)
TARGETS_STEPTYPE = (465, 420)        # step-type combo (Linear Stepping)

NEW_START = "2"
NEW_STEP = "3"


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
    g.select_combo(win, *COMBO_CYCLING, key="down", presses=1)   # -> Cycle Targets
    time.sleep(0.4)
    g.set_field(win, *TARGETS_START, NEW_START)                  # now-enabled fields
    g.set_field(win, *TARGETS_STEP, NEW_STEP)
    g.select_combo(win, *TARGETS_STEPTYPE, key="down", presses=1)  # LINEAR -> next
    time.sleep(0.3)

    if not g.gui_save(win, OUT):
        g.kill_iometer(); return None, f"{OUT} not written"
    lines = OUT.read_text().splitlines()
    g.close_iometer(win)
    return lines, None


def _data_after(lines, marker):
    """First non-comment data line after a 'marker comment line."""
    grab = False
    for ln in lines:
        if ln.strip() == marker:
            grab = True; continue
        if grab and not ln.strip().startswith("'"):
            return ln.split()
    return None


def main():
    lines, err = run()
    if err:
        print(f"FAIL: {err}"); return 1

    disk = _data_after(lines, "'Disk Cycling")     # start step steptype
    print(f"Disk Cycling line: {disk}")
    if disk and len(disk) >= 3 and disk[0] == NEW_START and disk[1] == NEW_STEP and disk[2] != "LINEAR":
        print(f"PASS: cycling fields edited (start {NEW_START}, step {NEW_STEP}, step-type {disk[2]})")
        return 0
    print(f"FAIL: expected start={NEW_START} step={NEW_STEP} step-type!=LINEAR, got {disk}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
