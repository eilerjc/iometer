"""Launch the MFC GUI with a real disk config + connected dynamotest, then
capture the fully-populated main window for inspection."""
import sys
import time
import subprocess

import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"


def main():
    g.kill_iometer()
    time.sleep(1.0)
    print(f"launch: IOmeter.exe /c {ICF.name}")
    g.launch(ICF)
    win = g.find_window(timeout=20.0)
    if not win:
        print("FAIL: no window"); g.kill_iometer(); return 1

    print("connect: dynamotest -n TESTMGR -m localhost")
    subprocess.Popen([str(DYNAMOTEST), "-n", "TESTMGR", "-m", "localhost",
                      "--rdelay", "50", "-i", "127.0.0.1"])
    time.sleep(7.0)   # let the manager connect; the Waiting dialog auto-closes

    g.position(win, 50, 30, 1500, 950)
    time.sleep(1.5)
    win = g.find_window() or win   # refresh geometry after move/resize
    shot = g.screenshot_window(win, "mfc_main.png")
    print(f"captured: {shot}  ({win.width}x{win.height})")

    g.kill_iometer()
    return 0


if __name__ == "__main__":
    sys.exit(main())
