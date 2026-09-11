"""Read and drive another process's Win32 UI without touching the real input.

The shared half of the port's UI probes. Everything here is either a read that
the window manager answers for any process, or a message aimed at one named
window -- never SendInput, SetCursorPos, SetForegroundWindow or anything else
that goes to whatever is in front, because during a session that is the user's
own window. See rundesktop.py for the desktop the editor runs on for this.

Three rules that cost time to learn and are encoded below:

  * **Text comes from WM_GETTEXT, not GetWindowText.** The system marshals
    WM_GETTEXT across processes and the control answers from what it holds
    now; GetWindowText on another process's edit returns the window's stored
    caption, which an edit control does not keep current.
  * **Some messages are marshalled, some are not.** WM_GETTEXT, CB_GETLBTEXT
    and LB_GETTEXT copy into a buffer in *this* process. LVM_GETITEMTEXT,
    TVM_GETITEM, LVM_GETITEMRECT and friends write through a pointer the
    target dereferences, so the buffer has to live in the target (Remote).
  * **A hand-made WM_NOTIFY from another process is refused**, posted or sent
    (ERROR_ACCESS_DENIED). To make a control notify its parent, post the
    control the input that makes it do so itself -- a click at a row, a
    double-click at a tree item -- and let it hit-test.

Importing this attaches the calling thread to a desktop only when asked
(attach()); a fresh Python process owns no windows, so SetThreadDesktop works.
"""
import ctypes as C
import time
from ctypes import wintypes

u = C.WinDLL('user32', use_last_error=True)
k = C.WinDLL('kernel32', use_last_error=True)

WNDENUMPROC = C.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
u.OpenDesktopW.restype = wintypes.HANDLE
u.OpenDesktopW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
u.SetThreadDesktop.argtypes = [wintypes.HANDLE]
u.EnumDesktopWindows.argtypes = [wintypes.HANDLE, WNDENUMPROC, wintypes.LPARAM]
u.EnumChildWindows.argtypes = [wintypes.HWND, WNDENUMPROC, wintypes.LPARAM]
u.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, C.c_int]
u.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, C.c_int]
u.GetWindowLongPtrW.argtypes = [wintypes.HWND, C.c_int]
u.GetWindowLongPtrW.restype = C.c_ssize_t
u.GetWindowRect.argtypes = [wintypes.HWND, C.POINTER(wintypes.RECT)]
u.GetClientRect.argtypes = [wintypes.HWND, C.POINTER(wintypes.RECT)]
u.ScreenToClient.argtypes = [wintypes.HWND, C.POINTER(wintypes.POINT)]
u.GetWindow.argtypes = [wintypes.HWND, wintypes.UINT]
u.GetWindow.restype = wintypes.HWND
u.GetParent.argtypes = [wintypes.HWND]
u.GetParent.restype = wintypes.HWND
u.GetAncestor.argtypes = [wintypes.HWND, wintypes.UINT]
u.GetAncestor.restype = wintypes.HWND
u.IsWindowEnabled.argtypes = [wintypes.HWND]
u.IsWindowVisible.argtypes = [wintypes.HWND]
u.IsWindow.argtypes = [wintypes.HWND]
u.GetDlgCtrlID.argtypes = [wintypes.HWND]
u.GetWindowThreadProcessId.argtypes = [wintypes.HWND, C.POINTER(wintypes.DWORD)]
u.SendMessageTimeoutW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM,
                                  wintypes.UINT, wintypes.UINT, C.POINTER(C.c_size_t)]
u.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
u.GetDpiForWindow.argtypes = [wintypes.HWND]
k.OpenProcess.restype = wintypes.HANDLE
k.VirtualAllocEx.restype = wintypes.LPVOID
k.VirtualAllocEx.argtypes = [wintypes.HANDLE, wintypes.LPVOID, C.c_size_t, wintypes.DWORD, wintypes.DWORD]
k.VirtualFreeEx.argtypes = [wintypes.HANDLE, wintypes.LPVOID, C.c_size_t, wintypes.DWORD]
k.WriteProcessMemory.argtypes = [wintypes.HANDLE, wintypes.LPVOID, wintypes.LPCVOID, C.c_size_t,
                                 C.POINTER(C.c_size_t)]
