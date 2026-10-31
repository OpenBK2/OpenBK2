# Editor icons

The editor's toolbar, tree, property-grid and movie-editor icons. They are SVG, drawn in the
text colour of the current theme, and rendered at whatever size and DPI the control asks for.

- `lucide/` holds icons from [Lucide](https://lucide.dev) 1.66.0 (`lucide-static`), copied
  unmodified. Lucide is under the ISC licence, and a few of its icons, inherited from Feather,
  are under MIT; both notices are in `lucide/LICENSE`, which has to travel with them.
- `composed/` holds icons made for this editor out of Lucide parts, in Lucide's style: 24 unit
  grid, 2 unit round stroke, `currentColor`. Each base icon is redrawn with its bottom-right
  corner left open and a badge drawn into the gap, as Lucide's own `*-corner` icons do. The
  gap cannot be cut with a `<mask>` or `<clipPath>`, because nanosvg, which wx renders SVG
  with, supports neither. Being derived from Lucide, these are under the same licence.

The terrain height palette (`ED_B2_M1/res/tmith_bitmap.bmp`), the cursors, the application icons
and the Run Game crest are still bitmaps, on purpose.

## Changing or adding an icon

1. Put the `.svg` in `lucide/` (copied from `lucide-static`'s `icons/`) or `composed/`. The file
   name without `.svg` is the icon's name, and has to be unique across both folders.
2. Regenerate the table the editor embeds them from:

   ```
   python3 scripts/port/svg2table.py Versions/Temporary/Engine/Sources/MapEditorLib/res/icons Versions/Temporary/Engine/Sources/MapEditorLib/ResourceSvg.cpp
   ```

3. Name it where it is used:
   - a toolbar button: the command's line in `MapEditor/ResourceIcons.cpp` or
     `ED_B2_M1/ResourceIcons.cpp`;
   - a property type: `PC_TYPE_ICONS` in `MapEditor/PropertyPaneViewWx.cpp`;
   - the object tree, its header, or the property grid's header: `ObjectBrowserViewWx.cpp` and
     `PropertyPaneViewWx.cpp`;
   - a movie editor button: `LoadBitmaps` in `ED_B2_M1/MoviesEditorViewWx.cpp`.

A toolbar command with no icon named keeps the glyph from its old bitmap strip.
