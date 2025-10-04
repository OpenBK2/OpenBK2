#include "stdafx.h"
#include "Fader.h"
#include "3DLib/ExpFader.h"

namespace NGScene
{

enum EFadeType
{
	E_FADE_NONE,
  E_FADE_TO_BLACK,
	E_FADE_TO_WHITE,
	E_FORCE_HIDE,
	E_FORCE_SHOW
};

#define FADE_EPS 0.01f

class CSimpleFader : public IFader
{
	OBJECT_BASIC_METHODS(CSimpleFader)
	//
	ZDATA
	CDGPtr<CFuncBase<STime> > pTimer;
	NGScene::SExpFader expFader;
	bool bHide;
	ZSKIP
	EFadeType eFadeType;
	int nStartTime;
	CPtr<CObjectBase> pOwner;
	bool bForceWOTimer;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTimer); f.Add(3,&expFader); f.Add(4,&bHide); f.Add(6,&eFadeType); f.Add(7,&nStartTime); f.Add(8,&pOwner); f.Add(9,&bForceWOTimer); return 0; }
	bool bNeedUpdate;
protected:
	bool NeedUpdate() { return bNeedUpdate && ( !pTimer || pTimer.Refresh() || bForceWOTimer ); }
	void Recalc()
	{
		if ( pTimer && eFadeType == E_FORCE_HIDE )
		{
			const int nCurTime = pTimer->GetValue();
			if ( bForceWOTimer || ( nCurTime > nStartTime + 500 ) )
			{
				nStartTime = nCurTime;
				value = 0.0f;
				bNeedUpdate = false;
				bForceWOTimer = false;
			}
		} 
		else if ( pTimer && eFadeType == E_FORCE_SHOW )
		{
			const int nCurTime = pTimer->GetValue();
			nStartTime = nCurTime;
			value = 1.0f;
			bNeedUpdate = false;
			bForceWOTimer = false;
		} 
		else 
		{
			if ( !bHide )
			{
				if ( pTimer && eFadeType != E_FADE_NONE )
				{
					pTimer.Refresh();
					float fTargetSize = eFadeType == E_FADE_TO_WHITE ? 1 : 0;
					expFader.Update( pTimer->GetValue(), fTargetSize );
					value = expFader.GetSize();

					if ( eFadeType == E_FADE_TO_BLACK )
					{
						if ( fabs( value ) < FADE_EPS )
						{
							bNeedUpdate = false;
							value = 0.0f;
						}
					}
					else if ( fabs( value ) > ( 1.0f - FADE_EPS ) )
					{
						bNeedUpdate = false;
						value = 1.0f;
					}

					return;
				}
				else
					value = 1.0f;
			}
			else
				value = 0.0f;
			bNeedUpdate = false;
		}
	}
	void SetStartTime( STime t )
	{
		nStartTime = t;
		expFader.SetPrevTime( nStartTime );
	}
private:
	CSimpleFader() : nStartTime(0), bHide(false), eFadeType(E_FADE_NONE), bForceWOTimer(false), expFader(0), bNeedUpdate(false) { value = 1.0f; }
public:
	CSimpleFader( EFadeType _eFadeType, CObjectBase *_pOwner ) : nStartTime(0), eFadeType(_eFadeType), bHide(false), bNeedUpdate(true),
		expFader( _eFadeType == E_FADE_TO_WHITE ? 0 : 1 ),
		pOwner(_pOwner), bForceWOTimer(false)
	{ 
		value = 1;
	}
	//
	CSimpleFader( EFadeType _eFadeType, int _nStartTime, float _fLatency, CObjectBase *_pOwner ) : 
		expFader(_fLatency, _fLatency, _eFadeType == E_FADE_TO_WHITE ? 0 : 1 ),
		eFadeType(_eFadeType), bHide(false), bNeedUpdate(true), pOwner(_pOwner), bForceWOTimer(false) 
	{ 
		value = 1;
		SetStartTime( _nStartTime );
	}
	//
	void SetTimer( CFuncBase<STime> *_pTimer )
	{
		pTimer = _pTimer;
		pTimer.Refresh();
		SetStartTime( pTimer->GetValue() );
		bNeedUpdate = true;
	}
	//
	void Hide()
	{
		if ( pTimer )
		{
			pTimer.Refresh();
			if ( ( eFadeType == E_FORCE_HIDE ) && ( value < EPS_VALUE ) )
				nStartTime = pTimer->GetValue();
			else
			{
				bNeedUpdate = true;
				eFadeType = E_FORCE_HIDE;
				bForceWOTimer = true;
			}
		}
		else
		{
			bHide = true;
			bNeedUpdate = true;
		}
	}
	//
	void Show()
	{
		if ( pTimer )
		{
			pTimer.Refresh();
			if ( ( eFadeType == E_FORCE_SHOW ) && ( value > ( 1.0f - EPS_VALUE ) ) )
				nStartTime = pTimer->GetValue();
			else
			{
				bNeedUpdate = true;
				eFadeType = E_FORCE_SHOW;
				bForceWOTimer = true;
			}
		}
		else
		{
			bHide = false;
			bNeedUpdate = true;
		}
	}
	//
	CObjectBase *GetOwner() const { return pOwner; }
};

IFader *CreateSimpleFader()
{
	return new CSimpleFader( E_FADE_NONE, 0 );
}

IFader *CreateSimpleFader( const SFaderInfo &faderInfo )
{
	if ( faderInfo.bStartFade )
	{
		const float fOnTime = -faderInfo.fLatency / log( 0.01f );
		//return new CSimpleFader( faderInfo.bFadeToBlack ? E_FADE_TO_BLACK : E_FADE_TO_WHITE, faderInfo.nStartTime, faderInfo.fLatency/*fOnTime*/ );
		return new CSimpleFader( faderInfo.bFadeToBlack ? E_FADE_TO_BLACK : E_FADE_TO_WHITE, faderInfo.nStartTime, fOnTime, faderInfo.pOwner );
	}
	else
		return new CSimpleFader( E_FADE_NONE, faderInfo.pOwner );
}

} // namespace NGScene

using namespace NGScene;
REGISTER_SAVELOAD_CLASS(0x13168C40, CSimpleFader)
BASIC_REGISTER_CLASS( IFader )

