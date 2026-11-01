#include "stdafx.h"

#include "AILogic/DBAIConsts.h"
#include "CoopScriptChecksum.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "System/Streams.h"
#include "System/VFSOperations.h"

#include <cstdint>
#include <string>

#include <zlib.h>

namespace
{
uint32_t HashInteger( uint32_t checksum, uint32_t value )
{
	// Fixed byte order keeps the script fingerprint independent of the host ABI.
	const uint8_t bytes[] = { uint8_t( value ), uint8_t( value >> 8 ), uint8_t( value >> 16 ), uint8_t( value >> 24 ) };
	return crc32( checksum, bytes, sizeof( bytes ) );
}

uint32_t HashScriptFile( uint32_t checksum, const std::string &filename )
{
	checksum = HashInteger( checksum, uint32_t( filename.size() ) );
	if ( filename.empty() )
	{
		return HashInteger( checksum, 0 );
	}
	checksum = crc32( checksum, reinterpret_cast<const Bytef *>( filename.data() ), uInt( filename.size() ) );
	if ( !NVFS::GetMainVFS() )
	{
		return HashInteger( checksum, 1 );
	}

	// Use the same VFS lookup as CScripts::RunScriptFromFile, including active mod overlays.
	CFileStream stream( NVFS::GetMainVFS(), filename );
	if ( !stream.IsOk() )
	{
		return HashInteger( checksum, 1 );
	}
	// A missing named file differs from a successfully loaded empty file.
	checksum = HashInteger( checksum, 2 );
	const int nSize = stream.GetSize();
	checksum = HashInteger( checksum, uint32_t( nSize ) );
	if ( nSize > 0 )
	{
		checksum = crc32( checksum, stream.GetBuffer(), uInt( nSize ) );
	}
	return checksum;
}
} // namespace

uint32_t NGameX::CalculateCoopScriptChecksum( const NDb::SMapInfo *pMap, const NDb::SAIGameConsts *pAIConsts )
{
	// Version the fingerprint layout; entry boundaries and order are part of the input.
	uint32_t checksum = HashInteger( crc32( 0, Z_NULL, 0 ), 1 );
	checksum = HashInteger( checksum, pAIConsts ? uint32_t( pAIConsts->commonScriptFileRefs.size() ) : UINT32_MAX );
	if ( pAIConsts )
	{
		for ( const auto &entry : pAIConsts->commonScriptFileRefs )
		{
			checksum = HashScriptFile( checksum, entry.szScriptFileRef );
		}
	}
	checksum = HashInteger( checksum, pMap ? 1 : 0 );
	if ( pMap )
	{
		checksum = HashScriptFile( checksum, pMap->szScriptFileRef );
	}
	return checksum;
}
