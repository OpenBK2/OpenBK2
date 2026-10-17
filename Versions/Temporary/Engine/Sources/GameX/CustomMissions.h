#pragma once

#define CUSTOM_MISSIONS_FOLDER "Custom/Missions/"
#define CUSTOM_CAMPAIGNS_FOLDER "Custom/Campaigns/"
#define MULTIPLAYER_MAPS_FOLDER "Maps/Multiplayer/"

namespace NDb
{
struct SMapInfo;
}

namespace NCustom
{

void GetCustomMissions( std::vector<CDBID> *pDBIDs );
void GetCustomCampaigns( std::vector<CDBID> *pDBIDs );
void GetMultiplayerMaps( std::vector<CDBID> *pDBIDs );

//! Whether the map can be started at all: its record is there and so is the
//! terrain file (map.b2m) the record names by its own location.
//!
//! Not a question about custom missions only. A MapInfo record and the map file
//! beside it are separate things in the data tree, and a record whose file is
//! missing used to load, list and start, then crash as soon as anything walked
//! the terrain: the terrain manager is left holding an unloaded CTerraGen while
//! the map's crags and entrenchments dig into it. The released data trees are
//! full of such records - of the fourteen missions under Custom/Missions, not
//! one has a map.b2m beside its MapInfo.xdb - so this is the ordinary case for
//! anyone running the game off this repository.
bool IsMapPlayable( const NDb::SMapInfo *pMapInfo );
//! The same answer with the reason written to the log and the in-game console,
//! for the places that are about to start a mission and have to refuse.
bool VerifyMapPlayable( const NDb::SMapInfo *pMapInfo );

} // namespace NCustom
