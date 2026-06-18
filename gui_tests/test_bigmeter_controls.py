"""GUI interaction test (#G - Presentation Meter controls): open the meter, change
its displayed statistic, enable Show Trace, run a test from the meter's own Start
button, then Stop and close.

Drives the BigMeter's own controls (the stat combo, the trace checkbox, the
Start/Stop test buttons) and the trace/needle drawing in MeterCtrl - paths the
basic test_bigmeter.py (which only opens + runs) doesn't reach.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_bigmeter_controls.py
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
RESULTS = pathlib.Path("C:/tmp/bm_controls.csv")

TAB_RESULTS = (508, 98)
BTN_BIGMETER1 = (748, 217)
# Meter-window-relative controls (814x596):
M_STAT_COMBO = (125, 538)
M_SHOW_TRACE = (155, 564)
M_START = (655, 552)
M_STOP = (744, 552)


def _vis():
    return {w._hWnd: w for w in gw.getAllWindows() if w.visible}


def _handle_save_results(path):
    for _ in range(12):
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
    g.click_window(win, *BTN_BIGMETER1); time.sleep(1.2)
    meter = next((w for h, w in _vis().items()
                  if h not in base and w.width > 400 and w.height > 400), None)
    if not meter:
        g.kill_iometer(); return "BigMeter did not open"
    try: meter.activate()
    except Exception: pass
    time.sleep(0.4)

    g.select_combo(meter, *M_STAT_COMBO, key="down", presses=1)   # change displayed stat
    g.click_window(meter, *M_SHOW_TRACE)                          # enable trace
    g.click_window(meter, *M_START); time.sleep(1.0)             # start from the meter
    _handle_save_results(str(RESULTS))
    time.sleep(6.0)                                              # run completes; needle+trace
    g.click_window(meter, *M_STOP); time.sleep(0.5)             # Stop button

    try: meter.close()
    except Exception: pass
    time.sleep(0.6)
    meter_gone = meter._hWnd not in _vis()
    main = g.find_window(timeout=5.0)
    g.close_iometer(win)

    if not meter_gone:
        return "meter did not close"
    if not main:
        return "main window vanished"
    if not (RESULTS.exists() and RESULTS.stat().st_size > 0):
        return "test did not run from the meter (no results CSV)"
    return None


def main():
    err = run()
    if err:
        print(f"FAIL: {err}"); return 1
    print(f"PASS: drove the meter controls + ran a test from it (results {RESULTS.stat().st_size}B)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
