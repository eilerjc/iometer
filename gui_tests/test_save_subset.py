"""GUI interaction test (#10 - Save subset): in the Save dialog uncheck two of the
"Settings to save" boxes (Results Display, Global Access Specifications) and verify
those sections are omitted from the written ICF while Test Setup is still present.

Exercises ICFSaveDialog.cpp (the checkbox handlers) and the conditional
section-write paths that the all-sections save never takes.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_save_subset.py
"""
import sys
import time
import subprocess
import pathlib

import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
OUT = pathlib.Path("C:/tmp/gui_save_subset.icf")


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

    # Uncheck "Results Display tab settings" and "Global Access Specifications".
    ok = g.gui_save(win, OUT, uncheck=[g.SAVE_DLG_CHK_RESULTS, g.SAVE_DLG_CHK_GLOBALSPECS])
    if not ok:
        g.kill_iometer()
        return None, f"{OUT} not written"
    text = OUT.read_text()
    g.close_iometer(win)
    return text, None


def main():
    text, err = run()
    if err:
        print(f"FAIL: {err}"); return 1

    has_setup = "'TEST SETUP" in text                       # left checked -> present
    has_results = "'Bar chart 1 statistic" in text          # unchecked -> should be gone
    has_specdef = "'size,% of size,% reads" in text         # unchecked -> should be gone
    print(f"TEST SETUP present={has_setup}  Results-section present={has_results}  "
          f"global-spec-defs present={has_specdef}")
    if has_setup and not has_results and not has_specdef:
        print("PASS: unchecked sections omitted; Test Setup retained")
        return 0
    print("FAIL: expected Test Setup present and the two unchecked sections absent")
    return 1


if __name__ == "__main__":
    sys.exit(main())
