"""Dump or drive a dialog that is up on the probe desktop, whichever toolkit drew it.

    python dlgprobe.py --list                              # every dialog up now
    python dlgprobe.py "MapInfo View Filter"               # frame, modality, focus, controls
    python dlgprobe.py "Parcel properties" --choose EPATCH_DEFENCE --settext 0=2.5 --post OK
    python dlgprobe.py "MapInfo View Filter" --click "Wire frame" --toggle 2
    python dlgprobe.py "Game Database Tables" --move 100,100,500,600
    python dlgprobe.py --dismiss                           # IDOK every message box left up

The dump is what a migrated dialog is compared on: the frame's styles, whether
it is owned by the editor's frame and the frame disabled (a dialog that only
sits on top looks modal in a screenshot and is not), where the focus starts,
and each control with its content. Run the same command against the MFC and
the wx dialog and diff.

Actions apply in the order listed in --help and the dump follows them, except
--post, which ends the dialog and so is always last and dumps nothing.
"""
import argparse
import sys
import time

import uiprobe as P


def button(dlg, caption):
    for h in P.children(dlg):
        if P.cls(h) == 'Button' and P.text(h).replace('&', '') == caption:
            return h
    raise SystemExit('no button %r' % caption)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('title', nargs='?', help='the dialog caption, exactly')
    ap.add_argument('--desktop', default='bk2probe')
    ap.add_argument('--list', action='store_true', help='list the dialogs that are up')
    ap.add_argument('--dismiss', action='store_true', help='IDOK every message box left up')
    ap.add_argument('--settext', action='append', default=[], metavar='N=TEXT',
                    help='WM_SETTEXT on the Nth edit box (0-based); repeatable')
    ap.add_argument('--choose', action='append', default=[], metavar='[N=]ITEM',
                    help='pick ITEM in the Nth combo box (default the first), as a user would')
    ap.add_argument('--click', action='append', default=[], metavar='CAPTION',
                    help='click a button that does not end the dialog (sent, waits)')
    ap.add_argument('--toggle', action='append', default=[], type=int, metavar='ROW',
                    help='flip a row check box in the first list view')
    ap.add_argument('--move', metavar='X,Y,W,H', help='move and size the dialog')
    ap.add_argument('--post', metavar='CAPTION', help='click a button that ends the dialog (posted)')
    ap.add_argument('--close', action='store_true', help='post SC_CLOSE: the close box')
    ap.add_argument('--quiet', action='store_true', help='act, do not dump')
    a = ap.parse_args()
    P.attach(a.desktop)

    if a.dismiss:
        print('closed: %s' % (P.dismiss_message_boxes() or 'nothing'))
        return 0
    if a.list:
        for h in P.top_windows():
            if P.cls(h) == '#32770' and P.u.IsWindowVisible(h):
                owner = P.u.GetWindow(h, P.GW_OWNER)
                print('%s %r owner=%s' % (hex(h), P.text(h), hex(owner or 0)))
        return 0
    if not a.title:
        ap.error('give a dialog title, --list or --dismiss')
    dlg = P.find_top(a.title)
    if dlg is None:
        raise SystemExit('no %r on %s' % (a.title, a.desktop))

    kids = P.children(dlg)
    for spec in a.settext:
        n, value = spec.split('=', 1)
        edits = [h for h in kids if P.cls(h) == 'Edit']
        P.set_text(edits[int(n)], value)
        print('edit %s <- %r' % (n, value))
    for spec in a.choose:
        n, item = (spec.split('=', 1) if '=' in spec and spec.split('=', 1)[0].isdigit() else ('0', spec))
        combos = [h for h in kids if P.cls(h) == 'ComboBox']
        print('chose %r (index %d)' % (item, P.choose(combos[int(n)], item)))
    for caption in a.click:
        P.click_button(button(dlg, caption), wait=True)
        print('clicked %r' % caption)
    for row in a.toggle:
        lists = [h for h in kids if P.cls(h) == 'SysListView32']
        P.toggle_list_check(lists[0], row)
        print('toggled row %d' % row)
    if a.move:
        x, y, w, h = (int(v) for v in a.move.split(','))
        P.u.SetWindowPos.argtypes = [P.wintypes.HWND, P.wintypes.HWND] + [P.C.c_int] * 4 + [P.wintypes.UINT]
        P.u.SetWindowPos(dlg, 0, x, y, w, h, 0x0014)             # NOZORDER | NOACTIVATE
        print('moved to %d,%d %dx%d' % (x, y, w, h))
    if a.post:
        P.click_button(button(dlg, a.post))
        print('posted a click on %r' % a.post)
        return 0
    if a.close:
        P.post(dlg, P.WM_SYSCOMMAND, P.SC_CLOSE, 0)
        print('posted SC_CLOSE')
        return 0
    if not a.quiet:
        time.sleep(0.3)
        P.dump_dialog(dlg)
    return 0


if __name__ == '__main__':
    sys.exit(main())
