# Rendering and selection beyond 20 x 20 map patches

The rendering and geometry collision trees assigned objects outside their
initial octree extent to cells at the edge of that extent. Those cells' culling
bounds did not enclose their contents, so camera rendering, shadow-caster
selection and unit picking could reject visible geometry.

A patch is 16 visual tiles at 2.75 render units per tile: 44 units. The scene
initializes each octree at (-128, -128, -128), with side length 1024, ending at
896 on each axis. A 20-patch map reaches 880; a 21-patch map reaches 924.
COcTreeNode::GetNode previously selected children by midpoint comparisons alone,
even when a center was outside the root. Its small edge cells still reported
their original bounds through GetBound.

## Rendering

CGScene::SelectNodes rejects whole subtrees using those bounds. Both the camera
render list and FormDepthList use this traversal. In addition, the selected
visible geometry supplies the bounds used to fit the shadow projection in
UpdateDepthTexture / MakeShadowMatrix. Incorrect selection can thus affect
terrain, objects, shadow casters and the shadow projection's input.

The fix opts the rendering tree into automatic root growth. When an insertion
falls outside the current extent, the root doubles until it contains the center.
Old children move beneath a node with their original extent; their addresses and
geometry owners remain valid. The root itself stays at the same address and
retains its objects and update lists. Large scene-wide update hints use a radius
of one million and intentionally remain at the root; that radius does not drive
growth. Scene-wide updates run before the root's culling test.

## Unit selection

SceneB2/ScenePick.cpp uses IAIMap::Trace for click selection and reads a software
selection grid for box selection. SceneB2/SceneInternal.cpp fills that 64 x 64
grid through IAIMap::TraceGrid. Both paths call CAIMap::SelectHulls in aiMap.cpp,
which rejects collision-tree nodes by their bounds before considering hulls.
The collision tree still had the original fixed extent after the rendering fix,
so units could render correctly but remain impossible to select.

The collision tree now also opts into root growth. Its nodes cache geometry
change notifications for trackers until Sync. Merely enabling growth would
insert parent nodes with an empty notification mask above pending events;
CallCachedInforms would then skip those subtrees. The InitGrownChild hook copies
the root's pending notification bounds and mask into each new intermediate
parent. Trackers and hulls keep their owners, and notifications remain deferred
until the usual Sync rather than firing during insertion.

No serialized fields or object registration IDs were added. These changes
address spatial culling and selection; they do not establish larger simulation
or multiplayer map limits.

## Verification

OcTree_test exercises the actual octree template and CTransformStack culling
code. Before the rendering fix, three of its four tests failed, including camera
and shadow selection at (1400, 1400), near the corner of a 32-patch map. Coverage
includes 20, 21, 32, 64 and 128-patch extents, rectangular extents, growth in both
directions on all axes, existing node and parent references, tree pruning,
root-owned data and large update hints. Objects outside camera and shadow
volumes remain culled. The user has also confirmed that rendering now works.

LargeMapSelection_test exercises the actual IAIMap::Trace and TraceGrid APIs
with in-memory cube hulls. It checks click hits at 20, 21, 32, 64 and 128-patch
extents, selection masks, the contents of a box within the selection grid,
offscreen exclusion, relocation/removal of hulls across the old boundary, and
pending tracker notifications through repeated growth in both directions.
Three tests failed with the original fixed collision tree. Enabling growth
alone fixed selection but failed the pending-notification regression.

With the complete fix, all eight tests pass in x64 Debug and Release, and
3Dmotor builds successfully in both configurations using Visual Studio's bundled
CMake and Developer Environment. These are coordinate/culling and selection API
regressions, not full-map rendering, animated-unit or performance tests.

Build the engine and tests with Visual Studio's bundled CMake inside the VS
Developer Environment, or with the corresponding Visual Studio targets:

    cmake --build out/build/Windows-x64-Release --target 3Dmotor OcTree_test LargeMapSelection_test
    ctest --test-dir out/build/Windows-x64-Release -R "^(OcTree|LargeMapSelection)_test$" --output-on-failure

For in-game verification, reopen a 32 x 32 or larger map with the updated engine
to build fresh spatial trees. Pan across x=896 and y=896 and around every edge
and corner. Check terrain, buildings, trees, moving units and their shadows
while rotating and zooming the camera. Click individual units and drag selection
boxes on both sides of the old boundary, including units moving across it.
Repeat with a rectangular map and compare against a 20 x 20 map. The selection
fix has not yet been verified through the running game's UI in this session.
