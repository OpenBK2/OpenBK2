// WindowScrollBar.h: interface for the CWindowScrollBar class.
//

#pragma once
#include "Window.h"

class CWindowMSButton;
class CWindowSlider;
struct SWindowMSButton;
struct SWindowSlider;

// contains 1 slider and 2 buttons (up & down)
class CWindowScrollBar : public CWindow, public ISlider, public IButtonNotify, public ISliderNotify
{
	OBJECT_BASIC_METHODS( CWindowScrollBar )

	CObj<CWindowMSButton> pButtonLower;
	CObj<CWindowMSButton> pButtonGreater;
	CObj<CWindowSlider> pSlider;

	bool bScrollGreater; // timed scrolling up
	int animTime;
	bool bFirstTime;
	CDBPtr<NDb::SWindowScrollBarShared> pShared;
	CPtr<NDb::SWindowScrollBar> pInstance;

	void Scroll( const float fDist = 1.0f );
	void UpdateButtons();

protected:
	NDb::SWindow *GetInstance() override
	{
		return pInstance;
	}

public:
	int operator&( struct IBinSaver &saver ) override;
	void Reposition( const CTRect<float> &parentRect ) override;
	void InitByDesc( const struct NDb::SUIDesc *pDesc ) override;

	void Init() override;
	void Segment( const int timeDiff ) override;
	void AfterLoad() override;

	//IButtonNotify
	void Released( class CWindow *pWho ) override;
	void Pushed( class CWindow *pWho ) override;
	void Entered( class CWindow *pWho ) override;
	void Leaved( class CWindow *pWho ) override;

	void StateChanged( class CWindow *pWho ) override {}

	void AllowMouseScrolling( const bool _bAllow ) override;

	bool IsLeverVisible() const override;

	void SetRange( const float fMin, const float fMax, const float fPageSize ) override;
	void GetRange( float *pMax, float *pMin ) const override;
	void SetPos( const float fCur ) override;
	float GetPos() const override;
	void SetNotifySink( struct ISliderNotify *pNotify ) override;
	bool IsHorisontal() const override;
	void SliderPosition( const float fPosition, class CWindow *pWho ) override;

	int GetNSpecialPositions() override;
	int GetCurrentSpecialPosition() const override;
	void SetNSpecialPositions( int nPositions ) override;
	void SetSpecialPosition( int nPosition ) override;
};
