#include "../stdafx.h"

#include "../DBMPConsts.h"
#include "../GetConsts.h"
#include "libdb/Db.h"
#include "System/Text.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <cstdlib>
#include <string>

#include <gtest/gtest.h>

// Which SVG flag each multiplayer side of an installed game resolves to, through
// NGameX::GetSideFlagImage, against the installation's own archives.
//
// An installed game is not part of this tree, so the test only runs when
// OBK2_FLAGS_TEST_DATA names one's Data directory (with obk2_flags.pak in it),
// and OBK2_FLAGS_TEST_EXPECT lists, side by side and tech level by tech level,
// the drawing each must give: sides separated by ';', tech levels by ',', a
// drawing by its file name without the path, '-' for none. Without the
// variables it skips, so CI, which has no installed game, passes it over.
//
// This is what found that an installed game's types.xml, older than the flag
// records, left Flags/Mapping.xdb found but impossible to create.
namespace
{
std::string FileNameOf( const std::string &szPath )
{
	if ( szPath.empty() )
	{
		return "-";
	}
	const size_t nSlash = szPath.find_last_of( "/\\" );
	return nSlash == std::string::npos ? szPath : szPath.substr( nSlash + 1 );
}

std::vector<std::string> Split( const std::string &szText, const char cSeparator )
{
	std::vector<std::string> parts;
	size_t nStart = 0;
	while ( true )
	{
		const size_t nEnd = szText.find( cSeparator, nStart );
		parts.push_back( szText.substr( nStart, nEnd == std::string::npos ? std::string::npos : nEnd - nStart ) );
		if ( nEnd == std::string::npos )
		{
			return parts;
		}
		nStart = nEnd + 1;
	}
}

TEST( MPSideFlags, InstalledGame )
{
	const char *pszData = std::getenv( "OBK2_FLAGS_TEST_DATA" );
	const char *pszExpect = std::getenv( "OBK2_FLAGS_TEST_EXPECT" );
	if ( !pszData || !pszExpect )
	{
		GTEST_SKIP() << "OBK2_FLAGS_TEST_DATA and OBK2_FLAGS_TEST_EXPECT name no installed game";
	}

	CObj<NVFS::IVFS> pVFS = NVFS::CreateWinVFS( std::string( pszData ) + "/" );
	NVFS::SetMainVFS( pVFS );
	ASSERT_TRUE( NDb::OpenDatabase( pVFS, 0, NDb::DATABASE_MODE_GAME ) );

	const NDb::SMultiplayerConsts *pMPConsts =
			NDb::Get<NDb::SMultiplayerConsts>( CDBID( "Consts/Test/Test_MultiplayerConsts.xdb" ) );
	ASSERT_NE( pMPConsts, nullptr );

	const std::vector<std::string> expected = Split( pszExpect, ';' );
	ASSERT_EQ( expected.size(), pMPConsts->sides.size() );
	for ( int nSide = 0; nSide < pMPConsts->sides.size(); ++nSide )
	{
		const std::vector<std::string> levels = Split( expected[nSide], ',' );
		ASSERT_EQ( levels.size(), pMPConsts->techLevels.size() ) << "side " << nSide;
		for ( int nLevel = 0; nLevel < pMPConsts->techLevels.size(); ++nLevel )
		{
			EXPECT_EQ( FileNameOf( NGameX::GetSideFlagImage( pMPConsts, nSide, nLevel ) ), levels[nLevel] )
					<< "side " << nSide << ", tech level " << nLevel;
		}
	}

	NDb::CloseDatabase();
	NVFS::SetMainVFS( 0 );
}
} // namespace
