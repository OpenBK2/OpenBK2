#include "stdafx.h"

#include "Commander.h"
#include "GeneralHelper.h"
#include "TimeCounter.h"

#include <algorithm>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern NAI::CTimeCounter timeCounter;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCommander::STaskCalcSeverityPredicate::STaskCalcSeverityPredicate()
: nNumberNegative( 0 ), 
	fSeverityNegative( 0 ),
	nNumberPositive( 0 ),
	fSeverityPositive( 0 )
{  
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const float CCommander::STaskCalcSeverityPredicate::GetSeverity() const
{
	if ( nNumberPositive + nNumberNegative )
	{
		return ( fSeverityNegative + fSeverityPositive ) / ( nNumberNegative + nNumberPositive ) ;
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

	// проделать раблту всеми тасками
	SSegmentPredicate segmentPred;
	for_each( tasks.begin(), tasks.end(), segmentPred );

	// выбрать завершенные задачи
	SFinishedPredicate finishedPred;
	Tasks::iterator finishedFirst = remove_if( tasks.begin(), tasks.end(), finishedPred );

	// забрать всех работников у завершенных задач
	STakeWorkersPredicate takeWorkers( this );
	for_each( finishedFirst, tasks.end(), takeWorkers );
	
	// удалить завершенные задачи
	tasks.erase( finishedFirst, tasks.end() );

	// отсортировать задачи по приоритетам
	STaskSortPresicate pr;
	sort( tasks.begin(), tasks.end(), pr );

	// посчитать среднюю сложность всех тасков
	STaskCalcSeverityPredicate calcSeverity;
	calcSeverity = for_each( tasks.begin(), tasks.end(), calcSeverity );
	fMeanSeverity = calcSeverity.GetSeverity();

	
	// у тасков, которых все хорошо попытаться забрать лишние юниты
	for ( Tasks::iterator i = tasks.begin(); i != tasks.end(); ++i )
		(*i)->ReleaseWorker( this, calcSeverity.nNumberNegative ? -1.0f : fMeanSeverity );
	

	if ( !tasks.empty() ) 
	{
		for ( Tasks::iterator it = tasks.end(); ; )
		{
			--it;
			(*it)->AskForWorker( this, fMeanSeverity );
			if ( it == tasks.begin() ) 
				break;
		}
	}
	/*
	for ( Tasks::reverse_iterator i = tasks.rbegin(); i != tasks.rend(); ++i )
		(*i)->AskForWorker( this, fMeanSeverity );
	*/

}
struct SCommonUnitCompare
{
	CPtr<CCommonUnit> pUnit;
	SCommonUnitCompare( CCommonUnit *_pUnit ) : pUnit( _pUnit ) {  }
	bool operator()( const CPtr<CCommonUnit> _pUnit ) const
	{
		return pUnit == _pUnit;
	}
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CCommander::EnumWorkersInternal( const enum EForceType eType, IWorkerEnumerator *pEn, CommonUnits *pUnits )
{
	// the simple optimisation
	if ( pEn->NeedNBest( eType ) > 1 )
	{
		std::vector< std::pair<float, CPtr<CCommonUnit> > > units;
		for ( CommonUnits::iterator it = pUnits->begin(); it != pUnits->end(); ++it )
			units.push_back( std::pair<float, CPtr<CCommonUnit> >( 0, *it ) );
		pUnits->clear();
		SCalcRatingPredicate calcRating( pEn, eType );
		for_each( units.begin(), units.end(), calcRating );
		sort( units.begin(), units.end(), calcRating );
		
		bool bNeedMore = true;
		for ( int i = 0; i < units.size(); ++i )
		{
			if ( bNeedMore && pEn->EvaluateWorker( units[i].second, eType ) )
			{
				if ( pEn->EnumWorker( units[i].second, eType ) )
					bNeedMore = true;
			}
			else
				pUnits->push_back( units[i].second );
		}
	}
	else if ( 1 == pEn->NeedNBest( eType ) )
	{
		SGeneralHelper::SFindBestByEnumeratorPredicate prBest( pEn, eType );
		prBest = for_each( pUnits->begin(), pUnits->end(), prBest );
		if ( prBest.pBest )
		{
			pEn->EnumWorker( prBest.pBest, eType );
			SCommonUnitCompare pr( prBest.pBest );
			CommonUnits::iterator added = find_if( pUnits->begin(), pUnits->end(), pr );
			pUnits->erase( added );
		}
	}
	else
	{
  	SGeneralHelper::SFindByEnumeratorPredicate pr( pEn, eType );
		CommonUnits::iterator suitable = remove_if( pUnits->begin(), pUnits->end(), pr );
		
		bool bNeedMore = true;
		for ( CommonUnits::iterator it = suitable; it != pUnits->end() && bNeedMore; )
		{
			bNeedMore = pEn->EnumWorker( *it, eType );
			it = pUnits->erase( it );
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

