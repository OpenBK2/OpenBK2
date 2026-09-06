#pragma once

#include "MapEditorLib/Interface_Logger.h"
#include "LogView.h"

#include <cstdint>

class CDWLog : public SECControlBar, public ICommandHandler
{
	// Owned. Which implementation it is comes from NLogView::Create, and is a
	// run-time choice so the Scintilla and wx ones can be compared in the same
	// build. The pane itself never learns which it got.
	ILogView *pLogView;
	NLog::CLogBufferList logBufferList;

	void UpdateLog();
	void Append( const NLog::SLogBuffer &rLogBuffer );

protected:
	afx_msg int OnCreate( LPCREATESTRUCT pCreateStruct );
	afx_msg void OnSize( unsigned nType, int cx, int cy );

public:
	CDWLog();
	virtual ~CDWLog();

	void Log( ELogOutputType eLogOutputType, const std::string &szText );
	void ClearLog();

	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );

	DECLARE_MESSAGE_MAP()
};


