"""GUI interaction test (#11 - About + EULA/Legal box): open the "Iometer
Information" (About) dialog from the toolbar, open the Intel Open Source License
(EULA) box from it, then close both and confirm the GUI survives.

There's no ICF to assert against here - the check is that both modal dialogs open
and close cleanly. Exercises GalileoApp.cpp (OnAppAbout / CAboutDlg) and
LegalBox.cpp (CLegalBox), neither of which any other test reaches.

Run from an interactive desktop, FOREGROUND:  python gui_tests/test_about_eula.py
"""
import sys
import time
import subprocess

import pyautogui
import pygetwindow as gw
import iometer_gui as g

ICF = g.REPO / "test" / "fixtures" / "icf_compat" / "worker_rich.icf"
DYNAMOTEST = g.REPO / "src" / "msvs11" / "Release" / "x64" / "dynamotest.exe"

TB_ABOUT = (528, 54)                 # toolbar "?" button (ID_APP_ABOUT)
ABOUT_TITLE = "Iometer Information"


def _about():
    return next((w for w in gw.getAllWindows()
                 if w.visible and w.title.strip() == ABOUT_TITLE), None)


def _hwnds():
    return {w._hWnd: w for w in gw.getAllWindows() if w.visible}


def run():
    g.kill_iometer(); time.sleep(1.0)
    g.launch(ICF)
    win = g.find_window(timeout=20.0)
    if not win:
        return "no main window"
    subprocess.Popen([str(DYNAMOTEST), "-n", "TESTMGR", "-m", "localhost",
                      "--rdelay", "50", "-i", "127.0.0.1"])
    time.sleep(7.0)
    g.position(win, 50, 30, 1500, 950); time.sleep(1.5)
    win = g.find_window() or win

    g.click_window(win, *TB_ABOUT); time.sleep(1.0)        # open About
    about = _about()
    if not about:
        g.kill_iometer(); return "About dialog did not open"

    # The EULA button is the first control after the dialog's default focus;
    # a direct pixel click on it proved unreliable (modal foreground quirk), so
    # foreground the dialog and drive it with Tab+Space.
    pyautogui.click(about.left + 150, about.top + 15); time.sleep(0.4)
    base = set(_hwnds())
    legal = None
    for _ in range(3):
        pyautogui.press("tab"); time.sleep(0.2)
        pyautogui.press("space"); time.sleep(0.9)
        new = [w for h, w in _hwnds().items()
               if h not in base and w.title.strip() == "" and w.width > 400 and w.height > 300]
        if new:
            legal = new[0]; break
    if not legal:
        try: about.close()
        except Exception: pass
        g.close_iometer(win); return "EULA / Legal box did not open"

    try: legal.close()                                     # WM_CLOSE the LegalBox
    except Exception: pass
    time.sleep(0.6)
    a2 = _about()
    if a2:
        try: a2.close()                                    # then the About dialog
        except Exception: pass
    time.sleep(0.6)

    still_about = _about() is not None
    main = g.find_window(timeout=5.0)
    g.close_iometer(win)
    if still_about:
        return "About dialog did not close"
    if not main:
        return "main window vanished after closing dialogs"
    return None


def main():
    err = run()
    if err:
        print(f"FAIL: {err}"); return 1
    print("PASS: opened the About + EULA/Legal dialogs and closed both cleanly")
    return 0


if __name__ == "__main__":
    sys.exit(main())
