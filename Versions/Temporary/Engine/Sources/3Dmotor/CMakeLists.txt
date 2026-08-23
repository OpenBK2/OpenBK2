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
    dbscene.cpp
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
include(GenerateExportHeader)
add_library(3Dmotor SHARED ${SOURCES})
generate_export_header(3Dmotor EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/3Dmotor_export.h)
target_link_libraries(3Dmotor PRIVATE 3DLib Image libdb MemoryLib Misc System zlib granny::granny dxsdk::d3d9 dxsdk::d3dx9 dxsdk::dxerr dxsdk::dxguid winapi Boost::sort fmt::fmt-header-only)
target_link_libraries(3Dmotor PUBLIC glm::glm-header-only)
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

add_subdirectory(test)
add_subdirectory(benchmark)