k.ReadProcessMemory.argtypes = [wintypes.HANDLE, wintypes.LPCVOID, wintypes.LPVOID, C.c_size_t,
                                C.POINTER(C.c_size_t)]
k.CloseHandle.argtypes = [wintypes.HANDLE]


class GUITHREADINFO(C.Structure):
    _fields_ = [('cbSize', wintypes.DWORD), ('flags', wintypes.DWORD),
                ('hwndActive', wintypes.HWND), ('hwndFocus', wintypes.HWND),
                ('hwndCapture', wintypes.HWND), ('hwndMenuOwner', wintypes.HWND),
                ('hwndMoveSize', wintypes.HWND), ('hwndCaret', wintypes.HWND),
                ('rcCaret', wintypes.RECT)]


u.GetGUIThreadInfo.argtypes = [wintypes.DWORD, C.POINTER(GUITHREADINFO)]


class LVITEMW(C.Structure):
    _fields_ = [('mask', wintypes.UINT), ('iItem', C.c_int), ('iSubItem', C.c_int),
                ('state', wintypes.UINT), ('stateMask', wintypes.UINT),
                ('pszText', C.c_void_p), ('cchTextMax', C.c_int), ('iImage', C.c_int),
                ('lParam', C.c_ssize_t), ('iIndent', C.c_int), ('iGroupId', C.c_int),
                ('cColumns', wintypes.UINT), ('puColumns', C.c_void_p),
                ('piColFmt', C.c_void_p), ('iGroup', C.c_int)]


class TVITEMW(C.Structure):
    _fields_ = [('mask', wintypes.UINT), ('hItem', C.c_void_p), ('state', wintypes.UINT),
                ('stateMask', wintypes.UINT), ('pszText', C.c_void_p), ('cchTextMax', C.c_int),
                ('iImage', C.c_int), ('iSelectedImage', C.c_int), ('cChildren', C.c_int),
                ('lParam', C.c_ssize_t)]


# Messages, grouped by control.
WM_SETTEXT, WM_GETTEXT, WM_CLOSE, WM_COMMAND, WM_SYSCOMMAND = 0x000C, 0x000D, 0x0010, 0x0111, 0x0112
WM_KEYDOWN, WM_KEYUP, WM_CHAR, WM_HSCROLL = 0x0100, 0x0101, 0x0102, 0x0114
WM_LBUTTONDOWN, WM_LBUTTONUP, WM_LBUTTONDBLCLK = 0x0201, 0x0202, 0x0203
SC_CLOSE, MK_LBUTTON, IDOK, IDCANCEL = 0xF060, 0x0001, 1, 2
BM_GETCHECK, BM_CLICK = 0x00F0, 0x00F5
CB_GETCOUNT, CB_GETCURSEL, CB_GETLBTEXT, CB_SETCURSEL = 0x0146, 0x0147, 0x0148, 0x014E
CBN_SELCHANGE, CBN_SELENDOK = 1, 9
LB_GETTEXT, LB_GETCURSEL, LB_GETCOUNT, LB_GETSEL, LB_SETTOPINDEX, LB_GETITEMHEIGHT = \
    0x0189, 0x0188, 0x018B, 0x0187, 0x0197, 0x01A1
LVM_FIRST = 0x1000
LVM_GETITEMCOUNT, LVM_GETITEMRECT, LVM_GETCOLUMNWIDTH, LVM_GETHEADER = \
    LVM_FIRST + 4, LVM_FIRST + 14, LVM_FIRST + 29, LVM_FIRST + 31
LVM_SETITEMSTATE, LVM_GETITEMSTATE, LVM_GETEXTENDEDLISTVIEWSTYLE, LVM_GETITEMTEXTW = \
    LVM_FIRST + 43, LVM_FIRST + 44, LVM_FIRST + 55, LVM_FIRST + 115
LVIS_FOCUSED, LVIS_SELECTED, LVIS_STATEIMAGEMASK = 0x1, 0x2, 0xF000
HDM_GETITEMCOUNT = 0x1200
TVM_GETITEMRECT, TVM_GETNEXTITEM, TVM_SELECTITEM, TVM_EXPAND, TVM_GETITEMW = \
    0x1104, 0x110A, 0x110B, 0x1102, 0x113E
