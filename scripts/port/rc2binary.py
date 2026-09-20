#!/usr/bin/env python3
#
# Turn a Win32 .rc file's BITMAP, ICON and CURSOR statements into a C++ byte
# table.
#
# Windows keeps resources in a section of the PE file. ELF has no equivalent,
# so the editor's bitmaps become arrays in initialised read-only data, which is
# where the PE's resource section put them anyway. This is the one-time
# conversion: its output is committed as the new source and the BITMAP
# statements are deleted from the .rc.
#
# The whole file is embedded, header and all, rather than the shape the PE
# resource section holds (a headerless DIB for RT_BITMAP, a hotspot and a DIB
# for RT_CURSOR, a directory plus images for RT_GROUP_ICON). wx reads .bmp,
# .ico and .cur files directly, so nothing has to reconstruct a header to hand
# it one.
#
# The ICON statements stay in the .rc as well as being embedded: the shell
# reads an executable's icon out of the PE and nothing else can give it one.
#
# The ids are not resolved: the generated file includes the same
# ResourceDefines.h the .rc did and names the macros.
#
# Usage:
#   scripts/port/rc2binary.py <input.rc> <output.cpp> --defines <header>
#                             [--include <header> ...] [--skip <ID> ...]

import argparse
import os
import re
import sys

STATEMENT = re.compile( r'^\s*([A-Za-z_]\w*)\s+(?:BITMAP|ICON|CURSOR)\s+(?:[A-Z]+\s+)*"([^"]+)"\s*$', re.M )


def resolve( root, spelling ):
    """The file a .rc path names, which is relative to the .rc and spelled with
    backslashes and whatever case the author felt like."""
    relative = spelling.replace( "\\\\", "/" ).replace( "\\", "/" )
    path = os.path.normpath( os.path.join( root, relative ) )
    if os.path.exists( path ):
        return path
    # Case-insensitive fallback: the .rc mixes MapEditor and mapeditor, which
    # only NTFS forgives.
    parts = path.replace( "\\", "/" ).split( "/" )
    walk = parts[0] + os.sep if parts[0] else os.sep
    for part in parts[1:]:
        if not os.path.isdir( walk ):
            return None
        matches = [e for e in os.listdir( walk ) if e.lower() == part.lower()]
        if not matches:
            return None
        walk = os.path.join( walk, matches[0] )
    return walk if os.path.exists( walk ) else None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument( "source" )
    parser.add_argument( "output" )
    parser.add_argument( "--defines", required = True )
    parser.add_argument( "--include", action = "append", default = [] )
    parser.add_argument( "--skip", action = "append", default = [],
                         help = "an id to leave out; repeatable" )
    args = parser.parse_args()

    root = os.path.dirname( os.path.abspath( args.source ) )
    with open( args.source, encoding = "utf-8" ) as handle:
        text = handle.read()

    entries = []
    for name, spelling in STATEMENT.findall( text ):
        if name in args.skip:
            print( "  skipping {}".format( name ) )
            continue
        path = resolve( root, spelling )
        if path is None:
            sys.exit( "cannot find {} for {}".format( spelling, name ) )
        with open( path, "rb" ) as handle:
            entries.append( ( name, os.path.basename( path ), handle.read() ) )
    if not entries:
        sys.exit( "no BITMAP, ICON or CURSOR statements in {}".format( args.source ) )

    lines = [
        "// Generated from {} by scripts/port/rc2binary.py.".format( os.path.basename( args.source ) ),
        "//",
        "// The editor's bitmaps, icons and cursors, which were Win32 resources until",
        "// the PE resource section stopped being available on every platform the",
        "// editor builds for. This file is the source now.",
        "//",
        "// Each array is a whole file rather than the shape the resource section",
        "// held, so wx's own readers take them as they stand.",
        "",
        '#include "stdafx.h"',
        "",
        '#include "{}"'.format( args.defines ),
    ]
    lines += [ '#include "{}"'.format( h ) for h in args.include ]
    lines += [ "", "namespace", "{" ]

    for name, basename, data in entries:
        lines.append( "\t// {}, {} bytes.".format( basename, len( data ) ) )
        lines.append( "\tconst unsigned char DATA_{}[] =".format( name ) )
        lines.append( "\t{" )
        for start in range( 0, len( data ), 16 ):
            chunk = data[start:start + 16]
            lines.append( "\t\t" + " ".join( "0x{:02X},".format( b ) for b in chunk ) )
        lines.append( "\t};" )
        lines.append( "" )

    width = max( len( name ) for name, _, _ in entries )
    lines += [ "\tconst NResources::SBinaryEntry ENTRIES[] =", "\t{" ]
    for name, _, _ in entries:
        pad = " " * ( width - len( name ) )
        lines.append( "\t\t{{ {},{} DATA_{},{} sizeof( DATA_{} ) }},".format( name, pad, name, pad, name ) )
    lines += [
        "\t};",
        "",
        "\tconst NResources::CBinaryTable TABLE( ENTRIES, sizeof( ENTRIES ) / sizeof( ENTRIES[0] ) );",
        "}",
        "",
    ]

    with open( args.output, "w", encoding = "utf-8", newline = "\n" ) as handle:
        handle.write( "\n".join( lines ) )
    total = sum( len( d ) for _, _, d in entries )
    print( "{} -> {}: {} resources, {} bytes".format( args.source, args.output, len( entries ), total ) )


if __name__ == "__main__":
    main()
