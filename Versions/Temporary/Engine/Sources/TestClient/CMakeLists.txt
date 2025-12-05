project(TestClient)
set(SOURCES
        StdAfx.cpp
        CommandsInterface.cpp
        console.cpp
        TestClientApp.cpp
        TestClient.cpp
        TestClientProcessor.cpp
        InternalPackets.cpp
        MultiTester.cpp
        Pinger.cpp
)
add_executable(TestClient WIN32 ${SOURCES})
target_link_libraries(TestClient PRIVATE Misc System MemoryLib Scintilla Net Server_Client_Common Client crashpad_client)
target_compile_definitions(TestClient PUBLIC __AFX__ _AFXDLL)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(TestClient PUBLIC _AFX_FULL_DEBUG)
endif()
install(TARGETS TestClient DESTINATION bin)
add_version_info(TestClient)
