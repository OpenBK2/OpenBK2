#pragma once
#include "../ui/commandparam.h"
#include "../ui/dbuserinterface.h"
#include "../ui/ui.h"
#include "../misc/2darray.h"
#include "../zlib/zconf.h"
#include "../UI/Background.h"
#include "DBUISpecificB2.h"

class CBackgroundFrameSequence :	public CBackground
{
	OBJECT_BASIC_METHODS(CBackgroundFrameSequence);

	ZDATA_(CBackground)
	CDBPtr<NDb::SBackgroundFrameSequence> pStats;
	CDBPtr<NDb::STexture> pTexture;
	int nFrame;
	int nTime;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CBackground*)this); f.Add(2,&pStats); f.Add(3,&pTexture); f.Add(4,&nFrame); f.Add(5,&nTime); return 0; }

	NTimer::STime timeStart;
public:
	CBackgroundFrameSequence();

	//{ IWindowPart
	void InitByDesc( const struct NDb::SUIDesc *pDesc );
	void Visit( struct IUIVisitor *pVisitor );
	void SetTexture( const struct NDb::STexture *pDesc );
	//}
};


