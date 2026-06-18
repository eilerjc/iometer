"""GUI interaction test (#C - Network Targets): add a network worker to a manager,
view the Network Targets tab (which renders the synthetic network target), set the
Max # Outstanding Sends field, then verify a NETWORK worker is in the saved ICF.

Exercises PageNetwork.cpp (the tab render + a target field) and the Manager/Worker
network-worker creation path - a tab with almost no other coverage.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_network_worker.py
"""
import sys
import re
import time
import subprocess
import pathlib

import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
OUT = pathlib.Path("C:/tmp/gui_network_worker.icf")

TREE_TESTMGR = (110, 141)
TB_NEW_NET_WORKER = (192, 54)        # BNewNetWorker
TAB_NETWORK = (305, 98)
FIELD_MAX_SENDS = (450, 210)         # "Max # Outstanding Sends per VI target"


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

    g.click_window(win, *TREE_TESTMGR); time.sleep(0.4)
    g.click_window(win, *TB_NEW_NET_WORKER); time.sleep(1.0)   # add a NETWORK worker
    g.click_window(win, *TAB_NETWORK); time.sleep(0.7)         # render the Network Targets tab
    g.set_field(win, *FIELD_MAX_SENDS, "8")                    # exercise a network field
    g.click_window(win, *TREE_TESTMGR); time.sleep(0.4)        # commit

    if not g.gui_save(win, OUT):
        g.kill_iometer(); return None, f"{OUT} not written"
    text = OUT.read_text()
    g.close_iometer(win)
    return text, None


def main():
    text, err = run()
    if err:
        print(f"FAIL: {err}"); return 1

    types = re.findall(r"'Worker type\s*\n\s*(\w+)", text)
    print(f"worker types in ICF: {types}")
    if "NETWORK" in types:
        print("PASS: network worker added and saved (NETWORK worker present)")
        return 0
    print("FAIL: no NETWORK worker in the saved ICF")
    return 1


if __name__ == "__main__":
    sys.exit(main())
