#pragma once

#include "MapEditorLib/ResizeDialog.h"
#include "ResourceDefines.h"

#include <cstdint>

class CProgressDlg : public CResizeDialog
{
private:
	std::string szActionName;
	CStatic	m_ProgressLabel;
	CProgressCtrl	m_ProgressBar;
	static const uint32_t START_TIMER_ID;
	static const uint32_t START_TIMER_INTERVAL;
	//
	uint32_t dwStartTimer;

protected:
	virtual BOOL OnInitDialog();
	//
	//virtual void OnOK();
	//virtual void OnCancel();
	virtual void DoDataExchange( CDataExchange *pDX );
	//
	void SetStartTimer();
	void KillStartTimer();
	void OnStartTimer();

	// CResizeDialog
	void GetXMLFilePath( std::string *pszXMLFilePath ) { (*pszXMLFilePath) = "CProgressDlg"; }
	int GetMinimumXDimension() { return 300; }
	int GetMinimumYDimension() { return 125; }
	bool IsDrawGripper() { return true; }
	//
	afx_msg void OnDestroy();
	afx_msg void OnClearAll();
	afx_msg void OnTimer( unsigned nIDEvent );
	DECLARE_MESSAGE_MAP()

	void UpdateDialog();

public:
	enum { IDD = IDD_PROGRESS };
	//
	CProgressDlg( const std::string &rszActionName, CWnd *pParentWindow = 0 );
	virtual ~CProgressDlg() {}

	bool Create( CWnd *pParentWindow );

	void SetProgressRange( int nStart, int nFinish );
	void SetProgressPosition( int nPosition );
	void IterateProgressPosition();
	//
	void ClearLog();
	void AddLog( const std::string &rszLogMessage );
	//
	bool WasCancelled() const { return false; }
};



