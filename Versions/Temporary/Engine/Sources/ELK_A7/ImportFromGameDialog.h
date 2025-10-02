
#pragma once
#include "MapEditorLib/ResizeDialog.h"

class CImportFromGameDialog : public CResizeDialog
{
public:
	CImportFromGameDialog( CWnd* pParent = NULL );

	enum { IDD = IDD_IMPORT_FROM_GAME };
	CEdit	m_FolderEdit;
	CEdit	m_FileEdit;

protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog();
	afx_msg void OnFileBrowseButton();
	afx_msg void OnFolderBrowseButton();
	afx_msg void OnChangeFileBrowseEdit();
	afx_msg void OnChangeFolderBrowseEdit();
	virtual void OnOK();
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
	void GetGamePath( string *pszGamePath );
	void GetFilePath( string *pszFilePath );
};

