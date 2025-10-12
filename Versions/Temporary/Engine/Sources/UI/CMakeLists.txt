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

include(GenerateExportHeader)
add_library(UI SHARED ${SOURCES})
generate_export_header(UI EXPORT_FILE_NAME ${CMAKE_CURRENT_SOURCE_DIR}/UI_export.h)
target_link_libraries(UI PRIVATE 3DLib 3Dmotor Input libdb MemoryLib Misc Script System zlib winapi ffmpeg::avcodec ffmpeg::avformat ffmpeg::avutil ffmpeg::swscale ffmpeg::swresample SDL3_mixer::SDL3_mixer-shared SDL3::SDL3-shared)
install(TARGETS UI DESTINATION bin)
add_version_info(UI)
