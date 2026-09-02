#pragma once

#include "MapEditorLib/Interface_Logger.h"
#include "LogWindow.h"

#include <cstdint>

class CDWLog : public SECControlBar, public ICommandHandler
{
	CLogWindow wndContents;
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


