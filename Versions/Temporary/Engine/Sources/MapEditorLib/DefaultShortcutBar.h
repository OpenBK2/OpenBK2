
#pragma once
#include "Interface_CommandHandler.h"

#define INVALID_SHORTCUT_INDEX (0xFFFF)

class CDefaultShortcutBar : public SECShortcutBar
{
	vector<CWnd*> shortcutList;
	UINT nCommandHandlerID;
	UINT nCommandID;

protected:
	BOOL OnChangeBar( int nShortcutIndex );
	afx_msg LRESULT OnNotifyChangeTab( WPARAM wParam, LPARAM lParam );

	DECLARE_MESSAGE_MAP()

public:
	CDefaultShortcutBar() : nCommandHandlerID( INVALID_COMMAND_HANDLER_ID ), nCommandID( INVALID_COMMAND_ID )  {}
	virtual ~CDefaultShortcutBar();
	//
	template<class TSHORTCUT>
	TSHORTCUT* AddNewShortcut( TSHORTCUT *pwndCreatedShortcut ) 
	{
		TSHORTCUT *pwndNewShortcut = pwndCreatedShortcut;
		if ( pwndNewShortcut == 0 )
		{
			pwndNewShortcut = new TSHORTCUT();
		}
		shortcutList.push_back( pwndNewShortcut );
		return pwndNewShortcut;
	}
	//
	void RemoveAllShortcuts();
	CWnd* GetShortcutWindow( int nShortcutIndex );
	//
	void SetCommandHandlerID( UINT _nCommandHandlerID, UINT _nCommandID ) { nCommandHandlerID = _nCommandHandlerID; nCommandID = _nCommandID; }
	UINT GetCommandHandlerID() { return nCommandHandlerID; }
	UINT GetCommand() { return nCommandID; }
};


