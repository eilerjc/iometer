"""GUI interaction test (#E - Results Display stat menus): open the statistic
popup on bar-chart row 1 and pick a different statistic, and open the row-2 stat
menu and the worker-selection menu (dismissed), then verify the chosen statistic
in the saved RESULTS DISPLAY section.

Opening these popups builds the (large) hierarchical result/worker menus in
PageDisplay.cpp + GalileoView.cpp, and picking an item exercises the selection
handler - paths with no other coverage.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_results_stat_menu.py
"""
import sys
import time
import subprocess
import pathlib

import pyautogui
import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
OUT = pathlib.Path("C:/tmp/gui_stat_menu.icf")

TAB_RESULTS = (508, 98)
STAT_BTN_1 = (290, 217)              # bar-chart row 1 statistic button
STAT_BTN_2 = (290, 260)             # row 2
WORKER_BTN_1 = (415, 201)           # row 1 worker-selection ("All Managers")

# Down (1st category) -> Right (open submenu) -> Down (2nd item) -> Enter.
EXPECT_STAT = "Read I/Os per Second"


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

    # Row 1: open the stat menu and pick a different statistic.
    g.click_window(win, *STAT_BTN_1); time.sleep(0.7)
    for k in ("down", "right", "down", "enter"):
        pyautogui.press(k); time.sleep(0.4)

    # Extra coverage: open the row-2 stat menu and the worker menu, dismiss them.
    g.click_window(win, *STAT_BTN_2); time.sleep(0.6); pyautogui.press("escape"); time.sleep(0.3)
    g.click_window(win, *WORKER_BTN_1); time.sleep(0.6); pyautogui.press("escape"); time.sleep(0.3)

    if not g.gui_save(win, OUT):
        g.kill_iometer(); return None, f"{OUT} not written"
    lines = OUT.read_text().splitlines()
    g.close_iometer(win)
    return lines, None


def main():
    lines, err = run()
    if err:
        print(f"FAIL: {err}"); return 1

    stat = None
    for i, ln in enumerate(lines):
        if ln.strip() == "'Bar chart 1 statistic" and i + 1 < len(lines):
            stat = lines[i + 1].strip()
            break
    print(f"bar chart 1 statistic after menu pick: {stat!r}")
    if stat == EXPECT_STAT:
        print(f"PASS: stat popup menu changed bar-chart 1 to {EXPECT_STAT!r}")
        return 0
    print(f"FAIL: expected {EXPECT_STAT!r}, got {stat!r}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
