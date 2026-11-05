project(SceneB2)
set(SOURCES
    AIDebugInfo.cpp
    AIVisSubInfo.cpp
    AttachedObj.cpp
    BridgesTerraFormer.cpp
    CameraBasicMouseMutator.cpp
    CameraGameMouseMutator.cpp
    CameraInternal.cpp
    CameraMapIterator.cpp
    CameraMayaMouseMutator.cpp
    CameraScriptMutators.cpp
    CragsBuilder.cpp
    CursorInternal.cpp
    DBSceneConsts.cll
    DBSceneConsts.cpp
    DebrisBuilder.cpp
    DebugInfoManagerInternal.cpp
    DynamicDebrisBuilder.cpp
    DynamicDebrisManager.cpp
    ExplosionBuilder.cpp
    ExplosionsManager.cpp
    FootsCreator.cpp
    FullScreenFader.cpp
    GenTerrain.cpp
    HolesBuilder.cpp
    ImageTools.cpp
    LaserMark.cpp
    LineMesh.cpp
    LoadImage.cpp
    MechUnitJoggingMutator.cpp
    NoiseManager.cpp
    ObjectsTerraFormer.cpp
    PeaksCreator.cpp
    PerlinNoise.cpp
    PrecipicesManager.cpp
    PrecipicesRender.cpp
    RiversBuilder.cpp
    RoadsBuilder.cpp
    Scene.cpp
    SceneAttachedObjects.cpp
    SceneData.cpp
    SceneDebugInfo.cpp
    SceneEffects.cpp
    SceneHoldQueue.cpp
    SceneIcons.cpp
    SceneInternal.cpp
    ScenePick.cpp
    ScenePolylines.cpp
    SceneStuff.cpp
    SceneVisObjs.cpp
    ShootAreaMesh.cpp
    ShotTrace.cpp
    StatsSystemInternal.cpp
    stdafx.cpp
    SurfController.cpp
    TerraAIConnector.cpp
    TerraGeomBuilder.cpp
    TerraGeomModifier.cpp
    TerraGFXConnector.cpp
    TerraHeight.cpp
    Terrain.cpp
    TerrainGfxObserver.cpp
    TerrainGrid.cpp
    TerrainInfo.cpp
    TerrainManager.cpp
    TerraObjectsBuilder.cpp
    TerraSelection.cpp
    TerraSpotBuilder.cpp
    TerraTools.cpp
    TileMasks.cpp
    TrackBuilder.cpp
    TreeFallingMutator.cpp
    TreeWindMutator.cpp
    UnitIconRendering.cpp
    VisObjDesc.cpp
    VisObjFade.cpp
    VisObjIcons.cpp
    VisObjIconsManager.cpp
    VisObjSelection.cpp
    VSOMethods.cpp
    Water.cpp
    WaterController.cpp
    WaterModifier.cpp
    WeatherVisual.cpp
    WindController.cpp
    WingScaleMutator.cpp
)
bk2_add_library(SceneB2 SHARED ${SOURCES})
# Embed SSE4's corrections from one source for the CLI and editor installations.
set(ICON_OVERRIDES_FILE "${CMAKE_SOURCE_DIR}/scripts/iconrender/icon_overrides.txt")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${ICON_OVERRIDES_FILE}")
file(READ "${ICON_OVERRIDES_FILE}" UNIT_ICON_OVERRIDES)
configure_file(UnitIconOverrides.h.in UnitIconOverrides.h @ONLY)
target_include_directories(SceneB2 PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(SceneB2 PRIVATE 3DLib 3Dmotor B2_M1_Terrain DebugTools Image Input libdb MemoryLib Misc Sound Stats_B2_M1 System UI zlib granny fmt::fmt-header-only Boost::container)
if(NOT WIN32)
    # CursorInternal confines and warps the pointer through SDL
    target_link_libraries(SceneB2 PRIVATE SDL3::SDL3-shared)
endif()
add_subdirectory(test)
