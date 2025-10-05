// WindowTextView.h: interface for the CWindowTextView class.
//



#pragma once
#include "UI_export.h"

#include "Window.h"

struct ISound;

// plain text window
// user gives Font, Color, String, Alignment,
// and Window parameters
class UI_EXPORT CWindowTextView : public CWindow, public ITextView
{
	OBJECT_BASIC_METHODS(CWindowTextView);
	
	CPtr<NDb::SWindowTextView> pInstance;
	CDBPtr<NDb::SWindowTextViewShared> pShared;

	CObj<IML> pGfxText;								// text to display
	std::wstring wszCustomText;
	CVec2 vScreenRect;
	int nIDForMLHandler;
private:	
	bool InitText();
	//{ overrided
	const std::wstring& GetDBFormatText() const;
	const std::wstring& GetDBInstanceText() const;
	//}
protected:
	virtual NDb::SWindow* GetInstance() { return pInstance; }

public:
	CWindowTextView() : vScreenRect( VNULL2  ), nIDForMLHandler( -1 ) {}

	virtual void Visit( struct IUIVisitor *pVisitor );
	virtual int operator&( struct IBinSaver &saver );
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc );

	virtual std::wstring GetDBText() const;
	virtual const std::wstring& GetText() const;
	// return true if height of window is updated
	virtual bool SetText( const std::wstring &szText );
	virtual void SetWidth( const int nWidth );

	// do nothing (use SetText & GetText)
	void SetTextString( const std::wstring &szText ) {}

	virtual const CTPoint<int> GetSize() const;

	virtual void SetPlacement( const float x, const float y, const float sizeX, const float sizeY, const DWORD flags );
	virtual void Reposition( const CTRect<float> &parentRect );
	virtual void Init()
	{
		CWindow::Init();
		InitText();
	}
	void SetIDForMLHandler( int nID );
	int GetIDForMLHandler() const { return nIDForMLHandler; }
};


