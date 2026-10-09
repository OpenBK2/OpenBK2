"""Run real crashing child processes and validate their on-disk minidumps."""

import argparse
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import time


def validate_dump(path, mode):
    data = path.read_bytes()
    if len(data) < 32 or data[:4] != b"MDMP":
        raise AssertionError(f"Not a minidump: {path}")
    stream_count, directory = struct.unpack_from("<II", data, 8)
    if not 0 < stream_count < 1000 or directory + stream_count * 12 > len(data):
        raise AssertionError("Invalid minidump stream directory")
    streams = {}
    for index in range(stream_count):
        kind, size, offset = struct.unpack_from("<III", data, directory + index * 12)
        if offset + size > len(data):
            raise AssertionError(f"Truncated minidump stream {kind}")
        streams[kind] = data[offset:offset + size]
    # ThreadList, ModuleList and ExceptionStream are necessary to diagnose this
    # crash. Checking a .dmp filename alone would accept partial/empty reports.
    for kind in (3, 4):
        if len(streams.get(kind, b"")) < 4 or struct.unpack_from("<I", streams[kind])[0] == 0:
            raise AssertionError(f"Missing or empty minidump stream {kind}")
    exception = streams.get(6, b"")
    if len(exception) < 168:
        raise AssertionError("Missing exception context")
    thread_id, _, code = struct.unpack_from("<III", exception)
    context_size, context_offset = struct.unpack_from("<II", exception, 160)
    if not thread_id or not context_size or context_offset + context_size > len(data):
        raise AssertionError("Invalid exception thread/context")
    expected = {
        "access-violation": 0xC0000005,
        "worker-access-violation": 0xC0000005,
        "heap-corruption": 0xC0000374,
        "caught-exception": 0x0517A7ED,
        "fallback-directory": 0x0517A7ED,
        "invalid-parameter": 0x40000015,
        "purecall": 0x40000015,
    }
    if os.name == "nt" and mode in expected and code != expected[mode]:
        raise AssertionError(f"Exception code {code:#x}, expected {expected[mode]:#x}")
    return code, len(data)


def main():
    # CTest's redirected Windows console may use a legacy code page even
    # though the tested executable/database paths intentionally contain Unicode.
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--handler", type=Path, required=True)
    parser.add_argument("--case", required=True)
    parser.add_argument("--work-root", type=Path, required=True)
    args = parser.parse_args()
    args.work_root.mkdir(parents=True, exist_ok=True)
    # A unique directory also prevents a previous successful dump from masking
    # a new failure. Leave the evidence in the build tree for failed CI jobs.
    run = Path(tempfile.mkdtemp(prefix=f"{args.case}-", dir=args.work_root))
    binary_dir = run / "bin with spaces ž"
    binary_dir.mkdir()
    cwd = run / "unrelated working directory"
    cwd.mkdir()
    probe = binary_dir / args.probe.name
    shutil.copy2(args.probe, probe)
    # The project is built with shared dependencies on some platforms. Windows
    # runtime DLL copies beside the probe must travel with its isolated copy.
    for dll in args.probe.parent.glob("*.dll"):
        shutil.copy2(dll, binary_dir / dll.name)
    if args.case != "missing-handler":
        shutil.copy2(args.handler, binary_dir / args.handler.name)
    environment = os.environ.copy()
    dump_root = binary_dir
    if args.case == "fallback-directory":
        # A file at the database directory path models an unwritable install
        # without depending on administrator privileges or filesystem ACLs.
        (binary_dir / "crashpad_db").write_text("blocked", encoding="utf-8")
        dump_root = run / "user state with spaces ć"
        dump_root.mkdir()
        environment["LOCALAPPDATA"] = str(dump_root.resolve())
        environment["XDG_STATE_HOME"] = str(dump_root.resolve())
    try:
        result = subprocess.run(
            [str(probe.resolve()), args.case], cwd=cwd, capture_output=True,
            timeout=75, creationflags=subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0,
            env=environment,
        )
    except subprocess.TimeoutExpired as error:
        raise AssertionError(f"Crash handling timed out; evidence: {run}") from error
    output = (result.stdout + result.stderr).decode("utf-8", errors="replace")
    (run / "child-output.txt").write_text(output, encoding="utf-8")
    status_path = binary_dir / "crashpad-status.txt"
    if not status_path.is_file():
        raise AssertionError(f"Missing persistent startup diagnostics: {status_path}\n{output}")
    status = status_path.read_text(encoding="utf-8")
    handler_path = (binary_dir / args.handler.name).resolve()
    if f"handler={handler_path}" not in status:
        raise AssertionError(f"Status does not identify the absolute handler path: {status}")
    if args.case == "missing-handler":
        if result.returncode != 0:
            raise AssertionError(f"Missing handler was not reported; exit={result.returncode}\n{output}")
        if list(run.rglob("*.dmp")):
            raise AssertionError("Unexpected dump when initialization failed")
        if "Crashpad disabled" not in status or "missing" not in status:
            raise AssertionError(f"Missing handler diagnostic was not persisted: {status}")
        print("PASS missing-handler: initialization reports failure")
        return
    database = dump_root / "crashpad_db"
    if args.case == "fallback-directory":
        database = dump_root / "OpenBK2" / "crashpad_db"
    if "Crashpad enabled" not in status or f"database={database.resolve()}" not in status:
        raise AssertionError(f"Status does not identify the enabled database: {status}")
    if (database / "crashpad-status.txt").read_text(encoding="utf-8") != status:
        raise AssertionError("Database and executable startup diagnostics differ")
    if args.case in ("caught-exception", "fallback-directory"):
        if result.returncode != 0:
            raise AssertionError(f"Nonfatal dump terminated the child: {result.returncode}\n{output}")
    elif result.returncode == 0 or result.returncode in (2, 3, 4, 5, 6):
        raise AssertionError(f"Fatal test did not crash as expected: {result.returncode}\n{output}")
    deadline = time.monotonic() + 5
    dumps = []
    while not dumps and time.monotonic() < deadline:
        dumps = list(dump_root.rglob("*.dmp"))
        if not dumps:
            time.sleep(0.05)
    if len(dumps) != 1:
        raise AssertionError(f"Expected one dump in {dump_root}, found {len(dumps)}; evidence: {run}\n{output}")
    if list(cwd.rglob("*.dmp")):
        raise AssertionError("Dump unexpectedly depended on working directory")
    code, size = validate_dump(dumps[0], args.case)
    print(f"PASS {args.case}: {size} bytes, exception={code:#x}, {dumps[0]}")


if __name__ == "__main__":
    main()
