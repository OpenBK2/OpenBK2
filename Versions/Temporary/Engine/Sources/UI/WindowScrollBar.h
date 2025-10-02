// WindowScrollBar.h: interface for the CWindowScrollBar class.
//


#if !defined(AFX_WINDOWSCROLLBAR_H__46D0E093_95AF_4D78_9A5B_BB754D40FC3A__INCLUDED_)
#define AFX_WINDOWSCROLLBAR_H__46D0E093_95AF_4D78_9A5B_BB754D40FC3A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Window.h"



class CWindowMSButton;
class CWindowSlider;
struct SWindowMSButton;
struct SWindowSlider;

// contains 1 slider and 2 buttons (up & down)
class CWindowScrollBar : public CWindow, public ISlider, public IButtonNotify, public ISliderNotify
{
	OBJECT_BASIC_METHODS(CWindowScrollBar)

	CObj<CWindowMSButton> pButtonLower;
	CObj<CWindowMSButton> pButtonGreater;
	CObj<CWindowSlider> pSlider;

	bool bScrollGreater;													// timed scrolling up
	int animTime;	
	bool bFirstTime;
	CDBPtr<NDb::SWindowScrollBarShared> pShared;
	CPtr<NDb::SWindowScrollBar> pInstance;
	
	void Scroll( const float fDist = 1.0f );
	void UpdateButtons();
protected:
	NDb::SWindow* GetInstance() { return pInstance; }

public:

	int operator&( interface IBinSaver &saver );
	void Reposition( const CTRect<float> &parentRect );
	void InitByDesc( const struct NDb::SUIDesc *pDesc );

	void Init();
	void Segment( const int timeDiff );
	void AfterLoad();

	//IButtonNotify 
	void Released( class CWindow *pWho );
	void Pushed( class CWindow *pWho );
	void Entered( class CWindow *pWho );
	void Leaved( class CWindow *pWho );
	void StateChanged( class CWindow *pWho ) {  }
	void AllowMouseScrolling( const bool _bAllow );
	
	bool IsLeverVisible() const;


	void SetRange( const float fMin, const float fMax, const float fPageSize );
	void GetRange( float *pMax, float *pMin ) const;
	void SetPos( const float fCur );
	float GetPos() const;
	void SetNotifySink( interface ISliderNotify *pNotify );
	bool IsHorisontal() const;
	void SliderPosition( const float fPosition, class CWindow *pWho );

	virtual int GetNSpecialPositions();
	virtual int GetCurrentSpecialPosition() const;
	void SetNSpecialPositions( int nPositions );
	void SetSpecialPosition( int nPosition );
};

#endif // !defined(AFX_WINDOWSCROLLBAR_H__46D0E093_95AF_4D78_9A5B_BB754D40FC3A__INCLUDED_)
