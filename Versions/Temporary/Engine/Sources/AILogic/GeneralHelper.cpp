#include "stdafx.h"

#include "GeneralHelper.h"
#include "..\Stats_B2_M1\DBMapInfo.h"
#include "AIUnit.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern NTimer::STime curTime;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*											SSeverityCountPredicate*
//*******************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SGeneralHelper::SSeverityCountPredicate::operator()( const CCommonUnit *pUnit )
{
	//NI_ASSERT( CalcUnitSeverity( pUnit ) != 0.0f, StrFmt( "useless unit, pUnit" ) );
	fCount += CalcUnitSeverity( pUnit );
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*											SFindBestByEnumeratorPredicate*
//*******************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SGeneralHelper::SFindBestByEnumeratorPredicate::operator()( class CCommonUnit *pU1 )
{
	if ( pEn->EvaluateWorker( pU1, eType ) )
	{
		const float fNewRating = pEn->EvaluateWorkerRating( pU1, eType );
	
		if ( !pBest || fNewRating > fRating )
		{
			pBest = pU1;
			fRating = fNewRating;
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*											SDeadPredicate															*
//*******************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SGeneralHelper::SDeadPredicate::operator() ( CCommonUnit * pUnit )
{
	return !pUnit || !pUnit->IsRefValid() || !pUnit->IsAlive();
}

bool SGeneralHelper::SInactiveUnitPredicate::operator() ( class CCommonUnit * pUnit )
{
	return !pUnit || !pUnit->IsRefValid() || !pUnit->IsAlive() || pUnit->GetParty() != nParty;
}

//*******************************************************************
//*											SGeneralHelper*
//*******************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SGeneralHelper::SFindByEnumeratorPredicate::operator()( class CCommonUnit *pU1 )
{ 
	return pEn->EvaluateWorker( pU1, eType ); 
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*											SGeneralHelper*
//*******************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SGeneralHelper::IsUnitNearParcel( const CCommonUnit *pUnit, const struct NDb::SAIGeneralParcel &parcel ) 
{
	if ( !pUnit->IsRefValid() || !pUnit->IsAlive() ) return false;
	const CVec2 vDiff1( ( pUnit->GetCenterPlain() - parcel.vCenter ) );
	return  fabs( vDiff1 ) <=  parcel.fRadius + SConsts::SPY_GLASS_RADIUS / 2;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SGeneralHelper::IsUnitInParcel( const CCommonUnit *pUnit, const struct NDb::SAIGeneralParcel &parcel ) 
{
	if ( !pUnit->IsRefValid() || !pUnit->IsAlive() ) return false;
	const CVec2 vDiff( pUnit->GetCenterPlain() - parcel.vCenter );
	return fabs( vDiff ) <= parcel.fRadius;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float SGeneralHelper::CalcUnitSeverity( const CCommonUnit *pUnit )
{
	if ( !pUnit->IsRefValid() || !pUnit->IsAlive() ) return 0;
	return pUnit->GetPriceMax();
}

bool SGeneralHelper::RemoveDead( CommonUnits *pUnits )
{
	SGeneralHelper::SDeadPredicate deadPred;
	CommonUnits::iterator firstDead = remove_if( pUnits->begin(), pUnits->end(), deadPred );
	const bool bDeleted = firstDead != pUnits->end();
	pUnits->erase( firstDead, pUnits->end() );
	return bDeleted;
}

bool SGeneralHelper::RemoveNotCurrentPlayer( CommonUnits *pUnits, int nPlayer )
{
	SGeneralHelper::SInactiveUnitPredicate deadPred( nPlayer );
	CommonUnits::iterator firstDead = remove_if( pUnits->begin(), pUnits->end(), deadPred );
	const bool bDeleted = firstDead != pUnits->end();
	pUnits->erase( firstDead, pUnits->end() );
	return bDeleted;
}


