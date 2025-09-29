#if !defined( __UNIT_START_CMD_WINDOW__ )
#define __UNIT_START_CMD_WINDOW__
#pragma once

#include "../MapEditorLib/ResizeDialog.h"
#include "ResourceDefines.h"
#include "../MapEditorLib/Interface_CommandHandler.h"
#include "StringResources.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//		UNITS START COMMANDS WINDOW DATA
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SUnitStartCmdWindowData
{
	//
	struct SCmd // данные для создания row в списке команд
	{
		int nIndex;					// индекс команды в MapInfo.startCommandList[]
		string szType;			// тип команды
		string szTarget;	// местоназначение команды - имя юнита или координаты
		//
		SCmd()
			:	nIndex(-1),
			szType( RCSTR("<UNKNOWN>") )
		{
		}
		bool operator== ( const SCmd &cmd ) const { return ( this->nIndex == cmd.nIndex ); }
	};
	vector<SCmd> commands;
	//
	enum EAction
	{
		NO_CMD,
		ADD_CMD,
		DEL_CMD,
		EDIT_CMD,
		ORDER_DOWN_CMD,
		ORDER_UP_CMD,
		SEL_CHANGE
	};
	EAction eLastAction;							// последнее событие интерфейса пользователя
	vector<int> selectedCommands;		// SCmd::nIndex выбранных в списке команд
	//
	SUnitStartCmdWindowData() 
	{
		Clear();
	}
	//
	void Clear()
	{
		eLastAction = NO_CMD; 
		selectedCommands.clear();
		commands.clear();
	}
	//
	void SetLastAction( EAction eAction )
	{
		eLastAction = eAction;
	}
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//		UNITS START COMMANDS WINDOW
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CUnitStartCmdWindow : public CResizeDialog, public ICommandHandler
{
	// controls
	CButton btnAdd;
	CButton btnDel;
	CButton btnUp;
	CButton btnDown;
	CListCtrl lcCommands;
	//
	bool bIsDataBeginSet;
	SUnitStartCmdWindowData::EAction eLastAction;

	// CResizeDialog
	DECLARE_RESIZE_DLG_WND_COMMON_METHODS( CUnitStartCmdWindow )

	// CUnitStartCmdWindow
	void GetDialogData( SUnitStartCmdWindowData *pData );
	void SetDialogData( const SUnitStartCmdWindowData *pData );
	void NotifyHandler();
	void NotifyHandler( SUnitStartCmdWindowData::EAction eAction );

	void SetLastAction( SUnitStartCmdWindowData::EAction eAction ) { eLastAction = eAction; }

public:
	enum { IDD = IDD_TAB_MI_UNIT_START_CMD };

	CUnitStartCmdWindow( CWnd *pParentWindow = 0 );
	~CUnitStartCmdWindow();
	//
	virtual void DoDataExchange( CDataExchange *pDX );
	virtual BOOL OnInitDialog();
	//
	// ICommandHandler
	virtual bool HandleCommand( UINT nCommandID, DWORD dwData );
	virtual bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
	//
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonDel();
	afx_msg void OnButtonUp();
	afx_msg void OnButtonDown();
	afx_msg void OnLvnItemchangedListUnitCmd(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkListUnitCmd(NMHDR *pNMHDR, LRESULT *pResult);
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // #if !defined( __SCRIPT_AREA_WINDOW__ )
