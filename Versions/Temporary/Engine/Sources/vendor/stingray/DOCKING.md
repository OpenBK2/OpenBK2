# Editor docking

This compatibility layer uses **MFC**, specifically CControlBar, CDockBar
and CDockContext. ATL is not responsible for arranging these panels.

- CMainFrame creates the database browser, properties, log and module panes.
  Each pane has a stable control ID, which is also its saved-layout identity.
- SECControlBar is the pane container. It draws the caption, close button and
  resize strips. GetInsideRect reserves space for them before the editor
  positions the pane's tree, tabs, log or other child window.
- SECPaneDockBar in docklayout.h arranges panes along the four frame edges.
  MFC calls its layout method during frame resizing and visibility changes.
  Panes in one row share its thickness and divide its length by weight.
  A "row" on the left/right edge is a vertical stack of panes.
- SECDockContext retains MFC's caption dragging, docking previews and floating
  windows. Its floating resize adapter stores both dimensions, independently
  from the sizes used while docked.

The native dock array still owns placement and ordering. Null entries delimit
rows; small integer entries remember where floating panes used to be docked.
They must not be treated as pointers when walking the array on x64.

## Mouse controls

Drag a pane's caption to another edge, another position in a dock, or away from
the frame to float it. Hold Ctrl during dragging to prevent docking.
Double-click a caption to toggle floating/docking.

Drag the raised strip facing the document area to resize the row's thickness.
Drag the strip between two panes to change their relative shares. Floating
panes resize using their window edges. Escape cancels an unfinished drag or
resize. The caption's X hides a pane; the editor's View commands show it again.

Toolbars use MFC's native gripper (the handle before their buttons). Drag it to
move or float the whole toolbar; double-click it to toggle docking. Toolbar
definitions name the neighboring toolbar, so defaults share a horizontal row
at the top instead of assigning one row per toolbar.

## Persistence

The editor saves layout on normal close and restores it after the toolbar
windows have been created. MFC stores edge, row/order, floating placement and
visibility. Additional per-pane profile sections store horizontal/vertical dock
sizes, floating width/height and row weights.

The profile suffix -Docking-v2 separates this layout from older port layouts;
the first run with this implementation starts with defaults. Stored dimensions
are validated before use. The existing bar-ID check rejects layouts referring
to controls the current build does not create.

Toolbar placement is included in the same native bar state. A
ToolbarLayoutVersion marker upgrades older vertically stacked toolbar defaults
once, without resetting pane positions or sizes. Normal close records the
marker, so later startups restore the user's toolbar placement.

Module panes still follow their editor's lifecycle. Their editor settings
remember visibility, and their post-create callbacks hide them until that
editor opens. Their docking positions and sizes remain in the shared layout.

## Reset GUI

View -> Reset GUI restores the original pane sizes, proportions and docking
order, docks toolbars in their default horizontal arrangement, and restores
toolbar buttons and default visibility. Existing database browsers are retained.
Module visibility returns to that editor's defaults; inactive editors stay
hidden until opened. The current document and editing settings are preserved.
The replacement layout is saved immediately, and later moves are still saved
on normal close.

Pane defaults are recorded at the first DockControlBarEx call for each window,
before a saved layout or a layout pass changes its dimensions. Reset reuses MFC
to redock floating windows, then rebuilds the pane rows. IDs plus window handles
let it skip database browsers deleted since startup.

SECControlBar invalidates its whole caption on WM_SIZE. MFC's normal border-only
resize invalidation leaves the previous close button's pixels behind when the
caption widens, creating the appearance of multiple X buttons.

## Manual checks

Try the database/properties divider and the outside edge of their shared column;
resize the bottom log; float, resize and redock each pane; cancel a resize with
Escape; hide/reopen a pane; and shrink/maximize the main frame. Close normally
and restart to check saved positions, sizes, proportions and visibility.
Also open a map and repeat with its module panes, then switch editors. Drag
toolbar handles to reorder bars in a row, move them to another edge and float
them; restart to check that these placements survive. Widen and narrow panes
repeatedly and check that each caption has just one X. Use View -> Reset GUI
after floating, moving, hiding and resizing panels/toolbars; repeat the reset,
switch editors, and restart to verify the defaults remain correct.

The changed C++ files have passed an x64 MSVC parse-only check using the Visual
Studio Developer Environment. Interactive behavior and a full editor build
remain to be checked.
