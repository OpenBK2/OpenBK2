#pragma once

#include "Interface_Editor.h"

#include "MapEditorLib_export.h"

class MAPEDITORLIB_EXPORT CEditorBase : public IEditor
{
	bool bEditorBaseModified;

public:
	CEditorBase()
		: bEditorBaseModified( false )
	{
	}

	void Save( bool bSaveChanges ) override
	{
		SetModified( false );
	}

	bool IsModified() override
	{
		return bEditorBaseModified;
	}

	void SetModified( bool _bModified ) override;

	bool ShowProgress() override
	{
		return true;
	}
};
