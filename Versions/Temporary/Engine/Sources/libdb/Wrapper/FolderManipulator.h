#pragma once
#include "TableManipulator.h"
#include "System/FileUtils.h"

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
	CFolderManipulatorWrapper( const std::string &_szClassTypeName, const std::string &_szSrcPath, const std::string &_szDstPath );
	// IManipulator
	IManipulatorIterator* Iterate( bool bShowHidden, ECacheType eCache );
	const SIteratorDesc* GetDesc( const std::string &szName ) const { return 0; }
	bool GetType( const std::string &szName, std::string *pszType ) const { return false; }
	UINT GetID( const std::string &szName ) const;
	bool GetName( UINT nID, std::string *pszName ) const;
	bool InsertNode( const std::string &szName, int nNodeIndex = NODE_ADD_INDEX );
	bool RemoveNode( const std::string &szName, int nNodeIndex = NODE_REMOVEALL_INDEX );
	bool RemoveNodeByID( const std::string &szName, int nNodeID ) { return false; }
	bool RenameNode( const std::string &szName, const std::string &szNewName );
	bool GetValue( const std::string &szName, CVariant *pValue ) const;
	bool SetValue( const std::string &szName, const CVariant &value );
	bool CheckValue( const std::string &szName, const CVariant &value, bool *pResult ) const { return false; }
	NDb::IObjMan* GetObjMan() { return 0; }
	bool IsNameExists( const std::string &rszName ) const;
	void GetNameList( CNameMap *pNameMap ) const;
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
		//
		bool IsObject() const { return bObject; }
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
	CFolderManipulatorIteratorWrapper( const std::string &szSrcPath, const std::string &szTypeName );
	
	bool Next();
	bool IsEnd() const;
	const SIteratorDesc* GetDesc() const;
	bool GetName( std::string *pszName ) const;
	bool GetType( std::string *pszType ) const;
	UINT GetID() const;
	bool IsFolder() const;
	void Reset();
};


