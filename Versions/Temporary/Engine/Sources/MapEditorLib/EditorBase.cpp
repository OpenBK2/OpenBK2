#include "StdAfx.h"

#include "EditorBase.h"
#include "Interface_MainFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void CEditorBase::SetModified( bool _bModified )
{
	bEditorBaseModified = _bModified;
	if ( bEditorBaseModified )
	{
		SSWTParams swtParams;
		swtParams.dwFlags = SWT_MODIFIED;
		swtParams.bModified = bEditorBaseModified;
		Singleton<IMainFrameContainer>()->Get()->SetWindowTitle( swtParams );
	}
}

// basement storage  


