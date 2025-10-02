#pragma once

#include "SceneB2/WeatherVisual.h"

class CWeather
{
	ZDATA
		NDb::EWeatherState eState;
		bool bAutoChangeWeather;							// from AI, not from script
		NTimer::STime timeNextCheck;

public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&eState); f.Add(3,&bAutoChangeWeather); f.Add(4,&timeNextCheck); return 0; }
private:

	void Off();
	void On();
	const float GetNextTimeRandom();
	
public:
	CWeather();
	void Init();

	void Clear();
	
	bool IsActive() const;
	void Segment();
	
	// turn on/off weather manually
	void Switch( const bool bActive );
	
	// turn on/off automatic weather changes
	void SwitchAutomatic( const bool bSwitchAutomatic );
};


