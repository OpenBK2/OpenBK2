"""Compile translation units the way the default build does: without wxWidgets.

    python scripts/port/nowx-compile.py ED_B2_M1 MapInfoViewFilterWx.cpp MapInfoEditor.cpp
    python scripts/port/nowx-compile.py MapEditor SearchObjectViewWx.cpp --build out/build/agent-x64

BUILD_WX_EDITOR is off by default and the presets and CI leave it off, so the
configuration everyone else builds is the one a wx-enabled working tree never
compiles. Every migrated file has an `#ifdef OBK2_WITH_WX` half; this checks
the other half. It takes a real compile command for the target out of
`ninja -t commands`, drops the wx define, the wx include directories and the
precompiled header, and compiles each named file to a scratch object.

Run it from any shell: without cl.exe on PATH it re-runs itself inside the
newest Visual Studio's vcvarsall x64.
"""
import argparse
import os
import shlex
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
SOURCES = os.path.join(ROOT, 'Versions', 'Temporary', 'Engine', 'Sources')


def in_vcvars():
    if shutil.which('cl.exe'):
        return None
    vswhere = os.path.join(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)'),
                           'Microsoft Visual Studio', 'Installer', 'vswhere.exe')
    vs = subprocess.run([vswhere, '-latest', '-property', 'installationPath'], capture_output=True,
                        text=True).stdout.strip()
    vcvars = os.path.join(vs, 'VC', 'Auxiliary', 'Build', 'vcvarsall.bat')
    cmd = '"%s" x64 >nul 2>&1 && "%s" %s' % (vcvars, sys.executable,
                                             ' '.join('"%s"' % a for a in [os.path.abspath(__file__)] + sys.argv[1:]))
    return subprocess.run(cmd, shell=True).returncode


def template(build, target):
    out = subprocess.run(['ninja', '-C', build, '-t', 'commands', target], capture_output=True, text=True).stdout
    marker = '/%s/' % target
    for line in out.splitlines():
        if ' -c ' in line and '.cpp' in line and marker in line.replace('\\', '/') and 'cl.exe' in line.lower():
            return line
    raise SystemExit('no compile command for %s in %s' % (target, build))


def main():
    rc = in_vcvars()
    if rc is not None:
        return rc
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('target', help='the CMake target, which is also its directory under Sources')
    ap.add_argument('files', nargs='+')
    ap.add_argument('--build', default=os.path.join(ROOT, 'out', 'build', 'agent-x64'))
    a = ap.parse_args()
    args = shlex.split(template(a.build, a.target), posix=False)
    # From cl.exe on: what comes before it is the ccache and `cmake -E env` launcher.
    for n, arg in enumerate(args):
        if arg.lower().strip('"').endswith('cl.exe'):
            args = args[n:]
            break
    kept = []
    for arg in args:
        low = arg.lower()
        if low in ('-dobk2_with_wx', '-dwxusingdll', '/showincludes', '-c') or low.endswith('.cpp'):
            continue
        if (low.startswith('-external:i') or low.startswith('-i')) and 'wxwidgets' in low:
            continue
        if low.startswith(('/yu', '/fp', '/fi', '/fo', '/fd', '-fo', '-fd')):
            continue
        kept.append(arg)
    scratch = tempfile.mkdtemp(prefix='nowx-')
    failed = []
    for name in a.files:
        source = name if os.path.isabs(name) else os.path.join(SOURCES, a.target, name)
        obj = os.path.join(scratch, os.path.basename(name) + '.obj')
        cmd = ' '.join(kept + ['/Y-', '/Fo' + obj, '/Fd' + os.path.join(scratch, 'nowx.pdb'), '-c', '"%s"' % source])
        r = subprocess.run(cmd, shell=True, capture_output=True, text=True, cwd=a.build)
        errors = [l for l in (r.stdout + r.stderr).splitlines() if 'error' in l.lower()]
        print('%-32s %s' % (name, 'ok' if r.returncode == 0 else 'FAILED\n    ' + '\n    '.join(errors[:12])))
        if r.returncode != 0:
            failed.append(name)
    shutil.rmtree(scratch, ignore_errors=True)
    print('failed: %s' % (', '.join(failed) or 'none'))
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
