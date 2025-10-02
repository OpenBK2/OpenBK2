#if !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_DB_LINK_DIALOG__)
#define __COMMON_CONTROLS__PROPERTY_CONTROL_DB_LINK_DIALOG__
#pragma once

#include "pc_dialog.h"
#include "PC_MultilineStringEditor.h"
#include "ComboBox_GDBBrowser.h"
#include "../MapEditorLib/Interface_UserData.h" //CTableSet

#define  PC_DBL_TREE_COLUMN_COUNT (3)

class CPCDBLinkDialog : public CResizeDialog, public CPCBaseDialog
{
	static const UINT	PC_DBL_TREE_COLUMN_NAME  [PC_DBL_TREE_COLUMN_COUNT];
	static const int	PC_DBL_TREE_COLUMN_FORMAT[PC_DBL_TREE_COLUMN_COUNT];
	static const int	PC_DBL_TREE_COLUMN_WIDTH [PC_DBL_TREE_COLUMN_COUNT];

	bool bCreateControls;
	ICommandHandler *pPreviousCommandHandler;

public:
	enum EType
	{
		TYPE_OPEN = 0,
		TYPE_LINK = 1
	};

private:
	EType eType;
	bool bEmpty;
	bool bMultiRef;
	bool bTextEditor;
	int nFixedWidth;
	int nFixedHeight;
	//
	CImageList typesImageList;
	CImageList headerImageList;
	//
	CPCMainTreeControl tree;
	//C3DTabGDBBrowser tab;
	CComboBoxGDBBrowser tab;
	CStatic	wndTreeStatusStringWindow;
	CStatic	wndEditorStatusStringWindow;
	CPtr<CPCMultilineStringEditor> pPCMultilineStringEditor;
	//
	CTableSet selectedTables;
	list<string> tables;
	string szPreviousTable;
	string szCurrentTable;
	string szPreviousObject;
	string szCurrentObject;
	//
	void InitImageLists();
	void UpdateDialogTitle();
	void UpdateCurrentObjectLabel();
	void UpdatePreviousObjectLabel();
	void UpdateOK();
	
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog();
	//
	afx_msg void OnDestroy();
	//afx_msg void OnSelchangedTree( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg LRESULT OnMessageTreeGDBBrowser( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnTabSelected( WPARAM wParam, LPARAM lParam );
	afx_msg void OnTabSelected();
	afx_msg void OnBnClickedSetEmptyButton();
	afx_msg void OnSize( UINT nType, int cx, int cy );
	//
	// CResizeDialog
	int GetMinimumXDimension() { return 400; }
	int GetMinimumYDimension() { return 300; }
	void GetXMLFilePath( string *pszXMLFilePath ) { ( *pszXMLFilePath ) = "CPCDBLinkDialog"; }
	bool IsDrawGripper() { return true; }

public:
	CPCDBLinkDialog( EType _eType, bool _bMultiRef, bool _bTextEditor = false, int _nFixedWidth = 0, int _nFixedHeight = 0, CWnd* pParent = NULL );
	//
	void SetSelectedTables( const CTableSet &rSelectedTables );
	void SetCurrentTable( const string &rszCurrentTable );
	void SetCurrentObject( const string &rszCurrentObject );
	//
	void GetCurrentTable( string *pszCurrentTable );
	void GetCurrentObject( string *pszCurrentObject );
	bool IsEmpty() { return bEmpty; }

	//CPCBaseDialog
	IView* GetView();
	ICommandHandler* GetCommandHandler();
	void CreateTree();
	void UpdateValues();
	void EnableEdit( bool bEnable ) { tab.EnableEdit( bEnable ); tree.EnableEdit( bEnable ); }

	DECLARE_MESSAGE_MAP()
};

#endif // !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_DB_LINK_DIALOG__)

