# Cover mounted ammo reporting and weapons retained by occupants of destroyed buildings.
add_unit_test(BuildingWeapons AILogic Common_RTS_AI Stats_B2_M1 libdb System Misc zlib gr2)
target_compile_definitions(BuildingWeapons_test PRIVATE
    BUILDING_WEAPONS_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
set_tests_properties(BuildingWeapons_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(BuildingWeapons_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
    set_property(TARGET BuildingWeapons_test APPEND PROPERTY LINK_DEPENDS $<TARGET_FILE:AILogic>)
endif()

# Exercise data-backed AI settings, legacy fallback and save/load source selection.
add_unit_test(AIPropertyConsts AILogic Common_RTS_AI Stats_B2_M1 libdb System Misc zlib gr2)
target_compile_definitions(AIPropertyConsts_test PRIVATE
    AI_PROPERTIES_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml"
    AI_PROPERTIES_EXAMPLE_FILE="${CMAKE_SOURCE_DIR}/OBK2_Examples/AIPropertyConsts/Data/Consts/AIPropertyConsts.xdb")
set_tests_properties(AIPropertyConsts_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(AIPropertyConsts_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

# Check default compatibility and per-soldier formation modifiers in shelters.
add_unit_test(InfantryFormationModifiers AILogic Common_RTS_AI Stats_B2_M1 libdb System Misc zlib gr2)
target_compile_definitions(InfantryFormationModifiers_test PRIVATE
    FORMATION_MODIFIERS_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
set_tests_properties(InfantryFormationModifiers_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(InfantryFormationModifiers_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

# Exercise crack thresholds, legacy RNG/defaults, and the real direct/splash resolvers.
add_unit_test(HitProtection AILogic Common_RTS_AI Stats_B2_M1 libdb System Misc zlib gr2)
target_compile_definitions(HitProtection_test PRIVATE
    HIT_PROTECTION_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
set_tests_properties(HitProtection_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(HitProtection_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

# Exercise autonomous target selection through real tanks, weapons and saved attack states.
add_unit_test(AggressiveAttacker AILogic Common_RTS_AI Stats_B2_M1 libdb System Misc zlib gr2)
target_compile_definitions(AggressiveAttacker_test PRIVATE
    AGGRESSIVE_ATTACKER_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
set_tests_properties(AggressiveAttacker_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(AggressiveAttacker_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

# Exercise bounded distance queries against real party fog and script reveals.
add_unit_test(VisibleTerrainDistance AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
set_tests_properties(VisibleTerrainDistance_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(VisibleTerrainDistance_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
    set_property(TARGET VisibleTerrainDistance_test APPEND PROPERTY LINK_DEPENDS $<TARGET_FILE:AILogic>)
endif()

# Exercise combined fog sectors through real mechanical-unit stats and save/load.
add_unit_test(SpecialSights AILogic Common_RTS_AI Stats_B2_M1 libdb System Misc zlib gr2)
target_compile_definitions(SpecialSights_test PRIVATE
    SPECIAL_SIGHTS_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
set_tests_properties(SpecialSights_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(SpecialSights_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

# Exercise mechanical Binoculars aiming, additive visibility, interruption and save/load.
add_unit_test(MechBinoculars AILogic Common_RTS_AI Stats_B2_M1 libdb System Misc zlib gr2)
target_compile_definitions(MechBinoculars_test PRIVATE
    MECH_BINOCULARS_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
set_tests_properties(MechBinoculars_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(MechBinoculars_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

# Exercise the real deterministic trajectory and its save/load representation.
add_unit_test(BallisticOffset AILogic Stats_B2_M1 System Misc zlib gr2)
set_tests_properties(BallisticOffset_test PROPERTIES TIMEOUT 60)
add_unit_test(TowingPrecision AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
add_unit_test(TowingApproach AILogic Common_RTS_AI Stats_B2_M1 libdb System Misc zlib gr2)
target_compile_definitions(TowingApproach_test PRIVATE
    TOWING_APPROACH_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
set_tests_properties(TowingApproach_test PROPERTIES TIMEOUT 60)
add_unit_test(UnitTransfer System Misc zlib)
# Capture must refresh newly attached crew even when the gun's visibility is unchanged.
add_unit_test(ArtilleryCaptureVisibility AILogic Common_RTS_AI Stats_B2_M1 libdb System Misc zlib gr2)
target_compile_definitions(ArtilleryCaptureVisibility_test PRIVATE
    ARTILLERY_CAPTURE_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
set_tests_properties(ArtilleryCaptureVisibility_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(ArtilleryCaptureVisibility_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()
# Exercise the real unit selection refresh after recursive ownership changes.
add_unit_test(UnitTransferSelection AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
# Traverse the real registry and saved train/towing attachment graph.
add_unit_test(UnitTransferAttachments AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
add_unit_test(RepairTarget AILogic Common_RTS_AI Stats_B2_M1 libdb System Misc zlib gr2)
target_compile_definitions(RepairTarget_test PRIVATE
    REPAIR_TARGET_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
set_tests_properties(RepairTarget_test PROPERTIES TIMEOUT 60)
set_tests_properties(TowingPrecision_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(BallisticOffset_test TowingPrecision_test TowingApproach_test UnitTransferSelection_test UnitTransferAttachments_test RepairTarget_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

add_unit_test(BoredAcknowledgments AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
set_tests_properties(BoredAcknowledgments_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(BoredAcknowledgments_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

add_unit_test(ReinforcementSelection)

# Source replacement, zero multipliers, save/load and scoped ability cleanup.
add_unit_test(UnitStatsModifiers AILogic Common_RTS_AI Stats_B2_M1 libdb System Misc zlib gr2)
target_compile_definitions(UnitStatsModifiers_test PRIVATE
    UNIT_STATS_MODIFIERS_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
set_tests_properties(UnitStatsModifiers_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(UnitStatsModifiers_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

# Exercise real timed reinforcement income and explicit grants in both multiplayer modes.
add_unit_test(ReinforcementTimer AILogic Common_RTS_AI Stats_B2_M1 GameX libdb System Misc Main zlib gr2)
target_compile_definitions(ReinforcementTimer_test PRIVATE
    REINFORCEMENT_TIMER_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
set_tests_properties(ReinforcementTimer_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(ReinforcementTimer_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
    set_property(TARGET ReinforcementTimer_test APPEND PROPERTY
        LINK_DEPENDS $<TARGET_FILE:AILogic> $<TARGET_FILE:GameX>)
endif()

add_unit_test(RestStateMovement AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
set_tests_properties(RestStateMovement_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(RestStateMovement_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

add_unit_test(ResupplyTarget AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
set_tests_properties(ResupplyTarget_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(ResupplyTarget_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

add_unit_test(GroupMoverLifetime AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
set_tests_properties(GroupMoverLifetime_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(GroupMoverLifetime_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

add_unit_test(QueueUnitIDs AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
set_tests_properties(QueueUnitIDs_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(QueueUnitIDs_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

add_unit_test(BridgePaths AILogic Common_RTS_AI Stats_B2_M1 libdb System Misc zlib gr2)
target_compile_definitions(BridgePaths_test PRIVATE
    BRIDGE_PATHS_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
set_tests_properties(BridgePaths_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(BridgePaths_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

# Exercise bounded effect/construction sampling and wrapped turn angles with malformed data.
add_unit_test(EffectLoopProgress AILogic B2_M1_World Common_RTS_AI Stats_B2_M1 libdb System Misc zlib gr2)
set_tests_properties(EffectLoopProgress_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(EffectLoopProgress_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

# Exercise the real queue transition after casualties remove a squad's dig-in command.
add_unit_test(DugInSurvivor AILogic Common_RTS_AI Stats_B2_M1 libdb System Misc zlib gr2)
target_compile_definitions(DugInSurvivor_test PRIVATE
    DUG_IN_SURVIVOR_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
set_tests_properties(DugInSurvivor_test PROPERTIES TIMEOUT 30)
if(WIN32)
    set_tests_properties(DugInSurvivor_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

add_unit_test(ScriptGroupWait AILogic Common_RTS_AI Stats_B2_M1 Script System Misc zlib gr2)
set_tests_properties(ScriptGroupWait_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(ScriptGroupWait_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()

# COOP role state must be independent of the local client and survive replay/save loading.
add_unit_test(CooperativeDiplomacy AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
add_unit_test(Commander AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
add_unit_test(GeneralArtillery AILogic Common_RTS_AI Stats_B2_M1 System Misc zlib gr2)
set_tests_properties(GeneralArtillery_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(GeneralArtillery_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
    set_property(TARGET GeneralArtillery_test APPEND PROPERTY LINK_DEPENDS $<TARGET_FILE:AILogic>)
endif()
set_tests_properties(Commander_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(Commander_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
    set_property(TARGET Commander_test APPEND PROPERTY LINK_DEPENDS $<TARGET_FILE:AILogic>)
endif()
add_unit_test(CooperativeGeneral AILogic Common_RTS_AI Stats_B2_M1 GameX libdb System Misc Main zlib gr2)
target_compile_definitions(CooperativeGeneral_test PRIVATE
    COOPERATIVE_GENERAL_TYPES_FILE="${CMAKE_SOURCE_DIR}/Versions/Current/Data/types.xml")
set_tests_properties(CooperativeGeneral_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(CooperativeGeneral_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
    set_property(TARGET CooperativeGeneral_test APPEND PROPERTY
        LINK_DEPENDS $<TARGET_FILE:AILogic> $<TARGET_FILE:GameX>)
endif()
add_unit_test(ScriptCommands AILogic Common_RTS_AI Stats_B2_M1 Script System Misc GameX libdb zlib gr2)
set_tests_properties(CooperativeDiplomacy_test ScriptCommands_test PROPERTIES TIMEOUT 60)
if(WIN32)
    set_tests_properties(CooperativeDiplomacy_test ScriptCommands_test PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_append:${FFMPEG_BIN}")
endif()
