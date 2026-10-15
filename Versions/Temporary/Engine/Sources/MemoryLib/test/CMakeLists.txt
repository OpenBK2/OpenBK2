# A real DLL boundary is essential here: a single-module new/delete test would
# not detect the startup crash or independent allocator instances per DLL.
bk2_add_library(MimallocModule SHARED NO_EXPORT_HEADER NO_VERSION_INFO NO_INSTALL EXCLUDE_FROM_ALL MimallocModule.cpp)
target_compile_features(MimallocModule PRIVATE cxx_std_17)

bk2_add_library(MimallocForeign SHARED NO_EXPORT_HEADER NO_VERSION_INFO NO_INSTALL EXCLUDE_FROM_ALL MimallocForeign.cpp)
# Models an OS/third-party DLL that must retain its CRT allocator.
set_target_properties(MimallocForeign PROPERTIES OBK2_USE_SYSTEM_ALLOCATOR TRUE)

add_unit_test(MimallocIsolation MimallocModule MimallocForeign mimalloc ole32)
