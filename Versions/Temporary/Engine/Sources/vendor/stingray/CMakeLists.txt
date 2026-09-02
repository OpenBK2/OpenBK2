project(stingray)

add_library(stingray STATIC
    logging.cpp
    logging.h
    olbar.cpp # SECShortcutBar
    swinfrm.cpp # SECFrameWnd
    swinmdi.cpp # SECMDIFrameWnd
    secwb.cpp # SECWorkbook
    progress.cpp # SECProgressCtrl
    sbarcore.cpp # SECControlBar
    trctlx.cpp # SECTreeCtrl
    swsmgrex.cpp # SECWorkspaceManagerEx
    tabwnd3.cpp # SEC3DTabWnd
    tabwndb.cpp # SECTabWndBase
    tbarcust.cpp # SECCustomToolBar
    tbarmgr.cpp # SECToolBarManager
    tbarsdlg.cpp # SECToolBarsPage
    tbarpage.cpp # SECToolBarCmdPage
    tmenufrm.cpp # SECMenuBar
    trcore.cpp # SEC_TREECLASS
)
target_include_directories(stingray PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include)
if(MSVC)
    target_compile_definitions(stingray PUBLIC __AFX__ _AFXDLL)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(stingray PUBLIC _AFX_FULL_DEBUG)
    endif()
    target_link_libraries(stingray PRIVATE winapi)
endif()
target_link_libraries(stingray PRIVATE spdlog::spdlog_header_only Boost::assert)
