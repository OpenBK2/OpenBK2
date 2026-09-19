project(ED_B2)
set(SOURCES
    ChapterEditor.cpp
    ChapterInterface.cpp
    ChapterState.cpp
    EditorAI.cpp
    EditorScene.cpp
    ED_B2Dll.cpp
    stdafx.cpp
    TerraAIObserver.cpp
)
add_library(ED_B2 SHARED ${SOURCES})
add_stdafx_pch(ED_B2)
add_export_header(ED_B2)
# No mfc: MapEditorLib's headers no longer name CString, which was all this
# module used MFC for. Without mfc, __AFX__ is undefined and stdafx.h takes
# its plain <windows.h> branch, as in ED_RTS.
target_link_libraries(ED_B2 PRIVATE 3DLib 3Dmotor Common_RTS_AI ED_B2_M1 GameX Image libdb MapEditorLib MemoryLib Misc SceneB2 System UISpecificB2 UI zlib fmt::fmt-header-only)
if(WIN32)
    target_link_libraries(ED_B2 PRIVATE winapi)
endif()
install(TARGETS ED_B2 DESTINATION bin)
add_version_info(ED_B2)
