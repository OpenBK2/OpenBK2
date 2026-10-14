#pragma once
#include "GameX/DBScenario.h" // NDb::SChapter

namespace NDb
{
struct SCampaign;
}

namespace NDBWrap
{

// Название уровня опытности самого подкрепления (не командира)
const std::wstring &GetReinfXPLevelName( int nLevel );

float GetLeaderRankExp( const NDb::SCampaign *pCampaign, int nRank );

bool HasReinfUpgrade( const NDb::SChapter *pChapter, const NDb::SReinforcement *pCurReinf );

} //namespace NDBWrap
