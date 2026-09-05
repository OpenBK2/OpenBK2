"""Drag with the real mouse, for the things no window dump can answer.

Docking, undocking and resizing all happen inside a modal drag loop that reads
real mouse messages, so posting WM_LBUTTONDOWN at a window does not start one --
it has to be actual input. This injects it with SendInput, which is why it moves
the pointer and needs the editor in front.

    # grab a docked pane by its gripper and drop it back where it started
    python dragbar.py --id 136 --from-gripper --dx 0 --dy 0

    # tear it off towards the middle of the screen
    python dragbar.py --id 136 --from-gripper --dx 400 --dy 200

--from-gripper aims at the caption strip SECControlBar draws, left of the close
button. --at x,y aims at a point in the window's client area instead.

Nothing here is specific to this editor beyond the default image name.
"""
import argparse
import ctypes as C
import subprocess
import time
from ctypes import wintypes

u = C.WinDLL('user32', use_last_error=True)
WNDENUMPROC = C.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
u.EnumWindows.argtypes = [WNDENUMPROC, wintypes.LPARAM]
u.EnumChildWindows.argtypes = [wintypes.HWND, WNDENUMPROC, wintypes.LPARAM]
u.GetDlgCtrlID.argtypes = [wintypes.HWND]
u.GetWindowThreadProcessId.argtypes = [wintypes.HWND, C.POINTER(wintypes.DWORD)]
u.GetWindowRect.argtypes = [wintypes.HWND, C.POINTER(wintypes.RECT)]
u.GetClientRect.argtypes = [wintypes.HWND, C.POINTER(wintypes.RECT)]
u.ClientToScreen.argtypes = [wintypes.HWND, C.POINTER(wintypes.POINT)]
u.SetForegroundWindow.argtypes = [wintypes.HWND]
u.GetSystemMetrics.argtypes = [C.c_int]

SM_XVIRTUALSCREEN, SM_YVIRTUALSCREEN = 76, 77
SM_CXVIRTUALSCREEN, SM_CYVIRTUALSCREEN = 78, 79
SM_CYSMCAPTION = 51

INPUT_MOUSE = 0
INPUT_KEYBOARD = 1
MOUSEEVENTF_MOVE = 0x0001
MOUSEEVENTF_LEFTDOWN = 0x0002
MOUSEEVENTF_LEFTUP = 0x0004
MOUSEEVENTF_ABSOLUTE = 0x8000
MOUSEEVENTF_VIRTUALDESK = 0x4000
KEYEVENTF_KEYUP = 0x0002
VK_SHIFT, VK_CONTROL = 0x10, 0x11


class MOUSEINPUT(C.Structure):
    _fields_ = [('dx', wintypes.LONG), ('dy', wintypes.LONG),
                ('mouseData', wintypes.DWORD), ('dwFlags', wintypes.DWORD),
                ('time', wintypes.DWORD), ('dwExtraInfo', C.POINTER(C.c_ulong))]


class KEYBDINPUT(C.Structure):
    _fields_ = [('wVk', wintypes.WORD), ('wScan', wintypes.WORD),
                ('dwFlags', wintypes.DWORD), ('time', wintypes.DWORD),
                ('dwExtraInfo', C.POINTER(C.c_ulong))]


class INPUT(C.Structure):
    class _U(C.Union):
        _fields_ = [('mi', MOUSEINPUT), ('ki', KEYBDINPUT)]
    _anonymous_ = ('u',)
    _fields_ = [('type', wintypes.DWORD), ('u', _U)]


def key(vk, up):
    """Press or release one key, so a click can be modified.

    Real input, for the same reason the mouse is: the modifier has to be down
    in the system's own keyboard state when the click is dispatched, because
    that is where GetKeyState and the MK_ bits in WM_LBUTTONDOWN come from.
    Posting a WM_KEYDOWN would set neither.
    """
    inp = INPUT()
    inp.type = INPUT_KEYBOARD
    inp.ki.wVk = vk
    inp.ki.dwFlags = KEYEVENTF_KEYUP if up else 0
    u.SendInput(1, C.byref(inp), C.sizeof(INPUT))


