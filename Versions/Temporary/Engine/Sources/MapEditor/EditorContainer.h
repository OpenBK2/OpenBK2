#if !defined(__EDITOR__CONTAINER__)
#define __EDITOR__CONTAINER__
#pragma once

#include "..\MapEditorLib\Interface_Editor.h"

class CEditorContainer : public IEditorContainer
{
	OBJECT_NOCOPY_METHODS( CEditorContainer );
	//
	typedef hash_map<string, CPtr<IEditor> > CEditorMap;  // BaseObjectType -> Editor
	typedef hash_map<string, string> CExtendTypeMap;			// ExtendObjectType -> BaseObjectType
	CEditorMap editorMap;
	CExtendTypeMap extendTypeMap;
	string szActiveTypeName;

	string GetBaseObjectType( const string &rszExtendObjectTypeName );

	// Методы умеют обрадаться с уже созданными редакторами и ChildFrame
	void DestroyActiveEditor( const string &rszNewEditorTypeName, const string &rszNewChildFrameTypeName, bool bDestroyChildFrame );
	void CreateNewEditor( IManipulator* _pManipulator, const SObjectSet &rObjectSet, const string &rszNewChildFrameTypeName );

public:
	~CEditorContainer() { DestroyActiveEditor( true ); }

	// IEditorContainer
	bool CanCreate( const string &rszObjectTypeName );
	//
	void Create( const string &rszObjectTypeName );
	void AddExtendObjectType( const string &rszBaseObjectTypeName, const string &rszExtendObjectTypeName );
	void Destroy( const string &rszObjectTypeName, bool bDestroyChildFrame );
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

#endif // !defined(__EDITOR__CONTAINER__)


