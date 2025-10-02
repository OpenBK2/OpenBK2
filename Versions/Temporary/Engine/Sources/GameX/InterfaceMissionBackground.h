
#pragma once

#include "InterfaceMissionBase.h"

class CInterfaceMissionBackground : public CInterfaceMissionBase
{
	OBJECT_NOCOPY_METHODS( CInterfaceMissionBackground );

	ZDATA_(CInterfaceMissionBase)
	CObj<IScenarioTracker> pScenarioTracker;
	ZONSERIALIZE
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CInterfaceMissionBase*)this); f.Add(2,&pScenarioTracker); OnSerialize( f ); return 0; }
private:
	void StartMissionMap( const NDb::SMapInfo *pMapInfo );
	void MsgUnloadMission( const SGameMessage &msg );
	void MsgTryExitWindows( const SGameMessage &msg );
protected:
	void OnSerialize( IBinSaver &saver );

public:
	bool StepLocal( bool bAppActive );

	~CInterfaceMissionBackground();
public:
	CInterfaceMissionBackground();
	
	bool Init();
};

class CICMissionBackground : public CInterfaceCommandBase<CInterfaceMissionBackground>
{
	OBJECT_BASIC_METHODS( CICMissionBackground );
	//
	void PreCreate( );
	void PostCreate( IInterface *pInterface );
public:
	void Configure( const char *pszConfig );
	
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, (CInterfaceCommandBase<CInterfaceMissionBackground>*)( this ) );

		return 0;
	}
};


