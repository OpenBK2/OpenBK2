
#pragma once

#include "InterfaceScreenBase.h"

class CWorldClient;
interface ITransceiver;
interface IScenarioTracker;
namespace NDb
{
	struct SMapInfo;
}

class CInterfaceMissionBase : public CInterfaceScreenBase
{
private:
	void NewMap( const NDb::SMapInfo *_pMap, ITransceiver *_pTransceiver, IScenarioTracker *pScenarioTracker );
protected:
	CObj<CWorldClient> pWorld;
	
	CDBPtr<NDb::SMapInfo> pMap;
	bool bFrozen;
	CPtr<ITransceiver> pTransceiver;
protected:
	void StartNewMap( const NDb::SMapInfo *_pMap, ITransceiver *_pTransceiver, IScenarioTracker *pScenarioTracker );
	void Unload();

	~CInterfaceMissionBase();
public:	
	CInterfaceMissionBase( const string &szInterfaceType, const string &szBindSection );

	void OnGetFocus( bool bFocus );
	bool StepLocal( bool bAppActive );
	void Freeze( const bool bFreeze );
	void AfterLoad();

	int operator&( IBinSaver &saver );
};


