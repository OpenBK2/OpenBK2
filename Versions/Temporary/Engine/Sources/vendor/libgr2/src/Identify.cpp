#include "Identify.h"

#include "Structures.h"
#include "Trace.h"

#include <map>
#include <mutex>
#include <vector>

namespace NGr2
{

namespace
{

//! One loaded file: what it was, and which of its objects have been handed out.
struct SFileRecord
{
	//! Sequential, so that a log names files in the order they were read.
	uint64_t nId = 0;
	//! Zero when the file came in by name rather than as bytes.
	uint64_t nHash = 0;
	size_t nBytes = 0;
	//! The path the by-name entry point was given, empty otherwise.
	std::string sPath;
	//! What the exporter recorded, which is a .mb under J:/Complete on every
	//! file in this game and so says what the resource is even when it does not
	//! say which one.
	std::string sFromFileName;
	//! Every object pointer registered for this file, so that forgetting it is
	//! not a scan of the whole map.
	std::vector<const void *> Objects;
};

struct SRegistry
{
	std::map<const void *, SFileRecord> Files;
	//! Object pointer to the file handle that owns it.
	std::map<const void *, const void *> Owners;
	uint64_t nNextId = 0;
};

SRegistry &Registry()
{
	static SRegistry registry;
	return registry;
}

std::mutex &RegistryMutex()
{
	static std::mutex mutex;
	return mutex;
}

//! A safe read of a string that came out of a file's own bytes.
std::string Text( const char *psz )
{
	return psz != nullptr ? std::string( psz ) : std::string();
}

//! "file#3 8f0c1d2e3a4b5c6d \"J:/.../RIFLE/2.mb\"", the form every message uses.
//!
//! Called with the lock held.
std::string Name( const SFileRecord &record )
{
	std::string s = fmt::format( "file#{}", record.nId );
	if ( record.nHash != 0 )
	{
		s += fmt::format( " {:016x}", record.nHash );
	}
	if ( !record.sPath.empty() )
	{
		s += fmt::format( " \"{}\"", record.sPath );
	}
	if ( !record.sFromFileName.empty() )
	{
		s += fmt::format( " \"{}\"", record.sFromFileName );
	}
	return s;
}

//! Add one object to a file's registration, if it is not null.
void Own( SRegistry &registry, SFileRecord &record, const void *pFileHandle,
          const void *pObject )
{
	if ( pObject == nullptr )
	{
		return;
	}
	record.Objects.push_back( pObject );
	// Assigned rather than inserted. Two live files cannot share an object, but
	// a freed file's addresses can come back in a new one, and if a Forget was
	// ever missed the newer owner is the right answer.
	registry.Owners[pObject] = pFileHandle;
}

}

uint64_t HashBytes( const void *pMemory, size_t nBytes )
{
	// FNV-1a, 64-bit, with the published offset basis and prime.
	uint64_t nHash = 14695981039346656037ULL;
	const uint8_t *pBytes = static_cast<const uint8_t *>( pMemory );
	if ( pBytes == nullptr )
	{
		return 0;
	}
	for ( size_t i = 0; i < nBytes; ++i )
	{
		nHash ^= pBytes[i];
		nHash *= 1099511628211ULL;
	}
	return nHash;
}

void NoteFileBytes( const void *pFileHandle, const void *pMemory, size_t nBytes )
{
	if ( pFileHandle == nullptr )
	{
		// A refused file. There is nothing to attach the hash to, and the reason
		// it was refused has already been logged by whoever refused it.
		return;
	}
	const uint64_t nHash = HashBytes( pMemory, nBytes );

	std::lock_guard<std::mutex> lock( RegistryMutex() );
	SRegistry &registry = Registry();
	SFileRecord &record = registry.Files[pFileHandle];
	if ( record.nId == 0 )
	{
		record.nId = ++registry.nNextId;
	}
	record.nHash = nHash;
	record.nBytes = nBytes;
}

void NoteFileName( const void *pFileHandle, const char *pszFileName )
{
	if ( pFileHandle == nullptr )
	{
		return;
	}
	std::lock_guard<std::mutex> lock( RegistryMutex() );
	SRegistry &registry = Registry();
	SFileRecord &record = registry.Files[pFileHandle];
	if ( record.nId == 0 )
	{
		record.nId = ++registry.nNextId;
	}
	record.sPath = Text( pszFileName );
}

void NoteFileInfo( const void *pFileHandle, const SFileInfo *pInfo )
{
	if ( pFileHandle == nullptr )
	{
		return;
	}

	std::string sLine;
	{
		std::lock_guard<std::mutex> lock( RegistryMutex() );
		SRegistry &registry = Registry();
		SFileRecord &record = registry.Files[pFileHandle];
		if ( record.nId == 0 )
		{
			record.nId = ++registry.nNextId;
		}
		if ( !record.Objects.empty() )
		{
			// Already registered. GrannyGetFileInfo is called more than once per
			// file by the engine, and describing the same file at every call
			// would bury the ones that are new.
			return;
		}
		if ( pInfo == nullptr )
		{
			Logger().warn( "{} has no file info: nothing in it can be named later",
			               Name( record ) );
			return;
		}

		record.sFromFileName = Text( pInfo->pszFromFileName );

		std::string sBones;
		for ( int32_t i = 0; i < pInfo->nSkeletonCount && pInfo->ppSkeletons != nullptr; ++i )
		{
			const SSkeleton *pSkeleton = pInfo->ppSkeletons[i];
			Own( registry, record, pFileHandle, pSkeleton );
			if ( pSkeleton != nullptr )
			{
				sBones += fmt::format( "{}{}", sBones.empty() ? "" : ",",
				                       pSkeleton->nBoneCount );
			}
		}
		for ( int32_t i = 0; i < pInfo->nModelCount && pInfo->ppModels != nullptr; ++i )
		{
			// The model too, though the engine's own granny_model is the one it
			// instantiates. A file's model still identifies the file when
			// something else passes one in.
			Own( registry, record, pFileHandle, pInfo->ppModels[i] );
		}
		for ( int32_t i = 0; i < pInfo->nAnimationCount && pInfo->ppAnimations != nullptr; ++i )
		{
			Own( registry, record, pFileHandle, pInfo->ppAnimations[i] );
		}
		for ( int32_t i = 0; i < pInfo->nTrackGroupCount && pInfo->ppTrackGroups != nullptr; ++i )
		{
			Own( registry, record, pFileHandle, pInfo->ppTrackGroups[i] );
		}
		for ( int32_t i = 0; i < pInfo->nMeshCount && pInfo->ppMeshes != nullptr; ++i )
		{
			Own( registry, record, pFileHandle, pInfo->ppMeshes[i] );
		}

		// Built under the lock and logged outside it, since the sinks flush on
		// every line and holding a lock across that would serialise the reads.
		sLine = fmt::format(
			"{} {} bytes: skeletons={} bones=[{}] models={} meshes={} animations={} "
			"trackgroups={}",
			Name( record ), record.nBytes, pInfo->nSkeletonCount, sBones,
			pInfo->nModelCount, pInfo->nMeshCount, pInfo->nAnimationCount,
			pInfo->nTrackGroupCount );
	}

	// At info rather than trace. The per-call trace is compiled out of a release
	// build and floods the log when it is not, but one line per file is bounded
	// by the number of resources a level loads, and it is the line that makes
	// every later warning traceable to a resource. LIBGR2_LOG_LEVEL=info is the
	// mode this exists for.
	Logger().info( "{}", sLine );
}

void ForgetFile( const void *pFileHandle )
{
	if ( pFileHandle == nullptr )
	{
		return;
	}
	std::lock_guard<std::mutex> lock( RegistryMutex() );
	SRegistry &registry = Registry();
	const std::map<const void *, SFileRecord>::iterator it = registry.Files.find( pFileHandle );
	if ( it == registry.Files.end() )
	{
		return;
	}
	for ( size_t i = 0; i < it->second.Objects.size(); ++i )
	{
		const std::map<const void *, const void *>::iterator owner =
			registry.Owners.find( it->second.Objects[i] );
		// Only if it is still ours. A later file that reused the address has
		// already overwritten the entry and is the one that should keep it.
		if ( owner != registry.Owners.end() && owner->second == pFileHandle )
		{
			registry.Owners.erase( owner );
		}
	}
	registry.Files.erase( it );
}

namespace
{

//! The file an object belongs to, named, or an empty string if none claims it.
std::string OwnerOf( const void *pObject )
{
	if ( pObject == nullptr )
	{
		return std::string();
	}
	std::lock_guard<std::mutex> lock( RegistryMutex() );
	SRegistry &registry = Registry();
	const std::map<const void *, const void *>::const_iterator owner =
		registry.Owners.find( pObject );
	if ( owner == registry.Owners.end() )
	{
		return std::string();
	}
	const std::map<const void *, SFileRecord>::const_iterator file =
		registry.Files.find( owner->second );
	if ( file == registry.Files.end() )
	{
		return std::string();
	}
	return Name( file->second );
}

}

std::string DescribeSkeleton( const SSkeleton *pSkeleton )
{
	if ( pSkeleton == nullptr )
	{
		return "no skeleton";
	}
	const std::string sOwner = OwnerOf( pSkeleton );
	return fmt::format( "skeleton \"{}\" of {} bones, from {}", Text( pSkeleton->pszName ),
	                    pSkeleton->nBoneCount,
	                    sOwner.empty() ? "no file this library loaded" : sOwner );
}

std::string DescribeModel( const SModel *pModel )
{
	if ( pModel == nullptr )
	{
		return "no model";
	}
	std::string s = fmt::format( "model \"{}\"", Text( pModel->pszName ) );
	const std::string sModelFile = OwnerOf( pModel );
	const std::string sSkeletonFile = OwnerOf( pModel->pSkeleton );
	// Only when it says something the skeleton's file will not. The two come out
	// of one file in almost every case, and naming that file twice in one line
	// costs half the line and buys nothing.
	if ( !sModelFile.empty() && sModelFile != sSkeletonFile )
	{
		s += fmt::format( " from {}", sModelFile );
	}
	// A model no file claims is the normal case rather than a fault, and the
	// skeleton is then what leads back to one: CSkeletonAnimator keeps a
	// granny_model of its own as a member, fills in a name and a skeleton
	// borrowed from a loaded file, and instantiates that.
	return s + fmt::format( ", with {}", DescribeSkeleton( pModel->pSkeleton ) );
}

}
