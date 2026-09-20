#!/usr/bin/env python3
#
# Turn a Win32 .rc file's MENU, ACCELERATORS and TOOLBAR statements into C++
# tables.
#
# The last of the editor's resources to leave the PE section. These three are
# structure rather than bytes, so they become tables the editor walks directly
# instead of an HMENU it walks with GetMenuItemInfo, an HACCEL it copies out
# with CopyAcceleratorTable, and MFC's CToolBarData read raw out of a resource
# block.
#
# What is deliberately not carried over:
#
#   * MENUITEM flags. GRAYED and CHECKED are in the .rc but the editor has
#     never read them: MenuFromNative asks for MIIM_FTYPE, MIIM_ID, MIIM_STRING
#     and MIIM_SUBMENU and nothing else, and every item's enabled and checked
#     state comes from the editor's own UPDATE_UI handlers when a menu opens.
#   * Accelerators that are not VIRTKEY. AcceleratorsFromResource skips those
#     already.
#   * NOINVERT, which has meant nothing since Windows 3.
#
# Usage:
#   scripts/port/rc2menus.py <input.rc> <output.cpp> --defines <header>
#                            [--include <header> ...] [--skip <ID> ...]

import argparse
import os
import re
import sys

TOKEN = re.compile( r'''
      (?P<comment>//[^\n]*)
    | (?P<string>"(?:[^"]|"")*")
    | (?P<ident>[A-Za-z_][A-Za-z0-9_]*)
    | (?P<number>\d+)
    | (?P<punct>[,])
    | (?P<other>\S)
''', re.VERBOSE )


def tokenize( text ):
    out = []
    for m in TOKEN.finditer( text ):
        if m.lastgroup != "comment":
            out.append( ( m.lastgroup, m.group() ) )
    return out


class CReader:
    def __init__( self, tokens ):
        self.tokens = tokens
        self.i = 0

    def peek( self, ahead = 0 ):
        j = self.i + ahead
        return self.tokens[j] if j < len( self.tokens ) else ( "eof", "" )

    def next( self ):
        t = self.peek()
        self.i += 1
        return t

    def skip_commas( self ):
        while self.peek()[1] == ",":
            self.i += 1


def literal( token ):
    """An RC string literal's body, with "" turned into \\" for C++."""
    return token[1:-1].replace( '""', '\\"' )


def parse_menu( reader ):
    """The items between BEGIN and END, nested."""
    items = []
    while reader.peek()[1] != "BEGIN":
        reader.next()
    reader.next()
    while True:
        kind, value = reader.next()
        if value == "END" or kind == "eof":
            return items
        if value == "POPUP":
            text = literal( reader.next()[1] )
            # Flags between the text and BEGIN, if any.
            while reader.peek()[1] not in ( "BEGIN", ):
                reader.next()
            items.append( ( "popup", text, 0, parse_menu( reader ) ) )
        elif value == "MENUITEM":
            if reader.peek()[1] == "SEPARATOR":
                reader.next()
                items.append( ( "separator", None, 0, None ) )
                continue
            text = literal( reader.next()[1] )
            reader.skip_commas()
            command = reader.next()[1]
            # Trailing flags (GRAYED, CHECKED, ...), which are dropped; they
            # may sit on the next line after a comma.
            while reader.peek()[1] == "," :
                reader.next()
                if reader.peek()[0] == "ident" and reader.peek()[1] not in ( "MENUITEM", "POPUP", "END" ):
                    reader.next()
            items.append( ( "item", text, command, None ) )
        else:
            sys.exit( "unexpected {!r} in MENU".format( value ) )


MODIFIERS = { "SHIFT": "NResources::ACCEL_MOD_SHIFT",
              "CONTROL": "NResources::ACCEL_MOD_CONTROL",
              "ALT": "NResources::ACCEL_MOD_ALT" }


def parse_accelerators( reader ):
    entries = []
    while reader.peek()[1] != "BEGIN":
        reader.next()
    reader.next()
    while True:
        kind, value = reader.next()
        if value == "END" or kind == "eof":
            return entries
        key = value if kind != "string" else "'{}'".format( literal( value ) )
        reader.skip_commas()
        command = reader.next()[1]
        flags = []
        virtkey = False
        while reader.peek()[1] == ",":
            reader.next()
            flag = reader.next()[1]
            if flag == "VIRTKEY":
                virtkey = True
            elif flag in MODIFIERS:
                flags.append( MODIFIERS[flag] )
        if virtkey:
            entries.append( ( key, flags, command ) )


def parse_toolbar( reader ):
    commands = []
    while reader.peek()[1] != "BEGIN":
        reader.next()
    reader.next()
    while True:
        kind, value = reader.next()
        if value == "END" or kind == "eof":
            return commands
        if value == "SEPARATOR":
            commands.append( "0" )
        elif value == "BUTTON":
            commands.append( reader.next()[1] )
        else:
            sys.exit( "unexpected {!r} in TOOLBAR".format( value ) )


def parse( text, skip ):
    tokens = tokenize( text )
    reader = CReader( tokens )
    menus, accels, toolbars = [], [], []
    while reader.peek()[0] != "eof":
        kind, value = reader.peek()
        after = reader.peek( 1 )[1]
        if kind == "ident" and after in ( "MENU", "ACCELERATORS", "TOOLBAR" ):
            name = value
            reader.next(); reader.next()
            if after == "MENU":
                items = parse_menu( reader )
                if name not in skip:
                    menus.append( ( name, items ) )
            elif after == "ACCELERATORS":
                entries = parse_accelerators( reader )
                if name not in skip:
                    accels.append( ( name, entries ) )
            else:
                width = reader.next()[1]
                reader.skip_commas()
                height = reader.next()[1]
                commands = parse_toolbar( reader )
                if name not in skip:
                    toolbars.append( ( name, width, height, commands ) )
        else:
            reader.next()
    return menus, accels, toolbars


