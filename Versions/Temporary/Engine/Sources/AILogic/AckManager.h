#pragma once

#include "Stats_B2_M1/AIAckTypes.h"
#include "System/det_map.h"

class CAIUnit;
namespace NDb
{
	enum EUnitAckType : int;
}

class CAckManager
{
	public: int operator&( IBinSaver &saver ); private:;

	typedef std::pair<CPtr<CAIUnit>, bool> CUnitBoredPresence;
	typedef det_map< int/*unit unique ID */, CUnitBoredPresence> CBoredPresence;
	typedef det_map<int, CBoredPresence> CAckTypeBoredPrecence;
	CAckTypeBoredPrecence bored;

	typedef std::vector<SAIAcknowledgment> CAcknowledgments;
	int ackIndex;													// for giving acks to client - counter
	CAcknowledgments acknowledgements;		// накапливает Acknolegments идущие от AI

	void AddAcknowledgment( const SAIAcknowledgment &ack );
public:
	CAckManager();
	virtual ~CAckManager();
	//выдача клиенту Acknowledgements
//	void UpdateAcknowledgments( SAIAcknowledgment **pAckBuffer, int *pnLen );
	bool UpdateAcknowledgment( SAIAcknowledgment &pAck );
	bool UpdateAcknowledgment( SAIBoredAcknowledgement &pAck );

	//выдача клиенту Bored Acknowledgements
//	void UpdateAcknowledgments( SAIBoredAcknowledgement **pAckBuffer, int *pnLen );

	// для BORED acknowledgements
	void RegisterAsBored(	EUnitAckType eAck, class CAIUnit *pObject );
	void UnRegisterAsBored(	EUnitAckType eAck, class CAIUnit *pObject );
	
	void AddAcknowledgment(	EUnitAckType eAck, class CUpdatableObj *pObject, const int nSet = 0 );
	void Clear();
	
	void UnitDead( class CAIUnit *pObject );
};


