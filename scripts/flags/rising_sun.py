#!/usr/bin/env python3
# Draw a Rising Sun flag with its disc centred, as SVG: sixteen red rays on
# white, each a wedge of 11.25 degrees, one pointing straight toward the fly,
# and a red disc in the middle. Universal MOD-18 shows this for its Japan side
# (the Imperial Japanese Army's flag sets its disc toward the hoist instead), so
# it is drawn to that icon's proportions: 2:3, a disc 0.45 of the height across,
# and the icon's red, #be0026.
#
# usage: rising_sun.py Versions/Current/Data/Flags/Images/Japan_RisingSunCentred.svg
import math
import sys

W, H = 900, 600
CX, CY = W / 2, H / 2
DISC = 0.225 * H
RED = '#be0026'
# Far enough out that every ray reaches past the corners; the viewBox clips it.
REACH = 2 * math.hypot(W, H)

rays = []
for k in range(16):
    centre = math.radians(22.5 * k)
    half = math.radians(11.25 / 2)
    a = (CX + REACH * math.cos(centre - half), CY - REACH * math.sin(centre - half))
    b = (CX + REACH * math.cos(centre + half), CY - REACH * math.sin(centre + half))
    rays.append('M%.1f %.1f L%.1f %.1f L%.1f %.1f Z' % (CX, CY, a[0], a[1], b[0], b[1]))

svg = '''<?xml version="1.0" encoding="UTF-8"?>
<!--
  Rising Sun with a centred disc: sixteen red rays on white and a red disc in
  the middle, 2:3. Universal MOD-18 shows this for its Japan side; it is drawn
  to that icon's proportions and red, #be0026, rather than as the Imperial
  Japanese Army's flag, whose disc sits toward the hoist.

  Source: generated for OpenBK2 by scripts/flags/rising_sun.py; edit that and
  run it again rather than this file.
-->
<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d">
	<rect width="%d" height="%d" fill="#fff"/>
	<path fill="%s" d="%s"/>
	<circle cx="%g" cy="%g" r="%g" fill="%s"/>
</svg>
''' % (W, H, W, H, W, H, RED, ' '.join(rays), CX, CY, DISC, RED)

open(sys.argv[1], 'w', encoding='utf-8', newline='\n').write(svg)
