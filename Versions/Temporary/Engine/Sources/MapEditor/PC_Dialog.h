#pragma once

#include "ResourceDefines.h"

#include "PC_BaseDialog.h"
#include "../MapEditorLib/ResizeDialog.h"
#include "PC_MainTreeControl.h"

#define PC_TREE_COLUMN_COUNT (3)

class CPCDialog : public CResizeDialog, public CPCBaseDialog
{
	static const UINT	PC_TREE_COLUMN_NAME  [PC_TREE_COLUMN_COUNT];
	static const int	PC_TREE_COLUMN_FORMAT[PC_TREE_COLUMN_COUNT];
	static const int	PC_TREE_COLUMN_WIDTH [PC_TREE_COLUMN_COUNT];

	bool bCreateControls;
	ICommandHandler *pPreviousCommandHandler;
	string szXMLFileLabel;
	//
	CImageList typesImageList;
	CImageList headerImageList;
	//
	CPCMainTreeControl tree;
	CStatic	wndTreeStatusStringWindow;
	//
	void InitImageLists();
	//void SetStatusBarMessage( const string &rszMessage );
	
protected:
	//virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam );
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog();
	//
	afx_msg void OnDestroy();
	//afx_msg void OnSelchangedTree( NMHDR* pNMHDR, LRESULT* pResult );
	//
	// CResizeDialog
	void GetXMLFilePath( string *pszXMLFilePath ) { ( *pszXMLFilePath ) = szXMLFileLabel; }
	bool IsRestoreSize() { return false; }

public:
	enum { IDD = IDD_PC };
	//
	CPCDialog( CWnd* pParent = NULL );
	//
	void SetXMLOptionsLabel(const string &rszXMLFileLabel ) { szXMLFileLabel = rszXMLFileLabel; }
	
	//CPCBaseDialog
	IView* GetView();
	ICommandHandler* GetCommandHandler();
	void CreateTree();
	void UpdateValues();
	void EnableEdit( bool bEnable ) { tree.EnableEdit( bEnable ); }
	DECLARE_MESSAGE_MAP()
};


