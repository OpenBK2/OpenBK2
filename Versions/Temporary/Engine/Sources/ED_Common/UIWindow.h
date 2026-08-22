#pragma once

#include "UI/CommandParam.h"
#include "UI/DBUserInterface.h"
#include "UI/UI.h"

#include <cstdint>

class CUIWindow : public IWindow
{
	OBJECT_NOCOPY_METHODS(CUIWindow)

public:
	CUIWindow();
	CUIWindow( int x, int y, int w, int h, uint32_t _color, const NDb::STexture *_pTexture );
	virtual ~CUIWindow();

	// IWindow
	virtual int GetPriority() const { return 777; }
	virtual void SetPriority( int n ) {};
	bool ProcessEvent( const struct SGameMessage &msg ) { return false; }
	bool ProcessMessage( const struct SBUIMessage &msg ) { return false; }
	void SetModal( bool _bIsModal ) {}
	bool IsModal() const { return false; }
	bool OnButtonDown( const CVec2 &vPos, const int nButton ){ return false; }
	bool OnButtonUp( const CVec2 &vPos, const int nButton ) { return false; } 
	bool OnButtonDblClk( const CVec2 &vPos, const int nButton ) { return false; }
	bool OnMouseMove( const CVec2 &vPos, const int nMouseState ) { return false; }
	IWindow* Pick( const CVec2 &vPos, const bool bRecursive ) { return 0; }
	bool IsPointInsideOfChildren( const CVec2 &vPoint ) { return false; }
	void Visit( struct IUIVisitor *pVisitor );
	//void Segment( const int timeDiff ) { }
	bool IsInside( const CVec2 &vPos ) const { return true; }
	void Enable( const bool bEnable ) {}
	bool IsEnabled() const { return true; }
	void ShowWindow( const bool bShow ) {}
	bool IsVisible() const { return true; }
	void GetPlacement( int *pX, int *pY, int *pSizeX, int *pSizeY ) const {}
	void SetPlacement( const float x, const float y, const float sizeX, const float sizeY, const uint32_t flags ) {}
	CTRect<int> GetPlacement() const { return CTRect<int>(0,0,0,0); }
	void SetPlacement( const CTRect<int> &rc, const uint32_t flags ) {}
	void AddChild( IWindow *pWnd, bool bDoReposition = false ) {}
	int GetNumChildren() {return 0;}
	IWindow *GetChild( int nIndex ) { return 0; }
	IWindow* GetChild( const string &_szName, const bool bRecursive = false ) { return 0; }
	IWindow* GetVisibleChild( const string &_szName, const bool bRecursive = false ) { return 0; }
	void CopyBackground( const IWindow *pSrcWnd ) {}
	void SetTexture( const struct NDb::STexture *pDesc ) {}
	void InitByDesc( const struct NDb::SUIDesc *pDesc ) {}
	void RegisterObservers() {}
	void SetOutline( int eOutlineType ) {}
	void SetTextString( const wstring &szText ) {}
	const wchar_t *GetTextString() const { return 0; }
	virtual wstring GetDBText() const { return wstring(); };
	SWindowContext * GetContext() { return 0; }
	void Init() {}
	const struct NDb::SUIDesc * GetDesc() const { return 0; }
	void FillWindowRect( CTRect<float> *pRect ) const {}
	CTRect<float> GetWindowRect() const { return CTRect<float>(0,0,0,0); }
	IWindow* GetParentWindow() const { return 0; }
	struct IScreen* GetScreen() { return 0; }
	const struct NDb::SWindowShared * GetSharedDesc() const { return 0; }
	void RemoveChild( IWindow *_pChild ) {}
	IWindow* GetChild( const int _nTypeID, const int _nID, const bool bRecursive = false ) { return 0; }
	const string& GetName() const { return szFakeName; }
	void SetName( const string &szName ) {}

	// help context
	void SetTooltipIDForMLHandler( int nID ) {}
	int GetTooltipIDForMLHandler() const { return -1; }
	
	virtual void SetFocus( const bool bFocus ) { return; }
	bool IsFocused() const { return false; }
	void SetFadeValue( float fValue ) {}
	// methods

	// members
protected:
	string szFakeName;
	CTRect<float> pos;
	CDBPtr<NDb::STexture> pTexture;
	uint32_t color;
};

CUIWindow *CreateUIWindow( int x, int y, int w, int h, uint32_t _color, const NDb::STexture *_pTexture );



