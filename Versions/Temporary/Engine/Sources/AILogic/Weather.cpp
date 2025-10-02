#include "StdAfx.h"

#include "Weather.h"
#include "Diplomacy.h"
#include "UnitCreation.h"
#include "NewUpdater.h"
#include "UnitsIterators.h"
#include "Aviation.h"
#include "Stats_B2_M1/AIUpdates.h"
#include "Stats_B2_M1/DBMapInfo.h"

extern CEventUpdater updater; 
CWeather theWeather;

extern NTimer::STime curTime;
extern CDiplomacy theDipl;
extern CUnitCreation theUnitCreation;

void CWeather::Clear()
{
	bAutoChangeWeather = true;
	timeNextCheck = 0;
	eState = EWS_CLEAR;
	Init();
}

CWeather::CWeather()
	: eState( EWS_CLEAR ),
	bAutoChangeWeather( true ),
	timeNextCheck( 0 )
{
	Init();
}

void CWeather::Init()
{
	SwitchAutomatic( true );
}

const float CWeather::GetNextTimeRandom()
{
	if ( !theUnitCreation.GetMap() || theUnitCreation.GetMap()->weather.fWeatherPeriod == 0.0f )		// No bad weather
		return 0;

	return 1000 * ( theUnitCreation.GetMap()->weather.fWeatherPeriod + 
		NRandom::Random( 0.0f, theUnitCreation.GetMap()->weather.fWeatherPeriodRandom ));
}

void CWeather::SwitchAutomatic( const bool bSwitchAutomatic )
{
	if ( !theUnitCreation.GetMap() || theUnitCreation.GetMap()->weather.fWeatherPeriod == 0.0f )		// No bad weather
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
		On();
	else
		Off();
}

void CWeather::Off()
{
	if ( (eState == EWS_BAD) || (eState == EWS_FADE_IN) )
	{
		eState = EWS_FADE_OUT;
		CPtr<SWeatherChangedUpdate> pUpdate = new SWeatherChangedUpdate;
		pUpdate->bActive = false;
		pUpdate->nTimeTo = SConsts::TIME_TO_WEATHER_FADE_OFF * 1000;

		timeNextCheck = curTime + SConsts::TIME_TO_WEATHER_FADE_OFF * 1000;
		updater.AddUpdate( pUpdate, ACTION_NOTIFY_WEATHER_CHANGED, 0, 0 );
		CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW, "Bad weather stopped" );
		theUnitCreation.ApplyWeatherModifier( false );
		OutputDebugString( "Bad weather turning off\n" );
	}
}

void CWeather::On()
{
	if ( (eState == EWS_CLEAR) || (eState == EWS_FADE_OUT) )
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
			bReturnPlane = (*it)->GetPlayer() == theDipl.GetMyNumber() && (*it)->GetUnitAbilityDesc( NDb::ABILITY_MASTER_PILOT ) == 0;
		if ( bReturnPlane )
			updater.AddUpdate( new SPlaneReturnsUpdate, ACTION_NOTIFY_PLANE_RETURNS_DUE_WEATHER, 0, 0 );
		OutputDebugString( "Bad weather turning on\n" );
	}
}

bool CWeather::IsActive() const 
{ 
	return eState != EWS_CLEAR;
}

void CWeather::Segment()
{
	switch( eState )
	{
	case EWS_CLEAR:
		if ( bAutoChangeWeather && timeNextCheck < curTime )
			On();
		break;
		//
	case EWS_BAD:
		if ( bAutoChangeWeather && timeNextCheck < curTime + SConsts::TIME_TO_WEATHER_FADE_OFF * 1000 )
			Off();
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
	}
}


