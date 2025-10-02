// WindowSlider.h: interface for the CWindowSlider class.
//


#if !defined(AFX_WINDOWSLIDER_H__FF45A97C_D276_4BE2_BF2C_061AFFE51E2F__INCLUDED_)
#define AFX_WINDOWSLIDER_H__FF45A97C_D276_4BE2_BF2C_061AFFE51E2F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Window.h"


// window with button. button can move horisontally or vertically,
// 
class CWindowMSButton;

class CWindowSlider : public CWindow, public ISlider  
{
	OBJECT_BASIC_METHODS(CWindowSlider)

	CDBPtr<NDb::SWindowSliderShared> pShared;
	CPtr<NDb::SWindowSlider> pInstance;
	
	//
	float fMin;
	float fMax;
	float fPageSize;														// 
	float fCur;
	float fPickOffset;												// when slider is picked no on center
	//}
	
	//{ dynamic data
	bool bManualScrolling;
	// fast scrolling parameters
	bool bPressed;													// remember pressed state, scrolls by timer
	bool bFirstTime;												// first scroll should will wait for a longer time
	int animTime;
	CVec2 vPressedPos;
	bool bFastScrollForward;
	bool bFastScrolling;
	CObj<CWindowMSButton> pLever;
	CPtr<ISliderNotify> pNotifySink;						// parent
	NInput::CBind inputScroll;
	bool bMouseScrollingAllowed;
	bool bLeverOn;
	//}

	//Verifies that the position of a slider control is between the minimum and maximum values.
	void UpdatePos();
	// return fCur suitable for current mouse pressed pos
	float CalcPressedPos( const CVec2 &vPos, const float fPosOffset = 0 ) const;
	// return suitable position, that matches _fCur as close as possible
	float VerifyPos( const float _fCur ) const;
	void ScrollFast();
	float CalcPickOffset( const CVec2 &vPos ) const;
	bool NeedHandleMouseScrolling() const;
protected:
	virtual NDb::SWindow* GetInstance() { return pInstance; }

public:

	CWindowSlider() : fMin( 0.0f ), fMax( 0.0f ), fCur( 0.0f ), fPageSize( 100.0f ),
		bPressed( false ), bFastScrolling( false ), pNotifySink( 0 ), bManualScrolling( false ), 
		bMouseScrollingAllowed ( false ), inputScroll( "mouse_wheel_scroll" ), bLeverOn( false ){  }

	virtual bool IsHorisontal() const { return pShared->bHorisontal; }

	// message sinks
	void OnKeyUp( const struct SGameMessage &msg );
	void OnKeyDown( const struct SGameMessage &msg );

	void OnKeyRight( const struct SGameMessage &msg );
	void OnKeyLeft( const struct SGameMessage &msg );

	void OnKeyPgDn( const struct SGameMessage &msg );
	void OnKeyPgUp( const struct SGameMessage &msg );

	void OnKeyHome( const struct SGameMessage &msg );
	void OnKeyEnd( const struct SGameMessage &msg );
	// end message sinks

	
	bool ProcessEvent( const struct SGameMessage &msg );
	//ISlider{ 
	void  SetRange( const float _fMin, const float _fMax, const float _fPageSize );
	void GetRange( float *pMax, float *pMin ) const;
	void SetPos( const float _nCur );
	float GetPos() const;
	void SetNotifySink( interface ISliderNotify *_pNotifySink ) { pNotifySink = _pNotifySink; }
	bool IsLeverVisible() const;
	void AllowMouseScrolling( const bool _bAllow );
	//ISlider}

	// IWindow & CWindow
	int operator&( interface IBinSaver &saver );
	void Reposition( const CTRect<float> &parentRect );
	void InitByDesc( const struct NDb::SUIDesc *_pDesc );

	void Segment( const int timeDiff );
	
	bool OnMouseMove( const CVec2 &vPos, const int nButton );
	bool OnButtonDown( const CVec2 &vPos, const int nButton );
	bool OnButtonUp( const CVec2 &vPos, const int nButton );

	int GetNSpecialPositions();
	void SetNSpecialPositions( int nPositions );
	void SetSpecialPosition( int nPosition );

	int GetCurrentSpecialPosition() const;
};

#endif // !defined(AFX_WINDOWSLIDER_H__FF45A97C_D276_4BE2_BF2C_061AFFE51E2F__INCLUDED_)
