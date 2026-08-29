#pragma once

#include "VFS.h"
#include "ZipArchieve.h"
#include "FilePath.h"
#include "FileTime.h"
#include "FileUtils.h"

#include <cstdint>

namespace NVFS
{
class CWinVFS : public IVFS
{
	OBJECT_NOCOPY_METHODS( CWinVFS )

	class CFileEntry
	{
		uint32_t dwCheckTime;
		bool bChecked;
	public:
		CFileEntry( const uint32_t _dwCheckTime ) : dwCheckTime( _dwCheckTime ), bChecked( false ) {  }
		virtual ~CFileEntry() {}
		//
		void SetChecked() { bChecked = true; }
		bool IsChecked() const { return bChecked; }
		//
		uint32_t GetCheckTime() const { return dwCheckTime; }
		virtual CDataStream* OpenStream( const std::string &szPathName ) = 0;
		virtual bool GetStats( SFileStats *pStats, const std::string &szPathName ) const = 0;
	};
	//
	class CWinFileEntry : public CFileEntry
	{
		const std::string &szBasePath;
		// The name as it is spelled on disk, which is not always the name the entry
		// is filed under. The map's key folds case and separators, so an entry found
		// through it can have been asked for by a spelling no open() would accept,
		// and the szPathName parameter below carries that spelling rather than this
		// one. Storing it is what makes the two able to differ.
		//
		// It used to be left out on the grounds that the hash map held it already.
		// That was true while the only lookup that could succeed was an exact one.
		const NFile::CFilePath szRealPath;
	public:
		CWinFileEntry( const uint32_t _dwCheckTime, const std::string &_szBasePath, const NFile::CFilePath &_szRealPath )
			: CFileEntry( _dwCheckTime ), szBasePath( _szBasePath ), szRealPath( _szRealPath ) {  }
		CDataStream* OpenStream( const std::string &szPathName );
		bool GetStats( SFileStats *pStats, const std::string &szPathName ) const;
	};

	class CZipFileEntry : public CFileEntry
	{
		CZipFile &zipfile;							// zipfile structure
		const int nIndex;											// this file index in the zipfile
	public:
		CZipFileEntry( const uint32_t _dwCheckTime, CZipFile &_zipfile, const int _nIndex )
			: CFileEntry( _dwCheckTime ), zipfile( _zipfile ), nIndex( _nIndex ) {}
			CDataStream* OpenStream( const std::string &szPathName ) { return zipfile.OpenFile( nIndex ); }
			bool GetStats( SFileStats *pStats, const std::string &szPathName ) const
			{
				if ( pStats == 0 ) 
					return false;
				pStats->nSize = zipfile.GetFileLen( nIndex );
				pStats->mtime = zipfile.GetModDateTime( nIndex );
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
			const uint32_t dwCheckTime = PackFileTime( it.GetLastWriteTime() );
			NFile::CFilePath szFileName = it.GetFullName();
			szFileName.erase( 0, szBasePath.size() );
			// szFileName came from the directory scan, so it is already the real
			// spelling and is both the key and the name to open by.
			CStreamEntriesMap::iterator pos = pVFS->streamEntriesMap.find( szFileName );
			if ( pos == pVFS->streamEntriesMap.end() )
				pVFS->streamEntriesMap[szFileName] = new CWinFileEntry( dwCheckTime, szBasePath, szFileName );
			else
			{
				if ( pos->second->GetCheckTime() < dwCheckTime )
				{
					delete pos->second;
					pos->second = new CWinFileEntry( dwCheckTime, szBasePath, szFileName );
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