def send(flags, x=0, y=0):
    """One mouse event, in absolute virtual-desktop coordinates."""
    vx = u.GetSystemMetrics(SM_XVIRTUALSCREEN)
    vy = u.GetSystemMetrics(SM_YVIRTUALSCREEN)
    vw = u.GetSystemMetrics(SM_CXVIRTUALSCREEN) or 1
    vh = u.GetSystemMetrics(SM_CYVIRTUALSCREEN) or 1
    inp = INPUT()
    inp.type = INPUT_MOUSE
    inp.mi.dx = int(( x - vx ) * 65535 / vw)
    inp.mi.dy = int(( y - vy ) * 65535 / vh)
    inp.mi.dwFlags = flags | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK
    u.SendInput(1, C.byref(inp), C.sizeof(INPUT))


def pids_for(image):
    txt = subprocess.run(['tasklist', '/FI', 'IMAGENAME eq ' + image, '/FO', 'CSV', '/NH'],
                         capture_output=True, text=True).stdout
    return {int(l.split('","')[1]) for l in txt.splitlines() if l.startswith('"')}


def find(image, ctrl_id):
    want = pids_for(image)
    hits, tops = [], []

    def child(h, _):
        if u.GetDlgCtrlID(h) == ctrl_id:
            hits.append(h)
        return True

    def top(h, _):
        pid = wintypes.DWORD()
        u.GetWindowThreadProcessId(h, C.byref(pid))
        if pid.value in want:
            tops.append(h)
            u.EnumChildWindows(h, WNDENUMPROC(child), 0)
        return True

    u.EnumWindows(WNDENUMPROC(top), 0)
    return hits, tops


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--image', default='B2_MapEditor.exe')
    ap.add_argument('--id', type=int, required=True, help='control id of the bar to drag')
    ap.add_argument('--from-gripper', action='store_true',
                    help='start on the caption strip, left of the close button')
    ap.add_argument('--at', help='start at this client x,y instead')
    ap.add_argument('--dx', type=int, default=0)
    ap.add_argument('--dy', type=int, default=0)
    ap.add_argument('--steps', type=int, default=12,
                    help='intermediate moves; a drag loop needs more than one')
    ap.add_argument('--ctrl', action='store_true', help='hold Ctrl over the click')
    ap.add_argument('--shift', action='store_true', help='hold Shift over the click')
    args = ap.parse_args()

    hits, tops = find(args.image, args.id)
    if not hits:
        raise SystemExit('no window with id %d' % args.id)
    hwnd = hits[0]

    rc = wintypes.RECT()
    u.GetClientRect(hwnd, C.byref(rc))
    if args.at:
        cx, cy = (int(v) for v in args.at.split(','))
    elif args.from_gripper:
        cx = 12
        cy = 4 + (u.GetSystemMetrics(SM_CYSMCAPTION) or 16) // 2
    else:
        cx, cy = rc.right // 2, rc.bottom // 2
    pt = wintypes.POINT(cx, cy)
    u.ClientToScreen(hwnd, C.byref(pt))
    x0, y0 = pt.x, pt.y
    x1, y1 = x0 + args.dx, y0 + args.dy
    print('0x%X client %dx%d, dragging %d,%d -> %d,%d' % (hwnd, rc.right, rc.bottom, x0, y0, x1, y1))

    for h in tops:
        u.SetForegroundWindow(h)
    time.sleep(0.6)

    send(MOUSEEVENTF_MOVE, x0, y0)
    time.sleep(0.2)
    # Down before the click and up after it, so the modifier is in the system's
    # keyboard state for the whole of the button down, the moves and the up.
    if args.ctrl:
        key(VK_CONTROL, False)
    if args.shift:
        key(VK_SHIFT, False)
    time.sleep(0.1)
    send(MOUSEEVENTF_LEFTDOWN, x0, y0)
    time.sleep(0.2)
    for i in range(1, args.steps + 1):
        send(MOUSEEVENTF_MOVE, x0 + (x1 - x0) * i // args.steps,
             y0 + (y1 - y0) * i // args.steps)
        time.sleep(0.05)
    time.sleep(0.3)
    send(MOUSEEVENTF_LEFTUP, x1, y1)
    time.sleep(0.2)
    if args.shift:
        key(VK_SHIFT, True)
    if args.ctrl:
        key(VK_CONTROL, True)
    time.sleep(0.6)
    print('done')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
