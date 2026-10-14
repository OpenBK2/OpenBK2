#include "stdafx.h"

#include "BuildStamp.h"

#include <cstdint>
#include <cstring>
#include <vector>

#include <boost/predef.h>

#if BOOST_OS_WINDOWS
#include <windows.h>
#else
#include <elf.h>
#include <link.h>
#endif

namespace NBuildStamp
{

// The modules whose code decides what the simulation does, in a fixed order so
// that the digest is a function of their contents and nothing else.
//
// Add one here when it starts carrying simulation code. The list missing a
// module makes the stamp too quiet, which is the failure that costs debugging
// time; the list naming a presentation module makes it too loud, which only
// costs a line of output. Err towards including.
//
// 3Dmotor, Sound, UI and the editor libraries are deliberately absent: they are
// presentation, and a replay recorded before a renderer change is still a valid
// recording of the simulation.
static const char *SIMULATION_MODULES[] = {
	"AILogic",       // the simulation itself
	"Common_RTS_AI", // pathfinding, the AI map, checksums
	"B2_M1_World",   // the world the simulation updates
	"B2_M1_Terrain", // terrain the simulation reads
	"Stats_B2_M1",   // unit and weapon statistics, and the RPG stats
	"GameX",         // the transceiver, the commands history, the replay itself
	"System",        // the random generator, the savers, the checksum log
	"Misc",          // shared arithmetic reached from the simulation
};

// FNV-1a over each module's name and raw build id, which only has to separate
// one set of modules from another rather than resist anything.
static const uint64_t FNV_OFFSET = 0xcbf29ce484222325ull;
static const uint64_t FNV_PRIME = 0x100000001b3ull;

static void Digest( uint64_t *pHash, const void *pData, const size_t nSize )
{
	const uint8_t *pBytes = static_cast<const uint8_t *>( pData );
	for ( size_t i = 0; i < nSize; ++i )
	{
		*pHash ^= pBytes[i];
		*pHash *= FNV_PRIME;
	}
}

static std::string ToHex( const uint8_t *pBytes, const size_t nSize )
{
	static const char szDigits[] = "0123456789abcdef";
	std::string szResult;
	szResult.reserve( nSize * 2 );
	for ( size_t i = 0; i < nSize; ++i )
	{
		szResult += szDigits[pBytes[i] >> 4];
		szResult += szDigits[pBytes[i] & 0x0f];
	}
	return szResult;
}

#if BOOST_OS_WINDOWS

// The PDB GUID and age out of one loaded module's debug directory, which is the
// CodeView record a symbol server keys on. The mapping is walked in memory
// rather than the file being read off disk: it is already there, and the file
// may have been replaced since it was loaded.
//
// GetModuleHandle by name rather than enumerating the process's modules, which
// would need psapi and a second code path for ordering.
static bool ReadModuleId( std::vector<uint8_t> *pResult, const char *pszModule )
{
	const std::string szModule = std::string( pszModule ) + ".dll";
	const uint8_t *pBase = reinterpret_cast<const uint8_t *>( GetModuleHandleA( szModule.c_str() ) );
	if ( pBase == 0 )
		return false;

	const IMAGE_DOS_HEADER *pDos = reinterpret_cast<const IMAGE_DOS_HEADER *>( pBase );
	if ( pDos->e_magic != IMAGE_DOS_SIGNATURE )
		return false;
	const IMAGE_NT_HEADERS *pNt = reinterpret_cast<const IMAGE_NT_HEADERS *>( pBase + pDos->e_lfanew );
	if ( pNt->Signature != IMAGE_NT_SIGNATURE )
		return false;

	const IMAGE_DATA_DIRECTORY &dir = pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
	if ( dir.VirtualAddress == 0 || dir.Size < sizeof( IMAGE_DEBUG_DIRECTORY ) )
		return false;

	// The layout of a CV_INFO_PDB70 record. Not a type in the SDK headers, so it
	// is spelled out here; the signature field is 'RSDS' for this version.
	struct SCodeViewPdb70
	{
		uint32_t dwSignature;
		GUID guid;
		uint32_t dwAge;
		char szPdbFileName[1];
	};
	const uint32_t CODEVIEW_PDB70 = 0x53445352;

	const IMAGE_DEBUG_DIRECTORY *pDebug = reinterpret_cast<const IMAGE_DEBUG_DIRECTORY *>( pBase + dir.VirtualAddress );
	const size_t nEntries = dir.Size / sizeof( IMAGE_DEBUG_DIRECTORY );
	for ( size_t i = 0; i < nEntries; ++i )
	{
		if ( pDebug[i].Type != IMAGE_DEBUG_TYPE_CODEVIEW || pDebug[i].AddressOfRawData == 0 )
			continue;
		if ( pDebug[i].SizeOfData < sizeof( SCodeViewPdb70 ) )
			continue;
		const SCodeViewPdb70 *pCv = reinterpret_cast<const SCodeViewPdb70 *>( pBase + pDebug[i].AddressOfRawData );
		if ( pCv->dwSignature != CODEVIEW_PDB70 )
			continue;
		const uint8_t *pGuid = reinterpret_cast<const uint8_t *>( &pCv->guid );
		pResult->assign( pGuid, pGuid + sizeof( pCv->guid ) );
		const uint8_t *pAge = reinterpret_cast<const uint8_t *>( &pCv->dwAge );
		pResult->insert( pResult->end(), pAge, pAge + sizeof( pCv->dwAge ) );
		return true;
	}
	return false;
}

#else

struct SModuleIdSearch
{
	std::string szWanted;
	std::vector<uint8_t> *pResult = 0;
	bool bFound = false;
};

// The GNU build id of one loaded object, found by the file name the loader
// recorded for it. dlpi_name is a path, so only its last component is compared,
// and an object's name can carry a version suffix after the soname.
static int OnPhdr( struct dl_phdr_info *pInfo, size_t nSize, void *pData )
{
	SModuleIdSearch *pSearch = static_cast<SModuleIdSearch *>( pData );
	const char *pszName = pInfo->dlpi_name != 0 ? pInfo->dlpi_name : "";
	const char *pszSlash = strrchr( pszName, '/' );
	const std::string szBase = pszSlash != 0 ? pszSlash + 1 : pszName;
	if ( szBase.compare( 0, pSearch->szWanted.size(), pSearch->szWanted ) != 0 )
		return 0;

	for ( ElfW( Half ) i = 0; i < pInfo->dlpi_phnum; ++i )
	{
		const ElfW( Phdr ) &phdr = pInfo->dlpi_phdr[i];
		if ( phdr.p_type != PT_NOTE )
			continue;

		const uint8_t *pNote = reinterpret_cast<const uint8_t *>( pInfo->dlpi_addr + phdr.p_vaddr );
		const uint8_t *pEnd = pNote + phdr.p_memsz;
		// Notes are a chain of (header, name, descriptor), each of the two
		// trailing parts padded to four bytes. NT_GNU_ABI_TAG shares the "GNU"
		// owner name, so the type is what selects the right one.
		while ( pNote + sizeof( ElfW( Nhdr ) ) <= pEnd )
		{
			const ElfW( Nhdr ) *pHdr = reinterpret_cast<const ElfW( Nhdr ) *>( pNote );
			const uint8_t *pName = pNote + sizeof( ElfW( Nhdr ) );
			const uint8_t *pDesc = pName + ( ( pHdr->n_namesz + 3 ) & ~3u );
			if ( pDesc + pHdr->n_descsz > pEnd )
				break;
			if ( pHdr->n_type == NT_GNU_BUILD_ID && pHdr->n_namesz == 4 && memcmp( pName, "GNU", 4 ) == 0 &&
			     pHdr->n_descsz > 0 )
			{
				pSearch->pResult->assign( pDesc, pDesc + pHdr->n_descsz );
				pSearch->bFound = true;
				return 1;
			}
			pNote = pDesc + ( ( pHdr->n_descsz + 3 ) & ~3u );
		}
	}
	return 1;
}

static bool ReadModuleId( std::vector<uint8_t> *pResult, const char *pszModule )
{
	SModuleIdSearch search;
	search.szWanted = std::string( "lib" ) + pszModule + ".so";
	search.pResult = pResult;
	dl_iterate_phdr( OnPhdr, &search );
	return search.bFound;
}

#endif

static std::string ReadSimulationImageId()
{
	uint64_t uHash = FNV_OFFSET;
	int nFound = 0;
	for ( const char *pszModule : SIMULATION_MODULES )
	{
		std::vector<uint8_t> id;
		if ( !ReadModuleId( &id, pszModule ) )
			continue;
		// The name as well as the id, so that two modules swapping ids, or one
		// being absent, cannot produce the same digest.
		Digest( &uHash, pszModule, strlen( pszModule ) );
		Digest( &uHash, id.data(), id.size() );
		++nFound;
	}
	if ( nFound == 0 )
		return std::string();

	// Big endian, so the text reads in the order the bytes were hashed.
	uint8_t bytes[8];
	for ( int i = 0; i < 8; ++i )
		bytes[i] = static_cast<uint8_t>( uHash >> ( 56 - i * 8 ) );
	return ToHex( bytes, sizeof( bytes ) );
}

// Read off the compiler's own macros rather than passed down from CMake, so it
// cannot disagree with how this translation unit was actually built. GCC and
// clang spell the sanitizer macros differently: GCC defines __SANITIZE_*__,
// clang answers __has_feature.
static std::string ReadBuildConfiguration()
{
#if defined( NDEBUG )
	std::string szResult = "release";
#else
	std::string szResult = "debug";
#endif

#if defined( __SANITIZE_THREAD__ )
	szResult += "+tsan";
#elif defined( __has_feature )
#if __has_feature( thread_sanitizer )
	szResult += "+tsan";
#endif
#endif

#if defined( __SANITIZE_ADDRESS__ )
	szResult += "+asan";
#elif defined( __has_feature )
#if __has_feature( address_sanitizer )
	szResult += "+asan";
#endif
#endif

	// No macro for UBSan on either compiler, so the preset's own definition is
	// the only way to know. Absent that, an undefined-behaviour build simply
	// reports as the release or debug build it also is.
#if defined( ENABLE_UBSAN )
	szResult += "+ubsan";
#endif

	return szResult;
}

const std::string &GetBuildConfiguration()
{
	static const std::string szConfiguration = ReadBuildConfiguration();
	return szConfiguration;
}

const std::string &GetSimulationImageId()
{
	// Function-local static: the answer is fixed for the life of the process,
	// and this is read while writing a replay rather than in any hot path.
	static const std::string szImageId = ReadSimulationImageId();
	return szImageId;
}

}
