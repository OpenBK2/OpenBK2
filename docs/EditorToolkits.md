# The map editor's two toolkits

The map editor (`B2_MapEditor`) runs on wxWidgets by default: the main frame,
its panes and toolbars, the editors' palettes and dialogs, and the 3D
viewport's window. The MFC editor it was ported from is still built beside it,
so either can be started and the two compared, until MFC is taken out.

## Building

`BUILD_WX_EDITOR` is `ON` by default, and a normal editor build includes wx.
wxWidgets 3.3.3 is fetched and built as an external project the first time
(see `cmake/wxwidgets.cmake`), and its DLL is copied beside the editor and
installed with it.

```powershell
cmake --preset Windows-x64-Release
cmake --build --preset Windows-x64-Release --target B2_MapEditor
```

`-DBUILD_WX_EDITOR=OFF` builds the MFC editor alone, with no wx in it; the
switches below then do nothing.

## Choosing MFC for a session

Two environment variables, read at startup. Unset, empty, or anything but `0`
means wx.

| Variable | `0` gives | Otherwise |
| --- | --- | --- |
| `OBK2_WX_FRAME` | `CMainFrame`, the MFC main frame (Stingray panes and toolbars, MDI document window) | the wx frame (wxAUI panes and toolbars) |
| `OBK2_WX_DIALOGS` | the MFC views: palettes, panes' contents, dialogs, the viewport's window | the wx views |

They are independent, so all four combinations run:

| `OBK2_WX_FRAME` | `OBK2_WX_DIALOGS` | Session |
| --- | --- | --- |
| unset | unset | wx frame, wx views -- the default |
| `0` | `0` | MFC frame, MFC views -- the editor as it was |
| `0` | unset | MFC frame, wx views -- compares the views alone |
| unset | `0` | wx frame, MFC views -- compares the frame alone |

The frame also decides the message loop: a wx-frame session runs on wx's, an
MFC-frame session on MFC's.

From PowerShell:

```powershell
$env:OBK2_WX_FRAME = '0'; $env:OBK2_WX_DIALOGS = '0'
& C:\Games\bk2\bin\B2_MapEditor.exe
```

From `cmd`:

```bat
set OBK2_WX_FRAME=0
set OBK2_WX_DIALOGS=0
C:\Games\bk2\bin\B2_MapEditor.exe
```

A variable set this way stays set for everything started from that shell;
`Remove-Item Env:OBK2_WX_FRAME` (PowerShell) or `set OBK2_WX_FRAME=` (cmd) clears
it.

## Comparing side by side

One editor with the wx frame and one with the MFC frame can run at the same
time: each refuses a second editor of its own frame kind only. Start the MFC
one from a shell with both variables set to `0`, and the wx one from another
shell with neither.

What each keeps is separate where it has to be and shared where it is the
same data:

- **Layout.** The MFC frame keeps its bars under the registry section
  `...-Docking-v2`, the wx frame its panes under a section of its own ending in
  `-wx`. Rearranging one does not move the other. Reset GUI puts back the
  layout of the frame it is used in, and also the editors' own pane
  visibility, which is kept in the shared settings below.
- **Editor settings and the recent list** (`Editor\*.xml`, `UserData.xml`) are
  shared: which palette tab was open, the table choice, the maps opened. Both
  sessions read and write them, so the one closed last wins.
- **Game data** is shared, as it always was. Two editors saving the same map is
  as unsafe as it was with one editor opened twice.

## Known differences

The wx side is meant to behave as the MFC side does, and where it does not on
purpose it says so in the code. The ones worth knowing when comparing:

- Tools > Customize, Stingray's toolbar editor, has no wx counterpart.
- Pane captions and borders are wxAUI's, not Stingray's.
- The wx frame does not read the MFC frame's saved layout; it starts from its
  own default the first time.
- A notebook too narrow for its tab row scrolls it, as a native tab control
  does, where the Stingray tab window fitted it differently.
