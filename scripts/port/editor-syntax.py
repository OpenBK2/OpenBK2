#!/usr/bin/env python3
#
# Replay the editor's translation units through the Linux compiler with
# -fsyntax-only, and report what still does not parse.
#
# The editor's Linux port is a long sweep of Win32-isms, and a real build is no
# use while most of it fails: ninja stops, and a header that blocks thirty
# translation units looks exactly like thirty separate problems. This front end
# compiles every editor TU independently, never links, never writes an object,
# and aggregates the failures by the file and line that caused them. A handful
# of headers usually account for most of any count, and that ranking is the
# work list.
#
# Each TU is counted at its *first* error, so fixing a blocking header does not
# reduce the failure count by the number of TUs behind it -- it surfaces
# whatever those TUs hit next. Re-run after each step rather than predicting.
#
# Usage:
#   scripts/port/editor-syntax.py [--build-dir DIR] [--source-root DIR]
#                                 [--module NAME ...] [--jobs N]
#                                 [--errors] [--list-failures]
#
# Run it from inside WSL, from the clone that configured the build directory:
#
#   cd ~/src/OpenBK2 && scripts/port/editor-syntax.py
#
# By default it checks the *Windows* working tree (--source-root
# /mnt/c/projects/OpenBK2) rather than the WSL clone, reusing the WSL build
# directory only for its flags. That is deliberate: edits made on Windows can be
# checked from WSL without committing, pushing and merging them first. Pass
# --source-root with the clone's own path to check what is committed instead.

import argparse
import json
import os
import re
import shlex
import subprocess
import sys
from collections import Counter, defaultdict
from concurrent.futures import ThreadPoolExecutor

# The editor's own modules. Everything else in compile_commands.json is engine
# code that already builds on Linux.
EDITOR_MODULES = [
	"B2_MapEditor",
	"ED_B2",
	"ED_B2_M1",
	"ED_Common",
	"ED_RTS",
	"MapEditor",
	"MapEditorLib",
	"WxEditor",
]

SOURCES = "Versions/Temporary/Engine/Sources"

# clang and gcc both spell it "path:line:col: error: text".
ERROR_RE = re.compile( r"^(?P<file>[^:\n]+):(?P<line>\d+):(?P<col>\d+): (?:fatal )?error: (?P<text>.*)$" )


def module_of( path ):
	"""The editor module a source file belongs to, or None."""
	parts = path.replace( "\\", "/" ).split( "/" )
	if SOURCES.split( "/" )[-1] not in parts:
		return None
	index = len( parts ) - 1 - parts[::-1].index( "Sources" )
	if index + 1 < len( parts ) and parts[index + 1] in EDITOR_MODULES:
		return parts[index + 1]
	return None


def rewrite_root( text, clone, source_root ):
	"""Point source paths at another checkout, leaving the build tree alone.

	The build directory lives inside the clone, and its generated headers
	(GitRevision.h, the CMake ABI probes) only exist there, so it must keep its
	own path while the source tree is redirected."""
	if not clone or clone == source_root:
		return text
	pattern = re.escape( clone.rstrip( "/" ) ) + r"/(?!linux-build|build|out/)"
	return re.sub( pattern, source_root.rstrip( "/" ) + "/", text )


def syntax_command( entry, clone, source_root ):
	"""The compile command with the output, the dependency file and the
	precompiled header taken out, and -fsyntax-only put in.

	The PCH has to go: CMake's wrapper includes the *configuring* clone's
	stdafx.h by absolute path, and a .gch built for one source root is not valid
	for another. Including the module's own stdafx.h directly gives the TU the
	same prelude without the stale binary."""
	command = entry.get( "command" )
	if command is None:
		command = " ".join( shlex.quote( a ) for a in entry["arguments"] )
	command = rewrite_root( command, clone, source_root )
	source = rewrite_root( entry["file"], clone, source_root )

	args = shlex.split( command )
	out = []
	skip = 0
	pch_replacement = None
	for i, arg in enumerate( args ):
		if skip:
			skip -= 1
			continue
		# The ccache launcher: cmake -E env CCACHE_SLOPPINESS=... ccache <cc>.
		# Caching a syntax-only run under the real build's key would poison it.
		if arg.endswith( "cmake" ) and args[i + 1 : i + 3] == ["-E", "env"]:
			skip = 2
			continue
		if arg.startswith( "CCACHE_" ) or os.path.basename( arg ) == "ccache":
			continue
		if arg in ( "-o", "-MF", "-MT", "-MQ" ):
			skip = 1
			continue
		if arg in ( "-c", "-MD", "-MMD", "-Winvalid-pch" ):
			continue
		if arg == "-include" and i + 1 < len( args ) and args[i + 1].endswith( "cmake_pch.hxx" ):
			skip = 1
			pch_replacement = os.path.join( os.path.dirname( source ), "stdafx.h" )
			continue
		out.append( arg )

	if pch_replacement and os.path.exists( pch_replacement ):
		out[1:1] = ["-include", pch_replacement]
	out.append( "-fsyntax-only" )
	return out, source


