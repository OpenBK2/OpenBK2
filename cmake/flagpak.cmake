# obk2_flags.pak: the SVG flags, their records and the mapping that recognises
# installed games' multiplayer sides, as a patch archive that works over any
# existing install of the game, the way obk2_fonts.pak does for fonts.
#
# Everything in it is at a path no retail or mod archive uses, so it adds and
# replaces nothing. In particular it does not carry this tree's
# Consts/Test/Test_MultiplayerConsts.xdb: every installation ships its own
# multiplayer consts, which differ, and replacing them would replace that
# installation's sides and tech levels. Flags/Mapping.xdb reaches those instead,
# recognising a side by the bytes of its icon and a tech level by its name
# (NGameX::GetSideFlagImage).
#
# Built with cmake -E tar like obk2_fonts.pak, with every entry given the same
# fixed modification time, which is all the VFS compares between archives.
set(OBK2_FLAG_PAK "${CMAKE_BINARY_DIR}/obk2_flags.pak")
set(OBK2_FLAG_PAK_TIME "2026-10-31 00:00:00")
set(OBK2_FLAG_DATA "${CMAKE_SOURCE_DIR}/Versions/Current/Data")
# Listed rather than globbed, as every source list in this tree is
set(OBK2_FLAG_PAK_FILES
    Flags/Images/Allies_Star.svg
    Flags/Images/Allies_Star.xdb
    Flags/Images/Czechoslovakia.svg
    Flags/Images/Czechoslovakia.xdb
    Flags/Images/Finland.svg
    Flags/Images/Finland.xdb
    Flags/Images/Finland_StateFlag.svg
    Flags/Images/Finland_StateFlag.xdb
    Flags/Images/France.svg
    Flags/Images/France.xdb
    Flags/Images/Germany_StandIn.svg
    Flags/Images/Germany_StandIn.xdb
    Flags/Images/Hungary.svg
    Flags/Images/Hungary.xdb
    Flags/Images/Italy_Kingdom.svg
    Flags/Images/Italy_Kingdom.xdb
    Flags/Images/Italy_Republic.svg
    Flags/Images/Italy_Republic.xdb
    Flags/Images/Japan_ArmyRisingSun.svg
    Flags/Images/Japan_ArmyRisingSun.xdb
    Flags/Images/Poland.svg
    Flags/Images/Poland.xdb
    Flags/Images/Poland_PeoplesRepublicArms.svg
    Flags/Images/Poland_PeoplesRepublicArms.xdb
    Flags/Images/Random_Dice.svg
    Flags/Images/Random_Dice.xdb
    Flags/Images/Romania.svg
    Flags/Images/Romania.xdb
    Flags/Images/Spain_Republic.svg
    Flags/Images/Spain_Republic.xdb
    Flags/Images/Spain_RepublicArms.svg
    Flags/Images/Spain_RepublicArms.xdb
    Flags/Images/Sweden.svg
    Flags/Images/Sweden.xdb
    Flags/Images/UK_Union.svg
    Flags/Images/UK_Union.xdb
    Flags/Images/USA_1912.svg
    Flags/Images/USA_1912.xdb
    Flags/Images/USSR_1924.svg
    Flags/Images/USSR_1924.xdb
    Flags/Images/USSR_1936.svg
    Flags/Images/USSR_1936.xdb
    Flags/Images/USSR_1955.svg
    Flags/Images/USSR_1955.xdb
    Flags/Authorities/Czechoslovakia.xdb
    Flags/Authorities/Finland.xdb
    Flags/Authorities/France.xdb
    Flags/Authorities/German_Reich.xdb
    Flags/Authorities/Hungary.xdb
    Flags/Authorities/Imperial_Japanese_Army.xdb
    Flags/Authorities/Italian_Republic.xdb
    Flags/Authorities/Kingdom_of_Italy.xdb
    Flags/Authorities/Poland.xdb
    Flags/Authorities/Romania.xdb
    Flags/Authorities/Spanish_Republic.xdb
    Flags/Authorities/Sweden.xdb
    Flags/Authorities/USA.xdb
    Flags/Authorities/USSR.xdb
    Flags/Authorities/United_Kingdom.xdb
    Flags/Territories/Czechoslovakia.xdb
    Flags/Territories/Finland.xdb
    Flags/Territories/France.xdb
    Flags/Territories/Germany.xdb
    Flags/Territories/Hungary.xdb
    Flags/Territories/Italy.xdb
    Flags/Territories/Japan.xdb
    Flags/Territories/Poland.xdb
    Flags/Territories/Romania.xdb
    Flags/Territories/Sweden.xdb
    Flags/Territories/USA.xdb
    Flags/Territories/USSR.xdb
    Flags/Territories/United_Kingdom.xdb
    Flags/Mapping.xdb
)
list(TRANSFORM OBK2_FLAG_PAK_FILES PREPEND "${OBK2_FLAG_DATA}/" OUTPUT_VARIABLE OBK2_FLAG_PAK_DEPENDS)

# A checkout without the game data cannot make the archive; the engine does not
# need it, and draws the installed icons where it is absent.
foreach(OBK2_FLAG_PAK_INPUT IN LISTS OBK2_FLAG_PAK_DEPENDS)
    if(NOT EXISTS "${OBK2_FLAG_PAK_INPUT}")
        message(WARNING "obk2_flags.pak is not built: ${OBK2_FLAG_PAK_INPUT} is missing from this checkout")
        return()
    endif()
endforeach()

# Paths inside the archive are relative to Data, which is what the VFS looks
# them up by, hence the working directory.
add_custom_command(
    OUTPUT "${OBK2_FLAG_PAK}"
    COMMAND ${CMAKE_COMMAND} -E rm -f "${OBK2_FLAG_PAK}"
    COMMAND ${CMAKE_COMMAND} -E tar cf "${OBK2_FLAG_PAK}" --format=zip "--mtime=${OBK2_FLAG_PAK_TIME}" -- ${OBK2_FLAG_PAK_FILES}
    WORKING_DIRECTORY "${OBK2_FLAG_DATA}"
    DEPENDS ${OBK2_FLAG_PAK_DEPENDS}
    COMMENT "Packing the SVG flags, their records and the flag mapping into obk2_flags.pak"
    VERBATIM)
add_custom_target(flags_pak ALL DEPENDS "${OBK2_FLAG_PAK}")
set_target_properties(flags_pak PROPERTIES FOLDER "data")

install(FILES "${OBK2_FLAG_PAK}" DESTINATION Data)
