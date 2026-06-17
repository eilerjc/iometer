"""GUI interaction test (#A - topology add/remove workers): add two disk workers
to a manager via the toolbar, then remove one, verifying the worker count in the
saved ICF after each step (1 -> 3 -> 2).

Exercises WorkerView.cpp (RemoveSelectedItem / RemoveWorker + the tree) and the
Manager/Worker add+remove model paths.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_topology_workers.py
"""
import sys
import time
import subprocess
import pathlib

import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
OUT_ADD = pathlib.Path("C:/tmp/gui_topo_add.icf")
OUT_DEL = pathlib.Path("C:/tmp/gui_topo_del.icf")

TREE_TESTMGR = (110, 141)            # the TESTMGR node (manager)
TREE_EXPAND_TESTMGR = (52, 141)      # its [+] expand box
TREE_FIRST_WORKER = (130, 157)       # first worker once TESTMGR is expanded
TB_NEW_DISK_WORKER = (153, 54)       # BNewDiskWorker
TB_EXIT_ONE = (443, 54)              # BExitOne -> RemoveSelectedItem


def _workers(path):
    return sum(1 for ln in path.read_text().splitlines() if ln.strip() == "'Worker")


def run():
    OUT_ADD.unlink(missing_ok=True); OUT_DEL.unlink(missing_ok=True)
    g.kill_iometer(); time.sleep(1.0)
    g.launch(ICF)
    win = g.find_window(timeout=20.0)
    if not win:
        return None, None, "no main window"
    subprocess.Popen([str(DYNAMOTEST), "-n", "TESTMGR", "-m", "localhost",
                      "--rdelay", "50", "-i", "127.0.0.1"])
    time.sleep(7.0)
    g.position(win, 50, 30, 1500, 950); time.sleep(1.5)
    win = g.find_window() or win

    # Add two disk workers to TESTMGR (re-select the manager before each add).
    g.click_window(win, *TREE_TESTMGR); time.sleep(0.4)
    g.click_window(win, *TB_NEW_DISK_WORKER); time.sleep(0.6)
    g.click_window(win, *TREE_TESTMGR); time.sleep(0.4)
    g.click_window(win, *TB_NEW_DISK_WORKER); time.sleep(0.6)
    if not g.gui_save(win, OUT_ADD):
        g.kill_iometer(); return None, None, f"{OUT_ADD} not written"
    n_add = _workers(OUT_ADD)

    # Remove one worker: expand TESTMGR, select the first worker, BExitOne.
    g.click_window(win, *TREE_EXPAND_TESTMGR); time.sleep(0.4)
    g.click_window(win, *TREE_FIRST_WORKER); time.sleep(0.4)
    g.click_window(win, *TB_EXIT_ONE); time.sleep(0.6)
    if not g.gui_save(win, OUT_DEL):
        g.kill_iometer(); return None, None, f"{OUT_DEL} not written"
    n_del = _workers(OUT_DEL)

    g.close_iometer(win)
    return n_add, n_del, None


def main():
    n_add, n_del, err = run()
    if err:
        print(f"FAIL: {err}"); return 1
    print(f"worker count after +2 adds = {n_add}; after -1 remove = {n_del}")
    if n_add == 3 and n_del == 2:
        print("PASS: added two workers (1->3) and removed one (3->2) via the toolbar")
        return 0
    print("FAIL: expected 3 after adds and 2 after remove")
    return 1


if __name__ == "__main__":
    sys.exit(main())
