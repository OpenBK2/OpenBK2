#!/usr/bin/env python3
"""Profile one map load on Windows, from launch to exit, in one command.

This is the Windows side of what launch.sh --profile does on Linux. The game is started
with --exit-after-load, so the process loads the map and leaves, and the trace runs
from just before the launch to just after the exit. There is no attaching to a process
id and no guessing when to stop: the recording is the run.

  profile-map-load.cmd Custom/Missions/Razvedka/MapInfo.xdb
  profile-map-load.cmd "Custom/Missions/Mission Impossible/mapinfo.xdb" --mod MPBMod6
  profile-map-load.cmd --campaign "Scenario/Campaigns/GER/Campaign.xdb;0;0" --name ger1
  profile-map-load.cmd <map> --setvar deep_map_load=1 --name deep

The recorder is the same as start-profile.cmd and stop-profile.cmd: wpr's built-in CPU
profile, which is ETW and wants an elevated shell. The game inherits that elevation,
which unlike sudo on Linux leaves nothing behind that a normal run cannot open again.

Everything lands in the captures folder next to this script:

  <name>.etl                      raw trace, opens in WPA
  <name>-hot.txt                  the "map load:" line and the three hot lists below,
                                  small enough to paste  <- share this one
  <name>-butterfly.html           stacks under CInterfaceMission::NewMission only: the
                                  load itself on the main thread, nothing before it
  <name>-process-butterfly.html   the whole process, startup and teardown included
  <name>-loader-butterfly.html    the resource loader thread (GResource.cpp), which
                                  NewMission waits on but whose stacks never contain it

The NewMission report is the one comparable across runs, and compare-profiles.cmd
puts every *-butterfly.html in the folder side by side for a before and after.

Read the "map load:" line first. It is wall-clock time per phase, while the hot lists
are CPU samples, so a phase that is slow while the profile under it is thin is waiting
on something: the disk, the loader thread, or the GPU driver.
"""

import argparse
import ctypes
import datetime
import glob
import io
import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
CAPTURES = os.path.join(HERE, "captures")
SUMMARIZE = os.path.join(HERE, "summarize_profile.py")
XPERF = os.path.join(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"),
                     "Windows Kits", "10", "Windows Performance Toolkit", "xperf.exe")
# CMAKE_INSTALL_PREFIX of the Windows presets, plus the bin the game runs from. The game
# resolves ../Data and ../Profiles from its working directory, so that is where it runs.
DEFAULT_GAME_DIR = os.environ.get("BK2_GAME_DIR", r"C:\Games\bk2\bin")
# xperf's own default sampling interval, 1 ms in 100 ns units. Put back afterwards,
# since -setprofint is system wide and outlives the trace.
DEFAULT_PROFINT = 10000

# Each report is one xperf export of the same trace: (file suffix, heading, filter).
# xperf -symbol keeps only the samples whose stack contains a matching function.
REPORTS = (
    ("", "the load: stacks under CInterfaceMission::NewMission",
     ["-symbol", r"CInterfaceMission::NewMission"]),
    ("-process", "the whole process, startup and teardown included", []),
    ("-loader", "the resource loader thread",
     ["-symbol", r"NGScene::.*LoaderThread"]),
)


def is_admin():
    try:
        return bool(ctypes.windll.shell32.IsUserAnAdmin())
    except (AttributeError, OSError):
        return False


def run(cmd, check=True, **kwargs):
    print("  >", subprocess.list2cmdline(cmd))
    result = subprocess.run(cmd, **kwargs)
    if check and result.returncode != 0:
        raise RuntimeError(f"{os.path.basename(cmd[0])} failed with exit code {result.returncode}")
    return result


def pdb_path(module):
    """The PDB a module was linked against, from its CodeView record, or None.

    xperf finds engine symbols through this absolute path, so a build directory that has
    since been deleted or rebuilt leaves every engine frame as an address.
    """
    try:
        with open(module, "rb") as handle:
            data = handle.read()
    except OSError:
        return None
    # RSDS, a 16 byte GUID and a 4 byte age, then the path, NUL terminated.
    found = re.search(rb"RSDS.{20}([^\x00]{4,260}?\.pdb)\x00", data, re.S)
    return found.group(1).decode("utf-8", "replace") if found else None


