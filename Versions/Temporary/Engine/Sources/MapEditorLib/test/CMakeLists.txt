# gr2 is listed only so copy_runtime_dlls copies granny2*.dll: MapEditorLib
# links it PRIVATE, and $<TARGET_RUNTIME_DLLS> does not follow a shared
# library's private dependencies, so without it the test cannot load
# MapEditorLib.dll (0xC0000135). RuntimeFontScale_test does the same.
add_unit_test(TextResourceEncoding MapEditorLib System Misc gr2)
add_unit_test(ThumbnailAlpha MapEditorLib System Misc wx::wx)

# Both load wx, the first only through MapEditorLib, and copy_runtime_dlls does
# not see wx's DLL. See copy_wx_runtime in cmake/wxwidgets.cmake.
copy_wx_runtime(TextResourceEncoding_test)
copy_wx_runtime(ThumbnailAlpha_test)
