#pragma once

#include "System/det_map.h"

class CAAFeedBacks
{
	typedef std::list<int/*Planes unique id*/> CTargetList;
	typedef det_map<int/*AA Unique ID*/,CTargetList>  CAAFeedBacksList;

	CAAFeedBacksList feedbacks;
public:
	int operator&( IBinSaver &f ) 
	{ 
		if ( !f.IsChecksum() )
			f.Add(2,&feedbacks);
		return 0; 
	}
private:
	void SendFeedBack( class CAIUnit *pAA ) const;
public:
	void Clear();
	void Fired( class CAIUnit *pAA, class CAIUnit *pTarget );
	void PlaneDeleted( class CAIUnit *pTarget );
};
