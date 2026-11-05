add_unit_test(LuaKeywords)

# Exercise the real modal dialog and settings file, with an isolated user-data root.
add_unit_test(IconRenderingOptions MapEditorLib libdb System Misc gr2 wx::wx
    SOURCES ../OptionsDialogWx.cpp ../CommandHandlerContainer.cpp)
target_compile_definitions(IconRenderingOptions_test PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
copy_wx_runtime(IconRenderingOptions_test)
if(WIN32)
    set_property(TARGET IconRenderingOptions_test APPEND PROPERTY LINK_DEPENDS $<TARGET_FILE:MapEditorLib>)
endif()
