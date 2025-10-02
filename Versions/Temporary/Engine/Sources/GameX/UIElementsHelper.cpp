#include "StdAfx.h"
#include "UIElementsHelper.h"
#include "..\UI\UI.h"
#include "DBScenario.h"
#include "ScenarioTracker.h"

namespace NUIElementsHelper
{

const int ROLLER_FRAMES_PER_DIGIT = 36;
const int ROLLER_FPS = 30;

const int STEP_WAIT_TIME = 50; // msec
const float EXP_PROGRESS_STEP_FRACTION = 0.05f;

void InitRoller( IPlayer *pRoller )
{
	if ( pRoller )
	{
		pRoller->Pause( true );
		pRoller->SetCurrentFrame( 36 );
	}
}

void PlayRollerAnim( vector<IPlayer*> &rollers, int _nStart, int _nEnd, float fRollerTime )
{
	vector<int> digStart;
	vector<int> digEnd;
	vector<int> frameStart;
	vector<int> frameEnd;
	vector<int> frameSkip;
	
	digStart.resize( rollers.size() );
	digEnd.resize( rollers.size() );
	frameStart.resize( rollers.size() );
	frameEnd.resize( rollers.size() );
	frameSkip.resize( rollers.size() );

	bool bForward = ( _nStart <= _nEnd );

	int nStart = _nStart;
	int nEnd = _nEnd;
	for ( int i = 0; i < rollers.size(); ++i )
	{
		digStart[i] = nStart % 10;
		digEnd[i] = nEnd % 10;

		nStart /= 10;
		nEnd /= 10;

		if ( !bForward )
		{
			swap( digStart[i], digEnd[i] );
		}

		if ( digStart[i] < digEnd[i] )
		{
			frameStart[i] = digStart[i] + 1;			// Sequence starts with 9-0, so add 1
			frameEnd[i] = digEnd[i] + 1;
		}
		else if ( digStart[i] > digEnd[i] )
		{
			frameStart[i] = digStart[i] + 1;			
			frameEnd[i] = digEnd[i] + 11;
		}
		else
		{
			frameStart[i] = digStart[i] + 1;
			frameEnd[i] = frameStart[i];
		}

		if ( !bForward )
		{
			frameStart[i] = 40 - frameStart[i];
			frameEnd[i] = 40 - frameEnd[i];
			swap( frameStart[i], frameEnd[i] );
		}

		frameStart[i] *= ROLLER_FRAMES_PER_DIGIT;
		frameEnd[i] *= ROLLER_FRAMES_PER_DIGIT;
		// Calculate frameskip
		int nTime = ( frameEnd[i] - frameStart[i] ) / ROLLER_FPS + 1;
		if ( nTime <= fRollerTime )
		{
			frameSkip[i] = 0;
		}
		else
		{
			frameSkip[i] = nTime / fRollerTime;
		}
	}

	// Run Anim
	for ( int i = 0; i < rollers.size(); ++i )
	{
		if ( rollers[i] )
			rollers[i]->PlayFragment( frameStart[i], frameEnd[i], frameSkip[i] );
	}
}

// SExpProgressCareer

void SExpProgressCareer::Init()
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	const int nLocalPlayer = pST->GetLocalPlayer();

	float fExpEarned = pST->GetStatistics( nLocalPlayer, IScenarioTracker::ESK_EXP_EARNED );
	float fExpCurrent = pST->GetStatistics( nLocalPlayer, IScenarioTracker::ESK_CAMPAIGN_EXP_CURRENT );
	float fTargetLevelExp = 0.0f;
	int nRank = pST->GetPlayerRankIndex();

	const NDb::SCampaign *pCampaign = pST->GetCurrentCampaign();
	if ( pCampaign && !pCampaign->rankExperiences.empty() )
	{
		if ( nRank < pCampaign->rankExperiences.size() )
			fTargetLevelExp = pCampaign->rankExperiences[nRank].fExperience;
		else
			fTargetLevelExp = pCampaign->rankExperiences.back().fExperience;
	}

	bFinal = false;
	fStart = fExpCurrent - fExpEarned + fTargetLevelExp;
	fCur = fStart;
	fTarget = fExpCurrent + fTargetLevelExp;
	fStep = Max( 1.0f, fExpEarned * EXP_PROGRESS_STEP_FRACTION );
	fTotal = 0.0f;

	if ( pCampaign && !pCampaign->rankExperiences.empty() )
	{
		fTotal = pCampaign->rankExperiences.back().fExperience;
	}
}

