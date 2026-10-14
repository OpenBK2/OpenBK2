#pragma once
#include "System/FileUtils.h"
#include "TableManipulator.h"

class CFolderManipulatorWrapper : public IManipulator
{
	OBJECT_NOCOPY_METHODS( CFolderManipulatorWrapper );
	//
	std::string szClassTypeName;
	std::string szSrcPath;
	std::string szDstPath;

protected:
	CFolderManipulatorWrapper() {}

public:
	CFolderManipulatorWrapper( const std::string &_szClassTypeName,
	                           const std::string &_szSrcPath,
	                           const std::string &_szDstPath );
	// IManipulator
	IManipulatorIterator *Iterate( bool bShowHidden, ECacheType eCache ) override;

	const SIteratorDesc *GetDesc( const std::string &szName ) const override
	{
		return 0;
	}

	bool GetType( const std::string &szName, std::string *pszType ) const override
	{
		return false;
	}

	unsigned GetID( const std::string &szName ) const override;
	bool GetName( unsigned nID, std::string *pszName ) const override;
	bool InsertNode( const std::string &szName, int nNodeIndex = NODE_ADD_INDEX ) override;
	bool RemoveNode( const std::string &szName, int nNodeIndex = NODE_REMOVEALL_INDEX ) override;

	bool RemoveNodeByID( const std::string &szName, int nNodeID ) override
	{
		return false;
	}

	bool RenameNode( const std::string &szName, const std::string &szNewName ) override;
	bool GetValue( const std::string &szName, CVariant *pValue ) const override;
	bool SetValue( const std::string &szName, const CVariant &value ) override;

	bool CheckValue( const std::string &szName, const CVariant &value, bool *pResult ) const override
	{
		return false;
	}

	NDb::IObjMan *GetObjMan() override
	{
		return 0;
	}

	bool IsNameExists( const std::string &rszName ) const override;
	void GetNameList( CNameMap *pNameMap ) const override;
};

class CFolderManipulatorIteratorWrapper : public IManipulatorIterator
{
	OBJECT_NOCOPY_METHODS( CFolderManipulatorIteratorWrapper );
	//
	NFile::CFileIterator fileIterator;
	std::string szBasePath;

	//
	struct SEntry
	{
		CDBID dbid;
		bool bObject;
		std::string szFolderName; // Physical empty folders have no DB resource ID.

		//
		bool IsObject() const
		{
			return bObject;
		}
	};

	//
	typedef std::vector<SEntry> CEntriesList;
	CEntriesList entriesList;
	CEntriesList::const_iterator posCurrElement;
	std::string szClassTypeName;
	//
protected:
	CFolderManipulatorIteratorWrapper() {}

public:
	CFolderManipulatorIteratorWrapper( const std::string &szSrcPath,
	                                   const std::string &szDstPath,
	                                   const std::string &szTypeName );

	bool Next() override;
	bool IsEnd() const override;
	const SIteratorDesc *GetDesc() const override;
	bool GetName( std::string *pszName ) const override;
	bool GetType( std::string *pszType ) const override;
	unsigned GetID() const override;
	bool IsFolder() const override;
	void Reset();
};
