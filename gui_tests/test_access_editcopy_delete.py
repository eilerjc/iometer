"""GUI interaction test (#3 - PageAccess Edit Copy / Delete): delete one global
access spec and Edit-Copy another into a new named spec, then verify both changes
in the saved ICF.

Exercises the PageAccess.cpp OnBDelete and OnBEditCopy handlers (and the global
spec list management) - paths the assign/remove/new tests don't reach.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_access_editcopy_delete.py
"""
import sys
import time
import subprocess
import pathlib

import pygetwindow as gw
import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
OUT = pathlib.Path("C:/tmp/gui_access_editcopy_delete.icf")

TREE_EXPAND_TESTMGR = (52, 141)
TREE_WORKER = (130, 157)
TAB_ACCESS_SPECS = (409, 98)
GLOBAL_SPEC_64K = (560, 152)         # "64 KiB; 100% Read; 0% random"
GLOBAL_SPEC_4K = (560, 168)          # "4 KiB; 50% Read; 100% random"
BTN_EDIT_COPY = (715, 192)
BTN_DELETE = (715, 220)

DLG_TITLE = "Edit Access Specification"
DLG_NAME = (190, 63)
DLG_OK = (596, 512)

COPY_NAME = "COPY SPEC GUI"
DELETED = "4 KiB; 50% Read; 100% random"


def _dlg():
    return next((w for w in gw.getAllWindows()
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

    # Delete the (unassigned) 4 KiB spec first (positions are stable before the copy).
    g.click_window(win, *GLOBAL_SPEC_4K); time.sleep(0.4)
    g.click_window(win, *BTN_DELETE); time.sleep(0.5)

    # Edit-Copy the 64 KiB spec into a new named spec.
    g.click_window(win, *GLOBAL_SPEC_64K); time.sleep(0.4)
    g.click_window(win, *BTN_EDIT_COPY); time.sleep(1.2)
    dlg = _dlg()
    if not dlg:
        g.kill_iometer(); return None, "Edit Copy dialog did not open"
    try: dlg.activate()
    except Exception: pass
    time.sleep(0.4)
    g.set_field(dlg, *DLG_NAME, COPY_NAME)
    g.click_window(dlg, *DLG_OK); time.sleep(0.8)

    if not g.gui_save(win, OUT):
        g.kill_iometer(); return None, f"{OUT} not written"
    text = OUT.read_text()
    g.close_iometer(win)
    return text, None


def main():
    text, err = run()
    if err:
        print(f"FAIL: {err}"); return 1

    has_copy = (COPY_NAME + ",") in text
    deleted_gone = (DELETED + ",NONE") not in text
    print(f"copy present={has_copy}  4KiB-deleted={deleted_gone}")
    if has_copy and deleted_gone:
        print("PASS: Edit Copy created a new spec and Delete removed the 4 KiB spec")
        return 0
    print("FAIL: expected the copy present and the 4 KiB global spec removed")
    return 1


if __name__ == "__main__":
    sys.exit(main())
