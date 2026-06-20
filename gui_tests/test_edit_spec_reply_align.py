"""GUI interaction test (Edit access spec - Reply Size + Alignment): select an
existing global spec, click Edit, enable a non-zero Reply Size and switch the
"Align I/Os on" radio from Sector to Request-Size boundaries, then verify the
spec's reply and align fields in the saved ICF.

These two controls had no prior coverage. AccessDialog maps them as:
  - Reply Size "Size" radio + 8 Kilobytes      -> reply = 8192
  - Align "Request Size Boundaries"            -> align = transfer size (SetAlign(GetSize()))
For the 64 KiB spec that means align = 65536. Locking both guards the
AccessDialog -> core consolidation against silently dropping reply/alignment.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_edit_spec_reply_align.py
"""
import sys
import time
import subprocess
import pathlib

import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
OUT = pathlib.Path("C:/tmp/gui_edit_spec_reply_align.icf")

TREE_EXPAND_TESTMGR = (52, 141)
TREE_WORKER = (130, 157)
TAB_ACCESS_SPECS = (409, 98)
GLOBAL_SPEC_64K = (560, 152)         # "64 KiB; 100% Read; 0% random" in Global list
BTN_EDIT = (715, 165)

DLG_TITLE = "Edit Access Specification"
DLG_NAME = (190, 63)
DLG_ALIGN_REQSIZE = (527, 383)       # "Align I/Os on: Request Size Boundaries" radio
DLG_REPLY_SIZE_RADIO = (40, 497)     # Reply Size: the "Size" radio (not "No Reply")
DLG_REPLY_KILOBYTES = (127, 497)     # Reply Size: Kilobytes edit box
DLG_OK = (596, 512)

NAME = "REPLY ALIGN TEST"
EXP_ALIGN = "65536"                  # Request-Size alignment of a 64 KiB spec
EXP_REPLY = "8192"                   # 8 Kilobytes


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

    g.set_field(dlg, *DLG_NAME, NAME)
    g.click_window(dlg, *DLG_ALIGN_REQSIZE); time.sleep(0.3)     # align -> request size
    g.click_window(dlg, *DLG_REPLY_SIZE_RADIO); time.sleep(0.3)  # enable reply size
    g.set_field(dlg, *DLG_REPLY_KILOBYTES, "8"); time.sleep(0.3) # reply = 8 KiB
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
    if fields and len(fields) >= 8 and fields[6] == EXP_ALIGN and fields[7] == EXP_REPLY:
        print(f"PASS: align={EXP_ALIGN} reply={EXP_REPLY} in the ICF")
        return 0
    print(f"FAIL: expected align={EXP_ALIGN}, reply={EXP_REPLY}; got {fields}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
