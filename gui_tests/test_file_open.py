"""GUI interaction test (#F - File > Open): load a different config through the
custom "Open Test Configuration File" dialog (with Managers and Workers unchecked
so it doesn't wait for managers), then verify the opened file's Test Description
replaced the current one in a subsequent save.

Exercises ICFOpenDialog.cpp (the only GUI source file still at 0%): file
selection, the "Settings to restore" checkboxes, OnOK, and the load path through
ICF_ifstream.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_file_open.py
"""
import sys
import time
import subprocess
import pathlib

import iometer_gui as g

START_ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
OPEN_ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "rampup_prefix.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
OUT = pathlib.Path("C:/tmp/gui_file_open.icf")

# rampup_prefix.icf's Test Description (comma-free, so it round-trips verbatim).
EXPECT_DESC = "icf compat: bare ramp up time key"


def run():
    g.kill_iometer(); time.sleep(1.0)
    g.launch(START_ICF)
    win = g.find_window(timeout=20.0)
    if not win:
        return None, "no main window"
    subprocess.Popen([str(DYNAMOTEST), "-n", "TESTMGR", "-m", "localhost",
                      "--rdelay", "50", "-i", "127.0.0.1"])
    time.sleep(7.0)
    g.position(win, 50, 30, 1500, 950); time.sleep(1.5)
    win = g.find_window() or win

    # Open the other config, skipping the manager list (so no manager wait).
    if not g.gui_open(win, OPEN_ICF, uncheck=[g.SAVE_DLG_CHK_MANAGERS]):
        g.kill_iometer(); return None, "Open dialog did not appear"
    time.sleep(1.0)
    win = g.find_window() or win

    if not g.gui_save(win, OUT):
        g.kill_iometer(); return None, f"{OUT} not written"
    lines = OUT.read_text().splitlines()
    g.close_iometer(win)
    return lines, None


def main():
    lines, err = run()
    if err:
        print(f"FAIL: {err}"); return 1

    desc = None
    for i, ln in enumerate(lines):
        if ln.strip() == "'Test Description" and i + 1 < len(lines):
            desc = lines[i + 1].strip()
            break
    print(f"description after File>Open: {desc!r}")
    if desc == EXPECT_DESC:
        print("PASS: File>Open loaded the config via ICFOpenDialog (description applied)")
        return 0
    print(f"FAIL: expected {EXPECT_DESC!r}, got {desc!r}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
