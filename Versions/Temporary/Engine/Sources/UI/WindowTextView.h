// WindowTextView.h: interface for the CWindowTextView class.
//


#if !defined(AFX_WINDOWTEXTVIEW_H__1660DBF3_B2C3_40F5_B322_906F49DC1A41__INCLUDED_)
#define AFX_WINDOWTEXTVIEW_H__1660DBF3_B2C3_40F5_B322_906F49DC1A41__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "UI_export.h"

#include "Window.h"

interface IML;

// plain text window
// user gives Font, Color, String, Alignment,
// and Window parameters
class UI_EXPORT CWindowTextView : public CWindow, public ITextView
{
	OBJECT_BASIC_METHODS(CWindowTextView);
	
	CPtr<NDb::SWindowTextView> pInstance;
	CDBPtr<NDb::SWindowTextViewShared> pShared;

	CObj<IML> pGfxText;								// text to display
	wstring wszCustomText;
	CVec2 vScreenRect;
	int nIDForMLHandler;
private:	
	bool InitText();
	//{ overrided
	const wstring& GetDBFormatText() const;
	const wstring& GetDBInstanceText() const;
	//}
protected:
	virtual NDb::SWindow* GetInstance() { return pInstance; }

public:
	CWindowTextView() : vScreenRect( VNULL2  ), nIDForMLHandler( -1 ) {}

	virtual void Visit( interface IUIVisitor *pVisitor );
	virtual int operator&( interface IBinSaver &saver );
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc );

	virtual wstring GetDBText() const;
	virtual const wstring& GetText() const;
	// return true if height of window is updated
	virtual bool SetText( const wstring &szText );
	virtual void SetWidth( const int nWidth );

	// do nothing (use SetText & GetText)
	void SetTextString( const wstring &szText ) {}

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

#endif // !defined(AFX_WINDOWTEXTVIEW_H__1660DBF3_B2C3_40F5_B322_906F49DC1A41__INCLUDED_)