TVGN_ROOT, TVGN_NEXT, TVGN_CHILD, TVGN_CARET, TVE_EXPAND = 0, 1, 4, 9, 2
TCM_GETITEMCOUNT, TCM_GETITEMRECT, TCM_GETCURSEL = 0x1304, 0x130A, 0x130B
TBM_GETPOS = 0x0400
GW_OWNER, GA_ROOT = 4, 2
SMTO_ABORTIFHUNG = 0x0002

STYLES = [(0x80000000, 'POPUP'), (0x40000000, 'CHILD'), (0x10000000, 'VISIBLE'),
          (0x08000000, 'DISABLED'), (0x00C00000, 'CAPTION'), (0x00800000, 'BORDER'),
          (0x00080000, 'SYSMENU'), (0x00040000, 'THICKFRAME'), (0x00010000, 'TABSTOP'),
          (0x00020000, 'GROUP')]
EXSTYLES = [(0x80, 'TOOLWINDOW'), (0x1, 'DLGMODALFRAME'), (0x200, 'CLIENTEDGE'),
            (0x10000, 'CONTROLPARENT'), (0x8, 'TOPMOST')]
LVEX = [(0x1, 'GRIDLINES'), (0x4, 'CHECKBOXES'), (0x20, 'FULLROWSELECT'), (0x400, 'INFOTIP'),
        (0x4000, 'LABELTIP'), (0x10000, 'DOUBLEBUFFER')]

DESKTOP = ['bk2probe']


# --- windows -----------------------------------------------------------------

def attach(desktop='bk2probe'):
    """Point this thread at a desktop, so enumeration and handles work there."""
    DESKTOP[0] = desktop
    desk = u.OpenDesktopW(desktop, 0, False, 0x10000000)
    if not desk:
        raise SystemExit('no desktop %s -- is the editor running there? (rundesktop.py)' % desktop)
    if not u.SetThreadDesktop(desk):
        raise SystemExit('SetThreadDesktop(%s) failed: %d' % (desktop, C.get_last_error()))
    return desk


def send(h, msg, wparam=0, lparam=0, timeout=4000):
    """SendMessageTimeout; None if it timed out or failed."""
    out = C.c_size_t(0)
    if not u.SendMessageTimeoutW(h, msg, wparam, lparam, SMTO_ABORTIFHUNG, timeout, C.byref(out)):
        return None
    return out.value


def post(h, msg, wparam=0, lparam=0):
    return u.PostMessageW(h, msg, wparam, lparam)


def cls(h):
    b = C.create_unicode_buffer(256)
    u.GetClassNameW(h, b, 256)
    return b.value


def text(h):
    b = C.create_unicode_buffer(1024)
    if send(h, WM_GETTEXT, 1024, C.addressof(b)) is None:
        u.GetWindowTextW(h, b, 1024)
    return b.value


def rect(h):
    r = wintypes.RECT()
    u.GetWindowRect(h, C.byref(r))
    return r


def rel_rect(parent, h):
    """(x, y, w, h) of h in parent's client coordinates."""
    r = rect(h)
    p = wintypes.POINT(r.left, r.top)
    u.ScreenToClient(parent, C.byref(p))
    return (p.x, p.y, r.right - r.left, r.bottom - r.top)


def style(h):
    return u.GetWindowLongPtrW(h, -16) & 0xFFFFFFFF


def exstyle(h):
    return u.GetWindowLongPtrW(h, -20) & 0xFFFFFFFF


def flags(value, table):
    return '|'.join(name for bit, name in table if value & bit == bit) or '-'


def pid_of(h):
    pid = wintypes.DWORD()
    tid = u.GetWindowThreadProcessId(h, C.byref(pid))
    return pid.value, tid


def top_windows():
    """Every top-level window on the attached desktop."""
    desk = u.OpenDesktopW(DESKTOP[0], 0, False, 0x10000000)
    found = []
    u.EnumDesktopWindows(desk, WNDENUMPROC(lambda h, _: found.append(h) or True), 0)
    return found


def descendants(h):
    found = []
    u.EnumChildWindows(h, WNDENUMPROC(lambda w, _: found.append(w) or True), 0)
    return found


