#include "stdafx.h"
#include "AckManager.h"
#include "AIUnit.h"
#include "Diplomacy.h"

CAckManager theAckManager;

extern NTimer::STime curTime;
extern CDiplomacy theDipl;

//*******************************************************************
//*		 								   CAckManager*
//*******************************************************************

int CAckManager::operator&( IBinSaver &saver )
{
	
	saver.Add( 1, &acknowledgements );
	saver.Add( 4, &bored );
	saver.Add( 5, &ackIndex );
	return 0;
}

CAckManager::CAckManager()
: ackIndex ( 0 )
{
}

CAckManager::~CAckManager()
{
}

void CAckManager::Clear()
{
	acknowledgements.clear();
	bored.clear();
	ackIndex = 0;
}

bool CAckManager::UpdateAcknowledgment( SAIAcknowledgment &pAck )
{
	while ( ackIndex < acknowledgements.size() )
	{
		pAck = acknowledgements[ackIndex];
		++ackIndex;
		const CUpdatableObj *pObj = GetObjectByUniqueIdSafe<CUpdatableObj>( pAck.nObjUniqueID );
		if ( pObj && pObj->IsRefValid() && pObj->IsAlive() )
			return true;
	}
	acknowledgements.clear();
	ackIndex = 0;
	return false;
}

bool CAckManager::UpdateAcknowledgment( SAIBoredAcknowledgement &pAck )
{
	if ( bored.size() == 0 )
		return false;

	CAckTypeBoredPrecence::iterator it = bored.begin();
	if ( it->second.size() == 0 ) 
	{
		bored.erase( it );
		return false;
	}
	CBoredPresence::iterator it2 = it->second.begin();
	pAck.bPresent = it2->second.second;
	pAck.nObjUniqueID = it2->second.first->GetUniqueId(); //	pAck.pObj = it2->second.first;
	pAck.nAck = it->first;
	it->second.erase( it2 );
	if ( it->second.size() == 0 ) 
	{
		bored.erase( it );
		return false;
	}
	return true;
}

void CAckManager::AddAcknowledgment( const SAIAcknowledgment &ack )
{
	acknowledgements.push_back( ack );
}

void CAckManager::AddAcknowledgment(	EUnitAckType eAck, CUpdatableObj *pObject, const int nSet )
{
	SAIAcknowledgment ack( eAck, pObject->GetUniqueId(), nSet );
	AddAcknowledgment( ack );
}

void CAckManager::RegisterAsBored( EUnitAckType eBored, class CAIUnit *pObject )
{
	if ( pObject->GetPlayer() != theDipl.GetMyNumber() ) return;
	//NI_ASSERT( eBored <= _ACK_BORED_END && eBored >= _ACK_BORED_BEGIN, "not bored ack passed" );
	bored[eBored][pObject->GetUniqueId()] = CUnitBoredPresence( pObject, true );
}

void CAckManager::UnRegisterAsBored( EUnitAckType eBored, class CAIUnit *pObject )
{
	if ( pObject->GetPlayer() != theDipl.GetMyNumber() ) return;
	// если этот юнит есть в списке, то он удаляется.
	//NI_ASSERT( eBored <= _ACK_BORED_END && eBored >= _ACK_BORED_BEGIN, "not bored ack passed" );
	bored[eBored][pObject->GetUniqueId()] = CUnitBoredPresence( pObject, false );
	if ( bored[eBored].empty() )
	{
		bored.erase( eBored );
	}
}

void CAckManager::UnitDead( class CAIUnit *pObject )
{
	for ( CAckTypeBoredPrecence::iterator it = bored.begin(); it != bored.end(); ++it )
		it->second.erase( pObject->GetUniqueId() );
	// common acks from dead units will be deleted in update.
}