def check( entry, clone, source_root, directory ):
	args, source = syntax_command( entry, clone, source_root )
	try:
		result = subprocess.run( args, cwd = entry.get( "directory", directory ),
														 capture_output = True, text = True, timeout = 600 )
	except ( OSError, subprocess.SubprocessError ) as error:
		return source, False, None, str( error )
	if result.returncode == 0:
		return source, True, None, ""
	# The first error is the one that matters: everything after it may be
	# fallout from the same missing declaration.
	for line in result.stderr.splitlines():
		match = ERROR_RE.match( line.strip() )
		if match:
			where = "{}:{}".format( os.path.relpath( match.group( "file" ), source_root ), match.group( "line" ) )
			return source, False, where, match.group( "text" )
	return source, False, "<no diagnostic>", result.stderr.strip().splitlines()[:1]


def main():
	parser = argparse.ArgumentParser( description = __doc__,
																		formatter_class = argparse.RawDescriptionHelpFormatter )
	parser.add_argument( "--build-dir", default = "linux-build",
											 help = "configured build tree holding compile_commands.json" )
	parser.add_argument( "--source-root", default = "/mnt/c/projects/OpenBK2",
											 help = "checkout to compile; defaults to the Windows working tree" )
	parser.add_argument( "--module", action = "append", metavar = "NAME",
											 help = "only this editor module; repeatable" )
	parser.add_argument( "--jobs", type = int, default = os.cpu_count(),
											 help = "parallel compilations" )
	parser.add_argument( "--errors", action = "store_true",
											 help = "print the error text beside each blocking line" )
	parser.add_argument( "--list-failures", action = "store_true",
											 help = "print every failing translation unit" )
	args = parser.parse_args()

	database = os.path.join( args.build_dir, "compile_commands.json" )
	if not os.path.exists( database ):
		sys.exit( "error: {} not found; configure the build tree first".format( database ) )
	with open( database ) as handle:
		entries = json.load( handle )

	# The clone that configured this build tree, taken from the database rather
	# than assumed: the build directory is inside it.
	clone = os.path.dirname( os.path.abspath( args.build_dir ) )

	wanted = set( args.module or EDITOR_MODULES )
	todo = [e for e in entries if ( module_of( e["file"] ) or "" ) in wanted]
	if not todo:
		sys.exit( "error: no translation units matched {}".format( sorted( wanted ) ) )

	print( "{} translation units in {}".format( len( todo ), ", ".join( sorted( wanted ) ) ) )
	print( "source root {}, flags from {}\n".format( args.source_root, args.build_dir ) )

	results = []
	with ThreadPoolExecutor( max_workers = args.jobs ) as pool:
		futures = [pool.submit( check, e, clone, args.source_root, args.build_dir ) for e in todo]
		for done, future in enumerate( futures, 1 ):
			results.append( future.result() )
			print( "\r  {}/{}".format( done, len( futures ) ), end = "", file = sys.stderr, flush = True )
	print( "\r" + " " * 24 + "\r", end = "", file = sys.stderr )

	per_module = defaultdict( lambda: [0, 0] )
	blocking = Counter()
	messages = {}
	failures = []
	for source, ok, where, text in results:
		module = module_of( source ) or "?"
		per_module[module][0 if ok else 1] += 1
		if not ok:
			blocking[where] += 1
			messages.setdefault( where, text )
			failures.append( ( where, os.path.relpath( source, args.source_root ) ) )

	total_ok = sum( v[0] for v in per_module.values() )
	print( "{} ok, {} fail\n".format( total_ok, len( results ) - total_ok ) )

	print( "By module:" )
	for module in sorted( per_module ):
		ok, fail = per_module[module]
		print( "  {:<14} {:>4} ok  {:>4} fail".format( module, ok, fail ) )

	if blocking:
		print( "\nBlocking lines, most translation units first:" )
		for where, count in blocking.most_common():
			print( "  {:>4}  {}".format( count, where ) )
			if args.errors:
				print( "        {}".format( messages.get( where ) ) )

	if args.list_failures:
		print( "\nFailing translation units:" )
		for where, source in sorted( failures ):
			print( "  {:<44} {}".format( where, source ) )

	return 0 if total_ok == len( results ) else 1


if __name__ == "__main__":
	sys.exit( main() )
