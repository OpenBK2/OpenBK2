#include "StdAfx.h"

#include "AAFeedBacks.h"
CAAFeedBacks theAAFeedBacks;

#include "AIUnit.h"
#include "NewUpdater.h"
#include "Diplomacy.h"
#include "FeedbackSystem.h"

extern CEventUpdater updater;
extern CDiplomacy theDipl;
extern CFeedBackSystem theFeedBackSystem;

void CAAFeedBacks::SendFeedBack( CAIUnit *pAA ) const
{
	const CVec2 vCenter( pAA->GetCenterPlain() );
	theFeedBackSystem.AddFeedbackAndForget( pAA->GetUniqueId(), pAA->GetCenterPlain(), EFB_AAGUN_FIRED, -1 );
}

void CAAFeedBacks::Clear()
{
	feedbacks.clear();
}

void CAAFeedBacks::Fired( CAIUnit *pAA, CAIUnit *pTarget )
{
	if ( theDipl.GetDiplStatus( pAA->GetPlayer(), theDipl.GetMyNumber() ) != EDI_ENEMY ) 
		return;
	
	const int nAAID = pAA->GetUniqueId();
	const int nTargetID = pTarget->GetUniqueId();

	CTargetList &targets = feedbacks[nAAID];	// yes, explisit creation. i need it.

	if ( find( targets.begin(), targets.end(), nTargetID ) == targets.end() )		
	{
		// register this target, send feedback
		targets.push_back( nTargetID );
		SendFeedBack( pAA );
	}
}

void CAAFeedBacks::PlaneDeleted( CAIUnit *pTarget )
{
	const int nTargetID = pTarget->GetUniqueId();
	for ( CAAFeedBacksList::iterator it = feedbacks.begin(); it != feedbacks.end(); ++it )
		it->second.remove( nTargetID );
}

