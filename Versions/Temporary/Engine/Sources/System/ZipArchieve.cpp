#include "StdAfx.h"
#include "..\zlib\zconf.h"
#include "ZipArchieve.h"
#include "..\zlib\zlib.h"

// ************************************************************************************************************************ //
// **
// ** single zip file
// **
// ** format:
// **		//files data
// **		[local file header 1]
// **		[file data 1]
// **		[data descriptor 1]
// **		...
// **		[local file header N]
// **		[file data N]
// **		[data descriptor N]
// **		// central directory
// **		[file header 1]
// **		...
// **		[file header N]
// **		[digital signature]
// **		[central dir header]
// **
// **
// ************************************************************************************************************************ //

#pragma pack( 1 )
struct CZipFile::SZipLocalFileHeader
{
	enum { SIGNATURE = 0x04034b50, COMP_STORE = 0, COMP_DEFLAT = 8 };
	DWORD dwSignature;										// local file header signature
	WORD  version;												// version needed to extract
	WORD  flag;														// general purpose bit flag
	WORD  wCompression;										// compression method: COMP_xxxx
	WORD  wModTime;												// last mod file time (MS-DOS)
	WORD  wModDate;												// last mod file date (MS-DOS)
	DWORD dwCRC32;												// CRC-32
	DWORD dwCSize;												// compressed size
	DWORD dwUSize;												// uncompressed size
	WORD  wFileNameLen;										// filename length (w/o zero terminator!)
	WORD  wExtraLen;											// extra field length
	// here filename follows (wFileNameLen bytes)
	// here extra field follows (wExtraLen bytes)
	bool IsDataDescriptorExist() const { return (flag & 4) != 0; }
};

// NOTE: data descriptor exist only, if bit 3 of general purpose flag of the corresponding local file header is set
// one can call IsDataDescriptorExist() to check this fact
// if data descriptor exist, then one must get CRC32, CSize and USize from the data descriptor instead of from local file header
struct CZipFile::SZipDataDescriptor
{
	DWORD dwCRC32;												// CRC-32
	DWORD dwCSize;												// compressed size
	DWORD dwUSize;												// uncompressed size
};

struct CZipFile::SZipCentralDirHeader
{
	enum { SIGNATURE = 0x06054b50 };
	DWORD dwSignature;										// end of central dir signature
	WORD  wDisk;													// number of this disk
	WORD  wStartDisk;											// number of disk with start central dir
	WORD  wDirEntries;										// total number of entries in central dir on this disk
	WORD  wTotalDirEntries;								// total number entries in central dir
	DWORD dwDirSize;											// size of central directory
	DWORD dwDirOffset;										// offset of start of central directory with respect to the starting disk nuber
	WORD  wCommentLen;										// zipfile comment length
	// comment follows here (wCommentLen bytes)
};

struct CZipFile::SZipFileHeader
{
	enum { SIGNATURE = 0x02014b50, COMP_STORE = 0, COMP_DEFLAT = 8 };
	DWORD dwSignature;										// central file header signature
	BYTE  verMade;												// version made by
	BYTE  os;															// host operating system
	BYTE  verNeeded;											// version needed to extract
	BYTE  osNeeded;												// OS of version needed for extraction
	WORD  flag;														// general purpose bit flag
	WORD  wCompression;										// compression method: COMP_xxxx
	WORD  wModTime;												// last mode file time (MS-DOS)
	WORD  wModDate;												// last mode file date (MS-DOS)
	DWORD dwCRC32;												// CRC-32
	DWORD dwCSize;												// compressed size
	DWORD dwUSize;												// uncompressed size
	WORD  wFileNameLen;										// filename length
	WORD  wExtraLen;											// extra field length
	WORD  wCommentLen;										// file comment length
	WORD  wDiskStart;											// disk number start
	WORD  wIntAttr;												// internal file attributes: bit0 == 1 => ASCII or text file, == 0 => binary data
	DWORD wExtAttr;												// external file attributes, host system dependent
	DWORD dwHdrOffset;										// relative offset of local header from the start of the first disk, on which this file appears
	// filename follows here (wFileNameLen bytes)
	// extra field follows here (wExtraLen bytes)
	// file comment follows here (wCommentLen bytes)
	const char* GetName() const { return (const char *)(this + 1); }
	const char* GetExtra() const { return GetName() + wFileNameLen; }
	const char* GetComment() const { return GetExtra() + wExtraLen; }
};
// host operating system codes
// 0  - MS-DOS and OS/2 (FAT / VFAT / FAT32 file systems) 
// 1  - Amiga 
// 2  - OpenVMS 
// 3  - Unix 
// 4  - VM/CMS 
// 5  - Atari ST 
// 6  - OS/2 H.P.F.S. 
// 7  - Macintosh 
// 8  - Z-System 
// 9  - CP/M 
// 10 - Windows NTFS 
// 11 - MVS 
// 12 - VSE 
// 13 thru 255 - unused

