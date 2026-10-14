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
	void ClearUndoData() override
	{
		undoDataList.clear();
	}

	void UndoChanges() override;
	void LockObjects( const SObjectSet &rObjectSet ) override;
	void UnockObjects( const SObjectSet &rObjectSet ) override;
	bool IsObjectLocked( const std::string &rszTypeName, const CDBID &rDBID ) const override;
	bool IsUniqueName( const std::string &rszTypeName, const std::string &rszName ) override;
	bool UniqueName( const std::string &szTypeName, std::string *pszName ) override;
	bool InsertObject( const std::string &rszObjectTypeName, const std::string &rszObjectName ) override;
	bool CopyObject( const std::string &rszObjectTypeName,
	                 const std::string &rszDestination,
	                 const std::string &rszSource ) override;
	bool RenameObject( const std::string &rszObjectTypeName,
	                   const std::string &rszDestination,
	                   const std::string &rszSource ) override;
	bool RemoveObject( const std::string &rszObjectTypeName, const std::string &rszObjectName, bool bRecursive ) override;
	bool SetColor( const std::string &rszObjectTypeName, const std::string &rszObjectName, const int nNewColor ) override;
};
