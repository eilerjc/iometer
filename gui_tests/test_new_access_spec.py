"""GUI interaction test (#1 - New access spec dialog): on the Access
Specifications tab click "New", give the spec a known name and Transfer Request
Size in the "Edit Access Specification" dialog, click OK, then verify the new
global spec (name + size) appears in the saved ICF.

Exercises AccessDialog.cpp (CAccessDialog: OnInitDialog / DoDataExchange / the
size + name handlers / OnOK) - the largest GUI source file with no prior
coverage - plus the PageAccess "New" handler.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_new_access_spec.py
"""
import sys
import time
import subprocess
import pathlib

import pyautogui
import pygetwindow as gw
import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
OUT = pathlib.Path("C:/tmp/gui_new_access_spec.icf")

TREE_EXPAND_TESTMGR = (52, 141)
TREE_WORKER = (130, 157)
TAB_ACCESS_SPECS = (409, 98)
BTN_NEW = (715, 137)                 # "New" on the Access Specifications tab

DLG_TITLE = "Edit Access Specification"
DLG_NAME = (190, 63)                 # Name field (opens with "Untitled 1" selected)
DLG_SIZE_KB = (127, 307)             # Transfer Request Size: Kilobytes spin-edit
DLG_OK = (596, 512)

SPEC_NAME = "GUI NEW SPEC 8K"
SIZE_KB = "8"
EXPECT_BYTES = "8192"                # 8 KiB


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
    g.click_window(win, *BTN_NEW); time.sleep(1.2)

    dlg = _dlg()
    if not dlg:
        g.kill_iometer(); return None, "Edit Access Specification dialog did not open"
    try: dlg.activate()
    except Exception: pass
    time.sleep(0.4)

    g.set_field(dlg, *DLG_NAME, SPEC_NAME)        # replace "Untitled 1"
    g.set_field(dlg, *DLG_SIZE_KB, SIZE_KB)       # 2 KiB -> 8 KiB
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

    size = None
    for i, ln in enumerate(lines):
        if ln.strip().startswith(SPEC_NAME + ","):     # 'name,default assignment' data line
            # the size detail line is two lines down (after the 'size,...' comment)
            if i + 2 < len(lines):
                size = lines[i + 2].strip().split(",")[0]
            break
    print(f"new spec {SPEC_NAME!r} size in ICF: {size!r}")
    if size == EXPECT_BYTES:
        print(f"PASS: New access spec created via AccessDialog and saved ({EXPECT_BYTES} bytes)")
        return 0
    print(f"FAIL: expected {SPEC_NAME!r} with size {EXPECT_BYTES}, got size {size!r}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
