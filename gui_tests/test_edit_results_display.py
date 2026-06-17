"""GUI interaction test (#5 - Results Display config): on the Results Display tab
switch "Results Since" to Start of Test and bump the Update Frequency, then verify
both land in the RESULTS DISPLAY section of the saved ICF.

Exercises PageDisplay.cpp (the "Results Since" radio + update-frequency combo
handlers) - a tab with no other automated coverage.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_edit_results_display.py
"""
import sys
import time
import subprocess
import pathlib

import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
OUT = pathlib.Path("C:/tmp/gui_results_display.icf")

TAB_RESULTS = (508, 98)
RADIO_START_OF_TEST = (517, 140)     # "Results Since: Start of Test" -> WHOLE_TEST
COMBO_UPDATE_FREQ = (672, 150)       # "Update Frequency (seconds)", shows 1

# Default is "DISABLED,1,LAST_UPDATE"; expect freq 2 and WHOLE_TEST after edits.
EXPECT = "DISABLED,2,WHOLE_TEST"


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

    g.click_window(win, *TAB_RESULTS); time.sleep(0.6)
    g.click_window(win, *RADIO_START_OF_TEST); time.sleep(0.4)   # -> WHOLE_TEST
    g.select_combo(win, *COMBO_UPDATE_FREQ, key="down", presses=1)  # 1 -> 2
    time.sleep(0.3)

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

    val = None
    for i, ln in enumerate(lines):
        if ln.strip().startswith("'Record Last Update Results") and i + 1 < len(lines):
            val = lines[i + 1].strip()
            break
    print(f"saved results-display line: {val!r}")
    if val == EXPECT:
        print(f"PASS: GUI Results Display edits reached the ICF ({EXPECT})")
        return 0
    print(f"FAIL: expected {EXPECT!r}, got {val!r}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
