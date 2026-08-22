#pragma once

#include "UI/CommandParam.h"
#include "UI/DBUserInterface.h"
#include "UI/UI.h"
#include "Input/GameMessage.h"
#include "UI/UIFactory.h"
#include "Misc/2Darray.h"
#include "UI/Window.h"
#include "DBUISpecificB2.h"
#include "UISpecificB2.h"

#include <zconf.h>

class CWindow3DControl : public CWindow, public IWindow3DControl
{
	OBJECT_NOCOPY_METHODS( CWindow3DControl );
	
	ZDATA_(CWindow)
	CPtr<NDb::SWindow3DControl> pInstance;
	CDBPtr<NDb::SWindow3DControlShared> pShared;
	std::vector<SObject> objects;
	bool bIsObjectsVisible;
	CTRect<float> rectPrev;
	int nBaseID3D;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CWindow*)this); f.Add(2,&pInstance); f.Add(3,&pShared); f.Add(4,&objects); f.Add(5,&bIsObjectsVisible); f.Add(6,&rectPrev); f.Add(7,&nBaseID3D); return 0; }
private:
	void HideObjects();
	void UpdateObjects( bool bForced );
protected:
	NDb::SWindow* GetInstance() { return pInstance; }
	void OnChangeVisibility( bool bShow );
public:
	CWindow3DControl();
	~CWindow3DControl();

	//{ CWindow
	void Visit( struct IUIVisitor *pVisitor );
	void InitByDesc( const struct NDb::SUIDesc *pDesc );
	//}
	
	//{ IWindow3DControl
	void SetObjects( const std::vector<SObject> &objects );
	SParam GetDBObjectParam( int nIndex ) const;
	void SetBaseID3D( int nID );
	//}
};


