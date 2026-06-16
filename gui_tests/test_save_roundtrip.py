"""GUI interaction test: load a config, Save it through the GUI's
"Save Test Configuration File" dialog, and assert the written ICF byte-matches
the save-golden (the same artifact the /s batch flag produces).

This proves the GUI File>Save path end to end and reuses the deterministic golden
as the oracle - the template for the data-model safety-net tests.

Run from an interactive desktop:  python gui_tests/test_save_roundtrip.py
"""
import sys
import time
import subprocess
import pathlib

import iometer_gui as g

FIXTURE = "worker_rich"
ICF = g.REPO / "test" / "fixtures" / "icf_compat" / (FIXTURE + ".icf")
GOLDEN = g.REPO / "test" / "fixtures" / "icf_compat" / "golden_save" / (FIXTURE + ".save.txt")
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
OUT = pathlib.Path("C:/tmp") / ("gui_save_" + FIXTURE + ".icf")


def normalize(lines):
    """Match the save-golden harness: collapse the build Version lines to VERSION."""
    return ["VERSION" if ln.startswith("Version ") else ln.rstrip("\r\n") for ln in lines]


def run_gui_save():
    g.kill_iometer(); time.sleep(1.0)
    OUT.unlink(missing_ok=True)

    g.launch(ICF)
    win = g.find_window(timeout=20.0)
    if not win:
        return None, "no main window"
    subprocess.Popen([str(DYNAMOTEST), "-n", "TESTMGR", "-m", "localhost",
                      "--rdelay", "50", "-i", "127.0.0.1"])
    time.sleep(7.0)                       # manager connects; Waiting dialog closes
    g.position(win, 50, 30, 1500, 950); time.sleep(1.5)
    win = g.find_window() or win

    if not g.gui_save(win, OUT):          # drive File>Save (all sections)
        g.kill_iometer()
        return None, "GUI Save did not write the file"
    lines = OUT.read_text().splitlines(keepends=True)
    g.kill_iometer()
    return lines, None


def main():
    if not GOLDEN.exists():
        print(f"ERROR: golden {GOLDEN} missing"); return 2
    saved, err = run_gui_save()
    if err:
        print(f"FAIL: {err}"); return 1

    got = normalize(saved)
    # golden = ["RUN EXIT=0", <normalized ICF lines...>]; drop the RUN tag.
    golden = [ln.rstrip("\r\n") for ln in GOLDEN.read_text().splitlines()][1:]

    if got == golden:
        print(f"PASS: GUI Save of {FIXTURE} byte-matches the save-golden ({len(got)} lines)")
        return 0

    print(f"FAIL: GUI Save differs from golden (got {len(got)} lines, golden {len(golden)})")
    import difflib
    diff = list(difflib.unified_diff(golden, got, "golden", "gui-save", lineterm=""))
    for ln in diff[:30]:
        print("  " + ln)
    return 1


if __name__ == "__main__":
    sys.exit(main())
