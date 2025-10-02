
#pragma once
#include "Interface_CommandHandler.h"

#define INVALID_TAB_INDEX (0xFFFF)

class CDefault3DTabWindow : public SEC3DTabWnd
{
	vector<CWnd*> tabList;
	UINT nCommandHandlerID;
	UINT nCommandID;

protected:
	virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam );
	void OnNotifyChangeTab( WPARAM wParam, LPARAM lParam );

	DECLARE_MESSAGE_MAP()

public:
	CDefault3DTabWindow() : nCommandHandlerID( INVALID_COMMAND_HANDLER_ID ), nCommandID( INVALID_COMMAND_ID ) {}
	virtual ~CDefault3DTabWindow();
	//
	template<class TTAB>
	TTAB* AddNewTab( TTAB *pwndCreatedTab ) 
	{
		TTAB *pwndNewTab = pwndCreatedTab;
		if ( pwndNewTab == 0 )
		{
			pwndNewTab = new TTAB();
		}
		tabList.push_back( pwndNewTab );
		return pwndNewTab;
	}
	//
	void RemoveAllTabs();
	CWnd* GetTabWindow( int nTabIndex );

	void SetCommandHandlerID( UINT _nCommandHandlerID, UINT _nCommandID ) { nCommandHandlerID = _nCommandHandlerID; nCommandID = _nCommandID; }
	UINT GetCommandHandlerID() { return nCommandHandlerID; }
	UINT GetCommand() { return nCommandID; }
};


