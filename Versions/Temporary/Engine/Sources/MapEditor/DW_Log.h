#if !defined(__DOCKING_WINDOW__LOG__)
#define __DOCKING_WINDOW__LOG__

#include "..\MapEditorLib\Interface_Logger.h"
#include "LogWindow.h"


class CDWLog : public SECControlBar, public ICommandHandler
{
	CLogWindow wndContents;
	NLog::CLogBufferList logBufferList;

	void UpdateLog();
	void Append( const NLog::SLogBuffer &rLogBuffer );

protected:
	afx_msg int OnCreate( LPCREATESTRUCT pCreateStruct );
	afx_msg void OnSize( UINT nType, int cx, int cy );

public:
	CDWLog();
	virtual ~CDWLog();

	void Log( ELogOutputType eLogOutputType, const string &szText );
	void ClearLog();

	// ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );

	DECLARE_MESSAGE_MAP()
};

#endif // !defined(__DOCKING_WINDOW__LOG__)