// compression methods
// 0  - The file is stored (no compression) 
// 1  - The file is Shrunk 
// 2  - The file is Reduced with compression factor 1 
// 3  - The file is Reduced with compression factor 2 
// 4  - The file is Reduced with compression factor 3 
// 5  - The file is Reduced with compression factor 4 
// 6  - The file is Imploded  
// 7  - Reserved for Tokenizing compression algorithm  
// 8  - The file is Deflated  
// 9  - Enhanced Deflating using Deflate64(tm) 
// 10 - PKWARE Date Compression Library Imploding 


#pragma pack()


void CZipFile::SFileHeader::Init( const SZipFileHeader &hdr )
{
	dwModDateTime = DWORD( (hdr.wModDate) << 16 ) | DWORD( hdr.wModTime );
	dwCSize = hdr.dwCSize == 0 ? hdr.dwUSize : hdr.dwCSize;
	dwUSize = hdr.dwUSize;
	wExtAttr = hdr.wExtAttr;
	dwHdrOffset = hdr.dwHdrOffset;
	pszFileName = new char[hdr.wFileNameLen + 1];
	memcpy( (void*)pszFileName, hdr.GetName(), hdr.wFileNameLen );
	((char*)pszFileName)[hdr.wFileNameLen] = 0;
}

CZipFile::CZipFile( const char *pszName ) : mmf( pszName, STREAM_ACCESS_READ )
{
	NI_ASSERT( pszName != 0, "NULL stream passed to zip file" );
	if ( !mmf.IsOk() )
		return;
	nTotalSize = mmf.GetFileSize();
	if ( ( GetVersion() & 0x80000000 ) == 0 )
		mmf.MapFile( nTotalSize, false );
	// Assuming no extra comment at the end, read the whole end record.
	int cdhOffset = nTotalSize - sizeof( SZipCentralDirHeader );
	if ( cdhOffset < 0 )
		return;
	CMemoryMappedFileFragment cdhRead( &mmf, cdhOffset, sizeof( SZipCentralDirHeader ) );
	const SZipCentralDirHeader &cdh = *(const SZipCentralDirHeader*) cdhRead.GetBuffer();

	// Check
	NI_ASSERT( cdh.dwSignature == SZipCentralDirHeader::SIGNATURE, "Can't recognize zip dir header" );
	if ( cdh.dwSignature != SZipCentralDirHeader::SIGNATURE )
		return;

	int nDirStart = cdh.dwDirOffset;//cdhOffset - cdh.dwDirSize;
	int nDirSize = cdh.dwDirSize;
	
	CMemoryMappedFileFragment dirData( &mmf, nDirStart, nDirSize );

	// Go to the beginning of the directory & process each entry
	const unsigned char *pfhStart = dirData.GetBuffer();
	const unsigned char *pfh = pfhStart;
	// first, count number of entries
	int nNumEntries = 0;
	while ( pfh - pfhStart < nDirSize )
	{
		const SZipFileHeader &hdr = *reinterpret_cast<const SZipFileHeader*>( pfh );
		// Check the directory entry integrity.
		if ( hdr.dwSignature != SZipFileHeader::SIGNATURE )
			return;
		else if ( hdr.wCompression == SZipLocalFileHeader::COMP_DEFLAT || hdr.wCompression == SZipLocalFileHeader::COMP_STORE )
		{
			pfh += sizeof( hdr ) + hdr.wFileNameLen + hdr.wExtraLen + hdr.wCommentLen;
			++nNumEntries;
		}
	}
	//
	papDir.reserve( nNumEntries );
	// add dir entries
	pfh = pfhStart;
	while ( pfh - pfhStart < nDirSize )
	{
		const SZipFileHeader &hdr = *reinterpret_cast<const SZipFileHeader*>( pfh );
		// Check the directory entry integrity.
		if ( hdr.dwSignature != SZipFileHeader::SIGNATURE )
		{
			papDir.clear();
			return;
		}
		else if ( hdr.wCompression == SZipLocalFileHeader::COMP_DEFLAT || hdr.wCompression == SZipLocalFileHeader::COMP_STORE )
		{
			vector<SFileHeader>::iterator posFileHeader = papDir.insert( papDir.end(), SFileHeader() );
			posFileHeader->Init( hdr );
			//
			pfh += sizeof(hdr) + hdr.wFileNameLen + hdr.wExtraLen + hdr.wCommentLen;
		}
		else
		{
			NI_ASSERT( false, StrFmt("File \"%s\" has wrong compression! Only Deflate and Store methods allowed (-9 and -0 options for zip.exe)", hdr.GetName()) );
		}
	}
}

