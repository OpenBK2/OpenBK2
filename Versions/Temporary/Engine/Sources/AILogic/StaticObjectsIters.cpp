#include "stdafx.h"

#include "StaticObjectsIters.h"
#include "Mine.h"

//*******************************************************************
//*												CMinesIter																*
//*******************************************************************

CMinesIter::CMinesIter( const CVec2 &vCenter, float fR, const int _nParty, const bool _bAllMines )
: CStObjCircleIter<false>( vCenter, fR ), nParty( _nParty ), bAllMines( _bAllMines )
{
	Reset();
}

void CMinesIter::Iterate()
{
	// сместиться от предыдущего значения.
	CStObjCircleIter<false>::Iterate();
	// 
	IterateToNextMine();
}

void CMinesIter::IterateToNextMine()
{
	CMineStaticObject *pMineMine = 0;
	for ( CExistingObject *pMine = 0; !CStObjCircleIter<false>::IsFinished(); CStObjCircleIter<false>::Iterate() )
	{
		pMine = CStObjCircleIter<false>::operator*();
		if (	pMine->GetObjectType() == ESOT_MINE )
		{
			pMineMine = checked_cast<CMineStaticObject*>( pMine );
			if ( pMineMine && ( bAllMines || !pMineMine->IsVisible( nParty ) ) )
			{
				//found needed object (mine).
				break;
			}
		}
	}
}

class CMineStaticObject* CMinesIter::operator->()
{
	return checked_cast<CMineStaticObject*>(CStObjCircleIter<false>::operator*());
};

class CMineStaticObject* CMinesIter::operator*()
{
	return checked_cast<CMineStaticObject*>(CStObjCircleIter<false>::operator*());
};

bool CMinesIter::IsFinished() const
{
	return CStObjCircleIter<false>::IsFinished();
}

void CMinesIter::Reset() 
{
	CStObjCircleIter<false>::Reset();
	IterateToNextMine();
}

