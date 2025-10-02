#pragma once

struct ICommonB2M1AI : public CObjectBase
{
	enum { tidTypeID = 0x3015CB00 };
	
	virtual bool UpdateAcknowledgment( struct SAIAcknowledgment &pAck ) = 0;
	virtual bool UpdateAcknowledgment( struct SAIBoredAcknowledgement &pAck ) = 0;
	virtual bool IsCombatSituation() = 0;

	virtual void PrepareUpdates() = 0;
	virtual CObjectBase* GetUpdate() = 0;

	virtual void ProcessCommand( CObjectBase *pCommand ) { }
	virtual void DumpAfterAssinc() {}
};


