#pragma once

#include "3Dmotor_export.h"


#include "System/DG.h"
#include "System/Time.hpp"

namespace NGScene
{

struct SFaderInfo
{
	ZDATA
	bool bCreate;
	CPtr<CObjectBase> pOwner;
	bool bStartFade;
	bool bFadeToBlack;
	int nStartTime;
	float fLatency;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&bCreate); f.Add(3,&pOwner); f.Add(4,&bStartFade); f.Add(5,&bFadeToBlack); f.Add(6,&nStartTime); f.Add(7,&fLatency); return 0; }
	//
	SFaderInfo() : bCreate(true), bStartFade(false), bFadeToBlack(false), pOwner(0), nStartTime(0), fLatency(0.f) {}
	SFaderInfo( bool _bCreate ) : bCreate(_bCreate), bStartFade(false), bFadeToBlack(false), pOwner(0), nStartTime(0), fLatency(0.f) {}
	SFaderInfo( bool _bCreate, CObjectBase *_pOwner ) : bCreate(_bCreate), bStartFade(false), bFadeToBlack(false), pOwner(_pOwner), nStartTime(0), fLatency(0.f) {}
	SFaderInfo( CObjectBase *_pOwner, bool _bFadeToBlack, int _nStartTime, float _fLatency ) : bCreate(true), bStartFade(true), pOwner(_pOwner), bFadeToBlack(_bFadeToBlack), nStartTime(_nStartTime), fLatency(_fLatency) {}
};

class IFader : public CFuncBase<float>
{
public:
	virtual void SetTimer( CFuncBase<STime> *_pTimer ) = 0;
	virtual void Show() = 0;
	virtual void Hide() = 0;
	virtual CObjectBase *GetOwner() const = 0;
};

_3DMOTOR_EXPORT IFader *CreateSimpleFader();
_3DMOTOR_EXPORT IFader *CreateSimpleFader( const SFaderInfo &faderInfo );

} // namespace NGScene


