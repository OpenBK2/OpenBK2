"""Compare every map-info palette tab between the MFC and the wx build, live.

    python palsweep.py                          # all panes, both toolkits, then the diff
    python palsweep.py --panes 2 --toolkits wx  # one pane, one toolkit
    python palsweep.py --diff-only              # compare what an earlier run wrote

For each toolkit and shortcut pane: restore the editor's saved state, make it
open on that pane, start the editor on bk2probe with the stub d3d9.dll (so a
map with objects opens and the state fills its palettes), open the map, and
dump every tab with paldump. Then diff the two toolkits tab by tab: content
only, so layout does not count and content does.

Needs a backup taken first (editorstate.py backup), which it restores before
each launch and once more at the end. The map is opened from the recent list
by name and never saved; the editor is killed, not closed, so nothing is
written back.
"""
import argparse
import difflib
import os
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
PY = sys.executable


def run(*args, env=None, check=False):
    return subprocess.run([PY] + [os.path.join(HERE, args[0])] + list(args[1:]), env=env,
                          capture_output=True, text=True, check=check)


def sweep(toolkit, pane, mapname, outdir):
    run('rundesktop.py', '--kill')
    time.sleep(2)
    run('editorstate.py', 'restore')
    run('editorstate.py', 'active', str(pane), '0')
    env = dict(os.environ)
    if toolkit == 'wx':
        env['OBK2_WX_DIALOGS'] = '1'
    else:
        env.pop('OBK2_WX_DIALOGS', None)
    run('rundesktop.py', '--d3d9stub', env=env)
    opened = run('openrecent.py', mapname)
    if opened.returncode != 0:
        raise SystemExit('%s pane %d: %s' % (toolkit, pane, (opened.stdout + opened.stderr).strip()))
    time.sleep(8)
    tabs = int(run('paldump.py', '--tabs').stdout.strip() or 0)
    path = os.path.join(outdir, 'pal_%s_%d.txt' % (toolkit, pane))
    with open(path, 'w', encoding='utf-8') as f:
        for tab in range(tabs):
            f.write('=== pane %d tab %d\n' % (pane, tab))
            f.write(run('paldump.py', '--tab', str(tab)).stdout)
    print('%s pane %d: %d tabs -> %s' % (toolkit, pane, tabs, path))


def sections(path):
    out, cur = {}, None
    for line in open(path, encoding='utf-8', errors='replace'):
        line = line.rstrip('\r\n')
        if line.startswith('==='):
            cur = line
            out[cur] = []
        elif cur is not None and line.strip():
            out[cur].append(line)
    return out


def diff(panes, outdir):
    differing = 0
    for pane in panes:
        m, w = [os.path.join(outdir, 'pal_%s_%d.txt' % (tk, pane)) for tk in ('mfc', 'wx')]
        if not (os.path.exists(m) and os.path.exists(w)):
            print('pane %d: no output for both toolkits' % pane)
            continue
        sm, sw = sections(m), sections(w)
        for key in sorted(set(sm) | set(sw)):
            a, b = sm.get(key, ['<absent>']), sw.get(key, ['<absent>'])
            if a == b:
                print('%s: identical (%d lines)' % (key, len(a)))
                continue
            differing += 1
            print('%s: DIFFERENT' % key)
            for line in difflib.unified_diff(a, b, 'mfc', 'wx', n=0, lineterm=''):
                if not line.startswith('@@'):
                    print('    ' + line[:220])
    return differing


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--panes', type=int, nargs='+', default=[0, 1, 2, 3])
    ap.add_argument('--toolkits', nargs='+', default=['mfc', 'wx'], choices=['mfc', 'wx'])
    ap.add_argument('--map', default='New Map\\', help='recent-list entry to open, by substring')
    ap.add_argument('--out', default=os.path.join(tempfile.gettempdir(), 'obk2-palsweep'))
    ap.add_argument('--diff-only', action='store_true')
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    if not a.diff_only:
        if run('editorstate.py', 'check').returncode == 2:
            raise SystemExit('take a backup first: editorstate.py backup')
        try:
            for toolkit in a.toolkits:
                for pane in a.panes:
                    sweep(toolkit, pane, a.map, a.out)
        finally:
            run('rundesktop.py', '--kill')
            time.sleep(2)
            print(run('editorstate.py', 'restore').stdout.strip().splitlines()[-1])
    return 1 if diff(a.panes, a.out) else 0


if __name__ == '__main__':
    sys.exit(main())
