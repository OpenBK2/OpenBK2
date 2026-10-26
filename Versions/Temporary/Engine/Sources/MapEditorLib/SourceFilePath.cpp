#include "stdafx.h"

#include "Interface_MOD.h"
#include "Interface_UserData.h"
#include "SourceFilePath.h"
#include "System/FilePath.h"

#include <filesystem>
#include <vector>

namespace NEditorSource
{
namespace
{
	namespace fs = std::filesystem;

	fs::path Normalize( const std::string &path )
	{
		std::string normalized = path;
		NFile::NormalizePath( &normalized );
		return fs::u8path( normalized ).lexically_normal();
	}

	std::vector<fs::path> Roots( bool includeSource = true )
	{
		const auto &data = Singleton<IUserDataContainer>()->Get()->constUserData;
		std::vector<fs::path> roots;
		for ( const auto &folder : { Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ),
		                             data.szDataStorageFolder,
		                             data.szExportDestinationFolder,
		                             includeSource ? data.szExportSourceFolder : std::string() } )
		{
			if ( !folder.empty() )
			{
				auto root = fs::absolute( Normalize( folder ) ).lexically_normal();
				if ( root.filename().empty() && root != root.root_path() )
				{
					root = root.parent_path();
				}
				roots.push_back( root );
			}
		}
		return roots;
	}
} // namespace

std::string MakeRelative( const std::string &path, bool includeSource )
{
	const auto source = Normalize( path );
	if ( source.is_absolute() )
	{
		for ( const auto &root : Roots( includeSource ) )
		{
			const auto relative = source.lexically_relative( root );
			// Compare whole path components, so DataExtra is never treated as Data.
			if ( !relative.empty() && !relative.is_absolute() && *relative.begin() != ".." )
			{
				return relative.generic_u8string();
			}
			// Native pickers can return a different spelling/case of the same
			// directory on Windows. Compare that prefix using filesystem identity.
			fs::path prefix;
			auto part = source.begin();
			for ( auto rootPart = root.begin(); rootPart != root.end() && part != source.end(); ++rootPart, ++part )
			{
				prefix /= *part;
			}
			std::error_code error;
			if ( part != source.end() && fs::equivalent( prefix, root, error ) )
			{
				fs::path tail;
				for ( ; part != source.end(); ++part )
				{
					tail /= *part;
				}
				return tail.generic_u8string();
			}
		}
	}
	return source.generic_u8string();
}

std::string Resolve( const std::string &path )
{
	const auto source = Normalize( path );
	if ( !source.is_absolute() )
	{
		for ( const auto &root : Roots() )
		{
			const auto candidate = ( root / source ).lexically_normal();
			std::error_code error;
			if ( fs::is_regular_file( candidate, error ) )
			{
				return candidate.generic_u8string();
			}
		}
		return ( Normalize( Singleton<IUserDataContainer>()->Get()->constUserData.szExportSourceFolder ) / source )
		    .lexically_normal()
		    .generic_u8string();
	}
	return source.generic_u8string();
}
} // namespace NEditorSource
