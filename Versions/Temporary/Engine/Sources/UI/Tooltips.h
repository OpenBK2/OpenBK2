#pragma once

class CWindow;

// manages tooltip window and tooltip contexts (types of tooltip windows)
class CTooltips
{
	ZDATA
		ZSKIP
		CPtr<IWindow> pTooltip;						// tooltip window
		CPtr<CWindow> pTooltipedWindow;		// tooltip owner window

		CVec2 vLastMousePos;
		int timeMouseFreese;
		int nContext;
		public:
  ZEND int operator&( IBinSaver &f ) { f.Add(3,&pTooltip); f.Add(4,&pTooltipedWindow); f.Add(5,&vLastMousePos); f.Add(6,&timeMouseFreese); f.Add(7,&nContext); return 0; }
		private:

	bool IsShown() const { return pTooltip != 0; }
	void AdjustTooltipPos( const CVec2 &vMousePos );
public:
	CTooltips();
	void SetTooltipContext( const int nContext, class CWindowScreen *pScreen );

	void OnMouseMove( const CVec2 &vPos, const int nButton, class CWindowScreen *pScreen );
	void Segment( const int timeDiff, class CWindowScreen *pScreen );
	void HideTooltip();

	IWindow *CreateTooltipWindow( const wstring &wszTooltipText, IWindow *pTooltipOwner, IScreen *pScreen );
};


