"""GUI interaction test (edit -> save -> verify): change the Test Description on
the Test Setup tab, Save, and confirm the new text lands in the written ICF.

This is the safety-net pattern that guards behavior the byte-round-trip can't: a
GUI edit must propagate into the saved config. (Template for the data-model
migration's interaction coverage.)

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_edit_testsetup.py
"""
import sys
import time
import subprocess
import pathlib

import pyautogui
import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
OUT = pathlib.Path("C:/tmp/gui_edit_testsetup.icf")

TAB_TEST_SETUP = (583, 98)     # "Test Setup" tab, relative to main window
DESC_FIELD = (475, 142)        # Test Description edit box
NEW_DESC = "GUI EDIT VERIFY 20260616"   # no commas (the GUI sanitizes ',' -> '-')


def run():
    g.kill_iometer(); time.sleep(1.0)
    g.launch(ICF)
    win = g.find_window(timeout=20.0)
    if not win:
        return None, "no main window"
    subprocess.Popen([str(DYNAMOTEST), "-n", "TESTMGR", "-m", "localhost",
                      "--rdelay", "50", "-i", "127.0.0.1"])
    time.sleep(7.0)                          # connect; Waiting dialog auto-closes
    g.position(win, 50, 30, 1500, 950); time.sleep(1.5)
    win = g.find_window() or win

    g.click_window(win, *TAB_TEST_SETUP)     # switch to Test Setup
    time.sleep(0.7)
    g.set_field(win, *DESC_FIELD, NEW_DESC)  # clear + type the new description
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

    # The ICF writes:  'Test Description\n\t<description>
    desc = None
    for i, ln in enumerate(lines):
        if ln.strip() == "'Test Description" and i + 1 < len(lines):
            desc = lines[i + 1].strip()
            break
    print(f"saved description: {desc!r}")
    if desc == NEW_DESC:
        print("PASS: GUI edit of Test Description propagated to the saved ICF")
        return 0
    print(f"FAIL: expected {NEW_DESC!r}, got {desc!r}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
