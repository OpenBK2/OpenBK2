"""Open a map from the editor's recent list by name, and wait until it is open.

    python openrecent.py Summer              # Editor\\Backgrounds\\Summer\\MapInfo.xdb
    python openrecent.py "New Map\\"          # a substring of the path; the first match wins
    python openrecent.py --list

File -> recent entry N is ID_MAIN_RECENT_0 + N (1048 up), and N is the entry's
place in <RecentList> in Editor\\UserData.xml -- which is the user's list and
changes whenever they open something. Posting 1048 "to open the Summer map" is
how a run once opened the wrong map and took the editor down, so this looks the
entry up by name every time.

Waits for the frame first (rundesktop.py can return before it has its title,
and a command posted then goes nowhere) and then for the frame's title to name
the map, which is when the editor has finished opening it.
"""
import argparse
import re
import sys
import time

import uiprobe as P

USERDATA = r'C:\Games\bk2\Editor\UserData.xml'
ID_MAIN_RECENT_0 = 1048


def recent(path):
    s = open(path, encoding='utf-8', errors='replace').read()
    block = re.search(r'<RecentList>(.*?)</RecentList>', s, re.S)
    return re.findall(r'<Item>(.*?)</Item>', block.group(1)) if block else []


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('name', nargs='?')
    ap.add_argument('--list', action='store_true')
    ap.add_argument('--userdata', default=USERDATA)
    ap.add_argument('--desktop', default='bk2probe')
    ap.add_argument('--timeout', type=int, default=90)
    a = ap.parse_args()
    entries = recent(a.userdata)
    if a.list or not a.name:
        for n, e in enumerate(entries):
            print('%d  %d  %s' % (n, ID_MAIN_RECENT_0 + n, e))
        return 0
    matches = [n for n, e in enumerate(entries) if a.name.lower() in e.lower()]
    if not matches:
        raise SystemExit('%r is not in the recent list: %s' % (a.name, entries))
    n = matches[0]
    P.attach(a.desktop)
    frame = P.wait_for_top('Blitzkrieg 2 Editor', seconds=a.timeout)
    if not frame:
        raise SystemExit('no editor frame on %s' % a.desktop)
    P.post(frame, P.WM_COMMAND, ID_MAIN_RECENT_0 + n, 0)
    print('posted %d for %s' % (ID_MAIN_RECENT_0 + n, entries[n]))
    # The title names the map as "[path WxH]" once it is open.
    stem = entries[n].split('\\')[-2] if '\\' in entries[n] else entries[n]
    for _ in range(a.timeout * 2):
        title = P.text(frame) if P.u.IsWindow(frame) else ''
        if stem.lower() in title.lower():
            print('open: %s' % title)
            return 0
        if not P.u.IsWindow(frame):
            raise SystemExit('the editor went away while opening it')
        time.sleep(0.5)
    raise SystemExit('not open after %ds; title %r' % (a.timeout, P.text(frame)))


if __name__ == '__main__':
    sys.exit(main())
