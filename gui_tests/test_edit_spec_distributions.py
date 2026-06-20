"""GUI interaction test (Edit access spec - distribution sliders): select an
existing global spec, click Edit, and drive the two distribution sliders that no
prior test touched - "Percent Read/Write Distribution" and "Percent
Random/Sequential Distribution" - to deterministic endpoints (Home=min, End=max),
then verify the spec's %reads and %random fields in the saved ICF.

This locks the CSliderCtrl -> Test_Spec field mapping (read fraction, random
fraction) that the AccessDialog -> core consolidation must preserve. The existing
edit test only exercised the Burstiness edit boxes and the default 100/0 sliders;
nothing asserted that moving the sliders changes the saved %reads / %random.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_edit_spec_distributions.py
"""
import sys
import time
import subprocess
import pathlib

import pyautogui
import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
OUT = pathlib.Path("C:/tmp/gui_edit_spec_distributions.icf")

TREE_EXPAND_TESTMGR = (52, 141)
TREE_WORKER = (130, 157)
TAB_ACCESS_SPECS = (409, 98)
GLOBAL_SPEC_64K = (560, 152)         # "64 KiB; 100% Read; 0% random" in Global list
BTN_EDIT = (715, 165)

DLG_TITLE = "Edit Access Specification"
DLG_NAME = (190, 63)
DLG_SLIDER_READ = (610, 297)         # "Percent Read/Write Distribution" track
DLG_SLIDER_RANDOM = (120, 388)       # "Percent Random/Sequential Distribution" track
DLG_OK = (596, 512)

NAME = "DIST TEST"
EXP_READ = "0"                       # %read slider Home -> 0% read
EXP_RANDOM = "100"                   # %random slider End -> 100% random


def _dlg():
    return next((w for w in __import__("pygetwindow").getAllWindows()
                 if w.visible and w.title.strip() == DLG_TITLE), None)


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
    g.click_window(win, *TAB_ACCESS_SPECS); time.sleep(0.6)
    g.click_window(win, *GLOBAL_SPEC_64K); time.sleep(0.4)
    g.click_window(win, *BTN_EDIT); time.sleep(1.2)

    dlg = _dlg()
    if not dlg:
        g.kill_iometer(); return None, "Edit Access Specification dialog did not open"
    try: dlg.activate()
    except Exception: pass
    time.sleep(0.4)

    g.set_field(dlg, *DLG_NAME, NAME)                          # custom name so we can find it
    g.click_window(dlg, *DLG_SLIDER_READ); time.sleep(0.2)     # focus %read slider...
    pyautogui.press("home"); time.sleep(0.3)                   # ...Home -> 0% read
    g.click_window(dlg, *DLG_SLIDER_RANDOM); time.sleep(0.2)   # focus %random slider...
    pyautogui.press("end"); time.sleep(0.3)                    # ...End -> 100% random
    g.click_window(dlg, *DLG_OK); time.sleep(0.8)

    if not g.gui_save(win, OUT):
        g.kill_iometer(); return None, f"{OUT} not written"
    lines = OUT.read_text().splitlines()
    g.close_iometer(win)
    return lines, None


def main():
    lines, err = run()
    if err:
        print(f"FAIL: {err}"); return 1

    fields = None
    for i, ln in enumerate(lines):
        if ln.strip().startswith(NAME + ",") and i + 2 < len(lines):
            fields = [f.strip() for f in lines[i + 2].strip().split(",")]
            break
    print(f"edited spec size-line fields: {fields}")
    # size,%ofsize,%reads,%random,delay,burst,align,reply
    if fields and len(fields) >= 4 and fields[2] == EXP_READ and fields[3] == EXP_RANDOM:
        print(f"PASS: sliders set %reads={EXP_READ} %random={EXP_RANDOM} in the ICF")
        return 0
    print(f"FAIL: expected %reads={EXP_READ}, %random={EXP_RANDOM}; got {fields}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
