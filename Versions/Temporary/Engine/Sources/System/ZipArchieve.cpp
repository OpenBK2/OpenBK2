#include "stdafx.h"

#include "ZipArchieve.h"

#include <cstdint>

#include <fmt/format.h>
#include <zconf.h>
#include <zlib.h>

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
	enum
	{
		SIGNATURE = 0x04034b50,
		COMP_STORE = 0,
		COMP_DEFLAT = 8
	};

	uint32_t dwSignature;  // local file header signature
	uint16_t version;      // version needed to extract
	uint16_t flag;         // general purpose bit flag
	uint16_t wCompression; // compression method: COMP_xxxx
	uint16_t wModTime;     // last mod file time (MS-DOS)
	uint16_t wModDate;     // last mod file date (MS-DOS)
	uint32_t dwCRC32;      // CRC-32
	uint32_t dwCSize;      // compressed size
	uint32_t dwUSize;      // uncompressed size
	uint16_t wFileNameLen; // filename length (w/o zero terminator!)
	uint16_t wExtraLen;    // extra field length

	// here filename follows (wFileNameLen bytes)
	// here extra field follows (wExtraLen bytes)
	bool IsDataDescriptorExist() const
	{
		return ( flag & 4 ) != 0;
	}
};

// NOTE: data descriptor exist only, if bit 3 of general purpose flag of the corresponding local file header is set
// one can call IsDataDescriptorExist() to check this fact
// if data descriptor exist, then one must get CRC32, CSize and USize from the data descriptor instead of from local file header
struct CZipFile::SZipDataDescriptor
{
	uint32_t dwCRC32; // CRC-32
	uint32_t dwCSize; // compressed size
	uint32_t dwUSize; // uncompressed size
};

struct CZipFile::SZipCentralDirHeader
{
	enum
	{
		SIGNATURE = 0x06054b50
	};

	uint32_t dwSignature;      // end of central dir signature
	uint16_t wDisk;            // number of this disk
	uint16_t wStartDisk;       // number of disk with start central dir
	uint16_t wDirEntries;      // total number of entries in central dir on this disk
	uint16_t wTotalDirEntries; // total number entries in central dir
	uint32_t dwDirSize;        // size of central directory
	uint32_t dwDirOffset;      // offset of start of central directory with respect to the starting disk nuber
	uint16_t wCommentLen;      // zipfile comment length
														 // comment follows here (wCommentLen bytes)
};

struct CZipFile::SZipFileHeader
{
	enum
	{
		SIGNATURE = 0x02014b50,
		COMP_STORE = 0,
		COMP_DEFLAT = 8
	};

	uint32_t dwSignature;  // central file header signature
	uint8_t verMade;       // version made by
	uint8_t os;            // host operating system
	uint8_t verNeeded;     // version needed to extract
	uint8_t osNeeded;      // OS of version needed for extraction
	uint16_t flag;         // general purpose bit flag
	uint16_t wCompression; // compression method: COMP_xxxx
	uint16_t wModTime;     // last mode file time (MS-DOS)
	uint16_t wModDate;     // last mode file date (MS-DOS)
	uint32_t dwCRC32;      // CRC-32
	uint32_t dwCSize;      // compressed size
	uint32_t dwUSize;      // uncompressed size
	uint16_t wFileNameLen; // filename length
	uint16_t wExtraLen;    // extra field length
	uint16_t wCommentLen;  // file comment length
	uint16_t wDiskStart;   // disk number start
	uint16_t wIntAttr;     // internal file attributes: bit0 == 1 => ASCII or text file, == 0 => binary data
	uint32_t wExtAttr;     // external file attributes, host system dependent
	uint32_t dwHdrOffset;  // relative offset of local header from the start of the first disk, on which this file appears

	// filename follows here (wFileNameLen bytes)
	// extra field follows here (wExtraLen bytes)
	// file comment follows here (wCommentLen bytes)
	const char *GetName() const
	{
		return (const char *)( this + 1 );
	}

	const char *GetExtra() const
	{
		return GetName() + wFileNameLen;
	}

