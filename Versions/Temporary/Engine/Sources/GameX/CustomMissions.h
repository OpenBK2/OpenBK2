#pragma once

#define CUSTOM_MISSIONS_FOLDER "Custom/Missions/"
#define CUSTOM_CAMPAIGNS_FOLDER "Custom/Campaigns/"
#define MULTIPLAYER_MAPS_FOLDER "Maps/Multiplayer/"

namespace NCustom
{

void GetCustomMissions( std::vector<CDBID> *pDBIDs );
void GetCustomCampaigns( std::vector<CDBID> *pDBIDs );
void GetMultiplayerMaps( std::vector<CDBID> *pDBIDs );

}
