#pragma once

#include "Stats_B2_M1/FeedBackUpdates.h"
#include "System/FreeIDs.h"

namespace NDb
{
	struct SAIGameConsts;
}

struct SEFBHash
{
	int operator()( const EFeedBack & eFB ) const { return int ( eFB ); }
};

typedef std::pair<int,int> CIDAndParam;

class CFeedBackSystem
{
	struct SFeedBackCell
	{
		ZDATA
		ZSKIP
		NTimer::STime timeNextTalk;
		CVec2 vCenter;
		int nClientID;				// id to remove feedback from client
		NTimer::STime timeToForget;
		std::vector<CIDAndParam> objectIDs;
		ZEND int operator&( IBinSaver &f ) { f.Add(3,&timeNextTalk); f.Add(4,&vCenter); f.Add(5,&nClientID); f.Add(6,&timeToForget); f.Add(7,&objectIDs); return 0; }
	public:
		SFeedBackCell( ) : vCenter( -1, -1 ), timeToForget( 0 ), timeNextTalk( 0 ) {  }
		SFeedBackCell( const CVec2 &_vCenter ) 
			: vCenter( _vCenter ), nClientID( -1 ), timeToForget( 0 ), timeNextTalk( 0 ) {  }
	};
	typedef std::list<SFeedBackCell> CCells;
	typedef std::unordered_map<EFeedBack, CCells, SEFBHash> CFeedbacks;

	CFeedbacks feedbacks;
	CFreeIds clientIDs;
	CDBPtr<NDb::SAIGameConsts> pConsts;

public:
	int operator&( IBinSaver &f ) { if ( !f.IsChecksum() ) { f.Add(2,&feedbacks); f.Add(3,&clientIDs); f.Add(4,&pConsts); } return 0; }

private:
	void AddFeedBackInternal( int nID, const CVec2 &vCenter, EFeedBack eFeedBack, bool bForget, int nParam );

public:
	CFeedBackSystem();
	void Clear()
	{
		feedbacks.clear();
		clientIDs.Clear();
	}
	void AddFeedback( int nID, const CVec2 &vCenter, EFeedBack eFeedBack, int nParam );
	void RemoveFeedback( int nID, EFeedBack eFeedBack );
	
	void AddFeedbackAndForget( int nID, const CVec2 &vCenter, EFeedBack eFeedBack, int nParam );

	void RemovedAllFeedbacks( int nID );

	void Segment();
};

