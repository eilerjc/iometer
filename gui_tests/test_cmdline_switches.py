"""GUI interaction test (#7 - more command-line switches): two non-default
launches - (1) a bad switch triggers the GalileoCmdLine error MessageBox, and
(2) "/p <port> /m 1" parses the port + auto-shows the Presentation Meter.

Covers the GalileoCmdLine.cpp default/Fail branch + MessageBox and the P and M
switch cases (and the /m auto-show-bigmeter path) - none reached by the batch
/c /r /s /t test.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_cmdline_switches.py
"""
import sys
import time
import subprocess

import pyautogui
import pygetwindow as gw
import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"


def _wait(pred, timeout=12.0):
    deadline = time.time() + timeout
    while time.time() < deadline:
        for w in gw.getAllWindows():
            try:
                if w.visible and pred(w):
                    return w
            except Exception:
                pass
        time.sleep(0.5)
    return None


def run():
    # --- Scenario 1: a bad switch -> Fail() MessageBox (title "Iometer", small) ---
    g.kill_iometer(); time.sleep(1.0)
    g.launch(ICF, extra_args=["/z"])
    errbox = _wait(lambda w: w.title.strip() == "Iometer" and w.width < 400, 12.0)
    if errbox:
        try: errbox.activate()
        except Exception: pass
        time.sleep(0.3)
        pyautogui.press("enter")        # dismiss the OK box
        time.sleep(0.5)
    g.kill_iometer(); time.sleep(1.0)

    # --- Scenario 2: batch run carrying /p (port) and /m (show-bigmeter) so the
    # P and M switch branches are parsed; assert the batch re-save still works. ---
    import pathlib
    out = pathlib.Path("C:/tmp/cw_save.icf"); out.unlink(missing_ok=True)
    csv = pathlib.Path("C:/tmp/cw_results.csv"); csv.unlink(missing_ok=True)
    g.launch(ICF, extra_args=["/r", str(csv), "/s", str(out), "/t", "40",
                              "/p", "1066", "/m", "1"])
    time.sleep(2.0)
    subprocess.Popen([str(DYNAMOTEST), "-n", "TESTMGR", "-m", "localhost",
                      "--rdelay", "50", "-i", "127.0.0.1"])
    for _ in range(60):
        if out.exists() and out.stat().st_size > 0:
            break
        time.sleep(0.5)
    g.kill_iometer()

    if not errbox:
        return "bad-switch error MessageBox did not appear"
    if not (out.exists() and out.stat().st_size > 0):
        return "batch run with /p /m did not re-save the config"
    return None


def main():
    err = run()
    if err:
        print(f"FAIL: {err}"); return 1
    print("PASS: bad-switch error box handled + batch run with /p /m re-saved the config")
    return 0


if __name__ == "__main__":
    sys.exit(main())
