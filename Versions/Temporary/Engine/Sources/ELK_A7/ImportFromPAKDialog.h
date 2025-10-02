
#pragma once
#include "../MapEditorLib/ResizeDialog.h"

class CImportFromPAKDialog : public CResizeDialog
{
public:
	CImportFromPAKDialog( CWnd* pParent = NULL );

	enum { IDD = IDD_IMPORT_FROM_PAK };
	CEdit	m_PAKEdit;
	CEdit	m_FileEdit;

protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnFileBrowseButton();
	afx_msg void OnPAKBrowseButton();
	afx_msg void OnChangeFileBrowseEdit();
	afx_msg void OnChangePAKBrowseEdit();
	DECLARE_MESSAGE_MAP()

protected:
	const static int vID[];

	virtual int GetMinimumXDimension() { return 350; }
	virtual int GetMinimumYDimension() { return 125; }
	virtual bool SerializeToRegistry() { return true; }
	virtual void GetRegistryKey( string *pszRegistryKey );
	virtual bool IsDrawGripper() { return true; }

	void UpdateControls();

public:
	void GetPAKPath( string *pszPAKPath );
	void GetFilePath( string *pszFilePath );
};


