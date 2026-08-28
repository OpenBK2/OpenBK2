// The container: reading a .gr2 and handing back the object tree inside it.
//
// GrannyReadEntireFileFromMemory is the first entry point the game reaches, and
// a traced run says so: it is call 1, with nothing before it, not even the
// allocator. Every model arrives out of a .pak already in memory, so this is the
// form that matters. GrannyReadEntireFile exists for the same reason it does in
// Granny, a loose file on disk; the editor and SceneB2/TerraTools.cpp use it, the
// game never does.
//
// What is here is the container and nothing above it: header validation, the
// section array, section bytes, and the pointer fixups. What is not here yet is
// the two Oodle codecs, so a compressed section is refused, and the type tree
// walk that would turn the root object into a granny_file_info, so
// GrannyGetFileInfo still returns null.
//
// Every rejection says why. A file this refuses is either corrupt or a dialect
// nobody has seen, and both are worth a line in the trace rather than a silent
// null, since the engine's answer to null is to carry on and dereference it.

#include "File.h"

#include "Trace.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <new>
#include <vector>

namespace NGr2
{

const uint8_t FILE_MAGIC[MAGIC_SIZE] = { 0xb8, 0x67, 0xb0, 0xca, 0xf8, 0x6d, 0xb1, 0x0f,
                                         0x84, 0x72, 0x8c, 0x7e, 0x5e, 0x19, 0x00, 0x1e };

namespace
{

//! A little-endian uint32 at nOffset. The caller has already bounds checked.
uint32_t ReadU32( const uint8_t *pBytes, uint32_t nOffset )
{
	uint32_t nValue = 0;
	memcpy( &nValue, pBytes + nOffset, sizeof( nValue ) );
	return nValue;
}

//! Does [nOffset, nOffset + nCount * nStride) fit inside nSize?
//!
//! 64-bit arithmetic throughout, because the whole point is the case where the
//! product does not fit in 32 bits. A count of 0x0fffffff times a 44-byte record
//! wraps to something small, and a loader that trusts the wrapped product reads a
//! quarter of a gigabyte past the end of the buffer.
bool FitsWithin( uint64_t nOffset, uint64_t nCount, uint64_t nStride, uint64_t nSize )
{
	return nOffset <= nSize && nCount * nStride <= nSize - nOffset;
}

//! Say why a file was refused, and return the null the caller hands back.
template <typename... TArgs>
granny_file *Reject( fmt::format_string<TArgs...> fmtArgs, TArgs... args )
{
	Logger().warn( "rejected: {}", fmt::format( fmtArgs, args... ) );
	return 0;
}

}

}

using namespace NGr2;

