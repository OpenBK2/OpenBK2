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



