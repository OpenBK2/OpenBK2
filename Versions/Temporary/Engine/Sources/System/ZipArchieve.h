#pragma once

#include "FileReaders.h"

#include <cstdint>

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
		uint32_t dwModDateTime;									// last mode file date & time (MS-DOS)
		uint32_t dwCSize;												// compressed size
		uint32_t dwUSize;												// uncompressed size
		uint32_t wExtAttr;												// external file attributes, host system dependent
		uint32_t dwHdrOffset;										// relative offset of local header from the start of the first disk, on which this file appears
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
	// The external attributes word is host-system dependent, and for the DOS and
	// Windows hosts its low byte holds MS-DOS attribute bits, where 0x10 marks a
	// directory. Win32 inherited that same value as FILE_ATTRIBUTE_DIRECTORY, but
	// this is a field read out of the archive rather than a question for the
	// operating system, so it is named here instead of taken from a Windows header.
	enum { DOS_ATTR_DIRECTORY = 0x10 };
	uint32_t GetFileAttribs( int nIndex ) const;
	uint32_t GetModDateTime( int nIndex ) const;	// high word - date, low word - time

	CDataStream *OpenFile( int nIndex );
};

