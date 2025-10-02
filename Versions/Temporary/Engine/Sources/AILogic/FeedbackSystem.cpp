#include "StdAfx.h"
#include "./feedbacksystem.h"
#include "NewUpdater.h"


typedef hash_map<EFeedBack, NTimer::STime, SEnumHash> TFeedBackTimes;
static TFeedBackTimes feedBackTimes;
static const NTimer::STime feedBackDefaultTime = 20000;

static int nFeedBackRadius = 2000;

CFeedBackSystem theFeedBackSystem;

extern CEventUpdater updater;
extern NTimer::STime curTime;

static NTimer::STime GetFeedBackTime( const EFeedBack eFeedBack )
{
	TFeedBackTimes::const_iterator pos = feedBackTimes.find( eFeedBack );
	if ( pos == feedBackTimes.end() )
		return feedBackDefaultTime;
	else
		return pos->second;
}

CFeedBackSystem::CFeedBackSystem()
{
	feedBackTimes[EFB_HOWITZER_GUN_FIRED] = 300000;
	feedBackTimes[EFB_AAGUN_FIRED] = 300000;
	feedBackTimes[EFB_UNDER_ATTACK] = 60000;
}

void CFeedBackSystem::AddFeedbackAndForget( int nID, const CVec2 &vCenter, EFeedBack eFeedBack, int nParam )
{
	AddFeedBackInternal( nID, vCenter, eFeedBack, true, nParam );
}

void CFeedBackSystem::AddFeedback( int nID, const CVec2 &vCenter, EFeedBack eFeedBack, int nParam )
{
	AddFeedBackInternal( nID, vCenter, eFeedBack, false, nParam );
}

void CFeedBackSystem::AddFeedBackInternal( int nID, const CVec2 &vCenter, EFeedBack eFeedBack, bool bForget, int nParam )
{
	// check if feedback is already exists, then add current to that feedback
	// otherwise add to new and make pObj to the center of the feedback cell
	bool bEmpty = feedbacks[eFeedBack].empty(); // and also create object
	CCells &cells = feedbacks.find( eFeedBack )->second;
	for ( CCells::iterator it = cells.begin(); it != cells.end(); ++it )
	{
		SFeedBackCell &cell = *it;
		if ( vCenter != CVec2( -1, -1 ) && fabs2( cell.vCenter - vCenter ) < sqr( nFeedBackRadius ) )
		{
			cell.objectIDs.push_back( CIDAndParam( nID, nParam ) );
			return;
		}
	}
	SFeedBackCell cell( vCenter );
	cell.timeToForget = bForget ? curTime + GetFeedBackTime( eFeedBack ) : 0;
	cell.objectIDs.push_back( CIDAndParam( nID, nParam ) );
	cells.push_back( cell );
}
/*

void CFeedBackSystem::MoveFeedback( int nID, const CVec2 &vCenter, EFeedBack eFeedBack  )
{
	RemoveFeedback( nID, eFeedBack );
	AddFeedback( nID, vCenter, eFeedBack );
}*/

struct SEqualTo
{
	int nID;
	SEqualTo( int _nID ) : nID( _nID ) {  }
	bool operator()( CIDAndParam _nTry ) const
	{
		return nID == _nTry.first;
	}
};

void CFeedBackSystem::RemoveFeedback( int nID, EFeedBack eFeedBack )
{
	bool bEmpty = feedbacks[eFeedBack].empty(); // and also create object
	CCells &cells = feedbacks.find( eFeedBack )->second;
	for ( CCells::iterator it = cells.begin(); it != cells.end(); )
	{
		SFeedBackCell &cell = *it;
		if ( cell.timeToForget != 0 )
		{
			++it;
			continue;
		}
		if ( !cell.objectIDs.empty() )
		{
			// search in cell, remove all
			cell.objectIDs.erase( remove_if( cell.objectIDs.begin(), cell.objectIDs.end(), SEqualTo( nID )), cell.objectIDs.end() );
			if ( cell.objectIDs.empty() )
			{
				updater.AddUpdate( EFB_REMOVE_FEEDBACK, it->nClientID, 0 );
				it = cells.erase( it );
			}
			else
				++it;
		}
		else
		{
			updater.AddUpdate( EFB_REMOVE_FEEDBACK, it->nClientID, 0 );
			it = cells.erase( it );
		}
	}
}

void CFeedBackSystem::RemovedAllFeedbacks( int nID )
{
	for ( CFeedbacks::iterator it = feedbacks.begin(); it != feedbacks.end(); ++it )
		RemoveFeedback( nID, it->first );
}

void CFeedBackSystem::Segment()
{
	for ( CFeedbacks::iterator it = feedbacks.begin(); it != feedbacks.end(); ++it )
	{
		CCells &cells = it->second;
		for ( CCells::iterator cellsIt = cells.begin(); cellsIt != cells.end();  )
		{
			// check if update not sent yet
			// generate client ID
			// send update
			if ( -1 == cellsIt->nClientID )
			{
				CPtr<SFeedBackUnitsArray> pParam = new SFeedBackUnitsArray;
				pParam->unitIDs.resize( cellsIt->objectIDs.size() );
				for ( int i = 0; i < cellsIt->objectIDs.size(); ++i )
					pParam->unitIDs[i] = cellsIt->objectIDs[i].first;
				pParam->nParam = cellsIt->objectIDs[0].second;
				pParam->nUpdateID = cellsIt->nClientID = clientIDs.Get();
				pParam->vCenterCamera = cellsIt->vCenter;
				updater.AddUpdate( it->first, -1, pParam );
				cellsIt->timeNextTalk = curTime + GetFeedBackTime( it->first );
				++cellsIt;
			}
			// check if update have to be removed
			// remove update
			// return client ID
			else
			{
				if ( cellsIt->objectIDs.empty() || ( cellsIt->timeToForget != 0 && cellsIt->timeToForget < curTime ) )
				{
					updater.AddUpdate( EFB_REMOVE_FEEDBACK, cellsIt->nClientID, 0 );
					clientIDs.Return( cellsIt->nClientID );
					cellsIt = cells.erase( cellsIt );
				}
				else if ( cellsIt->timeNextTalk < curTime ) // send again
				{
					CPtr<SFeedBackUnitsArray> pParam = new SFeedBackUnitsArray;
					pParam->unitIDs.resize( cellsIt->objectIDs.size() );
					for ( int i = 0; i < cellsIt->objectIDs.size(); ++i )
						pParam->unitIDs[i] = cellsIt->objectIDs[i].first;
					pParam->nParam = cellsIt->objectIDs[0].second;
					pParam->nUpdateID = cellsIt->nClientID;
					pParam->vCenterCamera = cellsIt->vCenter;
					updater.AddUpdate( it->first, -1, pParam );
					cellsIt->timeNextTalk = curTime + GetFeedBackTime( it->first );
					++cellsIt;
				}
				else
					++cellsIt;
			}
		}
	}
}


