"""Viability probe: can PyAutoGUI see and drive the MFC Iometer GUI?

Launches IOmeter with a non-local ICF, finds + positions its window, captures a
screenshot, and shuts down. No assertions - this just answers "is the GUI
reachable / automatable here" before we build real interaction tests on top.

Run from an interactive desktop session:  python gui_tests/probe_mfc.py
"""
import sys
import time

import iometer_gui as g


def main():
    if not g.IOMETER_EXE.exists():
        print(f"ERROR: {g.IOMETER_EXE} not found (build MFC Release|x64 first)")
        return 2

    print(f"launch: IOmeter.exe /c {g.NONLOCAL_ICF.name}")
    proc = g.launch()

    win = g.find_window(timeout=25.0)
    if not win:
        print("FAIL: no Iometer main window appeared")
        g.kill_iometer()
        return 1

    print(f"window: '{win.title}'  ({win.left},{win.top})  {win.width}x{win.height}")

    # Loading a config with an unconnected (non-local) manager raises this dialog;
    # dismiss it so the main window is interactable.
    dlg = g.find_dialog("Waiting for Managers", timeout=6.0)
    if dlg:
        print(f"dialog: '{dlg.title}' -> dismissing (Cancel)")
        import pyautogui
        dlg.activate()
        time.sleep(0.3)
        pyautogui.press("esc")   # Cancel the wait
        time.sleep(0.5)

    g.position(win)
    time.sleep(1.0)

    shot = g.screenshot("mfc_launch.png")
    print(f"screenshot: {shot}")

    # Prove keyboard reaches the app: open then close the File menu.
    try:
        win.activate()
        import pyautogui
        pyautogui.hotkey("alt", "f")
        time.sleep(0.5)
        g.screenshot("mfc_file_menu.png")
        pyautogui.press("escape")
    except Exception as e:
        print(f"(menu probe warning: {e})")

    g.kill_iometer()
    print("PROBE OK - GUI launched, captured, and accepted input")
    return 0


if __name__ == "__main__":
    sys.exit(main())
