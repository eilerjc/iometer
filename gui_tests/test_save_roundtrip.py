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

import pygetwindow as gw
import iometer_gui as g

FIXTURE = "worker_rich"
ICF = g.REPO / "test" / "fixtures" / "icf_compat" / (FIXTURE + ".icf")
GOLDEN = g.REPO / "test" / "fixtures" / "icf_compat" / "golden_save" / (FIXTURE + ".save.txt")
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"
OUT = pathlib.Path("C:/tmp") / ("gui_save_" + FIXTURE + ".icf")

# Save-dialog control offsets (relative to the dialog's top-left, 571x622 layout).
DLG_FILENAME = (320, 370)
DLG_SAVE_BTN = (519, 370)


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

    dlg = None                            # open Save dialog (retry: focus can flake)
    for _ in range(3):
        try: win.activate()
        except Exception: pass
        time.sleep(0.3)
        g.click_window(win, *g.TB_SAVE)
        for _ in range(8):
            dlg = next((w for w in gw.getAllWindows()
                        if w.title.strip() == "Save Test Configuration File" and w.visible), None)
            if dlg: break
            time.sleep(0.5)
        if dlg: break
    if not dlg:
        return None, "Save dialog did not appear"

    try: dlg.activate()
    except Exception: pass
    time.sleep(0.5)

    import pyautogui
    # The File name field opens with "Iometer" pre-selected, so typing replaces it
    # (clicking/Ctrl+A/triple-click don't reliably select-all in this combo edit).
    time.sleep(0.5)
    pyautogui.write(str(OUT), interval=0.02)
    g.click_window(dlg, *DLG_SAVE_BTN)          # Save (all six checkboxes default-checked)
    time.sleep(0.5)
    pyautogui.press("enter")                    # confirm any "replace?" prompt

    for _ in range(20):                   # wait for the file
        if OUT.exists() and OUT.stat().st_size > 0:
            break
        time.sleep(0.5)
    g.kill_iometer()
    if not OUT.exists():
        return None, f"{OUT} was not written"
    return OUT.read_text().splitlines(keepends=True), None


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