CZipFile::~CZipFile()
{
	if ( ( GetVersion() & 0x80000000 ) == 0 )
		mmf.UnmapFile();
}

void CZipFile::GetFileName( int nIndex, string *pString ) const
{
	*pString = papDir[nIndex].pszFileName;
}

int CZipFile::GetFileLen( int nIndex ) const
{
	return papDir[nIndex].dwUSize;
}

DWORD CZipFile::GetFileAttribs( int nIndex ) const
{
	return papDir[nIndex].wExtAttr;
}

DWORD CZipFile::GetModDateTime( int nIndex ) const
{
	return papDir[nIndex].dwModDateTime;
}

CDataStream *CZipFile::OpenFile( const SFileHeader &ghdr )
{
	CMemoryMappedFileFragment fLocalHdr( &mmf, ghdr.dwHdrOffset, sizeof( SZipLocalFileHeader ) );

	const SZipLocalFileHeader &hdr = *(const SZipLocalFileHeader*)fLocalHdr.GetBuffer();
	NI_ASSERT( hdr.dwSignature == SZipLocalFileHeader::SIGNATURE, "can't recognize zip local header" );

	// Skip extra fields
	int nBeginPos = ghdr.dwHdrOffset + hdr.wFileNameLen + hdr.wExtraLen + sizeof( hdr );

	// in the STORE case we can create range adaptor for direct read
	if ( hdr.wCompression == SZipLocalFileHeader::COMP_STORE ) 
	{
		if ( nBeginPos + hdr.dwCSize > nTotalSize )
			return 0;
		return new CMemoryMappedFileFragment( &mmf, nBeginPos, hdr.dwCSize );
	}
	// create new memory stream and setup stats
	CMemoryStream *pDstStream = new CMemoryStream;
	pDstStream->SetSizeDiscard( hdr.dwUSize );
	const void *pBuf = pDstStream->GetBuffer();
	// proceed with DEFLAT unpacking
	NI_ASSERT( hdr.wCompression == SZipLocalFileHeader::COMP_DEFLAT, "Can support only STORE and DEFLAT cpmpression methods" );

	// Alloc compressed data buffer and read the whole stream
	CMemoryMappedFileFragment cprData( &mmf, nBeginPos, hdr.dwCSize );
	const unsigned char *pcData = cprData.GetBuffer();//new char[hdr.dwCSize];

	// Setup the inflate stream.
	z_stream stream;
	stream.next_in = (Bytef*)pcData;
	stream.avail_in = (uInt)hdr.dwCSize;
	stream.next_out = (Bytef*)pBuf;
	stream.avail_out = hdr.dwUSize;
	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;

	// Perform inflation. wbits < 0 indicates no zlib header inside the data.
	int err = inflateInit2( &stream, -MAX_WBITS );
	if ( err == Z_OK )
	{
		err = inflate( &stream, Z_FINISH );
		inflateEnd( &stream );
		// CRAP{ почему-то иногда при распаковке возвращается "buffer error" всесто "stream end"...
		if ( (err == Z_STREAM_END) || (err == Z_BUF_ERROR) )
			err = Z_OK;
		// CRAP}
		inflateEnd( &stream );
	}

	if ( err == Z_OK )
		return pDstStream;

	delete pDstStream;
	return 0;
}

CDataStream *CZipFile::OpenFile( int nIndex )
{
	// Go to the actual file and read the local header.
	return OpenFile( papDir[nIndex] );
}

