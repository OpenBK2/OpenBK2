#pragma once
#include "../Input/Bind.h"

namespace NInput
{

class CKeyAccumulator
{
protected:
	bool bActive;
	int64 nValue;
	int64 nPower;
	DWORD dwTime;

public:
	CKeyAccumulator(): bActive( false ), nValue( 0 ), nPower( 0 ), dwTime( 0 ) {}

	void Activate( int _nPower, DWORD _dwTime )
	{
		if ( bActive )
		{
			if ( nPower != _nPower )
			{
				Deactivate( dwTime );
				Activate( _nPower, _dwTime );
			}
		}
		else
		{
			nValue = 0;
			nPower = _nPower;
			dwTime = _dwTime;
			bActive = true;
		}
	}
	void Deactivate( DWORD _dwTime )
	{
		nPower = 0;
		nValue += int64( _dwTime - dwTime ) * nPower;
		bActive = false;
	}

	int64 Sample( DWORD _dwTime )
	{
		int nTemp = nValue;
		DWORD dwTimeDelta = _dwTime - dwTime;

		nValue = 0;
		dwTime = _dwTime;

		if ( bActive )
			return nTemp + int64( dwTimeDelta ) * nPower;
		else
			return nTemp;
	}
	void Reset() { bActive = false; }
};

// При значении меньше данного CDoubleAccumulator считает что LimAxis вернулась в нулевое положение
const int POWER_MIN_LIMIT = 10;

class CDoubleAccumulator
{
protected:
	bool bActive;
	int64 nValue;
	int64 nPower;
	DWORD dwTime;

	int64 GetDelta( DWORD _dwTime )
	{
		// не надо так делать, из-за этого в aiMap камера дергается
		if ( nPower > -POWER_MIN_LIMIT && nPower < POWER_MIN_LIMIT )
			return 0;

		return int64( _dwTime - dwTime ) * nPower;
	}

public:
	CDoubleAccumulator(): bActive( false ), nValue( 0 ), nPower( 0 ), dwTime( 0 ) {}

	void Add( int _nPower, DWORD _dwTime )
	{
		if ( bActive )
		{
			nValue += GetDelta( _dwTime );
			nPower += _nPower;
			dwTime = _dwTime;
		}
		else
		{
			nValue = 0;
			nPower = _nPower;
			dwTime = _dwTime;
			bActive = true;
		}
	}
	void Deactivate( DWORD _dwTime )
	{
		if ( !bActive )
			return;

		nValue += GetDelta( _dwTime );
		nPower = 0;
		bActive = false;
	}
	int64 Sample( DWORD _dwTime )
	{
		int nTemp = nValue;
		DWORD dwTimeDelta = _dwTime - dwTime;

		nTemp += GetDelta( _dwTime );
		nValue = 0;
		dwTime = _dwTime;

		return nTemp;
	}
	void Reset() { bActive = false; }
};

class CAxisAccumulator
{
protected:
	int64 nValue;
	DWORD dwActivationTime;

public:
	CAxisAccumulator(): nValue( 0 ), dwActivationTime( 0 ) {}
	void Add( int nPoint, DWORD dwTime )
	{
		dwActivationTime = dwTime;
		nValue += nPoint;
	}
	int64 Sample( DWORD dwTime )
	{
		int64 nTemp = nValue;
		nValue = 0;
		return nTemp;
	}
};

struct SAccumulator
{
	CKeyAccumulator sKeyAccumulator;
	CKeyAccumulator sPOVAccumulator;
	CAxisAccumulator sAxisAccumulator;
	CDoubleAccumulator sLimAxisAccumulator;

	int64 Sample( DWORD dwTime ) { return sKeyAccumulator.Sample( dwTime ) + sPOVAccumulator.Sample( dwTime ) + sLimAxisAccumulator.Sample( dwTime ) + sAxisAccumulator.Sample( dwTime ); }
	void Reset() { sKeyAccumulator.Reset(); sPOVAccumulator.Reset(); sLimAxisAccumulator.Reset(); }
};

enum EActionState
{
	STATE_DEFAULT,
	STATE_INITIALIZED,
	STATE_CONFIGPRESET
};
struct SActionInfo
{
	EActionState eState;
	bool bActive;
	float fCoeff;
	float fGranularity;
	string szName;
	EControlType eType;

	SActionInfo(): eState( STATE_DEFAULT ), bActive( false ), fCoeff( 1.0f ), fGranularity( 1.0f ), eType( CT_UNKNOWN ) {}
};

struct SMapping
{
	int nPower;
	bool bActive;
	bool bDisabled;
	string szSection;
	vector<int> actionsSet;
	vector<int> fullActionsSet;
	EMappingType mType;
	SAccumulator sAccumulator;
	list<vector<int> > blockingGroupsSet;

	SMapping(): nPower( 0 ), bActive( false ), bDisabled( true ), mType( MTYPE_UNKNOWN ) {}
};

struct SCommand
{
	bool bActive;
	float fDelta;
	float fSpeed;
	DWORD dwTime;
	float fCoeff;
	list<SMapping> mappingsList;

	SCommand(): bActive(false), fCoeff( 1.0f ), fDelta( 0.0f ), fSpeed( 0.0f ), dwTime( 0 ) {}
};

};

