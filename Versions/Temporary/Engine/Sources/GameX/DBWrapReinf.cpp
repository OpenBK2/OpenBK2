#include "StdAfx.h"
#include "InterfaceState.h"

namespace NDBWrap
{

const wstring& GetReinfXPLevelName( int nLevel )
{
	switch ( nLevel )
	{
		case 0:
			return InterfaceState()->GetTextEntry( "T_REINF_EXP_LEVEL_NAME_01" );

		case 1:
			return InterfaceState()->GetTextEntry( "T_REINF_EXP_LEVEL_NAME_02" );

		case 2:
			return InterfaceState()->GetTextEntry( "T_REINF_EXP_LEVEL_NAME_03" );

		case 3:
			return InterfaceState()->GetTextEntry( "T_REINF_EXP_LEVEL_NAME_04" );
	};

	static wstring wszEmpty;
	return wszEmpty;
}

float GetLeaderRankExp( const NDb::SCampaign *pCampaign, int nRank )
{
	if ( pCampaign && !pCampaign->leaderRanks.empty() )
	{
		int nIndex = Min( nRank, pCampaign->leaderRanks.size() - 1 );
		float fRequiredXP = pCampaign->leaderRanks[nIndex].nExpNeeded;
		return fRequiredXP;
	}
	return 0.0f;
}

bool HasReinfUpgrade( const NDb::SChapter *pChapter, const NDb::SReinforcement *pCurReinf )
{
	if ( pChapter && pCurReinf )
	{
		for ( int i = 0; i < pChapter->missionPath.size(); ++i )
		{
			const NDb::SMissionEnableInfo &missionInfo = pChapter->missionPath[i];
			for ( int j = 0; j < missionInfo.reward.size(); ++j )
			{
				const NDb::SChapterBonus *pBonus = missionInfo.reward[j];
				if ( !pBonus )
					continue;
				if ( pBonus->eBonusType == NDb::CBT_REINF_CHANGE && !pBonus->bApplyToEnemy )
				{
					if ( pBonus->eReinforcementType == pCurReinf->eType && pBonus->pReinforcementSet != pCurReinf )
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}

} //namespace NDBWrap

