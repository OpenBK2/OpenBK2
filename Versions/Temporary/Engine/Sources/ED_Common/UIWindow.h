#pragma once

#include "UI/CommandParam.h"
#include "UI/DBUserInterface.h"
#include "UI/UI.h"

#include <cstdint>

class CUIWindow : public IWindow
{
	OBJECT_NOCOPY_METHODS( CUIWindow )

public:
	CUIWindow();
	CUIWindow( int x, int y, int w, int h, uint32_t _color, const NDb::STexture *_pTexture );
	~CUIWindow() override;

	// IWindow
	int GetPriority() const override
	{
		return 777;
	}

	void SetPriority( int n ) override {}

	bool ProcessEvent( const struct SGameMessage &msg ) override
	{
		return false;
	}

	bool ProcessMessage( const struct SBUIMessage &msg ) override
	{
		return false;
	}

	void SetModal( bool _bIsModal ) override {}

	bool IsModal() const override
	{
		return false;
	}

	bool OnButtonDown( const CVec2 &vPos, const int nButton ) override
	{
		return false;
	}

	bool OnButtonUp( const CVec2 &vPos, const int nButton ) override
	{
		return false;
	}

	bool OnButtonDblClk( const CVec2 &vPos, const int nButton ) override
	{
		return false;
	}

	bool OnMouseMove( const CVec2 &vPos, const int nMouseState ) override
	{
		return false;
	}

	IWindow *Pick( const CVec2 &vPos, const bool bRecursive ) override
	{
		return 0;
	}

	bool IsPointInsideOfChildren( const CVec2 &vPoint ) override
	{
		return false;
	}

	void Visit( struct IUIVisitor *pVisitor ) override;

	//void Segment( const int timeDiff ) { }
	bool IsInside( const CVec2 &vPos ) const override
	{
		return true;
	}

	void Enable( const bool bEnable ) override {}

	bool IsEnabled() const override
	{
		return true;
	}

	void ShowWindow( const bool bShow ) override {}

	bool IsVisible() const override
	{
		return true;
	}

	void GetPlacement( int *pX, int *pY, int *pSizeX, int *pSizeY ) const override {}

	void SetPlacement( const float x, const float y, const float sizeX, const float sizeY, const uint32_t flags ) override
	{
	}

	CTRect<int> GetPlacement() const override
	{
		return CTRect<int>( 0, 0, 0, 0 );
	}

	void SetPlacement( const CTRect<int> &rc, const uint32_t flags ) override {}

	void AddChild( IWindow *pWnd, bool bDoReposition = false ) override {}

	int GetNumChildren() override
	{
		return 0;
	}

	IWindow *GetChild( int nIndex ) override
	{
		return 0;
	}

	IWindow *GetChild( const std::string &_szName, const bool bRecursive = false ) override
	{
		return 0;
	}

	IWindow *GetVisibleChild( const std::string &_szName, const bool bRecursive = false ) override
	{
		return 0;
	}

	void CopyBackground( const IWindow *pSrcWnd ) override {}

	void SetTexture( const struct NDb::STexture *pDesc ) override {}

	void InitByDesc( const struct NDb::SUIDesc *pDesc ) override {}

	void RegisterObservers() override {}

	void SetOutline( int eOutlineType ) {}

	void SetTextString( const std::wstring &szText ) override {}

	const wchar_t *GetTextString() const override
	{
		return 0;
	}

	std::wstring GetDBText() const override
	{
		return std::wstring();
	}

	SWindowContext *GetContext() override
	{
		return 0;
	}

	void Init() override {}

	const struct NDb::SUIDesc *GetDesc() const override
	{
		return 0;
	}

	void FillWindowRect( CTRect<float> *pRect ) const override {}

	CTRect<float> GetWindowRect() const override
	{
		return CTRect<float>( 0, 0, 0, 0 );
	}

	IWindow *GetParentWindow() const override
	{
		return 0;
	}

	struct IScreen *GetScreen() override
	{
		return 0;
	}

	const struct NDb::SWindowShared *GetSharedDesc() const override
	{
		return 0;
	}

	void RemoveChild( IWindow *_pChild ) override {}

	IWindow *GetChild( const int _nTypeID, const int _nID, const bool bRecursive = false ) override
	{
		return 0;
	}

	const std::string &GetName() const override
	{
		return szFakeName;
	}

	void SetName( const std::string &szName ) override {}

	// help context
	void SetTooltipIDForMLHandler( int nID ) override {}

	int GetTooltipIDForMLHandler() const override
	{
		return -1;
	}

	void SetFocus( const bool bFocus ) override
	{
		return;
	}

	bool IsFocused() const override
	{
		return false;
	}

	void SetFadeValue( float fValue ) override {}

	// methods

	// members
protected:
	std::string szFakeName;
	CTRect<float> pos;
	CDBPtr<NDb::STexture> pTexture;
	uint32_t color;
};

CUIWindow *CreateUIWindow( int x, int y, int w, int h, uint32_t _color, const NDb::STexture *_pTexture );
