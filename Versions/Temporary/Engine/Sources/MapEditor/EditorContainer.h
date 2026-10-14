#pragma once

#include "MapEditorLib/Interface_Editor.h"

class CEditorContainer : public IEditorContainer
{
	OBJECT_NOCOPY_METHODS( CEditorContainer );
	//
	typedef std::unordered_map<std::string, CPtr<IEditor>> CEditorMap;   // BaseObjectType -> Editor
	typedef std::unordered_map<std::string, std::string> CExtendTypeMap; // ExtendObjectType -> BaseObjectType
	CEditorMap editorMap;
	CExtendTypeMap extendTypeMap;
	std::string szActiveTypeName;

	std::string GetBaseObjectType( const std::string &rszExtendObjectTypeName );

	// Методы умеют обрадаться с уже созданными редакторами и ChildFrame
	void DestroyActiveEditor( const std::string &rszNewEditorTypeName,
	                          const std::string &rszNewChildFrameTypeName,
	                          bool bDestroyChildFrame );
	void CreateNewEditor( IManipulator *_pManipulator,
	                      const SObjectSet &rObjectSet,
	                      const std::string &rszNewChildFrameTypeName );

public:
	~CEditorContainer() override
	{
		DestroyActiveEditor( true );
	}

	// IEditorContainer
	bool CanCreate( const std::string &rszObjectTypeName ) override;
	//
	void Create( const std::string &rszObjectTypeName ) override;
	void AddExtendObjectType( const std::string &rszBaseObjectTypeName,
	                          const std::string &rszExtendObjectTypeName ) override;
	void Destroy( const std::string &rszObjectTypeName, bool bDestroyChildFrame ) override;
	//
	IEditor *Create( IManipulator *_pManipulator, const SObjectSet &rObjectSet ) override;
	void DestroyActiveEditor( bool bDestroyChildFrame ) override;
	void ReloadActiveEditor( bool bClearResources ) override;
	IEditor *GetActiveEditor() override;
	IInputState *GetActiveInputState() override;
	//
	void CreateControls() override;
	void PostCreateControls() override;
	void ResetGUI() override;
	void PreDestroyControls() override;
	void DestroyControls() override;
	//
	void Save( bool bSaveChanges ) override;
	bool IsModified() override;
};
