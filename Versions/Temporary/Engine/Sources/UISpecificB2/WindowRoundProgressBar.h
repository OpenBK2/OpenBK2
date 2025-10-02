#pragma once

#include "../ui/commandparam.h"
#include "../ui/dbuserinterface.h"
#include "../ui/ui.h"
#include "../input/gamemessage.h"
#include "../ui/uifactory.h"
#include "../misc/2darray.h"
#include "../UI/Window.h"
#include "DBUISpecificB2.h"
#include "UISpecificB2.h"
#include "Components.h"

#include <zconf.h>

class CWindowRoundProgressBar : public CWindow, public IWindowRoundProgressBar
{
	OBJECT_BASIC_METHODS( CWindowRoundProgressBar );
	
	ZDATA_(CWindow)
	CPtr<NDb::SWindowRoundProgressBar> pInstance;
	CDBPtr<NDb::SWindowRoundProgressBarShared> pShared;
	float fStartAngle;
	float fFinishAngle;
	CTextureRoundSegmentVisitor roundVisitor;
	CTextureRoundSegmentVisitor roundVisitor2;
	float fPosition;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CWindow*)this); f.Add(2,&pInstance); f.Add(3,&pShared); f.Add(4,&fStartAngle); f.Add(5,&fFinishAngle); f.Add(6,&roundVisitor); f.Add(7,&roundVisitor2); f.Add(8,&fPosition); return 0; }
protected:
	NDb::SWindow* GetInstance() { return pInstance; }
public:
	CWindowRoundProgressBar();

	//{ CWindow
	void Visit( struct IUIVisitor *pVisitor );
	void InitByDesc( const struct NDb::SUIDesc *pDesc );
	//}
	
	//{ IWindowRoundProgressBar
	void SetAngles( float _fStartAngle, float _fFinishAngle );
	void SetPosition( const float );
	float GetPosition() const;
	void ShowFirstElement( bool bShow ) {}
	void SetForward( const NDb::SBackground *pForward ) {}
	//}
};