def children(h):
    """Direct children only."""
    return [w for w in descendants(h) if u.GetParent(w) == h]


def all_windows():
    """Every window on the desktop, top-level and child, in enumeration order."""
    out = []
    for t in top_windows():
        out.append(t)
        out += descendants(t)
    return out


def find(text_is=None, text_has=None, class_is=None, ctrl_id=None, visible=None, among=None):
    """Windows matching every criterion given."""
    hits = []
    for h in (among if among is not None else all_windows()):
        if class_is is not None and cls(h) != class_is:
            continue
        if ctrl_id is not None and u.GetDlgCtrlID(h) != ctrl_id:
            continue
        if visible is not None and bool(u.IsWindowVisible(h)) != visible:
            continue
        if text_is is not None or text_has is not None:
            t = text(h)
            if text_is is not None and t != text_is:
                continue
            if text_has is not None and text_has not in t:
                continue
        hits.append(h)
    return hits


def find_top(title, exact=True, visible=True):
    """The top-level window with this title, or None."""
    for h in top_windows():
        t = text(h)
        if (t == title if exact else title in t) and (not visible or u.IsWindowVisible(h)):
            return h
    return None


def wait_for_top(title, exact=False, seconds=60):
    for _ in range(int(seconds * 4)):
        h = find_top(title, exact)
        if h:
            return h
        time.sleep(0.25)
    return None


def ancestry(h):
    chain = []
    while h:
        chain.append(h)
        h = u.GetParent(h)
    return chain


# --- memory in the target ----------------------------------------------------

class Remote:
    """A page in the process that owns a window, for messages that write
    through a pointer the target dereferences. Freed on close()."""

    def __init__(self, h, size=4096):
        pid, _ = pid_of(h)
        self.proc = k.OpenProcess(0x1F0FFF, False, pid)
        self.mem = k.VirtualAllocEx(self.proc, None, size, 0x1000, 4)
        if not self.mem:
            raise SystemExit('VirtualAllocEx failed: %d' % C.get_last_error())

    def write(self, offset, data):
        k.WriteProcessMemory(self.proc, self.mem + offset, data, len(data), None)

    def read(self, offset, size):
        b = C.create_string_buffer(size)
        k.ReadProcessMemory(self.proc, self.mem + offset, b, size, None)
        return b.raw

    def close(self):
        k.VirtualFreeEx(self.proc, self.mem, 0, 0x8000)
        k.CloseHandle(self.proc)

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()


# --- reading controls --------------------------------------------------------

def checked(h):
    return send(h, BM_GETCHECK)


def combo_items(h):
    items = []
    for i in range(send(h, CB_GETCOUNT) or 0):
        b = C.create_unicode_buffer(512)
        send(h, CB_GETLBTEXT, i, C.addressof(b))     # marshalled: local buffer
        items.append(b.value)
    return items


def combo_selection(h):
    sel = send(h, CB_GETCURSEL)
    return None if sel is None or sel > 0x7FFFFFFF else sel


def listbox_items(h, limit=10000):
    items = []
    for i in range(min(send(h, LB_GETCOUNT) or 0, limit)):
        b = C.create_unicode_buffer(1024)
        send(h, LB_GETTEXT, i, C.addressof(b))       # marshalled: local buffer
        items.append(b.value)
    return items


def list_columns(h):
    header = send(h, LVM_GETHEADER)
    return (send(header, HDM_GETITEMCOUNT) or 1) if header else 1


def list_rows(h, limit=10000):
    """[(cells, state)] -- every column's text, and the LVIS_* state bits
    (selection, focus, and the check box in the state image)."""
    rows = []
    columns = list_columns(h)
    with Remote(h) as rm:
        for i in range(min(send(h, LVM_GETITEMCOUNT) or 0, limit)):
            cells = []
            for c in range(columns):
                rm.write(0, bytes(LVITEMW(mask=1, iItem=i, iSubItem=c, pszText=rm.mem + 1024, cchTextMax=1024)))
                n = send(h, LVM_GETITEMTEXTW, i, rm.mem) or 0
                cells.append(rm.read(1024, n * 2).decode('utf-16-le', 'replace'))
            state = send(h, LVM_GETITEMSTATE, i, LVIS_STATEIMAGEMASK | LVIS_SELECTED | LVIS_FOCUSED) or 0
            rows.append((cells, state))
    return rows


