#pragma once

#include "System/Dg.h"

namespace NGScene
{

class CObjectFader : public CObjectBase
{
	OBJECT_BASIC_METHODS(CObjectFader)
	//
	ZDATA
	CDGPtr<CFuncBase<float> > pFader;
	CPtr<CObjectBase> pObj;
	vector<CDGPtr<CFuncBase<float> > > transpChannels;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pFader); f.Add(3,&pObj); f.Add(4,&transpChannels); return 0; }
protected:
	CObjectFader() {}
public:
	CObjectFader( CObjectBase *_pObj, CFuncBase<float> *_pFader, vector<CPtr<CFuncBase<float> > > *_pTranspChannels );
	bool Update( void *p );
	const CPtr<CObjectBase> &getObj() { return pObj;};

};

} // namespace NGScene


