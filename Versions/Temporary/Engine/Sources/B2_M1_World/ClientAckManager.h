
#pragma once


#include "B2_M1_World_export.h"

namespace NDb
{
	enum EUnitAckType;
	struct SComplexSoundDesc;
	struct SClientGameConsts;
};

interface IClientAckManager : public CObjectBase
{
	// type ID
	enum { tidTypeID = 0x110AE400 };
	virtual void AddDeathAcknowledgement( const CVec3 &vPos, const NDb::SComplexSoundDesc *pSound, const unsigned int nTimeSiceStart ) = 0;
	virtual void AddAcknowledgement( interface IMOUnit *pUnit, const NDb::EUnitAckType eType, const NDb::SComplexSoundDesc *pSound, const int nSet, const unsigned int nTimeSiceStart ) = 0;
	virtual void UnitDead( struct IMOUnit *pUnit, interface ISoundScene *pSoundScene ) = 0;
	virtual void Update( interface ISoundScene *pSoundScene ) = 0;
	virtual void RegisterAsBored( const NDb::EUnitAckType eBored, interface IMOUnit *pObject ) = 0;
	virtual void UnRegisterAsBored( const NDb::EUnitAckType eBored, interface IMOUnit *pObject ) = 0;

	virtual void SetClientConsts( const NDb::SClientGameConsts *pConsts ) = 0;
	virtual void Init() = 0;
	virtual bool IsNegative( const NDb::EUnitAckType eAck ) = 0;
};

B2_M1_WORLD_EXPORT IClientAckManager * AckManager();
B2_M1_WORLD_EXPORT IClientAckManager *CreateAckManager();


