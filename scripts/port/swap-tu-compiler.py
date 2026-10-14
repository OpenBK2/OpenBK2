#!/usr/bin/env python3
"""Recompile named translation units with the other compiler, in place.

A bisect tool for "this builds under both compilers but only behaves under one".
It takes the exact command the configured tree uses for a .cpp out of
compile_commands.json, swaps the compiler driver, drops the precompiled header
(which belongs to the original compiler and is rejected by the other one) and
adds the prelude back as a plain -include. The object lands where the build
system expects it, so a following `ninja <target>` relinks rather than
recompiles: ninja compares timestamps and the recorded command, and neither
changed.

    swap-tu-compiler.py <build-dir> <g++|clang++> <source-suffix> [<source-suffix> ...]

Each suffix is matched against the end of the entry's file path, so
"Image/GUnpackDXT.cpp" is enough. Prints what it did and leaves verification to
the caller; `readelf -p .comment` on the object says which compiler won.

Undo by touching the sources and building normally.
"""

import json
import os
import re
import subprocess
import sys


def rewrite(command, driver, prelude):
	# The PCH is the original compiler's and the other one errors out on it,
	# so drop every form the generators emit and re-add the prelude by hand.
	command = command.replace("-Winvalid-pch ", "")
	command = re.sub(r"-Xclang -include-pch -Xclang \S+ ", "", command)
	command = re.sub(r"-Xclang -include -Xclang \S+cmake_pch\.hxx ", "", command)
	command = re.sub(r"-include-pch \S+ ", "", command)
	command = re.sub(r"-include \S+cmake_pch\.hxx ", "", command)

	# ccache keys on the command line, so the swapped build gets its own entry
	# rather than colliding with the real one.
	for old in ("/usr/bin/clang++", "/usr/bin/g++", "clang++", "g++"):
		if old in command:
			command = command.replace(old, "/usr/bin/" + driver, 1)
			break

	if prelude:
		command = command.replace(
			"/usr/bin/" + driver, "/usr/bin/{} -include {}".format(driver, prelude), 1
		)

	# SWAP_TU_FLAGS is how the optimisation level or a sanitizer gets tried on
	# one TU. Appended last so it beats the flags the tree already passes, which
	# is what makes -O0 over a -O2 tree work.
	extra = os.environ.get("SWAP_TU_FLAGS", "").strip()
	if extra:
		command = re.sub(r"( -c )", " " + extra + r"\1", command, count=1)
	return command


def main():
	if len(sys.argv) < 4:
		print(__doc__)
		return 2

	build_dir, driver, suffixes = sys.argv[1], sys.argv[2], sys.argv[3:]
	entries = json.load(open(os.path.join(build_dir, "compile_commands.json")))

	rc = 0
	for suffix in suffixes:
		matches = [e for e in entries if e["file"].endswith(suffix)]
		if len(matches) != 1:
			print("{}: {} matches, need exactly one".format(suffix, len(matches)))
			rc = 1
			continue

		entry = matches[0]
		prelude = os.path.join(os.path.dirname(entry["file"]), "stdafx.h")
		if not os.path.exists(prelude):
			prelude = None

		command = rewrite(entry["command"], driver, prelude)
		result = subprocess.run(command, shell=True, cwd=entry["directory"])
		if result.returncode != 0:
			print("{}: {} failed".format(suffix, driver))
			rc = 1
		else:
			print("{}: rebuilt with {}".format(suffix, driver))

	return rc


if __name__ == "__main__":
	sys.exit(main())
