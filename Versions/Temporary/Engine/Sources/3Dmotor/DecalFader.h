#pragma once

#include "../System/DG.h"
#include "../System/Time.hpp"

namespace NGScene
{

class CDecal;

class CDecalFader : public CObjectBase
{
	OBJECT_BASIC_METHODS( CDecalFader )
	//
	ZDATA
	CObj<CDecal> pDecal;
	CDGPtr<CFuncBase<STime> > pTime;
	STime tFadeInStart;
	STime tFadeInEnd;
	STime tFadeOutStart;
	STime tFadeOutEnd;
	bool bFrozen;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pDecal); f.Add(3,&pTime); f.Add(4,&tFadeInStart); f.Add(5,&tFadeInEnd); f.Add(6,&tFadeOutStart); f.Add(7,&tFadeOutEnd); f.Add(8,&bFrozen); return 0; }
private:
	float GetFadeValue( STime tCurrent ) const;
	void ShiftTimes( STime tDelta );

protected:
	CDecalFader() {}

public:
	CDecalFader( CObjectBase *_pDecal, STime tFadeInStart, STime tFadeInEnd, STime tFadeOutStart, STime tFadeOutEnd, CFuncBase<STime> *_pTime );
	bool Update( void *p ); // returns false: it is time to delete this
	void SetToFadeIn();
	bool IsFrozen() const { return bFrozen; }
	void Freeze();
	void Unfreeze();
};

}


