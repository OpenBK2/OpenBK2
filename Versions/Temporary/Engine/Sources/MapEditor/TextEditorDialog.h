#pragma once

#include "ResourceDefines.h"
#include "MapEditorLib/Tools_MnemonicsCollector.h"
#include "MapEditorLib/ResizeDialog.h"
#include "MapEditorLib/ScintillaEditor.h"


#define TE_XML_FILE_NAME_LABEL_PREFIX "CTextEditorDialog"


enum ETextEditorType
{
	TE_UNKNOWN	= 0,
	TE_LUA			= 1,
	TE_COUNT		= 2,
};


class CTEMnemonics : public CMnemonicsCollector<int>
{
	public:
	CTEMnemonics();
	ETextEditorType Get( const string &rszMnemonic );
};


extern CTEMnemonics typeTEMnemonics;


class CTextEditorDialog : public CResizeDialog
{
	enum { IDD = IDD_TEXT_EDITOR };
	//
	bool bCreateControls;
	//CToolBar toolBar;
	CScintillaEditorWindow scintillaEditorWindow;
	CStatic	wndEditorStatusStringWindow;

	//
	ETextEditorType editorType;
	string szText;
	string szTitle;
	bool bEnableEdit;
	//
	//void UpdateToolBar();

protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog();
	//
	void OnOK();
	void OnCancel();

	// CResizeDialog
	int GetMinimumXDimension() { return 250; }
	int GetMinimumYDimension() { return 180; }
	void GetXMLFilePath( string *pszXMLFilePath );
	bool IsDrawGripper() { return true; }

public:
	CTextEditorDialog( CWnd* pParent = NULL );
	//
	void SetTitle( const string &rszTitle ) { szTitle = rszTitle; }
	//
	void SetType( ETextEditorType _editorType ) { editorType = _editorType; }
	//
	void SetText( const string &rszText ) { szText = rszText; }
	void GetText( string *pszText );
	//
	void EnableEdit( bool bEnable ) { bEnableEdit = bEnable; }

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}