def check_symbols(game_dir):
    """Warn about engine modules whose PDB cannot be found. Not fatal: the trace is
    still worth keeping, and resolves later in WPA once the PDBs are back."""
    missing = []
    for module in sorted(glob.glob(os.path.join(game_dir, "*.exe")) +
                         glob.glob(os.path.join(game_dir, "*.dll"))):
        path = pdb_path(module)
        if not path or "Engine" not in path:
            continue  # third party, or no debug information at all
        beside = os.path.join(game_dir, os.path.basename(path))
        if not os.path.exists(path) and not os.path.exists(beside):
            missing.append((os.path.basename(module), path))
    if missing:
        print(f"\nwarning: {len(missing)} engine modules have no PDB where they were linked, "
              "so their frames will not resolve:")
        for name, path in missing[:8]:
            print(f"  {name:24} {path}")
        if len(missing) > 8:
            print(f"  ... and {len(missing) - 8} more")
        print("  reinstall from a build directory that still exists, or copy the PDBs beside the DLLs\n")


def default_name(args):
    target = args.mission or args.campaign or args.replay or "load"
    # The folder holding MapInfo.xdb is the map's name; for a campaign, its first field.
    parts = [p for p in re.split(r"[\\/]", target.split(";")[0]) if p]
    stem = parts[-2] if len(parts) >= 2 and "." in parts[-1] else (parts[-1] if parts else "load")
    stem = re.sub(r"[^A-Za-z0-9_.-]+", "_", os.path.splitext(stem)[0]).strip("_") or "load"
    return f"load-{stem}-{datetime.datetime.now():%Y%m%d-%H%M%S}"


def read_load_line(game_dir):
    """The "map load:" line from log.txt. The game deletes log.txt at startup and the
    console buffer writes it on exit, so whatever is there now is from this run."""
    path = os.path.join(game_dir, "log.txt")
    try:
        with io.open(path, encoding="utf-8", errors="replace") as handle:
            text = handle.read()
    except OSError:
        return None
    # Searched for rather than read line by line: not every console line ends in a
    # newline, so the report can share a line with whatever was logged before it. The
    # " patches in " also skips the LoadProfileDelay notice, "map load: pid ...".
    reports = re.findall(r"map load: [^\r\n]*? patches in [^\r\n]*?\)", text)
    return reports[-1] if reports else None


def game_command(args, game_exe):
    cmd = [game_exe]
    if args.mission:
        cmd.append(f"--mission={args.mission}")
    if args.campaign:
        cmd.append(f"--campaign={args.campaign}")
    if args.replay:
        cmd.append(f"--replay={args.replay}")
    if args.mod is not None:
        cmd.append(f"--mod={args.mod}")
    # The = form: the game takes both, but this is the one that is never mistaken for
    # a variable name and a stray argument.
    cmd += [f"--setvar={var}" for var in args.setvar]
    cmd.append("--exit-after-load")
    cmd += args.game_args
    return cmd


