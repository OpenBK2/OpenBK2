#!/usr/bin/env python3
#
# Turn a Win32 .rc file's STRINGTABLE blocks into a C++ table.
#
# Windows keeps resources in a section of the PE file and finds them with
# FindResource and LoadString. ELF has no equivalent, so the editor's string
# tables become generated C++ that NResources reads on every platform. This is
# the one-time conversion: its output is committed as the new source and the
# STRINGTABLE blocks are deleted from the .rc, so this script is a record of
# where the table came from rather than a build step.
#
# Only STRINGTABLE is handled. VERSIONINFO, the application icon and the
# manifest stay in the .rc, because the shell reads those out of the PE and
# nothing else can provide them.
#
# The ids are not resolved. The generated file includes the same
# ResourceDefines.h the .rc did and names the macros, so the headers stay the
# one place an id is given a number.
#
# Usage:
#   scripts/port/rc2strings.py <input.rc> <output.cpp> --defines <header> \
#                             [--include <header> ...]

import argparse
import os
import re
import sys

# An identifier, a quoted string, or a lone brace-ish keyword. RC strings use
# "" for an embedded quote, like Pascal; C++ wants \".
TOKEN = re.compile( r'''
      (?P<comment>//[^\n]*)
    | (?P<string>"(?:[^"]|"")*")
    | (?P<ident>[A-Za-z_][A-Za-z0-9_]*)
    | (?P<other>\S)
''', re.VERBOSE )


def tokenize( text ):
    for m in TOKEN.finditer( text ):
        if m.lastgroup != "comment":
            yield m.lastgroup, m.group()


def string_tables( text ):
    """Every (id, literal-body) pair in the file's STRINGTABLE blocks.

    The body is returned with RC's "" doubling turned into \\", and otherwise
    verbatim: RC and C++ spell \\n, \\r, \\t and \\\\ the same way, so the
    escapes carry over untouched."""
    pairs = []
    tokens = list( tokenize( text ) )
    i = 0
    while i < len( tokens ):
        kind, value = tokens[i]
        if kind == "ident" and value == "STRINGTABLE":
            # Skip the optional attributes (DISCARDABLE and friends) up to BEGIN.
            while i < len( tokens ) and tokens[i][1] != "BEGIN":
                i += 1
            i += 1
            while i < len( tokens ) and tokens[i][1] != "END":
                if tokens[i][0] != "ident":
                    sys.exit( "expected an id in STRINGTABLE, got {!r}".format( tokens[i][1] ) )
                name = tokens[i][1]
                i += 1
                # Adjacent literals concatenate, as in C.
                body = ""
                while i < len( tokens ) and tokens[i][0] == "string":
                    body += tokens[i][1][1:-1].replace( '""', '\\"' )
                    i += 1
                if body == "":
                    sys.exit( "no string for {}".format( name ) )
                pairs.append( ( name, body ) )
            i += 1
        else:
            i += 1
    return pairs


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument( "source" )
    parser.add_argument( "output" )
    parser.add_argument( "--defines", required = True,
                         help = "the header giving the ids their numbers" )
    parser.add_argument( "--include", action = "append", default = [],
                         help = "extra header to include; repeatable" )
    args = parser.parse_args()

    with open( args.source, encoding = "utf-8" ) as handle:
        text = handle.read()
    pairs = string_tables( text )
    if not pairs:
        sys.exit( "no STRINGTABLE blocks in {}".format( args.source ) )

    names = [name for name, _ in pairs]
    duplicates = sorted( set( n for n in names if names.count( n ) > 1 ) )
    if duplicates:
        sys.exit( "ids appear twice: {}".format( ", ".join( duplicates ) ) )

    width = max( len( name ) for name in names )
    lines = [
        "// Generated from {} by scripts/port/rc2strings.py.".format( os.path.basename( args.source ) ),
        "//",
        "// The editor's string table, which was a Win32 STRINGTABLE until the PE",
        "// resource section stopped being available on every platform the editor",
        "// builds for. This file is the source now: the .rc block it came from is",
        "// gone, and a new string is added here.",
        "//",
        "// The ids are the macros, not their numbers, so {} stays the one".format( os.path.basename( args.defines ) ),
        "// place that says what a number means.",
        "",
        '#include "stdafx.h"',
        "",
        '#include "{}"'.format( args.defines ),
    ]
    lines += [ '#include "{}"'.format( h ) for h in args.include ]
    lines += [
        "",
        "namespace",
        "{",
        "\tconst NResources::SStringEntry ENTRIES[] =",
        "\t{",
    ]
    for name, body in pairs:
        lines.append( '\t\t{{ {}, {}"{}" }},'.format( name, " " * ( width - len( name ) ), body ) )
    lines += [
        "\t};",
        "",
        "\t// Registered before main runs, like the resource section was mapped",
        "\t// before it. Order between modules is load order, which is the order",
        "\t// the executable's resources used to be searched in.",
        "\tconst NResources::CStringTable TABLE( ENTRIES, sizeof( ENTRIES ) / sizeof( ENTRIES[0] ) );",
        "}",
        "",
    ]
    with open( args.output, "w", encoding = "utf-8", newline = "\n" ) as handle:
        handle.write( "\n".join( lines ) )
    print( "{} -> {}: {} strings".format( args.source, args.output, len( pairs ) ) )


if __name__ == "__main__":
    main()
