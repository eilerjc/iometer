"""GUI golden test (File>New built-in access-spec list): launch the MFC GUI with
NO config (a fresh "File>New" config), save it, and assert the ACCESS
SPECIFICATIONS section byte-for-byte matches golden_builtin_specs.txt.

This is the safety net for the Tier 1 consolidation that moves the built-in
default access-spec library out of MFC's hard-coded AccessSpecList.cpp and into
the shared core (builtinAccessSpecs()). The MFC list is the canonical oracle; if
the core-generated list ever differs from what MFC ships today (a renamed spec, a
changed size/%read/%random/align, an added/dropped entry, the multi-line "All in
one"), the saved section diverges from this golden and the test fails.

A no-config launch starts a local manager (and its Dynamo), which we ignore and
kill - only the machine-independent ACCESS SPECIFICATIONS section is compared.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_new_config_builtin_specs.py
"""
import sys
import time
import subprocess
import pathlib

import iometer_gui as g

OUT = pathlib.Path("C:/tmp/gui_new_config_builtin_specs.icf")
GOLDEN = g.HERE / "golden_builtin_specs.txt"

START = "'ACCESS SPECIFICATIONS"
END = "'END access specifications"

PAT = r"^Iometer"


def _reposition(win):
    """Move the no-config window onto the primary monitor at stable coordinates.

    With no config loaded the window can spawn on a secondary monitor, and
    pygetwindow's moveTo/activate intermittently raise (Windows "error 0"), so
    retry the individual steps until the window actually lands somewhere sane -
    otherwise the Save-toolbar click can hit a screen corner (PyAutoGUI failsafe)."""
    for _ in range(5):
        try:
            if win.isMaximized:
                win.restore(); time.sleep(0.4)
        except Exception:
            pass
        try: win.moveTo(50, 30)
        except Exception: pass
        time.sleep(0.3)
        try: win.resizeTo(1500, 950)
        except Exception: pass
        time.sleep(0.4)
        win = g.find_window(pattern=PAT) or win
        if 0 <= win.left < 600 and 0 <= win.top < 400:
            return win
        time.sleep(0.5)
    return win


def _section(lines):
    """Slice the ACCESS SPECIFICATIONS section (inclusive of its end marker)."""
    out, inside = [], False
    for ln in lines:
        if ln.startswith(START):
            inside = True
        if inside:
            out.append(ln)
        if ln.startswith(END):
            break
    return out


def run():
    g.kill_iometer(); time.sleep(1.0)

    # GUARD: a no-config launch auto-loads DEFAULT_CONFIG_FILE ("iometer.icf")
    # from the process working directory if one exists - that would REPLACE the
    # compiled-in built-in specs (replace=TRUE on startup DeleteAll()s the list),
    # so the golden would silently validate that stray file instead of the
    # compiled-in list this test exists to lock. Pin the child's cwd to a known
    # dir and refuse to run if an iometer.icf is sitting in it.
    launch_dir = g.HERE
    stray = launch_dir / "iometer.icf"
    if stray.exists():
        return None, (f"refusing to run: a stray '{stray}' would be auto-loaded by the "
                      f"no-config launch and mask the compiled-in built-in specs - remove it")

    # No /c -> fresh config -> the built-in (File>New) global access-spec list.
    # /g = GUI-only: suppress the no-config auto-spawn of a local Dynamo (which
    # would raise a UAC elevation prompt). With no config the local manager never
    # connects, so the title stays "Iometer" (no version) - match loosely.
    subprocess.Popen([str(g.IOMETER_EXE), "/g"], cwd=str(launch_dir))
    win = g.find_window(pattern=PAT, timeout=25.0)
    if not win:
        g.kill_iometer(); return None, "no main window"
    time.sleep(3.0)
    win = _reposition(win)

    ok = g.gui_save(win, OUT)
    g.close_iometer(win)
    if not ok or not OUT.exists():
        return None, f"{OUT} not written"
    return OUT.read_text().splitlines(), None


def main():
    lines, err = run()
    if err:
        print(f"FAIL: {err}"); return 1
    if not GOLDEN.exists():
        print(f"FAIL: missing golden {GOLDEN}"); return 1

    actual = _section(lines)
    golden = [ln for ln in GOLDEN.read_text().splitlines()]
    print(f"built-in spec section: {len(actual)} lines (golden {len(golden)})")

    if actual == golden:
        nspecs = sum(1 for ln in actual if ln.startswith("\t") and ",NONE" in ln)
        print(f"PASS: File>New built-in access-spec list matches golden ({nspecs} specs)")
        return 0

    # Report the first divergence to make a real regression obvious.
    for i in range(max(len(actual), len(golden))):
        a = actual[i] if i < len(actual) else "<missing>"
        b = golden[i] if i < len(golden) else "<missing>"
        if a != b:
            print(f"FAIL: first diff at line {i}:")
            print(f"   golden : {b!r}")
            print(f"   current: {a!r}")
            break
    return 1


if __name__ == "__main__":
    sys.exit(main())
