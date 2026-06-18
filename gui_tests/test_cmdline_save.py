"""GUI interaction test (#H - command-line switches): launch IOmeter in batch
mode with several switches (/c config /r results /s save-config /t timeout),
connect a dynamotest, and verify the post-run re-save was written.

Exercises the GalileoCmdLine.cpp switch parser (C/R/S/T branches) and the batch
run + re-save path - none of which the default "/c <icf>" launch reaches.
Non-interactive (no mouse), so it's a robust addition to the suite.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_cmdline_save.py
"""
import sys
import time
import subprocess
import pathlib

import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
CSV = pathlib.Path("C:/tmp/gui_cmdline_results.csv")
OUT = pathlib.Path("C:/tmp/gui_cmdline_save.icf")


def run():
    CSV.unlink(missing_ok=True); OUT.unlink(missing_ok=True)
    g.kill_iometer(); time.sleep(1.0)
    # /c <icf> /r <results> /s <save-config> /t <timeout>: batch run, then re-save.
    g.launch(ICF, extra_args=["/r", str(CSV), "/s", str(OUT), "/t", "40"])
    time.sleep(2.0)
    subprocess.Popen([str(DYNAMOTEST), "-n", "TESTMGR", "-m", "localhost",
                      "--rdelay", "50", "-i", "127.0.0.1"])
    for _ in range(60):                      # wait for the run to finish + re-save
        if OUT.exists() and OUT.stat().st_size > 0:
            break
        time.sleep(0.5)
    g.kill_iometer()                         # flush coverage + clean up

    if not (OUT.exists() and OUT.stat().st_size > 0):
        return None, "batch /s did not write the config"
    return OUT.read_text(), None


def main():
    text, err = run()
    if err:
        print(f"FAIL: {err}"); return 1
    ok = "'TEST SETUP" in text and "'MANAGER LIST" in text
    print(f"batch-saved config: {OUT.stat().st_size}B, TEST SETUP={'TEST SETUP' in text}, "
          f"results CSV={CSV.exists()}")
    if ok:
        print("PASS: /c /r /s /t batch run + re-save wrote a valid config (GalileoCmdLine)")
        return 0
    print("FAIL: saved config missing expected sections")
    return 1


if __name__ == "__main__":
    sys.exit(main())
