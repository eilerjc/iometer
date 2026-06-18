"""GUI interaction test (#D - Edit access spec dialog): select an existing global
spec, click Edit, change its Burstiness (Transfer Delay + Burst Length) and
exercise Insert After / Delete row in the "Edit Access Specification" dialog, then
verify the spec's delay+burst changed in the saved ICF.

Edit (vs New) loads an existing spec into the dialog, hitting AccessDialog.cpp
paths the new-spec test doesn't (OnInitDialog populate, row insert/delete, OnOK
write-back), plus the PageAccess Edit handler.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_edit_access_spec.py
"""
import sys
import time
import subprocess
import pathlib

import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
OUT = pathlib.Path("C:/tmp/gui_edit_access_spec.icf")

TREE_EXPAND_TESTMGR = (52, 141)
TREE_WORKER = (130, 157)
TAB_ACCESS_SPECS = (409, 98)
GLOBAL_SPEC_64K = (560, 152)         # "64 KiB; 100% Read; 0% random" in Global list
BTN_EDIT = (715, 165)                # "Edit" on the Access Specifications tab

DLG_TITLE = "Edit Access Specification"
DLG_DELAY = (313, 408)               # Burstiness: Transfer Delay (ms)
DLG_BURST = (403, 408)               # Burstiness: Burst Length (I/Os)
DLG_INSERT_AFTER = (680, 136)
DLG_DELETE_ROW = (680, 168)
DLG_OK = (596, 512)

SPEC = "64 KiB; 100% Read; 0% random"
NEW_DELAY = "100"
NEW_BURST = "4"


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

    g.set_field(dlg, *DLG_DELAY, NEW_DELAY)       # on the spec's (single) row
    g.set_field(dlg, *DLG_BURST, NEW_BURST)
    g.click_window(dlg, *DLG_INSERT_AFTER); time.sleep(0.4)   # add a row...
    g.click_window(dlg, *DLG_DELETE_ROW); time.sleep(0.4)     # ...then remove it
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
        if ln.strip().startswith(SPEC + ",") and i + 2 < len(lines):
            fields = [f.strip() for f in lines[i + 2].strip().split(",")]
            break
    print(f"edited spec size-line fields: {fields}")
    # size,%ofsize,%reads,%random,delay,burst,align,reply
    if fields and len(fields) >= 6 and fields[4] == NEW_DELAY and fields[5] == NEW_BURST:
        print(f"PASS: Edit dialog changed delay={NEW_DELAY} burst={NEW_BURST} in the ICF")
        return 0
    print(f"FAIL: expected delay={NEW_DELAY}, burst={NEW_BURST}; got {fields}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
