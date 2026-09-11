"""Run one of these probes with its thread on another desktop.

    python ondesk.py windump.py B2_MapEditor.exe out.json
    python ondesk.py treetext.py --id 400 --max 30
    python ondesk.py --desktop other treeselect.py --id 400 --leaf

EnumWindows and friends see only the calling thread's desktop, and the older
probes here (windump, treetext, treeselect, menudump, ...) have no --desktop of
their own. A fresh Python process owns no windows yet, so SetThreadDesktop
succeeds for it; after that the probe runs unchanged and sees the editor that
rundesktop.py started. Probes built on uiprobe.py take --desktop and do not
need this.
"""
import ctypes as C
import os
import runpy
import sys
from ctypes import wintypes

u = C.WinDLL('user32', use_last_error=True)
u.OpenDesktopW.restype = wintypes.HANDLE
u.OpenDesktopW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
u.SetThreadDesktop.argtypes = [wintypes.HANDLE]

args = sys.argv[1:]
desktop = 'bk2probe'
if args[:1] == ['--desktop']:
    desktop, args = args[1], args[2:]
if not args:
    raise SystemExit(__doc__)

desk = u.OpenDesktopW(desktop, 0, False, 0x10000000)
if not desk:
    raise SystemExit('OpenDesktop(%s) failed: %d' % (desktop, C.get_last_error()))
if not u.SetThreadDesktop(desk):
    raise SystemExit('SetThreadDesktop failed: %d' % C.get_last_error())

script = args[0]
if not os.path.exists(script):
    # A bare name means one of the probes beside this file.
    script = os.path.join(os.path.dirname(os.path.abspath(__file__)), script)
sys.argv = [script] + args[1:]
sys.path.insert(0, os.path.dirname(script))
runpy.run_path(script, run_name='__main__')
