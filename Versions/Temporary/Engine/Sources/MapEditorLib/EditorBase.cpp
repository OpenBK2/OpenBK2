#include "stdafx.h"

#include "EditorBase.h"
#include "Interface_MainFrame.h"

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