void SExpProgressCareer::Step( bool bWait )
{
	if ( bFinal )
		return;

	bool bNextStep = false;
	NTimer::STime time = Singleton<IGameTimer>()->GetAbsTime();
	if ( timePrev == 0 || bWait )
		timePrev = time;
	else if ( time >= timePrev + STEP_WAIT_TIME )
	{
		timePrev = time;
		bNextStep = true;
	}

	if ( bNextStep )
	{
		fCur += fStep;
		if ( fCur >= fTarget )
		{
			fCur = fTarget;
			bFinal = true;
		}
	}
	fNewProgress = 0.0f;
	fProgress = 0.0f;
	if ( fTotal >= 1.0f )
	{
		fProgress = Clamp( fStart / fTotal, 0.0f, 1.0f );
		fNewProgress = Clamp( fCur / fTotal, 0.0f, 1.0f );
	}
}

// SExpProgressRank

void SExpProgressRank::Init()
{
	IScenarioTracker *pST = Singleton<IScenarioTracker>();
	const int nLocalPlayer = pST->GetLocalPlayer();

	float fExpEarned = pST->GetStatistics( nLocalPlayer, IScenarioTracker::ESK_EXP_EARNED );
	float fExpCurrent = pST->GetStatistics( nLocalPlayer, IScenarioTracker::ESK_CAMPAIGN_EXP_CURRENT );
	fTargetLevelExp = 0.0f;
	fTargetNextLevelExp = 0.0f;
	int nRank = pST->GetPlayerRankIndex();

	const NDb::SCampaign *pCampaign = pST->GetCurrentCampaign();
	if ( pCampaign && !pCampaign->rankExperiences.empty() )
	{
		if ( nRank < pCampaign->rankExperiences.size() )
			fTargetLevelExp = pCampaign->rankExperiences[nRank].fExperience;
		else
			fTargetLevelExp = pCampaign->rankExperiences.back().fExperience;

		if ( nRank + 1 < pCampaign->rankExperiences.size() )
			fTargetNextLevelExp = pCampaign->rankExperiences[nRank + 1].fExperience;
		else
			fTargetNextLevelExp = pCampaign->rankExperiences.back().fExperience;
	}

	bFinal = false;
	fStart = fExpCurrent - fExpEarned + fTargetLevelExp;
	fCur = fStart;
	fTarget = fExpCurrent + fTargetLevelExp;
	fStep = Max( 1.0f, fExpEarned * EXP_PROGRESS_STEP_FRACTION );
	nNextLevel = 0;
	fCurLevelExp = 0.0f;

	if ( pCampaign && !pCampaign->rankExperiences.empty() )
	{
		for ( int i = 0; i < pCampaign->rankExperiences.size(); ++i )
		{
			levels.push_back( pCampaign->rankExperiences[i].fExperience );
		}
	}
}

void SExpProgressRank::Step( bool bWait )
{
	if ( bFinal )
		return;

	bool bNextStep = false;
	NTimer::STime time = Singleton<IGameTimer>()->GetAbsTime();
	if ( timePrev == 0 || bWait )
		timePrev = time;
	else if ( time >= timePrev + STEP_WAIT_TIME )
	{
		timePrev = time;
		bNextStep = true;
	}
	if ( bNextStep )
	{
		fCur += fStep;
		if ( fCur >= fTarget )
		{
			fCur = fTarget;
			bFinal = true;
		}
	}
	fNewProgress = 0.0f;
	fProgress = 0.0f;

	float fNextLevExp = fCurLevelExp;
	while ( nNextLevel < levels.size() )
	{
		fNextLevExp = levels[nNextLevel];
		if ( fCur >= fNextLevExp )
		{
			fCurLevelExp = fNextLevExp;
			nNextLevel++;
		}
		else
			break;
	}
	float fLevelDeltaExp = fNextLevExp - fCurLevelExp;

	if ( fLevelDeltaExp >= 1.0f )
	{
		float fExpAtLevel = Max( fStart, fCurLevelExp ) - fCurLevelExp;
		float fExpCurAtLevel = fCur - fCurLevelExp;
		fProgress = Clamp( fExpAtLevel / fLevelDeltaExp, 0.0f, 1.0f );
		fNewProgress = Clamp( fExpCurAtLevel / fLevelDeltaExp, 0.0f, 1.0f );
	}
}

} //namespace NUIElementsHelper


