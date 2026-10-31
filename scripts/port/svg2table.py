#!/usr/bin/env python3
#
# Turn the editor's SVG icons into a C++ table of strings.
#
# The icons live as .svg files under MapEditorLib/res/icons/, one folder per
# origin (lucide/, composed/). The editor embeds them, as it embeds its
# bitmaps (see rc2binary.py), so there is nothing to install beside the
# executable and nothing to find at run time. Unlike rc2binary.py this one is
# not a one-time conversion: the .svg files stay the source, and the table is
# regenerated whenever one of them changes. Its output is committed.
#
# Each icon is keyed by its file name without .svg, which therefore has to be
# unique across the folders. The text is kept as SVG; only the comments, the
# class attribute and the whitespace between tags go, which nanosvg ignores.
#
# Usage:
#   scripts/port/svg2table.py <icons dir> <output.cpp>

import argparse
import os
import re
import sys

COMMENT = re.compile( r"<!--.*?-->", re.S )
CLASS = re.compile( r'\s+class="[^"]*"' )
BETWEEN_TAGS = re.compile( r">\s+<" )
SPACES = re.compile( r"\s+" )
# The raw string literal's delimiter, which the SVG text must not contain.
DELIMITER = "svg"


def compact( text ):
    text = COMMENT.sub( "", text )
    text = CLASS.sub( "", text )
    text = BETWEEN_TAGS.sub( "><", text.strip() )
    text = SPACES.sub( " ", text )
    # The space the class attribute leaves before the tag's end.
    return re.sub( r"\s+(/?>)", r"\1", text )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument( "source" )
    parser.add_argument( "output" )
    args = parser.parse_args()

    icons = {}
    for folder in sorted( os.listdir( args.source ) ):
        path = os.path.join( args.source, folder )
        if not os.path.isdir( path ):
            continue
        for name in sorted( os.listdir( path ) ):
            if not name.endswith( ".svg" ):
                continue
            key = name[: -len( ".svg" )]
            if key in icons:
                sys.exit( "svg2table: %s is in both %s and %s" % ( key, icons[key][0], folder ) )
            with open( os.path.join( path, name ), encoding="utf-8" ) as file:
                text = compact( file.read() )
            if ( ")" + DELIMITER + '"' ) in text:
                sys.exit( "svg2table: %s/%s contains the literal's delimiter" % ( folder, name ) )
            icons[key] = ( folder, text )

    lines = [
        "// Generated from MapEditorLib/res/icons by scripts/port/svg2table.py; do not",
        "// edit. Change the .svg files and run the script again, as",
        "// res/icons/README.md describes.",
        "//",
        "// The editor's SVG icons by name, for NResources::GetSvg. Each is the file's",
        "// text less its comments and the whitespace between tags.",
        "",
        '#include "stdafx.h"',
        "",
        '#include "Resources.h"',
        "",
        "namespace",
        "{",
        "\tconst NResources::SSvgEntry ENTRIES[] =",
        "\t{",
    ]
    for key in sorted( icons ):
        folder, text = icons[key]
        lines.append( "\t\t// %s/%s.svg" % ( folder, key ) )
        lines.append( '\t\t{ "%s", R"%s(%s)%s" },' % ( key, DELIMITER, text, DELIMITER ) )
    lines += [
        "\t};",
        "",
        "\tconst NResources::CSvgTable TABLE( ENTRIES, sizeof( ENTRIES ) / sizeof( ENTRIES[0] ) );",
        "}",
        "",
    ]
    with open( args.output, "w", encoding="utf-8", newline="\n" ) as file:
        file.write( "\n".join( lines ) )
    print( "svg2table: %d icons" % len( icons ) )


if __name__ == "__main__":
    main()