def list_row_rect(h, row):
    with Remote(h) as rm:
        rm.write(0, bytes(wintypes.RECT(0, 0, 0, 0)))      # left = LVIR_BOUNDS
        send(h, LVM_GETITEMRECT, row, rm.mem)
        return wintypes.RECT.from_buffer_copy(rm.read(0, 16))


def tree_text(h, item, rm):
    rm.write(64, bytes(TVITEMW(mask=1, hItem=item, pszText=rm.mem + 1024, cchTextMax=1000)))
    send(h, TVM_GETITEMW, 0, rm.mem + 64)
    return rm.read(1024, 2000).decode('utf-16-le', 'replace').split('\0')[0]


def tree_items(h, limit=2000, expand=False):
    """[(depth, item, text)] depth-first; expanded branches only unless expand."""
    out = []
    with Remote(h) as rm:
        def walk(item, depth):
            while item and len(out) < limit:
                out.append((depth, item, tree_text(h, item, rm)))
                if expand:
                    send(h, TVM_EXPAND, TVE_EXPAND, item)
                walk(send(h, TVM_GETNEXTITEM, TVGN_CHILD, item), depth + 1)
                item = send(h, TVM_GETNEXTITEM, TVGN_NEXT, item)
        walk(send(h, TVM_GETNEXTITEM, TVGN_ROOT, 0), 0)
    return out


def tree_caret(h):
    return send(h, TVM_GETNEXTITEM, TVGN_CARET, 0)


def tree_item_rect(h, item):
    with Remote(h) as rm:
        rm.write(0, bytes(C.c_size_t(item)))
        send(h, TVM_GETITEMRECT, 1, rm.mem)        # wParam TRUE: the text's rect
        return wintypes.RECT.from_buffer_copy(rm.read(0, 16))


def describe(h):
    """A control reduced to its content, one or more lines, no geometry."""
    c = cls(h)
    en = '' if u.IsWindowEnabled(h) else ' [disabled]'
    if c == 'Button':
        kind = style(h) & 0xF
        if kind in (2, 3, 5, 6):
            return ['check %r=%s%s' % (text(h), checked(h), en)]
        if kind in (4, 9):
            return ['radio %r=%s%s' % (text(h), checked(h), en)]
        if kind == 7:
            return ['group %r' % text(h)]
        if kind == 0xB:
            # BS_OWNERDRAW: its check state is the owner's and cannot be read.
            return ['ownerdrawn button %r%s' % (text(h), en)]
        return ['button %r%s' % (text(h), en)]
    if c == 'ComboBox':
        items = combo_items(h)
        sel = combo_selection(h)
        return ['combo sel=%r items(%d)=%s%s' % (items[sel] if sel is not None and sel < len(items) else None,
                                                 len(items), items[:12], en)]
    if c == 'SysListView32':
        rows = list_rows(h, 60)
        out = ['list rows=%d%s' % (send(h, LVM_GETITEMCOUNT) or 0, en)]
        for cells, state in rows:
            mark = '*' if state & LVIS_SELECTED else ' '
            check = {1: '[ ] ', 2: '[x] '}.get((state & LVIS_STATEIMAGEMASK) >> 12, '')
            out.append('  row %s%s%s' % (mark, check, ' | '.join(cells)))
        return out
    if c == 'ListBox':
        items = listbox_items(h, 60)
        return ['listbox n=%d sel=%s items=%s%s' % (send(h, LB_GETCOUNT) or 0, send(h, LB_GETCURSEL), items, en)]
    if c == 'SysTreeView32':
        items = tree_items(h, 60)
        return ['tree items=%d' % len(items)] + ['  %s%s' % ('  ' * d, t) for d, _, t in items]
    if c == 'Edit':
        return ['edit %r%s' % (text(h), en)]
    if c == 'Static':
        t = text(h)
        return ['static %r' % t] if t else []
    if c == 'msctls_trackbar32':
        return ['slider pos=%s%s' % (send(h, TBM_GETPOS), en)]
    return []


def content(root, visible_only=True):
    """Sorted content lines of every control under root, however deep."""
    lines = []
    for h in descendants(root):
        if not visible_only or u.IsWindowVisible(h):
            lines += describe(h)
    return sorted(lines)


