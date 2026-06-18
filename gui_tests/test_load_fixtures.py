"""GUI interaction test (#2 - parser-stress loads): open several varied-format
ICF fixtures (old-format specs, mixed-case keys, missing sections, ancient default
workers) through the Open dialog, verifying the GUI parses each without crashing
and a final save still produces a valid config.

Exercises ICF_ifstream.cpp / IcfDocument load branches across format variants the
single worker_rich fixture doesn't hit.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_load_fixtures.py
"""
import sys
import time
import subprocess
import pathlib

import iometer_gui as g

FIX = g.REPO / "test" / "fixtures" / "icf_compat"
START = FIX / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
OUT = pathlib.Path("C:/tmp/gui_load_fixtures.icf")

FIXTURES = ["oldformat_specs.icf", "mixedcase_keys.icf",
            "missing_sections.icf", "ancient_default_workers.icf"]


def run():
    g.kill_iometer(); time.sleep(1.0)
    g.launch(START)
    win = g.find_window(timeout=20.0)
    if not win:
        return None, "no main window"
    subprocess.Popen([str(DYNAMOTEST), "-n", "TESTMGR", "-m", "localhost",
                      "--rdelay", "50", "-i", "127.0.0.1"])
    time.sleep(7.0)
    g.position(win, 50, 30, 1500, 950); time.sleep(1.5)
    win = g.find_window() or win

    loaded = []
    for fx in FIXTURES:
        path = FIX / fx
        # Open without the manager list (parses test-setup/results/specs; no wait).
        if not g.gui_open(win, path, uncheck=[g.SAVE_DLG_CHK_MANAGERS]):
            return None, f"Open dialog did not appear for {fx}"
        time.sleep(1.0)
        win = g.find_window(timeout=8.0)        # survived the parse?
        if not win:
            return None, f"GUI crashed parsing {fx}"
        loaded.append(fx)

    if not g.gui_save(win, OUT):
        g.kill_iometer(); return None, f"{OUT} not written"
    text = OUT.read_text()
    g.close_iometer(win)
    return (loaded, text), None


def main():
    res, err = run()
    if err:
        print(f"FAIL: {err}"); return 1
    loaded, text = res
    print(f"parsed without crash: {loaded}")
    if len(loaded) == len(FIXTURES) and "'TEST SETUP" in text:
        print(f"PASS: all {len(FIXTURES)} format variants loaded via the Open dialog")
        return 0
    print("FAIL: not all fixtures loaded / final save invalid")
    return 1


if __name__ == "__main__":
    sys.exit(main())
