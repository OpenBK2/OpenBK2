// WindowTextView.h: interface for the CWindowTextView class.
//

#pragma once

#include "Window.h"

#include "UI_export.h"

#include <cstdint>

struct ISound;

// plain text window
// user gives Font, Color, String, Alignment,
// and Window parameters
class UI_EXPORT CWindowTextView : public CWindow, public ITextView
{
	OBJECT_BASIC_METHODS( CWindowTextView );

	CPtr<NDb::SWindowTextView> pInstance;
	CDBPtr<NDb::SWindowTextViewShared> pShared;

	CObj<IML> pGfxText; // text to display
	std::wstring wszCustomText;
	CVec2 vScreenRect;
	int nIDForMLHandler;
	// Track control sizing separately: GetSize can refresh the markup before
	// Visit has updated a ResizeOnTextSet window's height.
	float fFontScale = 0;

private:
	bool InitText();
	//{ overrided
	const std::wstring &GetDBFormatText() const override;
	const std::wstring &GetDBInstanceText() const override;
	//}
protected:
	NDb::SWindow *GetInstance() override
	{
		return pInstance;
	}

public:
	CWindowTextView()
		: vScreenRect( VNULL2 )
		, nIDForMLHandler( -1 )
	{
	}

	void Visit( struct IUIVisitor *pVisitor ) override;
	int operator&( struct IBinSaver &saver ) override;
	void InitByDesc( const struct NDb::SUIDesc *pDesc ) override;

	std::wstring GetDBText() const override;
	const std::wstring &GetText() const override;
	// return true if height of window is updated
	bool SetText( const std::wstring &szText ) override;
	void SetWidth( const int nWidth ) override;

	// do nothing (use SetText & GetText)
	void SetTextString( const std::wstring &szText ) override {}

	const CTPoint<int> GetSize() const override;

	void
	SetPlacement( const float x, const float y, const float sizeX, const float sizeY, const uint32_t flags ) override;
	void Reposition( const CTRect<float> &parentRect ) override;

	void Init() override
	{
		CWindow::Init();
		InitText();
	}

	void SetIDForMLHandler( int nID ) override;

	int GetIDForMLHandler() const override
	{
		return nIDForMLHandler;
	}
};
