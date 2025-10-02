#if !defined(__CONTROLLER__FOLDER_CALLBACK__)
#define __CONTROLLER__FOLDER_CALLBACK__
#pragma once

#include "..\MapEditorLib\Interface_FolderCallback.h"


class CFolderCallback : public IFolderCallback
{
	OBJECT_NOCOPY_METHODS( CFolderCallback );

	struct SUndoData
	{
		string szObjectTypeName;
		string szObjectName;
	};
	typedef list<SUndoData> CUndoDataList;
	CUndoDataList undoDataList;

	typedef hash_map<string, CObjectNameSet> CLockObjectMap;
	CLockObjectMap lockObjectMap;

protected:
	// IFolderCallback
	void ClearUndoData() { undoDataList.clear(); }
	void UndoChanges();
	void LockObjects( const SObjectSet &rObjectSet );
	void UnockObjects( const SObjectSet &rObjectSet );
	bool IsObjectLocked( const string &rszTypeName, const CDBID &rDBID ) const;
	bool IsUniqueName( const string &rszTypeName, const string &rszName );
	bool UniqueName( const string &szTypeName, string *pszName );
	bool InsertObject( const string &rszObjectTypeName, const string &rszObjectName );
	bool CopyObject( const string &rszObjectTypeName, const string &rszDestination, const string &rszSource );
	bool RenameObject( const string &rszObjectTypeName, const string &rszDestination, const string &rszSource );
	bool RemoveObject( const string &rszObjectTypeName, const string &rszObjectName, bool bRecursive );
	bool SetColor( const string &rszObjectTypeName, const string &rszObjectName, const int nNewColor );
};

#endif // !defined(__CONTROLLER__FOLDER_CALLBACK__)