def main():
    parser = argparse.ArgumentParser(
        description="Record a CPU profile of one map load on Windows.",
        epilog="Anything after -- goes to the game unchanged.")
    target = parser.add_mutually_exclusive_group(required=True)
    target.add_argument("mission", nargs="?", help="MapInfo record, e.g. Custom/Missions/Razvedka/MapInfo.xdb")
    target.add_argument("--campaign", help="<campaign>[;<chapter>[;<mission>[;<difficulty>]]]")
    target.add_argument("--replay", help="a recorded replay; loads its map")
    parser.add_argument("--mod", help="folder under MODs to attach; an empty string forces no mod")
    parser.add_argument("--setvar", action="append", default=[], metavar="NAME=VALUE",
                        help="set a game variable, repeatable, e.g. deep_map_load=1")
    parser.add_argument("--name", help="capture name; defaults to load-<map>-<time>")
    parser.add_argument("--rate", type=int, default=1000,
                        help="samples per second, 1000 to 8000 (default 1000)")
    parser.add_argument("--game-dir", default=DEFAULT_GAME_DIR,
                        help=f"the bin holding Game.exe (default {DEFAULT_GAME_DIR}, or BK2_GAME_DIR)")
    parser.add_argument("--timeout", type=int, default=900,
                        help="seconds to wait for the game before killing it (default 900)")
    parser.add_argument("--min-hits", type=int, default=5,
                        help="leave functions with fewer hits out of the reports (default 5)")

    argv = sys.argv[1:]
    game_args = []
    if "--" in argv:
        split = argv.index("--")
        argv, game_args = argv[:split], argv[split + 1:]
    args = parser.parse_args(argv)
    args.game_args = game_args

    if not is_admin():
        print("This needs administrator rights: ETW kernel sampling is not available otherwise.\n"
              "Run it from an elevated prompt, or right click profile-map-load.cmd and choose\n"
              '"Run as administrator".', file=sys.stderr)
        return 1
    if not os.path.exists(XPERF):
        print(f"xperf not found at {XPERF}\n"
              "Install the Windows Performance Toolkit, part of the Windows SDK.", file=sys.stderr)
        return 1
    game_dir = os.path.abspath(args.game_dir)
    game_exe = os.path.join(game_dir, "Game.exe")
    if not os.path.exists(game_exe):
        print(f"no Game.exe in {game_dir}; pass --game-dir or set BK2_GAME_DIR", file=sys.stderr)
        return 1
    if not 1000 <= args.rate <= 8000:
        print("--rate is 1000 to 8000: the kernel will not sample faster than 8 kHz", file=sys.stderr)
        return 1
    if args.name and args.name.startswith("-"):
        print("--name is the capture name, not a switch", file=sys.stderr)
        return 1

    check_symbols(game_dir)

    name = args.name or default_name(args)
    os.makedirs(CAPTURES, exist_ok=True)
    base = os.path.join(CAPTURES, name)
    etl = base + ".etl"
    cmd = game_command(args, game_exe)

    print(f"capture {name}, sampling at {args.rate} Hz")
    run([XPERF, "-setprofint", str(10000000 // args.rate)], stdout=subprocess.DEVNULL)
    # Clear anything an interrupted session left behind, or -start refuses.
    run(["wpr", "-cancel"], check=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    recording = False
    try:
        run(["wpr", "-start", "CPU", "-filemode"])
        recording = True

        print("loading the map...")
        started = time.monotonic()
        game = subprocess.Popen(cmd, cwd=game_dir)
        try:
            exit_code = game.wait(timeout=args.timeout)
        except subprocess.TimeoutExpired:
            game.kill()
            game.wait()
            raise RuntimeError(f"the game was still running after {args.timeout} s and was killed; "
                               "a map it could not load leaves it at the menus")
        except KeyboardInterrupt:
            # Ctrl+C reaches the console's processes, but Game.exe is a GUI app and has none.
            game.kill()
            raise
        wall = time.monotonic() - started
        pid = game.pid
        print(f"the game exited with {exit_code} after {wall:.1f} s")

        print("stopping the trace...")
        run(["wpr", "-stop", etl])
        recording = False
    finally:
        if recording:
            run(["wpr", "-cancel"], check=False)
        run([XPERF, "-setprofint", str(DEFAULT_PROFINT)], check=False, stdout=subprocess.DEVNULL)

    load_line = read_load_line(game_dir)
    if load_line:
        print(f"\n{load_line}\n")
    else:
        print(f"\nwarning: no \"map load:\" line in {os.path.join(game_dir, 'log.txt')}, so the map "
              "did not finish loading. The trace is kept; look at log.txt and error.txt.\n")

    # Local PDBs come from the path each module carries; this adds the game's own bin for
    # PDBs copied beside it, and Microsoft's server for the kernel, ntdll and the D3D9
    # runtime, without which those frames come out as ***unknown***.
    env = dict(os.environ)
    env["_NT_SYMBOL_PATH"] = ";".join(filter(None, [
        f"srv*{os.path.join(os.environ.get('TEMP', HERE), 'symcache')}*https://msdl.microsoft.com/download/symbols",
        game_dir,
        os.environ.get("_NT_SYMBOL_PATH"),
    ]))

    print("exporting stacks, the first run is slow while symbols download...")
    sections = []
    for suffix, heading, filters in REPORTS:
        report = f"{base}{suffix}-butterfly.html"
        hot = f"{base}{suffix}-hot.part"
        run([XPERF, "-i", etl, "-o", report, "-symbols", "-a", "stack",
             "-butterfly", str(args.min_hits), "-pid", str(pid)] + filters, env=env)
        result = run([sys.executable, SUMMARIZE, "--report", report, "--out", hot], check=False)
        if result.returncode == 0:
            with io.open(hot, encoding="utf-8") as handle:
                sections.append((heading, handle.read()))
            os.remove(hot)
        else:
            sections.append((heading, "no samples\n"))

    with io.open(base + "-hot.txt", "w", encoding="utf-8") as out:
        out.write(f"{load_line or 'map load: no report, the load did not finish'}\n")
        out.write(f"exit code {exit_code}, {wall:.1f} s from launch to exit, {args.rate} Hz\n")
        out.write(f"{subprocess.list2cmdline(cmd[1:])}\n")
        for heading, body in sections:
            out.write(f"\n{'=' * 8} {heading}\n\n{body}")

    print(f"\nsaved to {CAPTURES}")
    print(f"  {name}.etl                      raw trace, opens in WPA")
    print(f"  {name}-hot.txt                  load line and hot lists   <- share this one")
    print(f"  {name}-butterfly.html           the load, callers and callees, opens in a browser")
    print(f"  {name}-process-butterfly.html   the whole process")
    print(f"  {name}-loader-butterfly.html    the resource loader thread")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except RuntimeError as error:
        print(f"\n{error}", file=sys.stderr)
        sys.exit(1)