granny_file *granny_file::ReadFromMemory( const void *pMemory, granny_int32x nSize )
{
	if ( pMemory == 0 )
	{
		return Reject( "null buffer" );
	}
	// granny_int32x is signed and CGrannyMemFileLoader::RecalcValue passes it
	// straight through from CFileStream::GetSize, so a failed stat arrives here as
	// -1. Reading that as unsigned would ask for four gigabytes.
	if ( nSize <= 0 )
	{
		return Reject( "size {}", nSize );
	}

	const uint8_t *pBytes = static_cast<const uint8_t *>( pMemory );
	const uint32_t nBytes = static_cast<uint32_t>( nSize );

	if ( nBytes < MAGIC_SIZE || memcmp( pBytes, FILE_MAGIC, MAGIC_SIZE ) != 0 )
	{
		// Not a corrupted GR2 so much as a different kind of file. Entries under
		// bin/Geometries/ and its siblings are addressed by extensionless GUID and
		// a handful of them are not GR2 at all.
		return Reject( "not a GR2, {} bytes beginning {:02x}{:02x}{:02x}{:02x}", nBytes,
		               nBytes > 0 ? pBytes[0] : 0, nBytes > 1 ? pBytes[1] : 0,
		               nBytes > 2 ? pBytes[2] : 0, nBytes > 3 ? pBytes[3] : 0 );
	}
	if ( nBytes < PREFIX_SIZE )
	{
		return Reject( "{} bytes, too short for the {}-byte header", nBytes, PREFIX_SIZE );
	}

	const uint32_t nHeaderSize = ReadU32( pBytes, 16 );
	const uint32_t nHeaderFormat = ReadU32( pBytes, 20 );
	if ( nHeaderFormat != 0 )
	{
		// A non-zero headerFormat means the header itself is compressed. It is 0 in
		// all 7,844 files measured, so there is nothing to test an implementation
		// against, and guessing would be worse than refusing.
		return Reject( "headerFormat {}, only 0 is supported", nHeaderFormat );
	}

	const uint32_t nVersion = ReadU32( pBytes, HEADER_OFFSET + 0 );
	if ( nVersion != FILE_VERSION )
	{
		return Reject( "file format {}, expected {}", nVersion, FILE_VERSION );
	}

	const uint32_t nTotalSize = ReadU32( pBytes, HEADER_OFFSET + 4 );
	if ( nTotalSize > nBytes )
	{
		// Truncated. A short file is short, not rewritten, so its header still
		// claims the length it was written with.
		return Reject( "header says {} bytes, buffer holds {}", nTotalSize, nBytes );
	}
	// The CRC at HEADER_OFFSET + 8 is read by nobody here. Whether the real DLL
	// checks it, over what range and with which polynomial, is unestablished, and a
	// check invented now could refuse files the game has always loaded.

	const uint32_t nSectionArrayOffset = ReadU32( pBytes, HEADER_OFFSET + 12 );
	const uint32_t nSectionCount = ReadU32( pBytes, HEADER_OFFSET + 16 );
	if ( nSectionCount == 0 )
	{
		return Reject( "no sections" );
	}
	const uint64_t nArrayBegin = uint64_t( HEADER_OFFSET ) + nSectionArrayOffset;
	if ( !FitsWithin( nArrayBegin, nSectionCount, SECTION_RECORD_SIZE, nBytes ) )
	{
		return Reject( "{} sections at {} do not fit in {} bytes", nSectionCount, nArrayBegin,
		               nBytes );
	}
	const uint64_t nArrayEnd = nArrayBegin + uint64_t( nSectionCount ) * SECTION_RECORD_SIZE;
	if ( nHeaderSize != nArrayEnd )
	{
		// headerSize equals the end of the section array in all 7,844 measured
		// files. Disagreement means the two descriptions of the same boundary have
		// parted company, and neither can then be trusted.
		return Reject( "headerSize {} but the section array ends at {}", nHeaderSize, nArrayEnd );
	}

	// Owning from here on, so that every rejection below cleans up by returning.
	std::unique_ptr<granny_file> pFile( new ( std::nothrow ) granny_file );
	if ( !pFile )
	{
		return Reject( "out of memory" );
	}

	pFile->m_RootObjectType.nSection = ReadU32( pBytes, HEADER_OFFSET + 20 );
	pFile->m_RootObjectType.nOffset = ReadU32( pBytes, HEADER_OFFSET + 24 );
	pFile->m_RootObject.nSection = ReadU32( pBytes, HEADER_OFFSET + 28 );
	pFile->m_RootObject.nOffset = ReadU32( pBytes, HEADER_OFFSET + 32 );
	pFile->m_nTypeTag = ReadU32( pBytes, HEADER_OFFSET + 36 );

	pFile->m_Sections.resize( nSectionCount );
	pFile->m_SectionData.resize( nSectionCount );
	pFile->m_PointerFixups.resize( nSectionCount );

	for ( uint32_t i = 0; i < nSectionCount; ++i )
	{
		const uint32_t nBase = static_cast<uint32_t>( nArrayBegin ) + SECTION_RECORD_SIZE * i;
		SSection &section = pFile->m_Sections[i];
		section.nCompression = ReadU32( pBytes, nBase + 0 );
		section.nDataOffset = ReadU32( pBytes, nBase + 4 );
		section.nDataSize = ReadU32( pBytes, nBase + 8 );
		section.nExpandedDataSize = ReadU32( pBytes, nBase + 12 );
		section.nInternalAlignment = ReadU32( pBytes, nBase + 16 );
		section.nFirst16Bit = ReadU32( pBytes, nBase + 20 );
		section.nFirst8Bit = ReadU32( pBytes, nBase + 24 );
		section.nPointerFixupOffset = ReadU32( pBytes, nBase + 28 );
		section.nPointerFixupCount = ReadU32( pBytes, nBase + 32 );
		section.nMixedMarshallingOffset = ReadU32( pBytes, nBase + 36 );
		section.nMixedMarshallingCount = ReadU32( pBytes, nBase + 40 );

		if ( section.nCompression > COMPRESSION_OODLE_2 )
		{
			return Reject( "section {} compression {}, no such codec", i, section.nCompression );
		}
		if ( section.nCompression != COMPRESSION_NONE )
		{
			// The one thing between this and the shipped corpus. 61% of retail
			// sections take one codec and 38% the other, so almost nothing real
			// loads until both are written.
			return Reject( "section {} needs Oodle codec {}, which is not implemented", i,
			               section.nCompression );
		}
		if ( section.nExpandedDataSize != section.nDataSize )
		{
			return Reject( "section {} is uncompressed but expands {} to {}", i, section.nDataSize,
			               section.nExpandedDataSize );
		}
		if ( !FitsWithin( section.nDataOffset, 1, section.nDataSize, nBytes ) )
		{
			return Reject( "section {} data at {} plus {} bytes is outside {}", i,
			               section.nDataOffset, section.nDataSize, nBytes );
		}
		if ( !FitsWithin( section.nPointerFixupOffset, section.nPointerFixupCount,
		                  POINTER_FIXUP_SIZE, nBytes ) )
		{
			return Reject( "section {}: {} pointer fixups at {} do not fit in {}", i,
			               section.nPointerFixupCount, section.nPointerFixupOffset, nBytes );
		}
		if ( !FitsWithin( section.nMixedMarshallingOffset, section.nMixedMarshallingCount,
		                  MIXED_MARSHALLING_FIXUP_SIZE, nBytes ) )
		{
			return Reject( "section {}: {} marshalling fixups at {} do not fit in {}", i,
			               section.nMixedMarshallingCount, section.nMixedMarshallingOffset,
			               nBytes );
		}

		pFile->m_SectionData[i].assign( pBytes + section.nDataOffset,
		                                pBytes + section.nDataOffset + section.nDataSize );
	}

	// The mixed marshalling fixups are bounds checked above and then ignored. They
	// say which members need byte swapping when the file's byte order differs from
	// the host's, and this dialect is little endian read on little endian hosts, so
	// there is nothing to swap. 170 sections of 61,952 carry one, so the array is
	// not dead, only inapplicable.

	for ( uint32_t i = 0; i < nSectionCount; ++i )
	{
		const SSection &section = pFile->m_Sections[i];
		std::vector<SPointerFixup> &fixups = pFile->m_PointerFixups[i];
		fixups.resize( section.nPointerFixupCount );

		for ( uint32_t k = 0; k < section.nPointerFixupCount; ++k )
		{
			const uint32_t nBase = section.nPointerFixupOffset + POINTER_FIXUP_SIZE * k;
			SPointerFixup &fixup = fixups[k];
			fixup.nFromOffset = ReadU32( pBytes, nBase + 0 );
			fixup.To.nSection = ReadU32( pBytes, nBase + 4 );
			fixup.To.nOffset = ReadU32( pBytes, nBase + 8 );

			// A pointer slot is four bytes wide in the file whatever the host is, so
			// the whole slot has to fit, not just its first byte.
			if ( !FitsWithin( fixup.nFromOffset, 1, 4, section.nExpandedDataSize ) )
			{
				return Reject( "section {} fixup {}: slot at {} is outside the section's {} bytes",
				               i, k, fixup.nFromOffset, section.nExpandedDataSize );
			}
			if ( fixup.To.nSection >= nSectionCount )
			{
				return Reject( "section {} fixup {} points into section {}, of {}", i, k,
				               fixup.To.nSection, nSectionCount );
			}
			// A target may sit one past the end, which is what a pointer to an empty
			// array looks like, so this is the one bound that is not strict.
			if ( fixup.To.nOffset > pFile->m_Sections[fixup.To.nSection].nExpandedDataSize )
			{
				return Reject( "section {} fixup {} points at {}:{}, past that section's {} bytes",
				               i, k, fixup.To.nSection, fixup.To.nOffset,
				               pFile->m_Sections[fixup.To.nSection].nExpandedDataSize );
			}
		}

		// Sorted so that ResolvePointer can binary search. Files store them
		// unsorted: 15,287 of the 15,373 arrays measured are out of order.
		std::sort( fixups.begin(), fixups.end(),
		           []( const SPointerFixup &a, const SPointerFixup &b )
		           { return a.nFromOffset < b.nFromOffset; } );
	}

	if ( pFile->m_RootObject.nSection >= nSectionCount )
	{
		return Reject( "root object is in section {}, of {}", pFile->m_RootObject.nSection,
		               nSectionCount );
	}
	if ( pFile->m_RootObjectType.nSection >= nSectionCount )
	{
		return Reject( "root object type is in section {}, of {}",
		               pFile->m_RootObjectType.nSection, nSectionCount );
	}

	return pFile.release();
}

