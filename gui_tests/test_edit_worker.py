"""GUI interaction test (per-worker data model): change "# of Outstanding I/Os"
on the Disk Targets tab and verify it reaches the worker in the saved ICF.

Unlike the Test-Setup edit (a global page field), this exercises the live
Manager/Worker target settings - the data model the unification would migrate -
so it's the most relevant safety-net coverage for that work.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_edit_worker.py
"""
import sys
import time
import subprocess
import pathlib

import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
OUT = pathlib.Path("C:/tmp/gui_edit_worker.icf")

TREE_ALL_MANAGERS = (95, 125)    # topology root (applies a setting to all workers)
TAB_DISK_TARGETS = (224, 98)
FIELD_OUTSTANDING_IOS = (452, 262)   # "# of Outstanding I/Os" edit (shows 8 for worker_rich)
NEW_QD = "16"                        # change 8 -> 16


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

    g.click_window(win, *TREE_ALL_MANAGERS)   # select root -> setting applies to all workers
    time.sleep(0.4)
    g.click_window(win, *TAB_DISK_TARGETS)
    time.sleep(0.6)
    g.set_field(win, *FIELD_OUTSTANDING_IOS, NEW_QD)
    # Click elsewhere so the edit commits before saving.
    g.click_window(win, *TREE_ALL_MANAGERS)
    time.sleep(0.4)

    if not g.gui_save(win, OUT):
        g.kill_iometer()
        return None, f"{OUT} not written"
    lines = OUT.read_text().splitlines()
    g.kill_iometer()
    return lines, None


def main():
    lines, err = run()
    if err:
        print(f"FAIL: {err}"); return 1

    # The worker block writes:
    #   'Number of outstanding IOs,test connection rate,transactions per connection,...
    #   \t<qd>,ENABLED,<trans>,...
    qd = None
    for i, ln in enumerate(lines):
        if ln.strip().startswith("'Number of outstanding IOs") and i + 1 < len(lines):
            qd = lines[i + 1].strip().split(",")[0]
            break
    print(f"saved worker outstanding-IOs: {qd!r}")
    if qd == NEW_QD:
        print(f"PASS: GUI edit of # Outstanding I/Os ({NEW_QD}) reached the worker in the ICF")
        return 0
    print(f"FAIL: expected {NEW_QD!r}, got {qd!r}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
