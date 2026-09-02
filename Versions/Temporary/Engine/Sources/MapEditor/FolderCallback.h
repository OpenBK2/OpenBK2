#pragma once

#include "MapEditorLib/Interface_FolderCallback.h"


class CFolderCallback : public IFolderCallback
{
	OBJECT_NOCOPY_METHODS( CFolderCallback );

	struct SUndoData
	{
		std::string szObjectTypeName;
		std::string szObjectName;
	};
	typedef std::list<SUndoData> CUndoDataList;
	CUndoDataList undoDataList;

	typedef std::unordered_map<std::string, CObjectNameSet> CLockObjectMap;
	CLockObjectMap lockObjectMap;

protected:
	// IFolderCallback
	void ClearUndoData() { undoDataList.clear(); }
	void UndoChanges();
	void LockObjects( const SObjectSet &rObjectSet );
	void UnockObjects( const SObjectSet &rObjectSet );
	bool IsObjectLocked( const std::string &rszTypeName, const CDBID &rDBID ) const;
	bool IsUniqueName( const std::string &rszTypeName, const std::string &rszName );
	bool UniqueName( const std::string &szTypeName, std::string *pszName );
	bool InsertObject( const std::string &rszObjectTypeName, const std::string &rszObjectName );
	bool CopyObject( const std::string &rszObjectTypeName, const std::string &rszDestination, const std::string &rszSource );
	bool RenameObject( const std::string &rszObjectTypeName, const std::string &rszDestination, const std::string &rszSource );
	bool RemoveObject( const std::string &rszObjectTypeName, const std::string &rszObjectName, bool bRecursive );
	bool SetColor( const std::string &rszObjectTypeName, const std::string &rszObjectName, const int nNewColor );
};



