"""Draw the editor's up-arrow cursor and write it as a Windows .cur.

Win32 has IDC_UPARROW as a stock cursor and wx has no equivalent: there is no
wxCURSOR_UP_ARROW, wxCURSOR_BASED_ARROW_UP is X11 only ("Not yet implemented
for Windows" in wx's own header), and nothing in wx's MSW stock table maps to
IDC_UPARROW. The one place the editor uses it -- MapObjectState, to say "this
is a valid link target" -- therefore needs the cursor as an asset.

This draws a plain one rather than taking somebody's. It is a placeholder and
is meant to be replaced by anyone who wants to draw a better one; the point is
that the shape is ours, with no licence to trace.

    python mkcursor.py ../../Versions/Temporary/Engine/Sources/MapEditorLib/res/uparrow.cur
    python mkcursor.py out.cur --carray IDC_EDITOR_UP_ARROW

The format is the ICO family: a 6-byte directory, one 16-byte entry carrying
the hotspot, then a BITMAPINFOHEADER whose biHeight is twice the real height
because a colour (XOR) bitmap and an AND mask are stacked, both bottom-up.
Monochrome, so each 32 pixel row is exactly 4 bytes and needs no padding; the
result is 326 bytes, the same size as the game's own move2grid.cur.

Pixel meaning, which is the part that is easy to get wrong:

    AND=0 XOR=0  opaque black
    AND=0 XOR=1  opaque white
    AND=1 XOR=0  transparent
    AND=1 XOR=1  inverts the screen
"""
import argparse
import struct
import sys

SIZE = 32
# Top centre: an arrow that points at something should point with its tip.
HOTSPOT = ( SIZE // 2, 0 )


def inside( x, y ):
    """The arrow's solid body: a triangular head over a straight shaft."""
    tip_x = SIZE // 2
    head_bottom = 13
    if y < head_bottom:
        # Widens by one pixel a row on each side, from the tip down.
        half = ( y * 9 ) // head_bottom
        return abs( x - tip_x ) <= half
    if y < 27:
        return abs( x - tip_x ) <= 3
    return False


def build():
    """(xor, and) as lists of rows of booleans, top-down."""
    body = [ [ inside( x, y ) for x in range( SIZE ) ] for y in range( SIZE ) ]

    def is_body( x, y ):
        return 0 <= x < SIZE and 0 <= y < SIZE and body[y][x]

    # One pixel of white around the body, so it stays visible on dark ground.
    outline = [ [ False ] * SIZE for _ in range( SIZE ) ]
    for y in range( SIZE ):
        for x in range( SIZE ):
            if body[y][x]:
                continue
            for dy in ( -1, 0, 1 ):
                for dx in ( -1, 0, 1 ):
                    if is_body( x + dx, y + dy ):
                        outline[y][x] = True

    xor = [ [ outline[y][x] for x in range( SIZE ) ] for y in range( SIZE ) ]
    mask = [ [ not ( body[y][x] or outline[y][x] ) for x in range( SIZE ) ]
             for y in range( SIZE ) ]
    return xor, mask


def rows_to_bytes( rows ):
    """Bottom-up, one bit per pixel, high bit leftmost."""
    out = bytearray()
    for y in reversed( range( SIZE ) ):
        for byte_index in range( SIZE // 8 ):
            value = 0
            for bit in range( 8 ):
                if rows[y][byte_index * 8 + bit]:
                    value |= 0x80 >> bit
            out.append( value )
    return bytes( out )


def cursor_bytes():
    xor, mask = build()
    header = struct.pack( "<IiiHHIIiiII",
                          40,            # biSize
                          SIZE,          # biWidth
                          SIZE * 2,      # biHeight: XOR and AND stacked
                          1,             # biPlanes
                          1,             # biBitCount: monochrome
                          0, 0, 0, 0, 0, 0 )
    palette = struct.pack( "<BBBB", 0, 0, 0, 0 ) + struct.pack( "<BBBB", 255, 255, 255, 0 )
    image = header + palette + rows_to_bytes( xor ) + rows_to_bytes( mask )
    entry = struct.pack( "<BBBBHHII", SIZE, SIZE, 0, 0,
                         HOTSPOT[0], HOTSPOT[1], len( image ), 6 + 16 )
    return struct.pack( "<HHH", 0, 2, 1 ) + entry + image


def c_array( name, data ):
    lines = [ "\tconst unsigned char %s[] =" % name, "\t{" ]
    for start in range( 0, len( data ), 12 ):
        chunk = data[start:start + 12]
        lines.append( "\t\t" + " ".join( "0x%02x," % b for b in chunk ) )
    lines.append( "\t};" )
    return "\n".join( lines )


def main():
    ap = argparse.ArgumentParser( description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter )
    ap.add_argument( "out", help="the .cur file to write" )
    ap.add_argument( "--carray", metavar="NAME",
                     help="also print the bytes as a C array of this name" )
    ap.add_argument( "--preview", action="store_true",
                     help="print the shape as text, to look at before writing" )
    a = ap.parse_args()

    if a.preview:
        xor, mask = build()
        for y in range( SIZE ):
            print( "".join( "." if mask[y][x] else ( "o" if xor[y][x] else "#" )
                            for x in range( SIZE ) ) )

    data = cursor_bytes()
    with open( a.out, "wb" ) as f:
        f.write( data )
    print( "%s: %d bytes, %dx%d, hotspot %s" % ( a.out, len( data ), SIZE, SIZE, HOTSPOT ) )
    if a.carray:
        print( c_array( a.carray, data ) )
    return 0


if __name__ == "__main__":
    sys.exit( main() )
