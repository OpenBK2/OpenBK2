# The map editor's toolkit

The map editor (`B2_MapEditor`) is drawn with wxWidgets: the main frame, its
panes and toolbars, the editors' palettes and dialogs, and the 3D viewport's
window. The MFC editor it was ported from is gone, and so is MFC: none of the
editor's binaries links it, and wx owns the entry point. The resource scripts
still include MFC's `afxres.h` for its standard ids, so building needs Visual
Studio's MFC component installed; the running editor does not.

## Building

A build with `BUILD_EDITOR` (the default) builds wx with it. wxWidgets 3.3.3
is fetched and built as an external project the first time (see
`cmake/wxwidgets.cmake`), and its DLL is copied beside the editor and
installed with it. There is no build of the editor without wx.

```powershell
cmake --preset Windows-x64-Release
cmake --build --preset Windows-x64-Release --target B2_MapEditor
```

## Switches that are gone

`OBK2_WX_FRAME=0` used to start the MFC main frame, `OBK2_WX_DIALOGS=0` the MFC
palettes and dialogs, and `OBK2_WX_LOG` chose between the Scintilla and the wx
Log Window. None of them does anything now; `-DBUILD_WX_EDITOR=OFF` is gone
too.

The MFC frame kept its bars in the registry section `...-Docking-v2`, which
nothing reads any more. The wx frame's layout is in the section ending in
`-wx`.

## Known differences from the MFC editor

The wx editor is meant to behave as the MFC one did, and where it does not on
purpose it says so in the code. The ones worth knowing:

- Tools > Customize, Stingray's toolbar editor, has no wx counterpart.
- Pane captions and borders are wxAUI's, not Stingray's.
- The wx frame did not read the MFC frame's saved layout; it started from its
  own default the first time.
- A notebook too narrow for its tab row scrolls it, as a native tab control
  does, where the Stingray tab window fitted it differently.
- Cancel in the database link picker undoes the property edits made inside it,
  including those accepted in pickers opened from it. The MFC picker kept them.
