"""Helpers for driving the MFC Iometer GUI with PyAutoGUI.

Launch the GUI with a non-local-manager ICF (no local Dynamo spawn -> no UAC),
find/position its window, capture screenshots, and shut it down.
"""
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

pyautogui.FAILSAFE = True   # slam mouse to a corner to abort
pyautogui.PAUSE = 0.25      # small settle between actions


def kill_iometer():
    for exe in ("IOmeter.exe", "Iometer.exe", "Dynamo.exe", "dynamotest.exe"):
        subprocess.run(["taskkill", "/F", "/IM", exe], capture_output=True)


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
    """Launch IOmeter interactively (config only -> not batch mode)."""
    kill_iometer()
    time.sleep(1.0)
    args = [str(IOMETER_EXE), "/c", str(icf)] + (extra_args or [])
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
    """Replace a text/combo field's contents. Ctrl+A and triple-click don't
    reliably select-all in CFileDialog combo edits, so explicitly select the
    whole line (End, then Shift+Home) and delete before typing."""
    pyautogui.click(win.left + rx, win.top + ry)   # focus the field
    time.sleep(0.2)
    pyautogui.press("end")
    pyautogui.keyDown("shift"); pyautogui.press("home"); pyautogui.keyUp("shift")
    pyautogui.press("delete")
    time.sleep(0.1)
    pyautogui.write(text, interval=0.01)
    time.sleep(0.2)


# Toolbar button centers, relative to the main window's top-left (1500x950 layout).
TB_OPEN = (33, 54)
TB_SAVE = (70, 54)
TB_START = (275, 54)


def screenshot_window(win, name):
    """Capture just the given window's rectangle (for focused inspection)."""
    ARTIFACTS.mkdir(exist_ok=True)
    path = ARTIFACTS / name
    region = (max(win.left, 0), max(win.top, 0), win.width, win.height)
    pyautogui.screenshot(str(path), region=region)
    return path
