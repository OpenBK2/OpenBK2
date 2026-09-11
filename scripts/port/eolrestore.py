"""Put back the line endings an editing tool normalised, line by line, from HEAD.

    python scripts/port/eolrestore.py                 # report every modified file whose EOLs drifted
    python scripts/port/eolrestore.py --fix a.h b.cpp # restore those files
    python scripts/port/eolrestore.py --fix           # restore every modified file that drifted

This tree's line endings are not uniform, and not always uniform within a file:
B2_MapEditor/CMakeLists.txt and Sources/CMakeLists.txt mix CRLF and LF lines,
and several headers end with a line holding a lone carriage return
(`...\\n\\n\\r\\n`). An editor that rewrites a file normalises all of that, and a
five-line change becomes a diff of the whole file, or of its last line for no
reason. `git diff --stat` against the size of the change is how it shows.

The fix is mechanical: match the working file's lines to HEAD's by content
(difflib, ignoring the endings) and give every line that is unchanged the
ending HEAD had for it. A changed or added line takes the ending of the HEAD
line it replaces, or failing that of the unchanged line before it, so new lines
blend into whichever convention surrounds them. Content is never touched.
"""
import argparse
import difflib
import os
import shutil
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))


def git():
    """git, found even where it is not on PATH -- PowerShell here has none."""
    found = shutil.which('git')
    if found:
        return found
    for base in (os.environ.get('ProgramFiles', r'C:\Program Files'), os.environ.get('LOCALAPPDATA', '')):
        candidate = os.path.join(base, 'Git', 'cmd', 'git.exe') if base else ''
        if candidate and os.path.exists(candidate):
            return candidate
    raise SystemExit('git not found')


def split(data):
    """[(content, ending)] with the ending one of b'\\r\\n', b'\\n', b'\\r', b''."""
    lines = []
    for raw in data.splitlines(keepends=True):
        if raw.endswith(b'\r\n'):
            lines.append((raw[:-2], b'\r\n'))
        elif raw.endswith(b'\n') or raw.endswith(b'\r'):
            lines.append((raw[:-1], raw[-1:]))
        else:
            lines.append((raw, b''))
    return lines


def restored(head, work):
    h, w = split(head), split(work)
    matcher = difflib.SequenceMatcher(None, [c for c, _ in h], [c for c, _ in w], autojunk=False)
    out = []
    last = h[0][1] if h else b'\n'
    for op, i1, i2, j1, j2 in matcher.get_opcodes():
        if op == 'equal':
            for k in range(i2 - i1):
                content, ending = h[i1 + k]
                # HEAD's final line had no ending and is no longer final: it
                # takes the ending of the lines around it.
                if not ending and j1 + k != len(w) - 1:
                    ending = last
                out.append(content + ending)
                last = ending or last
        elif op in ('replace', 'insert'):
            for k in range(j2 - j1):
                content, ending = w[j1 + k]
                if op == 'replace' and i1 + k < i2:
                    target = h[i1 + k][1]
                else:
                    target = last
                # Keep a missing final ending missing: it is the file's real end.
                out.append(content + (target if ending else b''))
                last = target or last
    return b''.join(out)


def modified(paths):
    if paths:
        return paths
    out = subprocess.run([git(), 'diff', '--name-only', 'HEAD'], cwd=ROOT, capture_output=True, text=True).stdout
    return [p for p in out.splitlines() if p]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('paths', nargs='*', help='files, relative to the repository root; default every modified file')
    ap.add_argument('--fix', action='store_true')
    ap.add_argument('--rev', default='HEAD')
    a = ap.parse_args()
    drifted = 0
    for rel in modified(a.paths):
        rel = os.path.relpath(os.path.abspath(rel), ROOT).replace('\\', '/') if os.path.exists(rel) else rel
        path = os.path.join(ROOT, rel)
        blob = subprocess.run([git(), 'cat-file', 'blob', '%s:%s' % (a.rev, rel)], cwd=ROOT, capture_output=True)
        if blob.returncode != 0 or not os.path.isfile(path):
            continue
        head, work = blob.stdout, open(path, 'rb').read()
        if b'\0' in head[:8000]:
            continue
        fixed = restored(head, work)
        if fixed == work:
            continue
        drifted += 1
        crlf = (head.count(b'\r\n'), work.count(b'\r\n'), fixed.count(b'\r\n'))
        print('%-70s CRLF lines HEAD %d, now %d, restored %d%s' % (rel, crlf[0], crlf[1], crlf[2],
                                                                  '  -- fixed' if a.fix else ''))
        if a.fix:
            with open(path, 'wb') as f:
                f.write(fixed)
    print('%d file(s) %s' % (drifted, 'fixed' if a.fix else 'drifted'))
    return 1 if drifted and not a.fix else 0


if __name__ == '__main__':
    sys.exit(main())