bool granny_file::ResolvePointer( uint32_t nSection, uint32_t nOffset, SReference *pTarget ) const
{
	if ( nSection >= SectionCount() )
	{
		return false;
	}

	const std::vector<SPointerFixup> &fixups = m_PointerFixups[nSection];
	const auto it = std::lower_bound( fixups.begin(), fixups.end(), nOffset,
	                                  []( const SPointerFixup &fixup, uint32_t n )
	                                  { return fixup.nFromOffset < n; } );
	if ( it == fixups.end() || it->nFromOffset != nOffset )
	{
		return false;
	}

	if ( pTarget != 0 )
	{
		*pTarget = it->To;
	}
	return true;
}

extern "C"
{

GR2_API( granny_file * ) GrannyReadEntireFile( char const *FileName )
{
	GR2_TRACE( "FileName={}", FileName );

	if ( FileName == 0 )
	{
		return Reject( "null file name" );
	}

	std::ifstream in( FileName, std::ios::binary );
	if ( !in )
	{
		return Reject( "cannot open {}", FileName );
	}

	// The whole file into memory, which is what the name promises and what the
	// from-memory path needs anyway. The game never takes this route; the editor
	// and SceneB2/TerraTools.cpp do.
	const std::vector<uint8_t> bytes( ( std::istreambuf_iterator<char>( in ) ),
	                                  std::istreambuf_iterator<char>() );
	return granny_file::ReadFromMemory( bytes.data(),
	                                    static_cast<granny_int32x>( bytes.size() ) );
}

GR2_API( granny_file * ) GrannyReadEntireFileFromMemory( granny_int32x MemorySize,
                                                         void const *Memory )
{
	GR2_TRACE( "MemorySize={} Memory={}", MemorySize, Memory );
	return granny_file::ReadFromMemory( Memory, MemorySize );
}

GR2_API( void ) GrannyFreeFile( granny_file *File )
{
	GR2_TRACE( "File={}", File );

	// Null is expected rather than defended against. CGrannyMemFileLoader::
	// RecalcValue stores whatever the read returned and CGrannyFile::~CGrannyFile
	// frees it, so every refused file comes back through here.
	delete File;
}

GR2_API( granny_file_info * ) GrannyGetFileInfo( granny_file *File )
{
	GR2_STUB( "File={}", File );

	// Needs the type tree walk that turns the root object into native structures.
	// The file is loaded and the root object's location is known; the layout to
	// read it with is what is missing. Until then the engine gets the same null it
	// got when nothing at all was implemented.
	return 0;
}

}
