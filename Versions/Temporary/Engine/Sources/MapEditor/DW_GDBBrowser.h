#pragma once

#include "GDBBrowserPane.h"
#include "Empty_GDBBrowser.h"
#include "MapEditorLib/MfcWidget.h"

#include <cstdint>

// A Game Database pane in the MFC frame: a Stingray control bar around the
// browser. What the pane shows and the commands it answers are
// CGDBBrowserContents', shared with the wx frame's panes; see GDBBrowserPane.h.
class CDWGDBBrowser : public SECControlBar, public ICommandHandler, public CGDBBrowserContents::IPane
{
	CGDBBrowserContents contents;
	// Shown instead of the browser while no table is chosen.
	CEmptyGDBBrowser wndEmptyContents;
	// This pane, as what Select Tables opens over.
	CWndWidget ownerWidget;

	virtual BOOL OnGripperClose();

protected:
	afx_msg int OnCreate( LPCREATESTRUCT pCreateStruct );
	afx_msg void OnDestroy();
	afx_msg void OnSize( unsigned nType, int cx, int cy );
	afx_msg void OnLButtonDown( unsigned nFlags, CPoint point );
	afx_msg void OnRButtonDown( unsigned nFlags, CPoint point );
	afx_msg void OnRButtonUp( unsigned nFlags, CPoint point );
	afx_msg LRESULT OnTabSelected( WPARAM wParam, LPARAM lParam );
	afx_msg void OnTabSelected();

public:
	CDWGDBBrowser( int _nGDBBrowserID );
	virtual ~CDWGDBBrowser();

	// What answers CHID_OBJECT_STORAGE while this browser has the focus.
	ICommandHandler *GetContents() { return contents.GetObjectStorage(); }
	int GetDWGDBBrowserID() const { return contents.GetID(); }
	void EnableEdit( bool bEnable ) { contents.EnableEdit( bEnable ); }

	// ICommandHandler: the frame registers the pane as CHID_MAIN; the contents
	// answer.
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData ) { return contents.HandleCommand( nCommandID, dwData ); }
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck ) { return contents.UpdateCommand( nCommandID, pbEnable, pbCheck ); }

	// CGDBBrowserContents::IPane
	virtual void LayoutContents();
	virtual void ShowEmpty( bool bEmpty );
	virtual IWidget* GetOwner() { return &ownerWidget; }

	DECLARE_MESSAGE_MAP()
};
