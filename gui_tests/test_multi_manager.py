"""GUI interaction test (#1 - multi-manager run): load the two-manager config,
connect two dynamotests (M1, M2), run a test, and verify both managers appear in
the results CSV.

The largest remaining lever: a 2-manager run exercises ManagerList multi-manager
aggregation, Manager (x2), the GalileoView run/results path, and Worker - code a
single-manager run can't reach.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_multi_manager.py
"""
import sys
import time
import subprocess
import pathlib

import pyautogui
import pygetwindow as gw
import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "two_managers_run.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
RESULTS = pathlib.Path("C:/tmp/mm_results.csv")
TB_START = (275, 54)


def _dynamo(name):
    return subprocess.Popen([str(DYNAMOTEST), "-n", name, "-m", "localhost",
                             "--rdelay", "50", "-i", "127.0.0.1"])


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
    _dynamo("M1"); time.sleep(1.0); _dynamo("M2")        # both managers connect
    time.sleep(9.0)
    g.position(win, 50, 30, 1500, 950); time.sleep(1.5)
    win = g.find_window() or win

    g.click_window(win, *TB_START); time.sleep(1.0)
    _handle_save_results(str(RESULTS))
    time.sleep(6.0)                                       # 3s run + finish

    main = g.find_window(timeout=5.0)
    g.close_iometer(win)
    if not main:
        return "main window vanished during multi-manager run"
    if not (RESULTS.exists() and RESULTS.stat().st_size > 0):
        return "no results CSV written"
    text = RESULTS.read_text(errors="ignore")
    if "M1" not in text or "M2" not in text:
        return f"results CSV missing a manager (M1={'M1' in text}, M2={'M2' in text})"
    return None


def main():
    err = run()
    if err:
        print(f"FAIL: {err}"); return 1
    print(f"PASS: 2-manager run completed; both M1 and M2 in results ({RESULTS.stat().st_size}B)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
