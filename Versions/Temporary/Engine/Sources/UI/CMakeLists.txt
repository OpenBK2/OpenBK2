project(UI)

set(SOURCES
    Background.cpp
    BackgroundMutableTexture.cpp
    BackgroundSimpleScallingTexture.cpp
    BackgroundSimpleTexture.cpp
    BackgroundTiledTexture.cpp
    Bitmap.cpp
    ButtonGroup.cpp
    CustomCheck.cpp
    DBUIConsts.cpp
    DBUserInterface.cpp
    EffectorButtonState.cpp
    EffectorConsoleBufferCommand.cpp
    EffectorDirectRunReaction.cpp
    EffectorMoveTo.cpp
    EffectorRunReaction.cpp
    EffectorSendUIMessage.cpp
    ForegroundTextString.cpp
    GBinkPlayer.cpp
    IconOutline.cpp
    InterfaceConsts.cpp
    ListControlSorters.cpp
    MessageReaction.cpp
    MessageReactions.cpp
    mlHandlers.cpp
    mlMain.cpp
    mlObjects.cpp
    mlReflow.cpp
    mlVisObjects.cpp
    stdafx.cpp
    Tools.cpp
    Tooltips.cpp
    UI.cpp
    UIFactory.cpp
    UIInternal.cpp
    UIML.cpp
    UIScreen.cpp
    UIStates.cpp
    Window.cpp
    Window1LvlTreeControl.cpp
    WindowComboBox.cpp
    WindowConsole.cpp
    WindowConsoleOutput.cpp
    WindowEditLine.cpp
    WindowListCtrl.cpp
    WindowMessageHandle.cpp
    WindowMSButton.cpp
    WindowPlayer.cpp
    WindowProgressBar.cpp
    WindowScrollableContainer.cpp
    WindowScrollableContainerBase.cpp
    WindowScrollBar.cpp
    WindowSimple.cpp
    WindowSlider.cpp
    WindowStatsSystem.cpp
    WindowTabControl.cpp
    WindowTextView.cpp
    WindowTooltip.cpp
)

add_library(UI SHARED ${SOURCES})
add_stdafx_pch(UI)
# These four explicitly specialize CastToObjectBaseImpl and friends, through
# BASIC_REGISTER_CLASS and the REGISTER_*_CLASS macros built on it. UI/Specific.h
# is part of the prelude and pulls in UIFactory.h and DBUserInterface.h, so the
# PCH has already instantiated those templates for the module's own types before
# the specialization is seen, and MSVC rejects that (C2375, C2908). UI is the only
# module where the prelude reaches its own registered types this way: the same
# macros compile fine under a PCH in the other 462 translation units that use them.
set_source_files_properties(
    DBUIConsts.cpp
    DBUserInterface.cpp
    UIFactory.cpp
    UIML.cpp
    PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
add_export_header(UI)
target_link_libraries(UI PRIVATE 3DLib 3Dmotor Input libdb MemoryLib Misc Script System zlib winapi ffmpeg::avcodec ffmpeg::avformat ffmpeg::avutil ffmpeg::swscale ffmpeg::swresample SDL3_mixer::SDL3_mixer-shared SDL3::SDL3-shared fmt::fmt-header-only)
install(TARGETS UI DESTINATION bin)
add_version_info(UI)
