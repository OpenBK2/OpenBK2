project(3Dmotor)
set(SOURCES
    2DScene.cpp
    2DSceneSW.cpp
    4dCalcs.cpp
    aiInterval.cpp
    aiMap.cpp
    aiObject.cpp
    aiObjectLoader.cpp
    aiRender.cpp
    aiTrace.cpp
    aiVisitor.cpp
    AnimAnnotation.cpp
    AnimatedChannel.cpp
    Cache.cpp
    DBScene.cpp
    DecalFader.cpp
    DepthOfField.cpp
    DiscretePos.cpp
    Fader.cpp
    FontFormat.cpp
    FrameTransition.cpp
    G2DView.cpp
    GAnimation.cpp
    GAnimFormat.cpp
    GAnimLight.cpp
    GAnimUtils.cpp
    GAutoDetect.cpp
    GBind.cpp
    GCombiner.cpp
    GDecal.cpp
    GDecalGeometry.cpp
    GFont.cpp
    D3DError.h
    Gfx.cpp
    GfxBenchmark.cpp
    GfxBuffers.cpp
    GfxEffects.cpp
    GfxInternal.cpp
    GfxRender.cpp
    GfxShaders.cpp
    GfxUtils.cpp
    GInit.cpp
    GLightmap.cpp
    GLightmapCalc.cpp
    GLightmapLoader.cpp
    GLightPerVertex.cpp
    GLightPerVertexAVX2.cpp
    GLightPerVertexDispatch.cpp
    GLightPerVertexRef.cpp
    GLightPerVertexSSE2.cpp
    GLMLightState.cpp
    GLocale.cpp
    GMaterial.cpp
    GMatShare.cpp
    GMesh.cpp
    GObjectInfo.cpp
    GParticleFilter.cpp
    GParticleFormat.cpp
    GParticleInfo.cpp
    GParticles.cpp
    GParticlesRain.cpp
    GPartParticles.cpp
    GParts.cpp
    GPointLightGlow.cpp
    GPolyline.cpp
    GPostEffects.cpp
    GPostProcessors.cpp
    GrannyMemoryMap.cpp
    GRects.cpp
    GRenderClouds.cpp
    GRenderCore.cpp
    GRenderExecute.cpp
    GRenderFactor.cpp
    GRenderLight.cpp
    GRenderPathFastest.cpp
    GRenderPathLightmap.cpp
    GRenderPathOverdraw.cpp
    GRenderPathPolycount.cpp
    GRenderPathTnl.cpp
    GRenderUtils.cpp
    GRTShare.cpp
    GSceneInternal.cpp
    GSceneParticles.cpp
    GSceneUtils.cpp
    GShaderFX.cpp
    GShadowMap.cpp
    GShadowVolume.cpp
    GSkeleton.cpp
    GSprite.cpp
    GSSEtransform.cpp
    GText.cpp
    GTexture.cpp
    GTransparent.cpp
    GView.cpp
    HeightFog.cpp
    Locale.cpp
    MixerChannel.cpp
    ObjectFader.cpp
    ProgrammableChannel.cpp
    RandomGen.cpp
    RenderNode.cpp
    ScreenShot.cpp
    SkyDome.cpp
    stdafx.cpp
    SuperCollider.cpp
    SWRectLayout.cpp
    SWTexture.cpp
    VolumeContainer.cpp
    WindDeformer.cpp
)
add_library(3Dmotor SHARED ${SOURCES})
add_export_header(3Dmotor)
add_stdafx_pch(3Dmotor)
target_link_libraries(3Dmotor PRIVATE
    3DLib
    Image
    libdb
    MemoryLib
    Misc
    System
    zlib
    granny::granny
    Boost::sort
    Boost::uuid
)
target_link_libraries(3Dmotor PUBLIC glm::glm-header-only fmt::fmt-header-only)
if(WIN32)
    target_link_libraries(3Dmotor PRIVATE
        dxsdk::d3d9
        dxsdk::d3dx9
        dxsdk::dxguid
        winapi
    )
else()
    # d3d9.h and the windows.h shim; there is no d3dx9 to link against
    target_link_libraries(3Dmotor PRIVATE dxvk::d3d9)
endif()
install(TARGETS 3Dmotor DESTINATION bin)
add_version_info(3Dmotor)

# GLightPerVertexAVX2.cpp is the only translation unit allowed to contain AVX2
# instructions, and it is reached only through avx2LightingKernels after
# GetLightingKernels() has confirmed CPUID and XGETBV agree. Applying /arch:AVX2
# any wider than this single file would put AVX2 code on paths that run before
# and without that check.
if(MSVC)
    set_source_files_properties(GLightPerVertexAVX2.cpp PROPERTIES COMPILE_OPTIONS "/arch:AVX2")
else()
    set_source_files_properties(GLightPerVertexAVX2.cpp PROPERTIES COMPILE_OPTIONS "-mavx2")
endif()

# GLightPerVertexAVX2.cpp is the only source in this target with a compile line
# of its own, and MSVC rejects a translation unit whose options disagree with
# the ones the precompiled header was built on (C2855). The file includes
# stdafx.h itself, so dropping the PCH costs it only the parse.
set_source_files_properties(GLightPerVertexAVX2.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

add_subdirectory(test)
add_subdirectory(benchmark)
