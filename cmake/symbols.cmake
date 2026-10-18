# bk2_install_symbols: install a target's linker PDB into the "symbols" install
# component.
#
# /Z7 (see the top-level CMakeLists.txt) only changes where the compiler puts
# debug info: in each .obj instead of a shared compiler PDB. The linker still
# gathers it into <target>.pdb, and the binary records only that file's path and
# GUID. Without the PDB a crash dump resolves to the nearest export, so the PDBs
# of a shipped build have to be kept next to it.
#
# The component is EXCLUDE_FROM_ALL so that a plain `cmake --install` (what a
# player's install and the release zip use) stays as it is. The symbols are
# installed on request:
#
#     cmake --install <build dir> --component symbols --prefix <dir>
#
# MSVC only: $<TARGET_PDB_FILE> is an error for linkers that do not write one.
function(bk2_install_symbols target)
    if(MSVC)
        install(FILES $<TARGET_PDB_FILE:${target}>
                DESTINATION bin
                COMPONENT symbols
                EXCLUDE_FROM_ALL
                OPTIONAL)
    endif()
endfunction()