def emit_menu_items( lines, name, items, depth = 0 ):
    """Children first: an array has to exist before it is pointed at."""
    for index, ( kind, text, command, children ) in enumerate( items ):
        if kind == "popup":
            emit_menu_items( lines, "{}_{}".format( name, index ), children, depth + 1 )
    lines.append( "\tconst NResources::SMenuItem {}[] =".format( name ) )
    lines.append( "\t{" )
    for index, ( kind, text, command, children ) in enumerate( items ):
        child = "{}_{}".format( name, index )
        if kind == "separator":
            lines.append( "\t\t{ nullptr, 0, nullptr, 0 }," )
        elif kind == "popup":
            lines.append( '\t\t{{ "{}", 0, {}, sizeof( {} ) / sizeof( {}[0] ) }},'.format( text, child, child, child ) )
        else:
            lines.append( '\t\t{{ "{}", {}, nullptr, 0 }},'.format( text, command ) )
    lines.append( "\t};" )
    lines.append( "" )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument( "source" )
    parser.add_argument( "output" )
    parser.add_argument( "--defines", required = True )
    parser.add_argument( "--include", action = "append", default = [] )
    parser.add_argument( "--skip", action = "append", default = [] )
    args = parser.parse_args()

    with open( args.source, encoding = "utf-8" ) as handle:
        text = handle.read()
    menus, accels, toolbars = parse( text, set( args.skip ) )
    if not ( menus or accels or toolbars ):
        sys.exit( "nothing to convert in {}".format( args.source ) )

    lines = [
        "// Generated from {} by scripts/port/rc2menus.py.".format( os.path.basename( args.source ) ),
        "//",
        "// The editor's menus, accelerators and toolbar layouts, which were Win32",
        "// resources until the PE resource section stopped being available on every",
        "// platform the editor builds for. This file is the source now.",
        "//",
        "// The MENUITEM flags in the .rc are not here: the editor never read them,",
        "// and every item's enabled and checked state comes from its UPDATE_UI",
        "// handler when the menu opens.",
        "",
        '#include "stdafx.h"',
        "",
        '#include "{}"'.format( args.defines ),
    ]
    lines += [ '#include "{}"'.format( h ) for h in args.include ]
    if accels:
        # The accelerator entries name VK_ codes, which come from windows.h on
        # Windows and from here everywhere else.
        lines.append( '#include "port/vkcodes.h"' )
    lines += [ "", "namespace", "{" ]

    for name, items in menus:
        emit_menu_items( lines, "MENU_{}".format( name ), items )
    if menus:
        lines.append( "\tconst NResources::SMenuEntry MENUS[] =" )
        lines.append( "\t{" )
        for name, _ in menus:
            a = "MENU_{}".format( name )
            lines.append( "\t\t{{ {}, {}, sizeof( {} ) / sizeof( {}[0] ) }},".format( name, a, a, a ) )
        lines += [ "\t};", "", "\tconst NResources::CMenuTable MENU_TABLE( MENUS, sizeof( MENUS ) / sizeof( MENUS[0] ) );", "" ]

    for name, entries in accels:
        lines.append( "\tconst NResources::SAcceleratorEntry ACCEL_{}[] =".format( name ) )
        lines.append( "\t{" )
        for key, flags, command in entries:
            mods = " | ".join( flags ) if flags else "0"
            lines.append( "\t\t{{ {}, {}, {} }},".format( key, mods, command ) )
        lines += [ "\t};", "" ]
    if accels:
        lines.append( "\tconst NResources::SAcceleratorTableEntry ACCELERATORS[] =" )
        lines.append( "\t{" )
        for name, _ in accels:
            a = "ACCEL_{}".format( name )
            lines.append( "\t\t{{ {}, {}, sizeof( {} ) / sizeof( {}[0] ) }},".format( name, a, a, a ) )
        lines += [ "\t};", "", "\tconst NResources::CAcceleratorTable ACCEL_TABLE( ACCELERATORS, sizeof( ACCELERATORS ) / sizeof( ACCELERATORS[0] ) );", "" ]

    for name, width, height, commands in toolbars:
        lines.append( "\tconst unsigned TOOLBAR_{}[] =".format( name ) )
        lines.append( "\t{" )
        for start in range( 0, len( commands ), 4 ):
            lines.append( "\t\t" + " ".join( c + "," for c in commands[start:start + 4] ) )
        lines += [ "\t};", "" ]
    if toolbars:
        lines.append( "\tconst NResources::SToolBarEntry TOOLBARS[] =" )
        lines.append( "\t{" )
        for name, width, height, _ in toolbars:
            a = "TOOLBAR_{}".format( name )
            lines.append( "\t\t{{ {}, {}, {}, {}, sizeof( {} ) / sizeof( {}[0] ) }},".format( name, width, height, a, a, a ) )
        lines += [ "\t};", "", "\tconst NResources::CToolBarTable TOOLBAR_TABLE( TOOLBARS, sizeof( TOOLBARS ) / sizeof( TOOLBARS[0] ) );", "" ]

    lines += [ "}", "" ]
    with open( args.output, "w", encoding = "utf-8", newline = "\n" ) as handle:
        handle.write( "\n".join( lines ) )
    print( "{} -> {}: {} menus, {} accelerator tables, {} toolbars".format(
        args.source, args.output, len( menus ), len( accels ), len( toolbars ) ) )


if __name__ == "__main__":
    main()
