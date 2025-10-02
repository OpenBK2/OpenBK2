#pragma once
#include "TableManipulator.h"
#include "System/FileUtils.h"

class CFolderManipulatorWrapper : public IManipulator
{
	OBJECT_NOCOPY_METHODS( CFolderManipulatorWrapper );
	//
	string szClassTypeName;
	string szSrcPath;
	string szDstPath;
protected:
	CFolderManipulatorWrapper() {}
public:
	CFolderManipulatorWrapper( const string &_szClassTypeName, const string &_szSrcPath, const string &_szDstPath );
	// IManipulator
	IManipulatorIterator* Iterate( bool bShowHidden, ECacheType eCache );
	const SIteratorDesc* GetDesc( const string &szName ) const { return 0; }
	bool GetType( const string &szName, string *pszType ) const { return false; }
	UINT GetID( const string &szName ) const;
	bool GetName( UINT nID, string *pszName ) const;
	bool InsertNode( const string &szName, int nNodeIndex = NODE_ADD_INDEX );
	bool RemoveNode( const string &szName, int nNodeIndex = NODE_REMOVEALL_INDEX );
	bool RemoveNodeByID( const string &szName, int nNodeID ) { return false; }
	bool RenameNode( const string &szName, const string &szNewName );
	bool GetValue( const string &szName, CVariant *pValue ) const;
	bool SetValue( const string &szName, const CVariant &value );
	bool CheckValue( const string &szName, const CVariant &value, bool *pResult ) const { return false; }
	NDb::IObjMan* GetObjMan() { return 0; }
	bool IsNameExists( const string &rszName ) const;
	void GetNameList( CNameMap *pNameMap ) const;
};

class CFolderManipulatorIteratorWrapper : public IManipulatorIterator
{
	OBJECT_NOCOPY_METHODS( CFolderManipulatorIteratorWrapper );
	//
	NFile::CFileIterator fileIterator;
	string szBasePath;
	//
	struct SEntry
	{
		CDBID dbid;
		bool bObject;
		//
		bool IsObject() const { return bObject; }
	};
	//
	typedef vector<SEntry> CEntriesList;
	CEntriesList entriesList;
	CEntriesList::const_iterator posCurrElement;
	string szClassTypeName;
	//
protected:
	CFolderManipulatorIteratorWrapper() {}
public:
	CFolderManipulatorIteratorWrapper( const string &szSrcPath, const string &szTypeName );
	
	bool Next();
	bool IsEnd() const;
	const SIteratorDesc* GetDesc() const;
	bool GetName( string *pszName ) const;
	bool GetType( string *pszType ) const;
	UINT GetID() const;
	bool IsFolder() const;
	void Reset();
};


