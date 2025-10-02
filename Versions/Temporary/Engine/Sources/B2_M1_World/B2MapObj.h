#pragma once

namespace NDb
{
	enum EUnitAckType;
	struct SHPObjectRPGStats;
}

struct IB2MapObj : virtual public CObjectBase
{
	virtual int GetUniqueID() const = 0;

	virtual void Select( bool bSelect ) { }
	virtual void SetSelectionGroup( int nSelectionGroup ) { }
	virtual bool IsSelected() const { return false; }

	virtual void SendAcknowledgement( struct IClientAckManager *pAckManager, const NDb::EUnitAckType eAck ) { }
	virtual bool NeedShowInterrior() const { return false; }

	virtual const NDb::SHPObjectRPGStats *GetStats() const = 0;

	virtual void UpdateIcons() = 0;

	virtual void SetMousePicked( const bool ) {}
	virtual const bool IsMousePicked() { return false; }
};


