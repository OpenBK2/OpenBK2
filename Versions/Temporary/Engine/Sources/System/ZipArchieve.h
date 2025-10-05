#pragma once
#include "FileReaders.h"


class CZipFile : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CZipFile );
	struct SZipLocalFileHeader;
	struct SZipFileHeader;
	struct SZipDataDescriptor;
	struct SZipCentralDirHeader;
	//
	struct SFileHeader
	{
		DWORD dwModDateTime;									// last mode file date & time (MS-DOS)
		DWORD dwCSize;												// compressed size
		DWORD dwUSize;												// uncompressed size
		DWORD wExtAttr;												// external file attributes, host system dependent
		DWORD dwHdrOffset;										// relative offset of local header from the start of the first disk, on which this file appears
		const char *pszFileName;							// filename
		//
		SFileHeader(): pszFileName(0) {}
		SFileHeader( const SZipFileHeader &hdr );
		~SFileHeader() { if ( pszFileName ) delete []pszFileName; }
		//
		void Init( const SZipFileHeader &hdr );
		bool HasCompression() const { return dwCSize < dwUSize; }
	};
	//
	std::vector<SFileHeader> papDir;
	CMMFile mmf;
	int nTotalSize;
	//
	CDataStream *OpenFile( const SFileHeader &hdr );
	//
	~CZipFile();
public:
	CZipFile() {}
	CZipFile( const char *pszName );

	bool IsOk() const { return !papDir.empty(); }
	int GetNumFiles() const { return papDir.size(); }
	//
	void GetFileName( int nIndex, std::string *pString ) const;
	int GetFileLen( int nIndex ) const;
	DWORD GetFileAttribs( int nIndex ) const;
	DWORD GetModDateTime( int nIndex ) const;	// high word - date, low word - time

	CDataStream *OpenFile( int nIndex );
};