	const char *GetComment() const
	{
		return GetExtra() + wExtraLen;
	}
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

// Entry names.
//
// Narrow strings are UTF-8 throughout the engine, and the VFS hands names to
// std::filesystem::u8path, which throws on bytes that are not UTF-8. A zip says
// its names are UTF-8 with bit 11 of the flags; without it the format's default
// is code page 437, the IBM PC's, which is what the zip tools of the game's day
// wrote. MOD-18's Daten.pak is such an archive: "38_cm_Sturmm\x94rser" is
// "Sturmmoerser" with a CP437 o-umlaut, and reading it ended the process. Such a
// name is converted here, once, so everything past the reader sees UTF-8 and the
// name matches the UTF-8 hrefs in the records that point at it.
//
// Many writers put UTF-8 names in without setting the flag, so a name that is
// valid UTF-8 is taken as it is; CP437 text other than plain ASCII is very
// unlikely to happen to be valid UTF-8 too. An Info-ZIP Unicode Path extra field
// (0x7075), which some writers add beside a legacy name, is preferred when its
// checksum shows it belongs to this name.
namespace
{

const uint16_t ZIP_FLAG_UTF8 = 0x0800;
const uint16_t ZIP_EXTRA_UNICODE_PATH = 0x7075;

// The upper half of code page 437 as Unicode code points; the lower half is ASCII.
// Kept as a grid of 8 a row, in byte order from 0x80, so a byte can be found by eye.
// clang-format off
const uint16_t CP437_HIGH[128] = {
	0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
	0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
	0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
	0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
	0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
	0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
	0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
	0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
	0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
	0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
	0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
	0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
	0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,
	0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
	0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,
	0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0,
};
// clang-format on

// Strict UTF-8: shortest forms only, no surrogates, nothing above U+10FFFF,
// the same rules MSVC's conversion behind u8path applies.
bool IsValidUTF8( const std::string &szText )
{
	const size_t nSize = szText.size();
	size_t i = 0;
	while ( i < nSize )
	{
		const uint8_t c = szText[i];
		int nTail = 0;
		uint32_t nCode = 0;
		uint32_t nMin = 0;
		if ( c < 0x80 )
		{
			++i;
			continue;
		}
		else if ( ( c & 0xE0 ) == 0xC0 )
		{
			nTail = 1;
			nCode = c & 0x1F;
			nMin = 0x80;
		}
		else if ( ( c & 0xF0 ) == 0xE0 )
		{
			nTail = 2;
			nCode = c & 0x0F;
			nMin = 0x800;
		}
		else if ( ( c & 0xF8 ) == 0xF0 )
		{
			nTail = 3;
			nCode = c & 0x07;
			nMin = 0x10000;
		}
		else
		{
			return false;
		}
		// the continuation bytes must all be there
		if ( i + nTail >= nSize )
		{
			return false;
		}
		for ( int k = 1; k <= nTail; ++k )
		{
			const uint8_t t = szText[i + k];
			if ( ( t & 0xC0 ) != 0x80 )
			{
				return false;
			}
			nCode = ( nCode << 6 ) | ( t & 0x3F );
		}
		if ( nCode < nMin || nCode > 0x10FFFF || ( nCode >= 0xD800 && nCode <= 0xDFFF ) )
		{
			return false;
		}
		i += nTail + 1;
	}
	return true;
}

void AppendUTF8( std::string *pRes, const uint32_t nCode )
{
	if ( nCode < 0x80 )
	{
		pRes->push_back( char( nCode ) );
	}
	else if ( nCode < 0x800 )
	{
		pRes->push_back( char( 0xC0 | ( nCode >> 6 ) ) );
		pRes->push_back( char( 0x80 | ( nCode & 0x3F ) ) );
	}
	else
	{
		// CP437 reaches no further than the Basic Multilingual Plane
		pRes->push_back( char( 0xE0 | ( nCode >> 12 ) ) );
		pRes->push_back( char( 0x80 | ( ( nCode >> 6 ) & 0x3F ) ) );
		pRes->push_back( char( 0x80 | ( nCode & 0x3F ) ) );
	}
}

std::string CP437ToUTF8( const std::string &szText )
{
	std::string szRes;
	szRes.reserve( szText.size() * 2 );
	for ( const char c : szText )
	{
		const uint8_t b = c;
		AppendUTF8( &szRes, b < 0x80 ? b : CP437_HIGH[b - 0x80] );
	}
	return szRes;
}

// The UTF-8 name an Info-ZIP Unicode Path extra field carries for szRawName,
// or an empty string when there is no such field or it is not for this name:
// the field holds the CRC-32 of the legacy name it replaces, so a tool that
// renamed the entry without updating the field is caught.
std::string GetUnicodePathField( const char *pExtra, const int nExtraLen, const std::string &szRawName )
{
	int nPos = 0;
	while ( nPos + 4 <= nExtraLen )
	{
		const uint8_t *p = reinterpret_cast<const uint8_t *>( pExtra + nPos );
		const uint16_t wID = p[0] | ( p[1] << 8 );
		const uint16_t wSize = p[2] | ( p[3] << 8 );
		if ( nPos + 4 + wSize > nExtraLen )
		{
			break;
		}
		// version (1 byte), CRC-32 of the legacy name (4 bytes), then the UTF-8 name
		if ( wID == ZIP_EXTRA_UNICODE_PATH && wSize > 5 && p[4] == 1 )
		{
			const uint32_t dwCRC = p[5] | ( p[6] << 8 ) | ( p[7] << 16 ) | ( uint32_t( p[8] ) << 24 );
			const uint32_t dwNameCRC =
					crc32( 0, reinterpret_cast<const Bytef *>( szRawName.data() ), static_cast<uInt>( szRawName.size() ) );
			std::string szName( reinterpret_cast<const char *>( p + 9 ), wSize - 5 );
			if ( dwCRC == dwNameCRC && IsValidUTF8( szName ) )
			{
				return szName;
			}
		}
		nPos += 4 + wSize;
	}
	return std::string();
}

} // namespace

std::string CZipFile::DecodeEntryName(
		const char *pszRawName, const int nNameLen, const uint16_t wFlags, const char *pExtra, const int nExtraLen )
{
	const std::string szRawName( pszRawName, nNameLen );
	if ( ( wFlags & ZIP_FLAG_UTF8 ) != 0 || IsValidUTF8( szRawName ) )
	{
		return szRawName;
	}
	const std::string szUnicode = GetUnicodePathField( pExtra, nExtraLen, szRawName );
	if ( !szUnicode.empty() )
	{
		return szUnicode;
	}
	return CP437ToUTF8( szRawName );
}

void CZipFile::SFileHeader::Init( const SZipFileHeader &hdr )
{
	dwModDateTime = uint32_t( ( hdr.wModDate ) << 16 ) | uint32_t( hdr.wModTime );
	dwCSize = hdr.dwCSize == 0 ? hdr.dwUSize : hdr.dwCSize;
	dwUSize = hdr.dwUSize;
	wExtAttr = hdr.wExtAttr;
	dwHdrOffset = hdr.dwHdrOffset;
	// UTF-8 whatever the archive used, see DecodeEntryName
	const std::string szName =
			DecodeEntryName( hdr.GetName(), hdr.wFileNameLen, hdr.flag, hdr.GetExtra(), hdr.wExtraLen );
	pszFileName = new char[szName.size() + 1];
	memcpy( (void *)pszFileName, szName.c_str(), szName.size() + 1 );
}

CZipFile::CZipFile( const char *pszName )
	: mmf( pszName, STREAM_ACCESS_READ )
{
	NI_ASSERT( pszName != 0, "NULL stream passed to zip file" );
	if ( !mmf.IsOk() )
	{
		return;
	}
	nTotalSize = mmf.GetFileSize();
	// The check this replaces was for the Windows 9x family, which had no
	// file mapping. A failure here is still handled: CMemoryMappedFileFragment
	// reads the file directly whenever IsMapped is false, so an archive too
	// large to map is slower rather than fatal.
	mmf.MapFile( nTotalSize, false );
	// Assuming no extra comment at the end, read the whole end record.
	int cdhOffset = nTotalSize - sizeof( SZipCentralDirHeader );
	if ( cdhOffset < 0 )
	{
		return;
	}
	CMemoryMappedFileFragment cdhRead( &mmf, cdhOffset, sizeof( SZipCentralDirHeader ) );
	const SZipCentralDirHeader &cdh = *(const SZipCentralDirHeader *)cdhRead.GetBuffer();

	// Check
	NI_ASSERT( cdh.dwSignature == SZipCentralDirHeader::SIGNATURE, "Can't recognize zip dir header" );
	if ( cdh.dwSignature != SZipCentralDirHeader::SIGNATURE )
	{
		return;
	}

	int nDirStart = cdh.dwDirOffset; //cdhOffset - cdh.dwDirSize;
	int nDirSize = cdh.dwDirSize;

	CMemoryMappedFileFragment dirData( &mmf, nDirStart, nDirSize );

	// Go to the beginning of the directory & process each entry
	const unsigned char *pfhStart = dirData.GetBuffer();
	const unsigned char *pfh = pfhStart;
	// first, count number of entries
	int nNumEntries = 0;
	while ( pfh - pfhStart < nDirSize )
	{
		const SZipFileHeader &hdr = *reinterpret_cast<const SZipFileHeader *>( pfh );
		// Check the directory entry integrity.
		if ( hdr.dwSignature != SZipFileHeader::SIGNATURE )
		{
			return;
		}
		else if ( hdr.wCompression == SZipLocalFileHeader::COMP_DEFLAT ||
		          hdr.wCompression == SZipLocalFileHeader::COMP_STORE )
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
		const SZipFileHeader &hdr = *reinterpret_cast<const SZipFileHeader *>( pfh );
		// Check the directory entry integrity.
		if ( hdr.dwSignature != SZipFileHeader::SIGNATURE )
		{
			papDir.clear();
			return;
		}
		else if ( hdr.wCompression == SZipLocalFileHeader::COMP_DEFLAT ||
		          hdr.wCompression == SZipLocalFileHeader::COMP_STORE )
		{
			std::vector<SFileHeader>::iterator posFileHeader = papDir.insert( papDir.end(), SFileHeader() );
			posFileHeader->Init( hdr );
			//
			pfh += sizeof( hdr ) + hdr.wFileNameLen + hdr.wExtraLen + hdr.wCommentLen;
		}
		else
		{
			NI_ASSERT(
					false,
					fmt::format(
							"File \"{}\" has wrong compression! Only Deflate and Store methods allowed (-9 and -0 options for zip.exe)",
							hdr.GetName() ) );
		}
	}
}

CZipFile::~CZipFile()
{
	mmf.UnmapFile();
}

void CZipFile::GetFileName( int nIndex, std::string *pString ) const
{
	*pString = papDir[nIndex].pszFileName;
}

int CZipFile::GetFileLen( int nIndex ) const
{
	return papDir[nIndex].dwUSize;
}

uint32_t CZipFile::GetFileAttribs( int nIndex ) const
{
	return papDir[nIndex].wExtAttr;
}

uint32_t CZipFile::GetModDateTime( int nIndex ) const
{
	return papDir[nIndex].dwModDateTime;
}

CDataStream *CZipFile::OpenFile( const SFileHeader &ghdr )
{
	CMemoryMappedFileFragment fLocalHdr( &mmf, ghdr.dwHdrOffset, sizeof( SZipLocalFileHeader ) );

	const SZipLocalFileHeader &hdr = *(const SZipLocalFileHeader *)fLocalHdr.GetBuffer();
	NI_ASSERT( hdr.dwSignature == SZipLocalFileHeader::SIGNATURE, "can't recognize zip local header" );

	// Skip extra fields
	int nBeginPos = ghdr.dwHdrOffset + hdr.wFileNameLen + hdr.wExtraLen + sizeof( hdr );

	// The sizes come from the central directory entry, ghdr, not from this
	// local header. A zip written as a stream sets bit 3 of the flags and
	// leaves the local header's sizes zero, the real ones following the data
	// in a descriptor; the central directory always has them. libarchive, and
	// so cmake -E tar, writes zips that way, and so do other modern tools a
	// mod's pak might be made with: read through the local sizes, every file in
	// such an archive opened empty.
	const uint32_t dwCSize = ghdr.dwCSize;
	const uint32_t dwUSize = ghdr.dwUSize;

	// in the STORE case we can create range adaptor for direct read
	if ( hdr.wCompression == SZipLocalFileHeader::COMP_STORE )
	{
		if ( nBeginPos + dwCSize > nTotalSize )
		{
			return 0;
		}
		return new CMemoryMappedFileFragment( &mmf, nBeginPos, dwCSize );
	}
	// create new memory stream and setup stats
	CMemoryStream *pDstStream = new CMemoryStream;
	pDstStream->SetSizeDiscard( dwUSize );
	const void *pBuf = pDstStream->GetBuffer();
	// proceed with DEFLAT unpacking
	NI_ASSERT( hdr.wCompression == SZipLocalFileHeader::COMP_DEFLAT,
	           "Can support only STORE and DEFLAT cpmpression methods" );

	// Alloc compressed data buffer and read the whole stream
	CMemoryMappedFileFragment cprData( &mmf, nBeginPos, dwCSize );
	const unsigned char *pcData = cprData.GetBuffer(); //new char[dwCSize];

	// Setup the inflate stream.
	z_stream stream;
	stream.next_in = (Bytef *)pcData;
	stream.avail_in = (uInt)dwCSize;
	stream.next_out = (Bytef *)pBuf;
	stream.avail_out = dwUSize;
	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;

	// Perform inflation. wbits < 0 indicates no zlib header inside the data.
	int err = inflateInit2( &stream, -MAX_WBITS );
	if ( err == Z_OK )
	{
		err = inflate( &stream, Z_FINISH );
		inflateEnd( &stream );
		// CRAP{ почему-то иногда при распаковке возвращается "buffer error" всесто "stream end"...
		if ( ( err == Z_STREAM_END ) || ( err == Z_BUF_ERROR ) )
		{
			err = Z_OK;
		}
		// CRAP}
		inflateEnd( &stream );
	}

	if ( err == Z_OK )
	{
		return pDstStream;
	}

	delete pDstStream;
	return 0;
}

CDataStream *CZipFile::OpenFile( int nIndex )
{
	// Go to the actual file and read the local header.
	return OpenFile( papDir[nIndex] );
}
