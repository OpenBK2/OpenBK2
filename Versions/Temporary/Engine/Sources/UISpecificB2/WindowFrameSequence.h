#pragma once

#include "..\ui\commandparam.h"
#include "..\ui\dbuserinterface.h"
#include "..\ui\ui.h"
#include "..\input\gamemessage.h"
#include "..\ui\uifactory.h"
#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "../UI/Window.h"
#include "DBUISpecificB2.h"
#include "UISpecificB2.h"

class CWindowFrameSequence : public CWindow, public IWindowFrameSequence
{
	OBJECT_NOCOPY_METHODS( CWindowFrameSequence );
	
	ZDATA_(CWindow)
	CPtr<NDb::SWindowFrameSequence> pInstance;
	CDBPtr<NDb::SWindowFrameSequenceShared> pShared;
	int nFrame;
	bool bRun;
	int nTime;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CWindow*)this); f.Add(2,&pInstance); f.Add(3,&pShared); f.Add(4,&nFrame); f.Add(5,&bRun); f.Add(6,&nTime); return 0; }
	NTimer::STime timeStart;
protected:
	NDb::SWindow* GetInstance() { return pInstance; }
public:
	CWindowFrameSequence();
	~CWindowFrameSequence();

	//{ CWindow
	void Visit( interface IUIVisitor *pVisitor );
	void InitByDesc( const struct NDb::SUIDesc *pDesc );
	//}

	//{ IWindowFrameSequence
	void Run( bool bRun );
	void Reset();
	//}
};

