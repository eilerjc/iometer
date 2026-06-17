"""Helpers for driving the MFC Iometer GUI with PyAutoGUI.

Launch the GUI with a non-local-manager ICF (no local Dynamo spawn -> no UAC),
find/position its window, capture screenshots, and shut it down.
"""
import os
import re
import subprocess
import time
import pathlib

import pyautogui
import pygetwindow as gw

# The MFC main window titles itself "Iometer <version>  [Built: ...]". Anchor on
# that so we don't match e.g. an editor window that merely has "iometer" in its
# title (a project path).
MAIN_WINDOW_RE = r"^Iometer\s+\d"

HERE = pathlib.Path(__file__).resolve().parent
REPO = HERE.parent
IOMETER_EXE = REPO / "src" / "msvs11" / "Release" / "x64" / "IOmeter.exe"
NONLOCAL_ICF = REPO / "test" / "fixtures" / "nonlocal_manager.icf"
ARTIFACTS = HERE / "artifacts"

# Optional coverage: set IOCOV_DIR (raw .cov output dir) to launch IOmeter under
# OpenCppCoverage (PDB-based, no rebuild). IOCOV_TAG names the per-test .cov file.
OCC_EXE = pathlib.Path(r"C:\Program Files\OpenCppCoverage\OpenCppCoverage.exe")
COV_SRC_FILTER = "iometer\\github\\iometer\\src"   # --sources substring

pyautogui.FAILSAFE = True   # slam mouse to a corner to abort
pyautogui.PAUSE = 0.25      # small settle between actions

_cov_proc = None            # OpenCppCoverage process, when covering


def _wait_cov():
    """Wait for OpenCppCoverage (if any) to flush its .cov after IOmeter exits."""
    global _cov_proc
    if _cov_proc is not None:
        try:
            _cov_proc.wait(timeout=90)
        except Exception:
            pass
        _cov_proc = None


def _iometer_alive():
    r = subprocess.run(["tasklist", "/FI", "IMAGENAME eq IOmeter.exe", "/NH"],
                       capture_output=True, text=True)
    return "IOmeter.exe" in (r.stdout or "")


def kill_iometer():
    """Force-kill IOmeter and helpers (used for pre-launch cleanup / fallback)."""
    for exe in ("IOmeter.exe", "Iometer.exe", "Dynamo.exe", "dynamotest.exe"):
        subprocess.run(["taskkill", "/F", "/IM", exe], capture_output=True)
    _wait_cov()


def close_iometer(win):
    """Shut the GUI down gracefully via WM_CLOSE so the MFC exit/cleanup paths run
    and OpenCppCoverage sees a clean process exit. Iometer does not prompt to save
    on exit; if a stray modal appears it's answered 'No', and a stuck process is
    force-killed as a fallback. Also stops dynamotest/Dynamo and flushes coverage."""
    try:
        win.close()                       # WM_CLOSE
    except Exception:
        try:
            win.activate(); time.sleep(0.3)
            pyautogui.hotkey("alt", "f4")
        except Exception:
            pass

    for i in range(20):                   # wait ~10s for a clean exit
        if not _iometer_alive():
            break
        if i == 4:                        # still up after ~2s: nudge a possible prompt
            pyautogui.press("n")          # 'No' on a save-changes dialog, if any
        time.sleep(0.5)

    if _iometer_alive():                  # fallback: it didn't close on its own
        for exe in ("IOmeter.exe", "Iometer.exe"):
            subprocess.run(["taskkill", "/F", "/IM", exe], capture_output=True)
    for exe in ("Dynamo.exe", "dynamotest.exe"):
        subprocess.run(["taskkill", "/F", "/IM", exe], capture_output=True)
    _wait_cov()


def find_window(pattern=MAIN_WINDOW_RE, timeout=20.0):
    """Return the first visible top-level window whose title matches `pattern`."""
    rx = re.compile(pattern)
    deadline = time.time() + timeout
    while time.time() < deadline:
        for w in gw.getAllWindows():
            try:
                if w.visible and w.width > 200 and rx.search(w.title):
                    return w
            except Exception:
                pass
        time.sleep(0.5)
    return None


