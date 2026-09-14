#pragma once

#include "LogView.h"
#include "MapEditorLib/Interface_Logger.h"

#include <cstdint>
#include <string>

// The Log pane's contents and what the pane does with them, apart from the
// pane.
//
// CDWLog kept the log's last thousand lines, filtered them by the three show
// switches, and answered CHID_LOG and the selection commands for its view.
// None of that is about the pane being a Stingray control bar, and the wx main
// frame's Log pane does all of it the same way, so it lives here and each pane
// holds one. The pane makes the view in its own toolkit and hands it over.
class CLogPaneContents : public ICommandHandler
{
	// Owned, once handed over.
	ILogView *pLogView;
	NLog::CLogBufferList logBufferList;

	void UpdateLog();
	void Append( const NLog::SLogBuffer &rLogBuffer );

public:
	// Registers as CHID_LOG, as CDWLog did from its constructor.
	CLogPaneContents();
	virtual ~CLogPaneContents();

	// The view the pane made. It is created by the pane with this as its
	// selection handler.
	void SetView( ILogView *pView );
	ILogView* GetView() const { return pLogView; }

	void Log( ELogOutputType eLogOutputType, const std::string &szText );
	void ClearLog();

	// ICommandHandler: CHID_LOG, and CHID_SELECTION while the view has focus.
	virtual bool HandleCommand( unsigned nCommandID, uintptr_t dwData );
	virtual bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );
};
