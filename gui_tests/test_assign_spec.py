"""GUI interaction test (access-spec assignment): assign an additional access
spec to a worker via the Access Specifications tab and verify it lands in the
worker's "Assigned access specs" block in the saved ICF.

Assignment is the core of the Test_Spec data model the unification touches, and
it has no byte-golden coverage (the assigned list is a live per-worker thing), so
this is high-value safety-net coverage.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_assign_spec.py
"""
import sys
import time
import subprocess
import pathlib

import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
OUT = pathlib.Path("C:/tmp/gui_assign_spec.icf")

TREE_EXPAND_TESTMGR = (52, 141)     # [+] box next to TESTMGR
TREE_WORKER = (130, 157)            # "Rich Worker" node (after expand)
TAB_ACCESS_SPECS = (409, 98)
GLOBAL_SPEC_4K = (560, 168)         # "4 KiB; 50% Read; 100% random" in Global list
BTN_ADD = (435, 264)               # "<< Add"

SPEC_NAME = "4 KiB; 50% Read; 100% random"


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

    g.click_window(win, *TREE_EXPAND_TESTMGR); time.sleep(0.5)   # expand manager
    g.click_window(win, *TREE_WORKER); time.sleep(0.5)           # select the worker
    g.click_window(win, *TAB_ACCESS_SPECS); time.sleep(0.7)
    g.click_window(win, *GLOBAL_SPEC_4K); time.sleep(0.4)        # pick a global spec
    g.click_window(win, *BTN_ADD); time.sleep(0.5)               # << Add -> assign it

    if not g.gui_save(win, OUT):
        g.kill_iometer()
        return None, f"{OUT} not written"
    lines = OUT.read_text().splitlines()
    g.close_iometer(win)
    return lines, None


def assigned_specs(lines):
    """Return the worker's assigned-spec names (between the marker comments)."""
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
    print(f"assigned specs after GUI add: {specs}")
    if SPEC_NAME in specs and len(specs) == 2:
        print(f"PASS: GUI Add assigned {SPEC_NAME!r} to the worker in the saved ICF")
        return 0
    print(f"FAIL: expected 2 specs incl. {SPEC_NAME!r}, got {specs}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
