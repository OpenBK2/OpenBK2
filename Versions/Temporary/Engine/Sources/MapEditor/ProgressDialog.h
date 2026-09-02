#pragma once

#include "ResourceDefines.h"

#include <cstdint>

class CProgressDialog : public CDialog
{
public:
	CProgressDialog( CWnd* pParent = NULL );

	enum { IDD = IDD_PROGRESS_SIMPLE };
	CStatic	m_ProgressLabel;
	CProgressCtrl	m_ProgressBar;

protected:
	static const uint32_t START_TIMER_ID;
	static const uint32_t START_TIMER_INTERVAL;

  uint32_t dwStartTimer;

  void SetStartTimer();
  void KillStartTimer();
  void OnStartTimer();
	
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer( UINT_PTR nIDEvent );
	DECLARE_MESSAGE_MAP()

public:
	void UpdateControls();
	void SetProgressTitle( const std::string &rszProgressTitle );
	void SetProgressMessage( const std::string &rszProgressMessage );
	void SetProgressRange( int nStart, int nFinish );
	void SetProgressPosition( int nPosition );
	void IterateProgressPosition();
};



