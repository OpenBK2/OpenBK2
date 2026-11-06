#include "stdafx.h"

#include "Commander.h"
#include "CommonUnit.h"
#include "GeneralHelper.h"
#include "TimeCounter.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern NAI::CTimeCounter timeCounter;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCommander::STaskCalcSeverityPredicate::STaskCalcSeverityPredicate()
	: nNumberNegative( 0 )
	, fSeverityNegative( 0 )
	, nNumberPositive( 0 )
	, fSeverityPositive( 0 )
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const float CCommander::STaskCalcSeverityPredicate::GetSeverity() const
{
	if ( nNumberPositive + nNumberNegative )
	{
		return ( fSeverityNegative + fSeverityPositive ) / ( nNumberNegative + nNumberPositive );
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*											CCommander*
//*******************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCommander::CCommander()
	: fMeanSeverity( 0 )
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float CCommander::GetMeanSeverity() const
{
	return fMeanSeverity;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CCommander::Segment()
{
	for ( const auto &pTask : tasks )
	{
		pTask->Segment();
	}

	// Release the actual finished task before erasing it. remove_if's tail contains
	// moved entries and cannot be used to identify which tasks have finished.
	for ( auto it = tasks.begin(); it != tasks.end(); )
	{
		if ( ( *it )->IsFinished() )
		{
			( *it )->ReleaseWorker( this, 0 );
			it = tasks.erase( it );
		}
		else
		{
			++it;
		}
	}

	// The saved task sequence supplies a unique tie-breaker without pointer addresses.
	std::vector<size_t> order( tasks.size() );
	std::iota( order.begin(), order.end(), 0 );
	std::sort( order.begin(),
	           order.end(),
	           [this]( size_t a, size_t b )
	           {
							 const float fA = tasks[a]->GetSeverity();
							 const float fB = tasks[b]->GetSeverity();
							 if ( std::isnan( fA ) != std::isnan( fB ) )
							 {
								 return !std::isnan( fA );
							 }
							 if ( fA > fB )
							 {
								 return true;
							 }
							 if ( fA < fB )
							 {
								 return false;
							 }
							 return a < b;
						 } );
	Tasks ordered;
	ordered.reserve( tasks.size() );
	for ( size_t nIndex : order )
	{
		ordered.push_back( tasks[nIndex] );
	}
	tasks.swap( ordered );

	STaskCalcSeverityPredicate calcSeverity;
	calcSeverity = std::for_each( tasks.begin(), tasks.end(), calcSeverity );
	fMeanSeverity = calcSeverity.GetSeverity();

	for ( const auto &pTask : tasks )
	{
		pTask->ReleaseWorker( this, calcSeverity.nNumberNegative ? -1.0f : fMeanSeverity );
	}
	for ( auto it = tasks.rbegin(); it != tasks.rend(); ++it )
	{
		( *it )->AskForWorker( this, fMeanSeverity );
	}
}

void CCommander::EnumWorkersInternal( const enum EForceType eType, IWorkerEnumerator *pEn, CommonUnits *pUnits )
{
	SGeneralHelper::RemoveDead( pUnits );
	// Enumeration without ratings must also be independent of registration order.
	pUnits->sort( []( const CPtr<CCommonUnit> &a, const CPtr<CCommonUnit> &b )
	              { return a->GetUniqueId() < b->GetUniqueId(); } );
	pUnits->unique();

	const int nBest = pEn->NeedNBest( eType );
	if ( nBest > 1 )
	{
		std::vector<std::pair<float, CPtr<CCommonUnit>>> units;
		for ( const auto &pUnit : *pUnits )
		{
			float fRating = pEn->EvaluateWorkerRating( pUnit, eType );
			if ( std::isnan( fRating ) )
			{
				fRating = -std::numeric_limits<float>::infinity();
			}
			units.emplace_back( fRating, pUnit );
		}
		pUnits->clear();
		std::sort( units.begin(),
		           units.end(),
		           []( const auto &a, const auto &b )
		           {
								 if ( a.first != b.first )
								 {
									 return a.first > b.first;
								 }
								 return a.second->GetUniqueId() < b.second->GetUniqueId();
							 } );

		bool bNeedMore = true;
		for ( const auto &unit : units )
		{
			if ( bNeedMore && pEn->EvaluateWorker( unit.second, eType ) )
			{
				bNeedMore = pEn->EnumWorker( unit.second, eType );
			}
			else
			{
				pUnits->push_back( unit.second );
			}
		}
	}
	else if ( nBest == 1 )
	{
		SGeneralHelper::SFindBestByEnumeratorPredicate prBest( pEn, eType );
		prBest = std::for_each( pUnits->begin(), pUnits->end(), prBest );
		if ( prBest.pBest )
		{
			pEn->EnumWorker( prBest.pBest, eType );
			pUnits->remove( prBest.pBest );
		}
	}
	else
	{
		// Erase only accepted workers; rejected candidates remain available to other tasks.
		for ( auto it = pUnits->begin(); it != pUnits->end(); )
		{
			if ( pEn->EvaluateWorker( *it, eType ) )
			{
				CPtr<CCommonUnit> pWorker = *it;
				it = pUnits->erase( it );
				if ( !pEn->EnumWorker( pWorker, eType ) )
				{
					break;
				}
			}
			else
			{
				++it;
			}
		}
	}
}
