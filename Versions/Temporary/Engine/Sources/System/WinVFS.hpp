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
		virtual CDataStream* OpenStream( const std::string &szPathName ) = 0;
		virtual bool GetStats( SFileStats *pStats, const std::string &szPathName ) const = 0;
	};
	//
	class CWinFileEntry : public CFileEntry
	{
		const std::string &szBasePath;
	public:
		CWinFileEntry( const DWORD _dwCheckTime, const std::string &_szBasePath ) : CFileEntry( _dwCheckTime ), szBasePath( _szBasePath ) {  }
		// мы не храним имя файла, а передаём его в виде параметра, т.к. оно итак хранится в hash_map от storage, 
		// а эту хрень вызывают только здесь и только я... (теперь уже не только ты -)
		CDataStream* OpenStream( const std::string &szPathName );
		bool GetStats( SFileStats *pStats, const std::string &szPathName ) const;
	};

	class CZipFileEntry : public CFileEntry
	{
		CZipFile &zipfile;							// zipfile structure
		const int nIndex;											// this file index in the zipfile
	public:
		CZipFileEntry( const DWORD _dwCheckTime, CZipFile &_zipfile, const int _nIndex )
			: CFileEntry( _dwCheckTime ), zipfile( _zipfile ), nIndex( _nIndex ) {}
			CDataStream* OpenStream( const std::string &szPathName ) { return zipfile.OpenFile( nIndex ); }
			bool GetStats( SFileStats *pStats, const std::string &szPathName ) const
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
		const std::string &szBasePath;
		CWinVFS *pVFS;
	public:
		CWinFileAdder( const std::string &_szBasePath, CWinVFS *_pVFS ) : szBasePath(_szBasePath), pVFS(_pVFS) {}
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
	typedef std::list<CObj<CZipFile> > CZipFilesList;
	CZipFilesList zipFiles;
	typedef std::unordered_map<NFile::CFilePath, CFileEntry*> CStreamEntriesMap;
	CStreamEntriesMap streamEntriesMap;
	NFile::CFilePath szBasePath;
	bool bAllWinFilesChecked;
	bool bArchiveOnly;
	//
	CDataStream *OpenFileDirect( const std::string &szPath );
	// only for relative path!
	CFileEntry *UpdateFileEntry( const NFile::CFilePath &szPath );
	//
	CWinVFS() { }
public:
	CWinVFS( const std::string &szBasePath );
	~CWinVFS();
	//
	CDataStream* OpenFile( const std::string &szPath );
	bool DoesFileExist( const std::string &szPath );
	bool GetFileStats( SFileStats *pStats, const std::string &szPath );
	void GetAllFileNames( std::vector<std::string> *pFileNames, const std::string &rszFolder );

	void GetFileFullName( const std::string &szPath );
};

class CWinFileCreator : public IFileCreator
{
	OBJECT_NOCOPY_METHODS( CWinFileCreator )

	const std::string szBasePath;

	CWinFileCreator() { }
public:
	CWinFileCreator( const std::string &szBasePath );

	CDataStream* CreateFile( const std::string &szPath );
	bool RemoveFile( const std::string &szPath );
};

}

