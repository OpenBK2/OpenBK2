#if !defined(__EDITOR_BASE__)
#define __EDITOR_BASE__
#pragma once

#include "Interface_Editor.h"

class CEditorBase : public IEditor
{
	bool bEditorBaseModified;	

public:
	CEditorBase() : bEditorBaseModified( false ) {}
	
	virtual void Save( bool bSaveChanges ) { SetModified( false ); }
	virtual bool IsModified() { return bEditorBaseModified; }
	virtual void SetModified( bool _bModified );
	virtual bool ShowProgress() { return true; }
};

#endif // !defined(__EDITOR_BASE__)


