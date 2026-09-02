#pragma once

#include "MapEditorLib/Interface_Editor.h"

class CEditorContainer : public IEditorContainer
{
	OBJECT_NOCOPY_METHODS( CEditorContainer );
	//
	typedef std::unordered_map<std::string, CPtr<IEditor> > CEditorMap;  // BaseObjectType -> Editor
	typedef std::unordered_map<std::string, std::string> CExtendTypeMap;			// ExtendObjectType -> BaseObjectType
	CEditorMap editorMap;
	CExtendTypeMap extendTypeMap;
	std::string szActiveTypeName;

	std::string GetBaseObjectType( const std::string &rszExtendObjectTypeName );

	// Методы умеют обрадаться с уже созданными редакторами и ChildFrame
	void DestroyActiveEditor( const std::string &rszNewEditorTypeName, const std::string &rszNewChildFrameTypeName, bool bDestroyChildFrame );
	void CreateNewEditor( IManipulator* _pManipulator, const SObjectSet &rObjectSet, const std::string &rszNewChildFrameTypeName );

public:
	~CEditorContainer() { DestroyActiveEditor( true ); }

	// IEditorContainer
	bool CanCreate( const std::string &rszObjectTypeName );
	//
	void Create( const std::string &rszObjectTypeName );
	void AddExtendObjectType( const std::string &rszBaseObjectTypeName, const std::string &rszExtendObjectTypeName );
	void Destroy( const std::string &rszObjectTypeName, bool bDestroyChildFrame );
	//
	IEditor* Create( IManipulator* _pManipulator, const SObjectSet &rObjectSet );
	void DestroyActiveEditor( bool bDestroyChildFrame );
	void ReloadActiveEditor( bool bClearResources );
	IEditor* GetActiveEditor();
	IInputState *GetActiveInputState();
	//
	void CreateControls();
	void PostCreateControls();
	void PreDestroyControls();
	void DestroyControls();
	//
	void Save( bool bSaveChanges );
	bool IsModified();
};



