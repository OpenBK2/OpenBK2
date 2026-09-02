#pragma once
#include <afxcmn.h>
//#include "SyntaxColorizer.h"
#include "LuaEditor.h"

#include "MapEditorLib/ResizeDialog.h"

const int WM_ME_TEXTCHANGED = WM_USER + 5;

// CScriptEditor dialog
class CScriptEditor : public CResizeDialog
{
	DECLARE_DYNAMIC(CScriptEditor)

public:
	CScriptEditor( bool bInitiallySelected = false, CWnd* pParent = NULL);   // standard constructor
	virtual ~CScriptEditor();

	void CheckSyntax( bool bCheck );
// Dialog Data
	enum { IDD = IDD_SCRIPT_EDITOR };
	virtual BOOL OnInitDialog();

	void SetText( const std::string &szText );
	void SetTitle( const std::string &rszTitle ) { szTitle = rszTitle; }
	std::string GetText();
	void SetModal( bool _bModal ) { bModal = _bModal; }
	//CString m_szText;
	CString m_szErrLog;

protected:
	bool bInitiallySelected;
	BOOL bCheckSyntax;
	CFont m_fntDef;
	std::string szInitialText;
	std::string szTitle;
	std::string szLastText;
	bool bFreezeUpdate;
	bool bModal;
	bool bEnableEdit;
	//
	CPoint ptTextTL;
	CPoint ptErrorTL;
	int nSpaceTextError;
	int nErrorHeight;
	CPoint ptOK, ptCancel;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();
	virtual void OnCancel();
	void CheckSyntax();

	// CResizeDialog
	int GetMinimumXDimension() { return 400; }
	int GetMinimumYDimension() { return 300; }
	void GetXMLFilePath( std::string *pszXMLFilePath ) { ( *pszXMLFilePath ) = "CScriptEditor"; }
	bool IsDrawGripper() { return true; }

	DECLARE_MESSAGE_MAP()
public:
	CLuaEditor m_LuaEditor;
	void EnableEdit( bool bEnable );
	afx_msg void OnChar(unsigned nChar, unsigned nRepCnt, unsigned nFlags);
	afx_msg void OnKeyDown(unsigned nChar, unsigned nRepCnt, unsigned nFlags);
	CEdit m_ctrlErrLog;
	afx_msg void OnEnSelchangeEditText(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnChangeEditText();
	afx_msg void OnCnCharAdded(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCnModified(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSize(unsigned nType, int cx, int cy);
	CButton m_ctrlOK;
	CButton m_ctrlCancel;
};



