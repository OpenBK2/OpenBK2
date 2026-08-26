#!/usr/bin/env python3
"""Back up the shipped configs and rewrite their paths for a case sensitive
filesystem with forward slashes.

The game's own cfg files carry Windows paths: `exec .\\profiles\\consts.cfg`.
A backslash is an ordinary filename character off Windows, and the directory is
`Profiles` in what this repository ships, so neither the separator nor the case
survives the move. This is a local unblock, not the fix; see PORT_ROADMAP.md.

Every file is copied to <name>.orig first, and files that already have a backup
are left alone so a second run cannot lose the original.
"""
import io
import os
import re
import shutil
import sys

CANONICAL_DIRS = ['Profiles', 'Data', 'MODs']

def canonicalise(text):
    # separators first
    out = text.replace('\\', '/')
    # then the directory names, case insensitively, only where they look like a
    # path component rather than part of a longer word
    for name in CANONICAL_DIRS:
        out = re.sub(r'(?<![A-Za-z0-9_])' + name + r'(?=/)', name, out, flags=re.IGNORECASE)
    return out

def main():
    root = sys.argv[1] if len(sys.argv) > 1 else '.'
    changed = 0
    for dirpath, _dirnames, filenames in os.walk(root):
        for name in filenames:
            if not name.endswith('.cfg'):
                continue
            path = os.path.join(dirpath, name)
            with io.open(path, 'rb') as f:
                raw = f.read()
            try:
                text = raw.decode('utf-8')
            except UnicodeDecodeError:
                print('skipped (not utf-8): %s' % path)
                continue
            new = canonicalise(text)
            if new == text:
                continue
            backup = path + '.orig'
            if not os.path.exists(backup):
                shutil.copy2(path, backup)
            with io.open(path, 'wb') as f:
                f.write(new.encode('utf-8'))
            print('patched %s' % path)
            changed += 1
    print('%d file(s) changed' % changed)

if __name__ == '__main__':
    main()