def find_dialog(title_exact, timeout=10.0):
    """Find a child dialog by exact title (e.g. 'Waiting for Managers')."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        for w in gw.getAllWindows():
            try:
                if w.visible and w.title.strip() == title_exact:
                    return w
            except Exception:
                pass
        time.sleep(0.3)
    return None


def launch(icf=NONLOCAL_ICF, extra_args=None):
    """Launch IOmeter interactively (config only -> not batch mode).

    If IOCOV_DIR is set, launch under OpenCppCoverage so the interactive session
    is recorded to IOCOV_DIR/<IOCOV_TAG>.cov (merged into a report afterwards)."""
    global _cov_proc
    kill_iometer()
    time.sleep(1.0)
    args = [str(IOMETER_EXE), "/c", str(icf)] + (extra_args or [])

    cov_dir = os.environ.get("IOCOV_DIR")
    if cov_dir:
        os.makedirs(cov_dir, exist_ok=True)
        tag = os.environ.get("IOCOV_TAG", "gui")
        out = str(pathlib.Path(cov_dir) / (tag + ".cov"))
        occ = [str(OCC_EXE), "--quiet",
               "--sources", COV_SRC_FILTER, "--modules", "IOmeter.exe",
               "--excluded_sources", "tests",
               "--export_type", "binary:" + out, "--"] + args
        _cov_proc = subprocess.Popen(occ)
        return _cov_proc

    _cov_proc = None
    return subprocess.Popen(args)


def position(win, left=80, top=60, width=1000, height=720):
    """Move/resize so reference images and clicks are at stable coordinates."""
    try:
        if win.isMinimized:
            win.restore()
        win.moveTo(left, top)
        win.resizeTo(width, height)
        win.activate()
        time.sleep(0.5)
    except Exception as e:
        print(f"(position warning: {e})")


def screenshot(name):
    ARTIFACTS.mkdir(exist_ok=True)
    path = ARTIFACTS / name
    pyautogui.screenshot(str(path))
    return path


def click_window(win, rx, ry):
    """Click at a point given relative to the window's top-left corner."""
    pyautogui.click(win.left + rx, win.top + ry)
    time.sleep(0.3)


def set_field(win, rx, ry, text):
    """Replace a field's contents using only plain keys. Modifier-based selection
    (Ctrl+A, Shift+Home) and triple-click proved unreliable via PyAutoGUI in these
    MFC edits/combos, so just focus and clear in BOTH directions (backspace clears
    left of the cursor, delete clears right), then type."""
    pyautogui.click(win.left + rx, win.top + ry)
    time.sleep(0.25)
    pyautogui.press("backspace", presses=160, interval=0)   # clear left of cursor
    pyautogui.press("delete", presses=160, interval=0)       # clear right of cursor
    pyautogui.write(text, interval=0.01)
    time.sleep(0.2)


def select_combo(win, rx, ry, key="down", presses=1):
    """Change a dropdown-list combo by N arrow presses. Click to focus/open, send
    the arrow(s), then Enter to commit - robust whether the click opened the list
    or just focused it (in both cases the selection ends N items away)."""
    pyautogui.click(win.left + rx, win.top + ry)
    time.sleep(0.3)
    pyautogui.press(key, presses=presses, interval=0.1)
    time.sleep(0.2)
    pyautogui.press("enter")
    time.sleep(0.3)


# Toolbar button centers, relative to the main window's top-left (1500x950 layout).
TB_OPEN = (33, 54)
TB_SAVE = (70, 54)
TB_START = (275, 54)


SAVE_DIALOG_TITLE = "Save Test Configuration File"
SAVE_DLG_SAVE_BTN = (519, 370)   # "Save" button, relative to the dialog (571x622)
# "Settings to save" checkboxes, relative to the dialog top-left.
SAVE_DLG_CHK_TESTSETUP = (46, 460)
SAVE_DLG_CHK_RESULTS   = (46, 484)
SAVE_DLG_CHK_GLOBALSPECS = (46, 508)
SAVE_DLG_CHK_MANAGERS  = (46, 532)


def gui_save(win, out_path, uncheck=None):
    """Drive File>Save to `out_path`. By default all sections are saved (the
    default-checked boxes); pass `uncheck` as a list of dialog-relative checkbox
    (rx, ry) coords to toggle off first. Returns True once the file is written.
    Must run in the foreground."""
    import os
    out_path = str(out_path)
    if os.path.exists(out_path):
        os.remove(out_path)

    dlg = None                                  # opening can flake on focus; retry
    for _ in range(3):
        try: win.activate()
        except Exception: pass
        time.sleep(0.3)
        click_window(win, *TB_SAVE)
        for _ in range(8):
            dlg = next((w for w in gw.getAllWindows()
                        if w.title.strip() == SAVE_DIALOG_TITLE and w.visible), None)
            if dlg:
                break
            time.sleep(0.5)
        if dlg:
            break
    if not dlg:
        return False

    try: dlg.activate()
    except Exception: pass
    time.sleep(0.5)
    # File name field opens with "Iometer" pre-selected -> type to replace.
    pyautogui.write(out_path, interval=0.02)
    for rx, ry in (uncheck or []):              # toggle off any requested sections
        click_window(dlg, rx, ry)
    click_window(dlg, *SAVE_DLG_SAVE_BTN)
    time.sleep(0.5)
    pyautogui.press("enter")                    # confirm any "replace?" prompt
    for _ in range(20):
        if os.path.exists(out_path) and os.path.getsize(out_path) > 0:
            return True
        time.sleep(0.5)
    return os.path.exists(out_path)


def screenshot_window(win, name):
    """Capture just the given window's rectangle (for focused inspection)."""
    ARTIFACTS.mkdir(exist_ok=True)
    path = ARTIFACTS / name
    region = (max(win.left, 0), max(win.top, 0), win.width, win.height)
    pyautogui.screenshot(str(path), region=region)
    return path