def dump_dialog(dlg, out=print):
    """A top-level dialog: frame, owner and modality, focus, then each control."""
    _, tid = pid_of(dlg)
    gti = GUITHREADINFO(cbSize=C.sizeof(GUITHREADINFO))
    u.GetGUIThreadInfo(tid, C.byref(gti))
    owner = u.GetWindow(dlg, GW_OWNER)
    wr, cr = rect(dlg), wintypes.RECT()
    u.GetClientRect(dlg, C.byref(cr))
    out('dialog %s %r class=%s dpi=%s' % (hex(dlg), text(dlg), cls(dlg), u.GetDpiForWindow(dlg)))
    out('  style %s  ex %s' % (flags(style(dlg), STYLES), flags(exstyle(dlg), EXSTYLES)))
    out('  window %dx%d client %dx%d' % (wr.right - wr.left, wr.bottom - wr.top, cr.right, cr.bottom))
    # Both halves of modality: owned by the frame, and the frame disabled.
    out('  owner %s %r enabled=%s' % (hex(owner or 0), text(owner)[:50] if owner else '-',
                                      bool(u.IsWindowEnabled(owner)) if owner else '-'))
    focus = gti.hwndFocus
    out('  focus %s %s %r' % (hex(focus or 0), cls(focus) if focus else '-', text(focus) if focus else ''))
    for n, h in enumerate(children(dlg)):
        s = style(h)
        line = '  [%2d] %-16s id=%-6d %-26r %-20s %s' % (
            n, cls(h)[:16], u.GetDlgCtrlID(h), text(h)[:26], rel_rect(dlg, h),
            flags(s, STYLES) + ('' if u.IsWindowEnabled(h) else ' (disabled)'))
        if cls(h) == 'Button' and (s & 0xF) in (2, 3, 4, 5, 6, 9):
            line += ' check=%s' % checked(h)
        out(line)
        if cls(h) == 'SysListView32':
            out('       lvs=%#x ex=%s col0=%s' % (s & 0xFFFF, flags(send(h, LVM_GETEXTENDEDLISTVIEWSTYLE) or 0, LVEX),
                                              send(h, LVM_GETCOLUMNWIDTH, 0)))
        if cls(h) in ('ComboBox', 'SysListView32', 'ListBox', 'SysTreeView32'):
            for extra in describe(h):
                out('       ' + extra)


# --- actions -----------------------------------------------------------------

def _lparam(x, y):
    return ((y & 0xFFFF) << 16) | (x & 0xFFFF)


def click_at(h, x, y):
    """A left click at client (x, y), posted to h alone."""
    post(h, WM_LBUTTONDOWN, MK_LBUTTON, _lparam(x, y))
    post(h, WM_LBUTTONUP, 0, _lparam(x, y))


def double_click_at(h, x, y):
    """The four messages of a double-click, posted to h alone. The control
    hit-tests them itself, which is how its parent gets NM_DBLCLK."""
    for msg, wp in ((WM_LBUTTONDOWN, MK_LBUTTON), (WM_LBUTTONUP, 0), (WM_LBUTTONDBLCLK, MK_LBUTTON),
                    (WM_LBUTTONUP, 0)):
        post(h, msg, wp, _lparam(x, y))


def click_button(h, wait=False):
    """BM_CLICK. Posted by default: a button that opens a modal dialog does not
    return from a sent click until the dialog closes. It does not reliably
    dismiss a system MessageBox; dismiss_message_boxes() does."""
    if wait:
        return send(h, BM_CLICK)
    return post(h, BM_CLICK)


def choose(combo, item):
    """Select an entry by text or index and notify as a user's pick does:
    CBN_SELCHANGE, which MFC answers, then CBN_SELENDOK, which wxChoice answers.
    CB_SETCURSEL alone notifies nobody."""
    index = item if isinstance(item, int) else combo_items(combo).index(item)
    send(combo, CB_SETCURSEL, index)
    parent, cid = u.GetParent(combo), u.GetDlgCtrlID(combo)
    send(parent, WM_COMMAND, (CBN_SELCHANGE << 16) | (cid & 0xFFFF), combo)
    send(parent, WM_COMMAND, (CBN_SELENDOK << 16) | (cid & 0xFFFF), combo)
    return index


