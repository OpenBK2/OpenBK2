
#pragma once

#include "InterfaceScreenBase.h"

class CWorldClient;
struct ITransceiver;
struct IScenarioTracker;

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

	~CInterfaceMissionBase() override;

public:
	CInterfaceMissionBase( const std::string &szInterfaceType, const std::string &szBindSection );

	void OnGetFocus( bool bFocus ) override;
	bool StepLocal( bool bAppActive ) override;
	void Freeze( const bool bFreeze );
	void AfterLoad() override;

	int operator&( IBinSaver &saver ) override;
};
