"""Run the editor under cdb so that the next crash leaves a stack behind.

The map editor dies during tree population and leaves nothing to read. That is
not bad luck, it is that nothing is watching: crashpad is wired into Game.exe
and not into B2_MapEditor.exe, and the whole tree builds with /EHsc, under which
catch ( ... ) does not catch a structured exception. An access violation
therefore unwinds nothing, runs no handler and writes no dump; the process is
simply gone, and the stingray trace stops at whatever line was flushed last.

This wraps the editor in cdb from the Windows SDK and arms the fatal exceptions
with a second-chance command, so the fault reports itself and then writes a full
minidump instead of vanishing. Nothing is instrumented and nothing is rebuilt:
it observes the binary that is already installed, which for a fault that is not
reproducible on demand is the point.

    python crashwatch.py                              # launch the installed editor
    python crashwatch.py --exe D:\\other\\B2_MapEditor.exe
    python crashwatch.py --attach 12345               # watch one already running
    python crashwatch.py --out C:\\temp\\run7         # where log and dump land

Leave it running and leave the editor alone; population has taken up to three
minutes of idle to start. On a fault the log gets the exception record, every
thread's stack and !analyze -v, the dump gets everything else, and cdb detaches.
On a clean exit the log says so and the exit code with it, which distinguishes
"it crashed" from "something closed it" -- a distinction the trace alone cannot
make, since it ends the same way either way.

Second chance rather than first: first-chance access violations are normal in a
Win32 process (MFC and the C runtime both raise and handle them), so breaking on
those would stop on faults that are already someone's business. Second chance
means nobody handled it, which under /EHsc means nobody could.
"""
import argparse
import os
import subprocess
import sys
import tempfile
import time

# Where the SDK puts the debuggers. x64 first: the editor is built both ways but
# cdb has to match the target, and a wrong-bitness cdb fails with a message that
# does not obviously say so.
CDB_CANDIDATES = [
    r"C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe",
    r"C:\Program Files\Windows Kits\10\Debuggers\x64\cdb.exe",
    r"C:\Program Files (x86)\Windows Kits\10\Debuggers\x86\cdb.exe",
]

DEFAULT_EXE = r"C:\Games\bk2\bin\B2_MapEditor.exe"

# The exceptions worth arming. Everything here is fatal by the time it reaches
# second chance; there is no point arming the ones a program routinely handles.
#
#   av        access violation, the one a bad string pointer produces
#   sov       stack overflow, which a recursive tree walk can reach
#   ch        invalid handle, raised by the loader when a handle is closed twice
#   eh        a C++ exception nobody caught, which then calls terminate
#   c0000374  heap corruption, reported at the free that notices and not the
#             write that caused it
#   c0000409  fast fail, which is what /GS and the CRT's checks raise
FATAL = ["av", "sov", "ch", "eh", "c0000374", "c0000409"]


def find_cdb(override):
    if override:
        if not os.path.isfile(override):
            sys.exit("no cdb at %s" % override)
        return override
    for path in CDB_CANDIDATES:
        if os.path.isfile(path):
            return path
    sys.exit(
        "cdb.exe not found. Install the Debugging Tools for Windows feature of "
        "the Windows SDK, or pass --cdb."
    )


def write_script(path, dump, symbol_dir, no_symbol_server):
    """The cdb command file. A file rather than -c because the second-chance
    commands are themselves quoted, and one level of quoting is all that
    survives a command line."""
    lines = []
    if not no_symbol_server:
        # Resolves the ntdll, MFC and comctl32 frames, which are most of a stack
        # that dies inside a window message.
        lines.append(".symfix")
    if symbol_dir:
        lines.append(".sympath+ %s" % symbol_dir)
    lines.append(".reload")
    # Print the source line where there is one, and do not truncate the stacks.
    lines.append(".lines -e")

    onfault = "; ".join([
        ".echo ==== SECOND CHANCE, NOBODY HANDLED THIS ====",
        ".lastevent",
        ".echo ==== FAULTING THREAD ====",
        "kv 200",
        ".echo ==== ALL THREADS ====",
        "~*kv 100",
        ".echo ==== ANALYSIS ====",
        "!analyze -v",
        ".dump /ma %s" % dump,
        ".echo ==== DUMP WRITTEN, DETACHING ====",
        "qd",
    ])
    for code in FATAL:
        lines.append('sxd -c2 "%s" %s' % (onfault, code))

    # A clean exit should say so, so that a truncated trace is not read as a
    # crash when the process merely went away.
    lines.append('sxe -c ".echo ==== PROCESS EXIT ====; .lastevent; qd" epr')
    lines.append("g")

    with open(path, "w", encoding="ascii") as f:
        f.write("\n".join(lines) + "\n")
    return path


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--exe", default=DEFAULT_EXE, help="editor to launch")
    ap.add_argument("--attach", type=int, help="watch this pid instead of launching")
    ap.add_argument("--cdb", help="path to cdb.exe")
    ap.add_argument("--out", help="directory for the log and dump")
    ap.add_argument("--no-symbol-server", action="store_true",
                    help="skip .symfix, for a machine with no network")
    args = ap.parse_args()

    cdb = find_cdb(args.cdb)
    out = args.out or os.path.join(tempfile.gettempdir(), "editor-crashwatch")
    os.makedirs(out, exist_ok=True)
    stamp = time.strftime("%Y%m%d-%H%M%S")
    log = os.path.join(out, "crashwatch-%s.log" % stamp)
    dump = os.path.join(out, "crashwatch-%s.dmp" % stamp)

    if args.attach:
        symbol_dir = None
        target = ["-p", str(args.attach)]
    else:
        if not os.path.isfile(args.exe):
            sys.exit("no editor at %s (build and install it, or pass --exe)" % args.exe)
        symbol_dir = os.path.dirname(os.path.abspath(args.exe))
        target = [args.exe]

    script = write_script(os.path.join(out, "crashwatch-%s.cdb" % stamp),
                          dump, symbol_dir, args.no_symbol_server)

    # -g runs past the initial loader breakpoint, -G past the final one, so cdb
    # only ever stops for something that went wrong.
    cmd = [cdb, "-g", "-G", "-logo", log, "-cf", script] + target
    print("cdb:  %s" % cdb)
    print("log:  %s" % log)
    print("dump: %s (on a fault)" % dump)
    print("running, leave the editor alone; Ctrl+C here detaches\n", flush=True)

    # Hold cdb's stdin open and never write to it. cdb reads commands from stdin
    # while the debuggee runs, and treats end of input as "quit", so handing it
    # the null device makes it kill the editor the moment it has started it.
    # A pipe with nothing on the far end is what keeps it waiting for the fault.
    proc = subprocess.Popen(cmd, stdin=subprocess.PIPE)
    try:
        rc = proc.wait()
    except KeyboardInterrupt:
        proc.terminate()
        rc = proc.wait()
    finally:
        if proc.stdin:
            proc.stdin.close()

    print("\ncdb exited %d" % rc)
    if os.path.isfile(dump):
        print("FAULT: dump at %s" % dump)
        print("       open with: %s -z %s" % (cdb, dump))
    else:
        print("no dump written, so no second-chance fault was seen")
    print("log: %s" % log)
    return 0 if rc == 0 else rc


if __name__ == "__main__":
    sys.exit(main())
