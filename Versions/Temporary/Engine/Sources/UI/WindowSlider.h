// WindowSlider.h: interface for the CWindowSlider class.
//

#pragma once
#include "Window.h"

// window with button. button can move horisontally or vertically,
//
class CWindowMSButton;

class CWindowSlider : public CWindow, public ISlider
{
	OBJECT_BASIC_METHODS( CWindowSlider )

	CDBPtr<NDb::SWindowSliderShared> pShared;
	CPtr<NDb::SWindowSlider> pInstance;

	//
	float fMin;
	float fMax;
	float fPageSize; //
	float fCur;
	float fPickOffset; // when slider is picked no on center
	//}

	//{ dynamic data
	bool bManualScrolling;
	// fast scrolling parameters
	bool bPressed;   // remember pressed state, scrolls by timer
	bool bFirstTime; // first scroll should will wait for a longer time
	int animTime;
	CVec2 vPressedPos;
	bool bFastScrollForward;
	bool bFastScrolling;
	CObj<CWindowMSButton> pLever;
	CPtr<ISliderNotify> pNotifySink; // parent
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
	NDb::SWindow *GetInstance() override
	{
		return pInstance;
	}

public:
	CWindowSlider()
		: fMin( 0.0f )
		, fMax( 0.0f )
		, fCur( 0.0f )
		, fPageSize( 100.0f )
		, bPressed( false )
		, bFastScrolling( false )
		, pNotifySink( 0 )
		, bManualScrolling( false )
		, bMouseScrollingAllowed( false )
		, inputScroll( "mouse_wheel_scroll" )
		, bLeverOn( false )
	{
	}

	bool IsHorisontal() const override
	{
		return pShared->bHorisontal;
	}

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

	bool ProcessEvent( const struct SGameMessage &msg ) override;
	//ISlider{
	void SetRange( const float _fMin, const float _fMax, const float _fPageSize ) override;
	void GetRange( float *pMax, float *pMin ) const override;
	void SetPos( const float _nCur ) override;
	float GetPos() const override;

	void SetNotifySink( struct ISliderNotify *_pNotifySink ) override
	{
		pNotifySink = _pNotifySink;
	}

	bool IsLeverVisible() const override;
	void AllowMouseScrolling( const bool _bAllow ) override;
	//ISlider}

	// IWindow & CWindow
	int operator&( struct IBinSaver &saver ) override;
	void Reposition( const CTRect<float> &parentRect ) override;
	void InitByDesc( const struct NDb::SUIDesc *_pDesc ) override;

	void Segment( const int timeDiff ) override;

	bool OnMouseMove( const CVec2 &vPos, const int nButton ) override;
	bool OnButtonDown( const CVec2 &vPos, const int nButton ) override;
	bool OnButtonUp( const CVec2 &vPos, const int nButton ) override;

	int GetNSpecialPositions() override;
	void SetNSpecialPositions( int nPositions ) override;
	void SetSpecialPosition( int nPosition ) override;

	int GetCurrentSpecialPosition() const override;
};
