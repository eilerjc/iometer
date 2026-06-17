"""GUI interaction test (#4 - Presentation Meter): open the BigMeter from a
Results Display ">" button, run a short test so the needle updates, then close it.

Opening the meter covers CBigMeter::Create + the CMeterCtrl gauge paint; running a
real (synthetic, via dynamotest) test drives CBigMeter::UpdateDisplay /
CMeterCtrl::SetValue. Asserts the meter opened and the test actually ran (results
CSV written). Exercises BigMeter.cpp + MeterCtrl.cpp (both near-zero before) and
the GalileoView start/stop + results path.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_bigmeter.py
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
RESULTS = pathlib.Path("C:/tmp/bm_results.csv")

TAB_RESULTS = (508, 98)
BTN_BIGMETER1 = (748, 217)           # ">" on Results Display row 1 (BBigMeter1)
TB_START = (275, 54)


def _vis():
    return {w._hWnd: w for w in gw.getAllWindows() if w.visible}


def _handle_save_results(path):
    dlg = None
    for _ in range(12):
        dlg = next((w for w in gw.getAllWindows()
                    if w.visible and w.title.strip() == "Save Results"), None)
        if dlg:
            break
        time.sleep(0.5)
    if not dlg:
        return False
    try: dlg.activate()
    except Exception: pass
    time.sleep(0.4)
    pyautogui.write(path, interval=0.02)     # file name field opens pre-selected
    time.sleep(0.2)
    pyautogui.press("enter")
    time.sleep(0.5)
    return True


def run():
    RESULTS.unlink(missing_ok=True)
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

    g.click_window(win, *TAB_RESULTS); time.sleep(0.6)
    base = set(_vis())
    g.click_window(win, *BTN_BIGMETER1); time.sleep(1.2)     # open the meter
    meter = next((w for h, w in _vis().items()
                  if h not in base and w.width > 400 and w.height > 400), None)
    if not meter:
        g.kill_iometer(); return "BigMeter did not open"

    g.click_window(win, *TB_START); time.sleep(1.2)          # start the test
    _handle_save_results(str(RESULTS))                       # supply the results CSV
    time.sleep(6.0)                                          # 3s run + finish; needle updates

    try: meter.close()                                       # close the meter
    except Exception: pass
    time.sleep(0.6)
    meter_gone = meter._hWnd not in _vis()

    g.close_iometer(win)
    if not meter_gone:
        return "BigMeter did not close"
    if not (RESULTS.exists() and RESULTS.stat().st_size > 0):
        return "test did not run (no results CSV written)"
    return None


def main():
    err = run()
    if err:
        print(f"FAIL: {err}"); return 1
    print(f"PASS: opened the Presentation Meter, ran a test (results {RESULTS.stat().st_size}B), closed it")
    return 0


if __name__ == "__main__":
    sys.exit(main())
