"""GUI interaction test (access-spec reordering): with two specs assigned to a
worker, use "Move Up" on the Access Specifications tab to reorder them, then
verify the new order in the worker's "Assigned access specs" block in the ICF.

Assigned-spec ordering is meaningful (it's the run order), lives only in the
live model, and has no byte-golden coverage - so this guards another corner of
the Test_Spec model the data-model unification touches.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_reorder_spec.py
"""
import sys
import time
import subprocess
import pathlib

import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
OUT = pathlib.Path("C:/tmp/gui_reorder_spec.icf")

TREE_EXPAND_TESTMGR = (52, 141)
TREE_WORKER = (130, 157)
TAB_ACCESS_SPECS = (409, 98)
GLOBAL_SPEC_4K = (560, 168)         # "4 KiB; 50% Read; 100% random" in Global list
BTN_ADD = (435, 264)               # "<< Add"
ASSIGNED_ROW_2 = (290, 152)        # second item in the Assigned list
BTN_MOVE_UP = (250, 428)           # "Move Up"

SPEC_64K = "64 KiB; 100% Read; 0% random"   # initially assigned (row 1)
SPEC_4K = "4 KiB; 50% Read; 100% random"    # added, then moved up to row 1


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
    g.click_window(win, *TAB_ACCESS_SPECS); time.sleep(0.7)

    # Assign a 2nd spec so there's something to reorder -> [64 KiB, 4 KiB].
    g.click_window(win, *GLOBAL_SPEC_4K); time.sleep(0.4)
    g.click_window(win, *BTN_ADD); time.sleep(0.5)

    # Select the 2nd assigned row (4 KiB) and Move Up -> [4 KiB, 64 KiB].
    g.click_window(win, *ASSIGNED_ROW_2); time.sleep(0.4)
    g.click_window(win, *BTN_MOVE_UP); time.sleep(0.5)

    if not g.gui_save(win, OUT):
        g.kill_iometer()
        return None, f"{OUT} not written"
    lines = OUT.read_text().splitlines()
    g.kill_iometer()
    return lines, None


def assigned_specs(lines):
    out, grab = [], False
    for ln in lines:
        s = ln.strip()
        if s == "'Assigned access specs":
            grab = True; continue
        if s == "'End assigned access specs":
            break
        if grab and s:
            out.append(s)
    return out


def main():
    lines, err = run()
    if err:
        print(f"FAIL: {err}"); return 1

    specs = assigned_specs(lines)
    print(f"assigned specs after add + move-up: {specs}")
    if specs == [SPEC_4K, SPEC_64K]:
        print("PASS: GUI Move Up reordered the worker's assigned specs in the saved ICF")
        return 0
    print(f"FAIL: expected {[SPEC_4K, SPEC_64K]}, got {specs}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