u.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPCWSTR]
u.SendMessageW.restype = C.c_ssize_t


def set_text(h, value):
    """WM_SETTEXT, which the system marshals. An edit notifies EN_CHANGE."""
    return u.SendMessageW(h, WM_SETTEXT, 0, value)


def set_list_state(h, row, state, mask):
    with Remote(h) as rm:
        rm.write(0, bytes(LVITEMW(stateMask=mask, state=state)))
        return send(h, LVM_SETITEMSTATE, row, rm.mem)


def toggle_list_check(h, row):
    """Flip a LVS_EX_CHECKBOXES row's check the way a click on it does."""
    st = send(h, LVM_GETITEMSTATE, row, LVIS_STATEIMAGEMASK) or 0
    return set_list_state(h, row, 0x1000 if (st & LVIS_STATEIMAGEMASK) == 0x2000 else 0x2000, LVIS_STATEIMAGEMASK)


def click_list_row(h, row, double=False):
    """A click on the row, so the list selects it and reports the change the
    way it would for a user. Selecting an already-selected row with
    LVM_SETITEMSTATE reports nothing, and states learn the row from the report."""
    r = list_row_rect(h, row)
    x, y = r.left + min(20, max(1, (r.right - r.left) // 2)), (r.top + r.bottom) // 2
    (double_click_at if double else click_at)(h, x, y)
    return x, y


def open_tree_caret(h):
    """Double-click the tree's caret item, as opening it by hand does."""
    item = tree_caret(h)
    if not item:
        return None
    r = tree_item_rect(h, item)
    double_click_at(h, (r.left + r.right) // 2, (r.top + r.bottom) // 2)
    with Remote(h) as rm:
        return tree_text(h, item, rm)


def tree_select_first_leaf(h):
    """Expand down the first branch to its first leaf and select it."""
    item = send(h, TVM_GETNEXTITEM, TVGN_ROOT, 0)
    while item:
        send(h, TVM_EXPAND, TVE_EXPAND, item)
        child = send(h, TVM_GETNEXTITEM, TVGN_CHILD, item)
        if not child:
            send(h, TVM_SELECTITEM, TVGN_CARET, item)
            return item
        item = child
    return None


def click_tab(strip, n):
    """Click tab n of a SysTabControl32, scrolling it into view first: a strip
    too narrow for its tabs shows an up-down, and a click on a tab scrolled out
    of view lands on nothing. The strip then tells its parent the selection
    changed, as for a real click; TCM_SETCURSEL would tell nobody."""
    updown = [c for c in children(strip) if cls(c) == 'msctls_updown32']
    if updown:
        send(strip, WM_HSCROLL, (n << 16) | 4, updown[0])     # SB_THUMBPOSITION
        send(strip, WM_HSCROLL, 8, updown[0])                 # SB_ENDSCROLL
    with Remote(strip) as rm:
        send(strip, TCM_GETITEMRECT, n, rm.mem)
        r = wintypes.RECT.from_buffer_copy(rm.read(0, 16))
    click_at(strip, (r.left + r.right) // 2, (r.top + r.bottom) // 2)


def key(h, vk):
    """WM_KEYDOWN/WM_KEYUP posted to one control, for keyboard-driven paths."""
    post(h, WM_KEYDOWN, vk, 1)
    post(h, WM_KEYUP, vk, 0xC0000001)


def dismiss_message_boxes(frame_title='Blitzkrieg 2 Editor', rounds=6):
    """Close every #32770 left up besides the frame by sending it IDOK.
    BM_CLICK posted at a MessageBox's button is not reliable across processes;
    WM_COMMAND IDOK is what its dialog procedure waits for. A box left up keeps
    the frame disabled, which the next probe reads as a modality leak."""
    closed = []
    for _ in range(rounds):
        boxes = [h for h in top_windows() if cls(h) == '#32770' and u.IsWindowVisible(h)
                 and not text(h).startswith(frame_title)]
        if not boxes:
            break
        for h in boxes:
            closed.append(text(h))
            post(h, WM_COMMAND, IDOK, 0)
        time.sleep(1.0)
    return closed
