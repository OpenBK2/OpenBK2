r"""Back up, restore and check the editor's saved state around a probe run.

    python editorstate.py backup                # before a run
    python editorstate.py check                 # what the run changed
    python editorstate.py restore               # put it all back, new files removed too
    python editorstate.py active 2 2            # open on the Gameplay pane, third tab (AI General)

The files under Editor\ are the user's own: the recent-maps list and the table
choice in UserData.xml, each editor's settings (MapInfoEditor.xml keeps the
view filter, the palettes' edit parameters and which tab is active), and
ResizeDialogStyles\*.xml, the dialogs' remembered placement and parameters.
A probe run rewrites them -- opening a map reorders the recent list, a clean
exit saves everything, an MFC dialog that has never been opened creates its
file -- so every run that changes anything is bracketed by backup and restore.
restore also deletes files that were not there at backup, which is how a
dialog's first-use placement file goes away again.

The backup lives outside the repo, in %TEMP%\obk2-editor-state by default.
"""
import argparse
import filecmp
import glob
import json
import os
import re
import shutil
import sys
import tempfile

EDITOR = r'C:\Games\bk2\Editor'
PATTERNS = ['*.xml', os.path.join('ResizeDialogStyles', '*.xml')]


def files(root):
    out = []
    for pattern in PATTERNS:
        out += [os.path.relpath(p, root) for p in glob.glob(os.path.join(root, pattern))]
    return sorted(out)


def backup(editor, store):
    if os.path.isdir(store):
        shutil.rmtree(store)
    names = files(editor)
    for name in names:
        os.makedirs(os.path.dirname(os.path.join(store, name)) or store, exist_ok=True)
        shutil.copy2(os.path.join(editor, name), os.path.join(store, name))
    with open(os.path.join(store, 'manifest.json'), 'w') as f:
        json.dump(names, f, indent=1)
    print('backed up %d files from %s to %s' % (len(names), editor, store))


def manifest(store):
    path = os.path.join(store, 'manifest.json')
    if not os.path.exists(path):
        # 2, not 1: check answers 1 for "something changed".
        sys.stderr.write('no backup in %s; run backup first\n' % store)
        sys.exit(2)
    return json.load(open(path))


def check(editor, store):
    kept = manifest(store)
    now = files(editor)
    changed = [n for n in kept if n in now and not filecmp.cmp(os.path.join(editor, n), os.path.join(store, n), False)]
    added = [n for n in now if n not in kept]
    removed = [n for n in kept if n not in now]
    for n in changed:
        print('changed %s' % n)
    for n in added:
        print('new     %s' % n)
    for n in removed:
        print('missing %s' % n)
    print('%d changed, %d new, %d missing' % (len(changed), len(added), len(removed)))
    return len(changed) + len(added) + len(removed)


def restore(editor, store):
    kept = manifest(store)
    for name in kept:
        shutil.copy2(os.path.join(store, name), os.path.join(editor, name))
    for name in files(editor):
        if name not in kept:
            os.remove(os.path.join(editor, name))
            print('removed %s, which the run created' % name)
    print('restored %d files' % len(kept))
    return check(editor, store)


def active(editor, pane, tab):
    """Which shortcut pane the map-info editor opens on, and which tab in it."""
    path = os.path.join(editor, 'MapInfoEditor.xml')
    s = open(path, encoding='utf-8', newline='').read()
    s, n = re.subn(r'(<Item>\s*<Key>%d</Key>\s*<Data>)\d+(</Data>)' % pane, r'\g<1>%d\g<2>' % tab, s, count=1)
    if n != 1:
        raise SystemExit('no ActiveStateMap entry for pane %d' % pane)
    s = re.sub(r'(<ActiveStateIndex>)\d+(</ActiveStateIndex>)', r'\g<1>%d\g<2>' % pane, s, count=1)
    open(path, 'w', encoding='utf-8', newline='').write(s)
    print('map-info editor will open on pane %d, tab %d' % (pane, tab))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--editor', default=EDITOR, help='the install\'s Editor directory')
    ap.add_argument('--store', default=os.path.join(tempfile.gettempdir(), 'obk2-editor-state'))
    sub = ap.add_subparsers(dest='cmd', required=True)
    sub.add_parser('backup')
    sub.add_parser('check')
    sub.add_parser('restore')
    s = sub.add_parser('active')
    s.add_argument('pane', type=int)
    s.add_argument('tab', type=int)
    a = ap.parse_args()
    if a.cmd == 'backup':
        backup(a.editor, a.store)
    elif a.cmd == 'check':
        return 1 if check(a.editor, a.store) else 0
    elif a.cmd == 'restore':
        return 1 if restore(a.editor, a.store) else 0
    elif a.cmd == 'active':
        active(a.editor, a.pane, a.tab)
    return 0


if __name__ == '__main__':
    sys.exit(main())
