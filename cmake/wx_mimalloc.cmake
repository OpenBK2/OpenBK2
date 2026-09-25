# Loaded through CMAKE_PROJECT_wxWidgets_INCLUDE in the separate wx build.
# Defer until wxmono exists, then link the very same override object and shared
# allocator used by the engine. No modifications to fetched wx sources needed.
function(obk2_wx_mimalloc)
    if(NOT TARGET wxmono)
        message(FATAL_ERROR "The scoped allocator requires the monolithic wxWidgets target")
    endif()
    target_sources(wxmono PRIVATE "${OBK2_MIMALLOC_OBJECT}")
    target_link_libraries(wxmono PRIVATE "${OBK2_MIMALLOC_LIBRARY}")
endfunction()
cmake_language(DEFER CALL obk2_wx_mimalloc)
