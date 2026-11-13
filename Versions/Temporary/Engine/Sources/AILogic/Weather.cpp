#include "stdafx.h"

#include "Aviation.h"
#include "Diplomacy.h"
#include "NewUpdater.h"
#include "ScenarioTracker.h"
#include "Statistics.h"
#include "Stats_B2_M1/AIUpdates.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "UnitCreation.h"
#include "UnitsIterators.h"
#include "Weather.h"

extern CEventUpdater updater;
CWeather theWeather;

extern NTimer::STime curTime;
extern CDiplomacy theDipl;
extern CUnitCreation theUnitCreation;
extern CStatistics theStatistics;

void CWeather::Clear()
{
	bAutoChangeWeather = true;
	timeNextCheck = 0;
	eState = EWS_CLEAR;
	Init();
}

CWeather::CWeather()
	: eState( EWS_CLEAR )
	, bAutoChangeWeather( true )
	, timeNextCheck( 0 )
{
	Init();
}

void CWeather::Init()
{
	SwitchAutomatic( true );
}

const float CWeather::GetNextTimeRandom()
{
	if ( !theUnitCreation.GetMap() || theUnitCreation.GetMap()->weather.fWeatherPeriod == 0.0f ) // No bad weather
	{
		return 0;
	}

	RecordRandomCall();
	return 1000 * ( theUnitCreation.GetMap()->weather.fWeatherPeriod +
	                NRandom::Random( 0.0f, theUnitCreation.GetMap()->weather.fWeatherPeriodRandom ) );
}

void CWeather::SwitchAutomatic( const bool bSwitchAutomatic )
{
	if ( !theUnitCreation.GetMap() || theUnitCreation.GetMap()->weather.fWeatherPeriod == 0.0f ) // No bad weather
	{
		bAutoChangeWeather = false;
		return;
	}

	timeNextCheck = curTime + GetNextTimeRandom();
	bAutoChangeWeather = bSwitchAutomatic;
}

void CWeather::Switch( const bool bActive )
{
	if ( bActive )
	{
		On();
	}
	else
	{
		Off();
	}
}

void CWeather::Off()
{
	if ( ( eState == EWS_BAD ) || ( eState == EWS_FADE_IN ) )
	{
		eState = EWS_FADE_OUT;
		CPtr<SWeatherChangedUpdate> pUpdate = new SWeatherChangedUpdate;
		pUpdate->bActive = false;
		pUpdate->nTimeTo = SConsts::TIME_TO_WEATHER_FADE_OFF * 1000;

		timeNextCheck = curTime + SConsts::TIME_TO_WEATHER_FADE_OFF * 1000;
		updater.AddUpdate( pUpdate, ACTION_NOTIFY_WEATHER_CHANGED, 0, 0 );
		CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW, "Bad weather stopped" );
		theUnitCreation.ApplyWeatherModifier( false );
		DebugTrace( "Bad weather turning off" );
	}
}

void CWeather::On()
{
	if ( ( eState == EWS_CLEAR ) || ( eState == EWS_FADE_OUT ) )
	{
		eState = EWS_FADE_IN;
		CPtr<SWeatherChangedUpdate> pUpdate = new SWeatherChangedUpdate;
		pUpdate->bActive = true;
		pUpdate->nTimeTo = SConsts::TIME_TO_WEATHER_FADE_OFF * 1000;

		timeNextCheck = curTime + SConsts::TIME_TO_WEATHER_FADE_OFF * 1000;
		updater.AddUpdate( pUpdate, ACTION_NOTIFY_WEATHER_CHANGED, 0, 0 );
		CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW, "Bad weather started" );
		theUnitCreation.ApplyWeatherModifier( true );

		bool bReturnPlane = false;
		for ( CPlanesIter it; !it.IsFinished() && !bReturnPlane; it.Iterate() )
		{
			bReturnPlane = ( *it )->GetPlayer() == theDipl.GetMyNumber() &&
			               ( *it )->GetUnitAbilityDesc( NDb::ABILITY_MASTER_PILOT ) == 0;
		}
		if ( bReturnPlane )
		{
			updater.AddUpdate( new SPlaneReturnsUpdate, ACTION_NOTIFY_PLANE_RETURNS_DUE_WEATHER, 0, 0 );
		}
		DebugTrace( "Bad weather turning on" );
	}
}

bool CWeather::IsActive() const
{
	return eState != EWS_CLEAR;
}

bool CWeather::CanCallAviation( int nPlayer, NDb::EReinforcementType eType ) const
{
	if ( !IsActive() )
	{
		return true;
	}

	IAIScenarioTracker *pTracker = GetScenarioTracker();
	const NDb::SReinforcement *pReinforcement = pTracker ? pTracker->GetReinforcement( nPlayer, eType ) : nullptr;
	if ( !pReinforcement )
	{
		return false;
	}

	// Match newly created aircraft's ability level, including the owning player's leader cap.
	// Reading the local player here would make COOP decisions depend on the replay/client viewpoint.
	const int nAbilityLevel = theStatistics.GetAbilityLevel( nPlayer, eType );
	bool bHasAircraft = false;
	for ( const NDb::SReinforcementEntry &entry : pReinforcement->entries )
	{
		if ( !entry.pMechUnit || !entry.pMechUnit->IsAviation() )
		{
			continue;
		}
		bHasAircraft = true;
		// Helicopters do not use the fixed-wing patrol state's weather-forced departure.
		if ( entry.pMechUnit->etype == NDb::RPG_TYPE_AVIA_HELICOPTER )
		{
			continue;
		}
		const NDb::SUnitActions *pActions = entry.pMechUnit->pActions;
		bool bMasterPilot = false;
		if ( pActions )
		{
			for ( int i = 0; i < pActions->specialAbilities.size() && i <= nAbilityLevel; ++i )
			{
				const NDb::SUnitSpecialAblityDesc *pAbility = pActions->specialAbilities[i];
				if ( pAbility && pAbility->eName == NDb::ABILITY_MASTER_PILOT )
				{
					bMasterPilot = true;
					break;
				}
			}
		}
		// A mixed batch must not spend a call on planes that immediately leave.
		if ( !bMasterPilot )
		{
			return false;
		}
	}
	return bHasAircraft;
}

void CWeather::Segment()
{
	switch ( eState )
	{
		case EWS_CLEAR:
			if ( bAutoChangeWeather && timeNextCheck < curTime )
			{
				On();
			}
			break;
			//
		case EWS_BAD:
			if ( bAutoChangeWeather && timeNextCheck < curTime + SConsts::TIME_TO_WEATHER_FADE_OFF * 1000 )
			{
				Off();
			}
			break;
			//
		case EWS_FADE_IN:
			if ( timeNextCheck < curTime )
			{
				eState = EWS_BAD;
				timeNextCheck = curTime + GetNextTimeRandom();
			}
			break;
			//
		case EWS_FADE_OUT:
			if ( timeNextCheck < curTime )
			{
				eState = EWS_CLEAR;
				timeNextCheck = curTime + GetNextTimeRandom();
			}
			break;
		default:
			break;
	}
}
