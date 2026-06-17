"""GUI interaction test (#3 - Disk Targets full edit): change Maximum Disk Size,
Starting Disk Sector, and the Write IO Data Pattern on the Disk Targets tab, then
verify all three land in the worker's disk-geometry line in the saved ICF.

Exercises PageDisk.cpp (the disk-target field handlers + the data-pattern combo)
and the Worker disk-geometry model, beyond the single # Outstanding I/Os field
test_edit_worker.py already covers.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_edit_disk_targets.py
"""
import sys
import time
import subprocess
import pathlib

import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
OUT = pathlib.Path("C:/tmp/gui_disk_targets.icf")

TREE_EXPAND_TESTMGR = (52, 141)
TREE_WORKER = (130, 157)
TAB_DISK_TARGETS = (224, 98)
FIELD_MAX_DISK_SIZE = (450, 156)     # "Maximum Disk Size" (Sectors), shows 1024
FIELD_START_SECTOR = (450, 210)      # "Starting Disk Sector", shows 64
COMBO_DATA_PATTERN = (485, 420)      # "Write IO Data Pattern", shows "Pseudo random" (=1)

NEW_MAX = "2048"
NEW_START = "128"
# Data pattern starts at "Pseudo random" (index 1); one Up -> "Repeating bytes" (0).
EXPECT = [NEW_MAX, NEW_START, "0"]


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

    g.click_window(win, *TREE_EXPAND_TESTMGR); time.sleep(0.5)
    g.click_window(win, *TREE_WORKER); time.sleep(0.5)
    g.click_window(win, *TAB_DISK_TARGETS); time.sleep(0.6)
    g.set_field(win, *FIELD_MAX_DISK_SIZE, NEW_MAX)
    g.set_field(win, *FIELD_START_SECTOR, NEW_START)
    g.select_combo(win, *COMBO_DATA_PATTERN, key="up", presses=1)
    g.click_window(win, *TREE_WORKER)            # commit the edits
    time.sleep(0.4)

    if not g.gui_save(win, OUT):
        g.kill_iometer()
        return None, f"{OUT} not written"
    lines = OUT.read_text().splitlines()
    g.close_iometer(win)
    return lines, None


def main():
    lines, err = run()
    if err:
        print(f"FAIL: {err}"); return 1

    geom = None
    for i, ln in enumerate(lines):
        if ln.strip().startswith("'Disk maximum size") and i + 1 < len(lines):
            geom = [f.strip() for f in lines[i + 1].strip().split(",")]
            break
    print(f"saved disk geometry [max,start,pattern]: {geom}")
    if geom == EXPECT:
        print(f"PASS: GUI edits of disk geometry reached the worker ({EXPECT})")
        return 0
    print(f"FAIL: expected {EXPECT}, got {geom}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
