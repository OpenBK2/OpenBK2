#pragma once

#include "VFS.h"
#include "ZipArchieve.h"
#include "FilePath.h"
#include "FileTime.h"
#include "FileUtils.h"

namespace NVFS
{
class CWinVFS : public IVFS
{
	OBJECT_NOCOPY_METHODS( CWinVFS )

	class CFileEntry
	{
		DWORD dwCheckTime;
		bool bChecked;
	public:
		CFileEntry( const DWORD _dwCheckTime ) : dwCheckTime( _dwCheckTime ), bChecked( false ) {  }
		virtual ~CFileEntry() {}
		//
		void SetChecked() { bChecked = true; }
		bool IsChecked() const { return bChecked; }
		//
		DWORD GetCheckTime() const { return dwCheckTime; }
		virtual CDataStream* OpenStream( const string &szPathName ) = 0;
		virtual bool GetStats( SFileStats *pStats, const string &szPathName ) const = 0;
	};
	//
	class CWinFileEntry : public CFileEntry
	{
		const string &szBasePath;
	public:
		CWinFileEntry( const DWORD _dwCheckTime, const string &_szBasePath ) : CFileEntry( _dwCheckTime ), szBasePath( _szBasePath ) {  }
		// мы не храним имя файла, а передаём его в виде параметра, т.к. оно итак хранится в hash_map от storage, 
		// а эту хрень вызывают только здесь и только я... (теперь уже не только ты -)
		CDataStream* OpenStream( const string &szPathName );
		bool GetStats( SFileStats *pStats, const string &szPathName ) const;
	};

	class CZipFileEntry : public CFileEntry
	{
		CZipFile &zipfile;							// zipfile structure
		const int nIndex;											// this file index in the zipfile
	public:
		CZipFileEntry( const DWORD _dwCheckTime, CZipFile &_zipfile, const int _nIndex )
			: CFileEntry( _dwCheckTime ), zipfile( _zipfile ), nIndex( _nIndex ) {}
			CDataStream* OpenStream( const string &szPathName ) { return zipfile.OpenFile( nIndex ); }
			bool GetStats( SFileStats *pStats, const string &szPathName ) const
			{
				if ( pStats == 0 ) 
					return false;
				pStats->nSize = zipfile.GetFileLen( nIndex );
				pStats->atime = pStats->ctime = pStats->mtime = zipfile.GetModDateTime( nIndex );
				pStats->pszName = 0;
				return true;
			}
	};
	//
	//! functional for adding/registering win files to storage
	class CWinFileAdder
	{
		const string &szBasePath;
		CWinVFS *pVFS;
	public:
		CWinFileAdder( const string &_szBasePath, CWinVFS *_pVFS ) : szBasePath(_szBasePath), pVFS(_pVFS) {}
		//
		void operator()( NFile::CFileIterator &it ) const
		{
			if ( it.GetLength() <= 0 ) 
				return;
			//
			const DWORD dwCheckTime = FILETIMEToWin32DateTime( it.GetLastWriteTime() );
			NFile::CFilePath szFileName = it.GetFullName();
			szFileName.erase( 0, szBasePath.size() );
			CStreamEntriesMap::iterator pos = pVFS->streamEntriesMap.find( szFileName );
			if ( pos == pVFS->streamEntriesMap.end() ) 
				pVFS->streamEntriesMap[szFileName] = new CWinFileEntry( dwCheckTime, szBasePath );
			else
			{
				if ( pos->second->GetCheckTime() < dwCheckTime )
				{
					delete pos->second;
					pos->second = new CWinFileEntry( dwCheckTime, szBasePath );
					pos->second->SetChecked();
				}
			}
		}
	};
	typedef list<CObj<CZipFile> > CZipFilesList;
	CZipFilesList zipFiles;
	typedef hash_map<NFile::CFilePath, CFileEntry*> CStreamEntriesMap;
	CStreamEntriesMap streamEntriesMap;
	NFile::CFilePath szBasePath;
	bool bAllWinFilesChecked;
	bool bArchiveOnly;
	//
	CDataStream *OpenFileDirect( const string &szPath );
	// only for relative path!
	CFileEntry *UpdateFileEntry( const NFile::CFilePath &szPath );
	//
	CWinVFS() { }
public:
	CWinVFS( const string &szBasePath );
	~CWinVFS();
	//
	CDataStream* OpenFile( const string &szPath );
	bool DoesFileExist( const string &szPath );
	bool GetFileStats( SFileStats *pStats, const string &szPath );
	void GetAllFileNames( vector<string> *pFileNames, const string &rszFolder );

	void GetFileFullName( const string &szPath );
};

class CWinFileCreator : public IFileCreator
{
	OBJECT_NOCOPY_METHODS( CWinFileCreator )

	const string szBasePath;

	CWinFileCreator() { }
public:
	CWinFileCreator( const string &szBasePath );

	CDataStream* CreateFile( const string &szPath );
	bool RemoveFile( const string &szPath );
};

}

