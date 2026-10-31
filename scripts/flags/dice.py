#!/usr/bin/env python3
# Draw the multiplayer "random country" icon, two dice, as SVG.
#
# Each die is a cube, rotated, projected orthographically and lit by one
# directional light. A projected face is an affine image of the unit square, so
# each face is drawn in unit-square coordinates under one matrix() transform:
# pips are circles there and come out as exact ellipses, the face's rounded
# corners round correctly, and its shading gradient follows the face. lunasvg
# has no filters, so the soft shadow under each die is a radial gradient that
# fades to transparent instead of a blur.
#
# usage: dice.py Versions/Current/Data/Flags/Images/Random_Dice.svg
import math
import sys

W, H = 320, 220

# The pip layout of each face on the unit square.
PIPS = {
    1: [(0.5, 0.5)],
    2: [(0.27, 0.27), (0.73, 0.73)],
    3: [(0.27, 0.27), (0.5, 0.5), (0.73, 0.73)],
    4: [(0.27, 0.27), (0.73, 0.27), (0.27, 0.73), (0.73, 0.73)],
    5: [(0.27, 0.27), (0.73, 0.27), (0.5, 0.5), (0.27, 0.73), (0.73, 0.73)],
    6: [(0.27, 0.25), (0.27, 0.5), (0.27, 0.75), (0.73, 0.25), (0.73, 0.5), (0.73, 0.75)],
}


def rot_y(v, a):
    x, y, z = v
    return (x * math.cos(a) + z * math.sin(a), y, -x * math.sin(a) + z * math.cos(a))


def rot_x(v, a):
    x, y, z = v
    return (x, y * math.cos(a) - z * math.sin(a), y * math.sin(a) + z * math.cos(a))


def norm(v):
    l = math.sqrt(sum(c * c for c in v))
    return tuple(c / l for c in v)


# Light from the upper left and in front, as in the original icon.
LIGHT = norm((-0.55, 0.75, 0.6))


def die(cx, cy, size, yaw, pitch, faces, ident):
    """SVG for one die centred at (cx, cy), turned by yaw about the vertical and
    pitch toward the viewer. faces gives the pips on its top, front and right
    faces. y is up in 3D and down on screen."""
    def xf(v):
        return rot_x(rot_y(v, yaw), pitch)

    def proj(v):
        x, y, z = xf(v)
        return (cx + x * size, cy - y * size)

    out = []
    # Shadow: an ellipse on the ground below and away from the light.
    out.append('<ellipse cx="%.2f" cy="%.2f" rx="%.2f" ry="%.2f" fill="url(#shadow)"/>'
               % (cx + size * 0.22, cy + size * 0.62, size * 1.05, size * 0.42))
    # Every face of the unit cube, -0.5 to 0.5: its outward normal, a corner and
    # the two edge vectors from it, laid out so the pips read the right way up.
    # faces gives the pips on top, front and right; the opposite faces are the
    # 7-complements, as on a real die. Only the faces turned toward the viewer
    # are drawn, which for a convex body is all hidden-surface removal needs.
    top, front, right = faces
    face_defs = [
        ((0, 1, 0), (-0.5, 0.5, 0.5), (1, 0, 0), (0, 0, -1), top),
        ((0, -1, 0), (-0.5, -0.5, -0.5), (1, 0, 0), (0, 0, 1), 7 - top),
        ((0, 0, 1), (-0.5, 0.5, 0.5), (1, 0, 0), (0, -1, 0), front),
        ((0, 0, -1), (0.5, 0.5, -0.5), (-1, 0, 0), (0, -1, 0), 7 - front),
        ((1, 0, 0), (0.5, 0.5, 0.5), (0, 0, -1), (0, -1, 0), right),
        ((-1, 0, 0), (-0.5, 0.5, -0.5), (0, 0, 1), (0, -1, 0), 7 - right),
    ]
    for n, o, u, v, pips in face_defs:
        nn = xf(n)
        if nn[2] <= 1e-6:
            continue  # facing away
        p0 = proj(o)
        pu = proj(tuple(o[i] + u[i] for i in range(3)))
        pv = proj(tuple(o[i] + v[i] for i in range(3)))
        a, b = pu[0] - p0[0], pu[1] - p0[1]
        c, d = pv[0] - p0[0], pv[1] - p0[1]
        lit = max(0.0, sum(nn[i] * LIGHT[i] for i in range(3)))
        shade = 0.42 + 0.58 * lit
        hi = int(255 * min(1.0, shade + 0.06))
        lo = int(255 * shade * 0.9)
        gid = '%s_f%d' % (ident, pips)
        out.append('<linearGradient id="%s" x1="0" y1="0" x2="1" y2="1">'
                   '<stop offset="0" stop-color="rgb(%d,%d,%d)"/>'
                   '<stop offset="1" stop-color="rgb(%d,%d,%d)"/></linearGradient>'
                   % (gid, hi, hi, max(0, hi - 4), lo, lo, max(0, lo - 6)))
        out.append('<g transform="matrix(%.4f %.4f %.4f %.4f %.4f %.4f)">' % (a, b, c, d, p0[0], p0[1]))
        # The edge: a darker face under the face, which shows as a rim.
        rim = int(lo * 0.62)
        out.append('<rect x="0" y="0" width="1" height="1" rx="0.16" fill="rgb(%d,%d,%d)"/>' % (rim, rim, rim))
        out.append('<rect x="0.035" y="0.035" width="0.93" height="0.93" rx="0.14" fill="url(#%s)"/>' % gid)
        for (px, py) in PIPS[pips]:
            out.append('<circle cx="%.3f" cy="%.3f" r="0.095" fill="#1d1d1d"/>' % (px, py))
        out.append('</g>')
    return '\n\t'.join(out)


def main():
    body = [
        '<defs>',
        '<radialGradient id="ground" cx="0.42" cy="0.38" r="0.75">'
        '<stop offset="0" stop-color="#9a9a9a"/><stop offset="1" stop-color="#595959"/></radialGradient>',
        '<radialGradient id="shadow">'
        '<stop offset="0" stop-color="#000" stop-opacity="0.55"/>'
        '<stop offset="0.6" stop-color="#000" stop-opacity="0.25"/>'
        '<stop offset="1" stop-color="#000" stop-opacity="0"/></radialGradient>',
        '</defs>',
        '<rect width="%d" height="%d" fill="url(#ground)"/>' % (W, H),
        # The left die lower and tilted toward the viewer, the right one more upright
        # and higher, as in the original; given as top, front and right, any three
        # faces that meet at a corner of a real die.
        die(100, 112, 96, math.radians(-36), math.radians(30), (5, 1, 3), 'a'),
        die(240, 94, 84, math.radians(28), math.radians(24), (3, 6, 2), 'b'),
    ]
    head = '''<?xml version="1.0" encoding="UTF-8"?>
<!--
  Random country icon: two dice on a grey ground.

  Not a flag. The multiplayer game room shows this for "random country". It
  redraws the original game's 32x22 icon
  (UI/Game/Menu/MPGameRoom/Countries/FlagRandom_Texture.dds), two white dice
  with soft shadows, as a vector drawing that stays sharp at any size, in the
  icon's 16:11 frame.

  Source: generated for OpenBK2 by scripts/flags/dice.py, which says how; edit
  that and run it again rather than this file. 320 x 220.
-->
'''
    svg = head + '<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d">\n\t%s\n</svg>\n' % (
        W, H, W, H, '\n\t'.join(body))
    open(sys.argv[1], 'w', encoding='utf-8', newline='\n').write(svg)


main()
