"""Find, inspect and drive single controls on the probe desktop.

Finding (read-only):
    python uiact.py find --text "mobileScriptIDs"            # hwnd, class, id, rect, visible
    python uiact.py find --class SysTreeView32 --id 400
    python uiact.py ancestry 0x1D09CE                         # up to the top-level window
    python uiact.py subtree 0x140C32                          # everything below, with content
    python uiact.py content 0x140C32                          # just the content lines, sorted

Driving (messages to that one window, never the real mouse or keyboard):
    python uiact.py click 0x50D4A 154 63                      # client coordinates
    python uiact.py dblclick 0x50D4A 154 63
    python uiact.py button 0x3F0BDC                           # BM_CLICK, posted
    python uiact.py row 0x400C28 3 [--double]                 # click a list view row
    python uiact.py check 0x400C28 3                          # flip a list row's check box
    python uiact.py tab 0x1D09CE 4                            # click a tab, scrolling it into view
    python uiact.py choose 0x190C86 BuildingRPGStats          # pick a combo entry, notifying
    python uiact.py settext 0x20D5A 89
    python uiact.py key 0x50D4A 0x0D                          # VK_RETURN
    python uiact.py treeleaf 0x50D4A                          # select the first leaf
    python uiact.py treeopen 0x50D4A                          # double-click the caret item

Handles are only valid while the window lives; find them afresh each run.
"""
import argparse
import sys

import uiprobe as P


def hwnd(s):
    return int(s, 16) if not s.lower().startswith('0x') else int(s, 0)


def line(h, depth=0):
    r = P.rect(h)
    return '%s%s %-24s id=%-6d %-28r %s %s' % (
        '  ' * depth, hex(h), P.cls(h)[:24], P.u.GetDlgCtrlID(h), P.text(h)[:28],
        (r.left, r.top, r.right - r.left, r.bottom - r.top), 'visible' if P.u.IsWindowVisible(h) else 'hidden')


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--desktop', default='bk2probe')
    sub = ap.add_subparsers(dest='cmd', required=True)
    f = sub.add_parser('find')
    f.add_argument('--text')
    f.add_argument('--has', help='text contains')
    f.add_argument('--class', dest='klass')
    f.add_argument('--id', type=int)
    f.add_argument('--visible', action='store_true')
    for name in ('ancestry', 'subtree', 'content', 'button', 'treeleaf', 'treeopen'):
        sub.add_parser(name).add_argument('hwnd', type=hwnd)
    for name in ('click', 'dblclick'):
        s = sub.add_parser(name)
        s.add_argument('hwnd', type=hwnd)
        s.add_argument('x', type=int)
        s.add_argument('y', type=int)
    s = sub.add_parser('row')
    s.add_argument('hwnd', type=hwnd)
    s.add_argument('row', type=int)
    s.add_argument('--double', action='store_true')
    for name in ('check', 'tab'):
        s = sub.add_parser(name)
        s.add_argument('hwnd', type=hwnd)
        s.add_argument('n', type=int)
    for name in ('choose', 'settext'):
        s = sub.add_parser(name)
        s.add_argument('hwnd', type=hwnd)
        s.add_argument('value')
    s = sub.add_parser('key')
    s.add_argument('hwnd', type=hwnd)
    s.add_argument('vk', type=lambda v: int(v, 0))
    a = ap.parse_args()
    P.attach(a.desktop)

    if a.cmd == 'find':
        hits = P.find(text_is=a.text, text_has=a.has, class_is=a.klass, ctrl_id=a.id,
                      visible=True if a.visible else None)
        for h in hits:
            print(line(h))
        return 0 if hits else 1
    if a.cmd == 'ancestry':
        for depth, h in enumerate(reversed(P.ancestry(a.hwnd))):
            print(line(h, depth))
        return 0
    if a.cmd == 'subtree':
        def walk(h, depth):
            print(line(h, depth))
            for extra in P.describe(h):
                print('  ' * depth + '    ' + extra)
            for c in P.children(h):
                walk(c, depth + 1)
        walk(a.hwnd, 0)
        return 0
    if a.cmd == 'content':
        for text in P.content(a.hwnd):
            print(text)
        return 0
    if a.cmd == 'click':
        P.click_at(a.hwnd, a.x, a.y)
    elif a.cmd == 'dblclick':
        P.double_click_at(a.hwnd, a.x, a.y)
    elif a.cmd == 'button':
        P.click_button(a.hwnd)
    elif a.cmd == 'row':
        print('at %s' % (P.click_list_row(a.hwnd, a.row, a.double),))
    elif a.cmd == 'check':
        P.toggle_list_check(a.hwnd, a.n)
    elif a.cmd == 'tab':
        P.click_tab(a.hwnd, a.n)
    elif a.cmd == 'choose':
        print('index %d' % P.choose(a.hwnd, int(a.value) if a.value.isdigit() else a.value))
    elif a.cmd == 'settext':
        P.set_text(a.hwnd, a.value)
    elif a.cmd == 'key':
        P.key(a.hwnd, a.vk)
    elif a.cmd == 'treeleaf':
        item = P.tree_select_first_leaf(a.hwnd)
        print('selected %s' % (hex(item) if item else None))
    elif a.cmd == 'treeopen':
        print('opened %r' % P.open_tree_caret(a.hwnd))
    print('%s done' % a.cmd)
    return 0


if __name__ == '__main__':
    sys.exit(main())
