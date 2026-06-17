"""Run all GUI-interaction tests and report a summary.

MUST run on an interactive desktop in the FOREGROUND (these drive the real
mouse/keyboard; a backgrounded run loses focus and fails). Each test launches and
tears down its own IOmeter, so they run sequentially.

    python gui_tests/run_gui_tests.py
"""
import sys
import time
import subprocess
import pathlib

HERE = pathlib.Path(__file__).resolve().parent
TESTS = [
    "test_save_roundtrip.py",
    "test_edit_testsetup.py",
    "test_edit_worker.py",
    "test_assign_spec.py",
    "test_remove_spec.py",
    "test_reorder_spec.py",
    "test_edit_disk_targets.py",
    "test_edit_results_display.py",
    "test_edit_runtime_cycling.py",
    "test_save_subset.py",
    "test_about_eula.py",
    "test_new_access_spec.py",
]


def main():
    results = []
    for t in TESTS:
        print(f"\n=== {t} ===")
        r = subprocess.run([sys.executable, str(HERE / t)], capture_output=True, text=True)
        out = (r.stdout or "") + (r.stderr or "")
        verdict = next((ln.strip() for ln in out.splitlines()
                        if ln.startswith(("PASS", "FAIL", "ERROR"))),
                       f"(no verdict; exit {r.returncode})")
        ok = (r.returncode == 0)
        results.append((t, ok, verdict))
        print(("[PASS] " if ok else "[FAIL] ") + verdict)
        time.sleep(2.0)   # let processes settle between tests

    print("\n==================== SUMMARY ====================")
    for t, ok, _ in results:
        print(f"  [{'PASS' if ok else 'FAIL'}] {t}")
    npass = sum(1 for _, ok, _ in results if ok)
    print(f"\nResult: {npass}/{len(results)} passed")
    return 0 if npass == len(results) else 1


if __name__ == "__main__":
    sys.exit(main())
