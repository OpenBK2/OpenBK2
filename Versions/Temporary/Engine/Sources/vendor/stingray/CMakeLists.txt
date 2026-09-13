project(stingray)

add_library(stingray STATIC
    logging.cpp
    logging.h
    olbar.cpp # SECShortcutBar
    swinfrm.cpp # SECFrameWnd
    swinmdi.cpp # SECMDIFrameWnd
    secwb.cpp # SECWorkbook
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
# MFC-as-DLL comes from cmake/mfc.cmake, and PUBLIC so that every module linking
# the shim still gets it exactly as before -- see that file for why it moved.
target_link_libraries(stingray PUBLIC mfc)
if(MSVC)
    target_link_libraries(stingray PRIVATE winapi)
endif()
target_link_libraries(stingray PRIVATE spdlog::spdlog_header_only Boost::assert)
