#pragma once

#include "LogPane.h"

#include <cstdint>

// The Log pane in the MFC frame: a Stingray control bar around the log's
// contents. What the log keeps and the commands it answers are
// CLogPaneContents', shared with the wx frame's Log pane; see LogPane.h.
class CDWLog : public SECControlBar
{
	CLogPaneContents contents;

protected:
	afx_msg int OnCreate( LPCREATESTRUCT pCreateStruct );
	afx_msg void OnSize( unsigned nType, int cx, int cy );

public:
	void Log( ELogOutputType eLogOutputType, const std::string &szText ) { contents.Log( eLogOutputType, szText ); }
	void ClearLog() { contents.ClearLog(); }

	DECLARE_MESSAGE_MAP()
};
