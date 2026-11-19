#pragma once

#include "SceneB2/WeatherVisual.h"
#include "System/Time.h"

#include "AILogic_export.h"

namespace NDb
{
enum EReinforcementType : int;
struct SReinforcement;
} // namespace NDb

class CWeather
{
	ZDATA
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	NDb::EWeatherState eState = {};
	bool bAutoChangeWeather = {}; // from AI, not from script
	NTimer::STime timeNextCheck = {};

public:
	ZEND int operator&( IBinSaver &f )
	{
		f.Add( 2, &eState );
		f.Add( 3, &bAutoChangeWeather );
		f.Add( 4, &timeNextCheck );
		return 0;
	}

private:
	void Off();
	void On();
	const float GetNextTimeRandom();

public:
	CWeather();
	void Init();

	void Clear();

	AILOGIC_EXPORT bool IsActive() const;
	static AILOGIC_EXPORT bool RequiresClearWeather( int nPlayer, const NDb::SReinforcement &reinforcement );

	bool CanCallPlayerReinforcement( int nPlayer, const NDb::SReinforcement &reinforcement ) const
	{
		return !IsActive() || !RequiresClearWeather( nPlayer, reinforcement );
	}

	bool CanCallAviation( int nPlayer, NDb::EReinforcementType eType ) const;
	void Segment();

	// turn on/off weather manually
	void Switch( const bool bActive );

	// turn on/off automatic weather changes
	void SwitchAutomatic( const bool bSwitchAutomatic );
};

extern AILOGIC_EXPORT CWeather theWeather;
