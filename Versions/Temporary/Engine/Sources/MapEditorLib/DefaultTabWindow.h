
#pragma once
#include "Interface_CommandHandler.h"

#include "MapEditorLib_export.h"
#define INVALID_TAB_INDEX (0xFFFF)

class MAPEDITORLIB_EXPORT CDefault3DTabWindow : public SEC3DTabWnd
{
	std::vector<CWnd*> tabList;
	unsigned nCommandHandlerID;
	unsigned nCommandID;

protected:
	virtual LRESULT WindowProc( unsigned message, WPARAM wParam, LPARAM lParam );
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

	void SetCommandHandlerID( unsigned _nCommandHandlerID, unsigned _nCommandID ) { nCommandHandlerID = _nCommandHandlerID; nCommandID = _nCommandID; }
	unsigned GetCommandHandlerID() { return nCommandHandlerID; }
	unsigned GetCommand() { return nCommandID; }
};


