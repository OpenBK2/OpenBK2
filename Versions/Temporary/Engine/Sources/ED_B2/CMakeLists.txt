project(ED_B2)
set(SOURCES
    ChapterEditor.cpp
    ChapterInterface.cpp
    ChapterState.cpp
    EditorAI.cpp
    EditorScene.cpp
    ED_B2DLL.cpp
    stdafx.cpp
    TerraAIObserver.cpp
)
add_library(ED_B2 SHARED ${SOURCES})
add_export_header(ED_B2)
target_link_libraries(ED_B2 PRIVATE 3DLib 3Dmotor Common_RTS_AI ED_B2_M1 GameX Image libdb MapEditorLib MemoryLib Misc SceneB2 System UISpecificB2 UI winapi)
install(TARGETS ED_B2 DESTINATION bin)
add_version_info(ED_B2)
