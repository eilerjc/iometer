"""GUI interaction test (#B - run/stop/reset variants): start a test and Stop it
mid-run, Reset, then start again and let it complete before Stop All - exercising
the GalileoView Start/Stop/StopAll/Reset handlers and the ManagerList/Manager/
Worker run + results-aggregation paths beyond the single complete run in
test_bigmeter.py.

Asserts a completed run wrote a results CSV and the GUI survived the transitions.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_run_variants.py
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
# Iometer prompts for a results file only on the first Start, then reuses it, so
# both runs write here.
RES = pathlib.Path("C:/tmp/rv_results.csv")

TB_START = (275, 54)
TB_STOP = (315, 54)
TB_STOPALL = (352, 54)
TB_RESET = (397, 54)


def _handle_save_results(path, tries=12):
    for _ in range(tries):
        dlg = next((w for w in gw.getAllWindows()
                    if w.visible and w.title.strip() == "Save Results"), None)
        if dlg:
            try: dlg.activate()
            except Exception: pass
            time.sleep(0.4)
            pyautogui.write(path, interval=0.02); time.sleep(0.2)
            pyautogui.press("enter"); time.sleep(0.5)
            return True
        time.sleep(0.5)
    return False


def run():
    RES.unlink(missing_ok=True)
    g.kill_iometer(); time.sleep(1.0)
    g.launch(ICF)
    win = g.find_window(timeout=20.0)
    if not win:
        return "no main window"
    subprocess.Popen([str(DYNAMOTEST), "-n", "TESTMGR", "-m", "localhost",
                      "--rdelay", "50", "-i", "127.0.0.1"])
    time.sleep(7.0)
    g.position(win, 50, 30, 1500, 950); time.sleep(1.5)
    win = g.find_window() or win

    # Run 1: start (prompts for the results file), stop mid-run, reset.
    g.click_window(win, *TB_START); time.sleep(1.0)
    _handle_save_results(str(RES))
    time.sleep(1.5)
    g.click_window(win, *TB_STOP); time.sleep(1.5)       # OnBStop mid-run
    g.click_window(win, *TB_RESET); time.sleep(1.0)      # OnBReset

    # Run 2: start again (reuses the results file - no new prompt), let the 3s
    # test complete, then Stop All.
    g.click_window(win, *TB_START); time.sleep(1.0)
    _handle_save_results(str(RES), tries=4)              # defensive; usually no prompt
    time.sleep(5.0)                                      # completes -> full results
    g.click_window(win, *TB_STOPALL); time.sleep(1.0)    # OnBStopAll

    main = g.find_window(timeout=5.0)
    g.close_iometer(win)
    if not main:
        return "main window vanished after run variants"
    if not (RES.exists() and RES.stat().st_size > 0):
        return "no results CSV written across the runs"
    return None


def main():
    err = run()
    if err:
        print(f"FAIL: {err}"); return 1
    print(f"PASS: Start/Stop/Reset/StopAll all survived; runs wrote {RES.stat().st_size}B")
    return 0


if __name__ == "__main__":
    sys.exit(main())
