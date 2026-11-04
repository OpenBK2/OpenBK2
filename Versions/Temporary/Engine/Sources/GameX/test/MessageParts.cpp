#include "../stdafx.h"

#include "GameX/InterfaceState.h"
#include "GameX/MessageParts.h"
#include "libdb/Db.h"
#include "Misc/StrProc.h"
#include "System/Text.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"
#include "UI/test/MakeDescription.h"
#include "UI/UIML.h"
#include "UISpecificB2/DBUISpecificB2.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <set>

#include <gtest/gtest.h>

namespace
{
class MessageParts : public testing::Test
{
protected:
	std::filesystem::path directory;
	CObj<NVFS::ICombinerVFS> vfs;
	CObj<NDb::SUIConstsB2> constants;
	CObj<NDb::SMessageParts> parts;

	void WriteText( const std::string &name, const std::u16string &text )
	{
		{
			std::ofstream file( directory / name, std::ios::binary );
			for ( char16_t character : std::u16string( 1, u'\ufeff' ) + text )
			{
				file.put( char( character & 0xff ) );
				file.put( char( character >> 8 ) );
			}
		}
		NText::Reload( name );
	}

	void SetUp() override
	{
		ASSERT_TRUE( std::filesystem::is_regular_file( MESSAGE_PARTS_PAK ) );
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-message-parts-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( MESSAGE_PARTS_TYPES_FILE, directory / "types.xml" );
		// Open the actual ZIP through the game database, including relative text refs.
		vfs = NVFS::CreateCombinerVFS( 0 );
		std::vector<CObj<NVFS::IVFS>> sources;
		sources.push_back( NVFS::CreateWinVFS( directory.generic_string() + "/" ) );
		sources.push_back( NVFS::CreateWinVFS( MESSAGE_PARTS_PAK ) );
		vfs->SetVFSList( sources );
		NVFS::SetMainVFS( vfs );
		ASSERT_TRUE( NDb::OpenDatabase( vfs, nullptr, NDb::DATABASE_MODE_GAME ) );
		constants = NUITest::MakeDescription<NDb::SUIConstsB2>();
		parts = NUITest::MakeDescription<NDb::SMessageParts>();
		constants->pMessageParts = parts;
	}

	void TearDown() override
	{
		NGlobal::RemoveVar( "ui_consts" );
		NSingleton::UnRegisterSingleton( IInterfaceState::tidTypeID );
		constants = nullptr;
		parts = nullptr;
		NDb::CloseDatabase();
		NVFS::SetMainVFS( nullptr );
		vfs = nullptr;
		std::filesystem::remove_all( directory );
	}
};

TEST_F( MessageParts, ArchiveLoadsAllEnglishTextsAndMatchesBackups )
{
	const auto *record = NDb::Get<NDb::SMessageParts>( CDBID( "Other/Text/Game/Messages/MessageParts.xdb" ) );
	ASSERT_NE( record, nullptr );
	ASSERT_EQ( record->parts.size(), 29 );
	constants->pMessageParts = record;
	std::set<std::string> names;
	for ( const auto &part : record->parts )
	{
		SCOPED_TRACE( part.szValName );
		EXPECT_TRUE( names.insert( part.szValName ).second );
		const auto &text = NText::GetText( part.szValueTextFileRef );
		ASSERT_FALSE( text.empty() );
		const auto name = NStr::ToUnicode( part.szValName );
		EXPECT_EQ( NGameX::GetMessagePart( constants, name ), text );
		EXPECT_EQ( NGameX::GetMessagePart( nullptr, name ), text );
	}
	EXPECT_EQ( NGameX::GetMessagePart( constants, L"AI_PLAYER_NAME_PREFIX" ), L"AI " );
	EXPECT_EQ( NGameX::GetMessagePart( constants, L"TIME_MINUTES_SUFFIX" ), L" min" );
}

TEST_F( MessageParts, MissingAndEmptyTranslationsFallThroughToGeneralValuesThenEnglish )
{
	WriteText( "message.txt", u" (\u017d\u6771 team)" );
	WriteText( "general.txt", u" (General team)" );
	WriteText( "empty.txt", u"" );
	parts->parts.resize( 1 );
	parts->parts[0].szValName = "MP_CHAT_TEAM_SUFFIX";
	parts->parts[0].szValueTextFileRef = "message.txt";
	constants->tags.resize( 1 );
	constants->tags[0].szName = "MP_CHAT_TEAM_SUFFIX";
	constants->tags[0].szTextFileRef = "general.txt";
	EXPECT_EQ( NGameX::GetMessagePart( constants, L"MP_CHAT_TEAM_SUFFIX" ), L" (\u017d\u6771 team)" );
	for ( const char *path : { "missing-message.txt", "empty.txt", "" } )
	{
		parts->parts[0].szValueTextFileRef = path;
		EXPECT_EQ( NGameX::GetMessagePart( constants, L"MP_CHAT_TEAM_SUFFIX" ), L" (General team)" );
	}
	constants->pMessageParts = nullptr;
	EXPECT_EQ( NGameX::GetMessagePart( constants, L"MP_CHAT_TEAM_SUFFIX" ), L" (General team)" );
	constants->tags[0].szTextFileRef = "empty.txt";
	EXPECT_TRUE( NGameX::FindMessagePart( constants, L"MP_CHAT_TEAM_SUFFIX" ).empty() );
	EXPECT_EQ( NGameX::GetMessagePart( constants, L"MP_CHAT_TEAM_SUFFIX" ), L" (Team)" );
	EXPECT_EQ( NGameX::GetMessagePart( constants, L"REINFORCEMENT_TIMER_NEVER" ), L"never" );
	EXPECT_TRUE( NGameX::GetMessagePart( constants, L"UNDEFINED_MESSAGE" ).empty() );
	constants->tags[0].szName = "existing_custom_value";
	constants->tags[0].szTextFileRef = "general.txt";
	EXPECT_EQ( NGameX::GetMessagePart( constants, L"existing_custom_value" ), L" (General team)" );
}

TEST_F( MessageParts, RealValHandlerResolvesTheLinkedRecordAndOldDataBackups )
{
	{
		std::ofstream file( directory / "ui.xdb", std::ios::binary );
		file << "<UIConstsB2><MessageParts href=\"/Other/Text/Game/Messages/MessageParts.xdb\" /></UIConstsB2>";
	}
	NGlobal::SetVar( "ui_consts", "ui.xdb" );
	// These registered objects exercise the production virtual methods without
	// initializing a mission, renderer, audio device or user profile.
	CObj<IInterfaceState> state = MakeObject<IInterfaceState>( 0x300C8D44 );
	CObj<IMLHandler> handler = MakeObject<IMLHandler>( 0x1716F400 );
	ASSERT_NE( state, nullptr );
	ASSERT_NE( handler, nullptr );
	NSingleton::RegisterSingleton( state, IInterfaceState::tidTypeID );
	const auto *record = NDb::Get<NDb::SUIConstsB2>( CDBID( "ui.xdb" ) );
	ASSERT_NE( record, nullptr );
	ASSERT_NE( record->pMessageParts, nullptr );
	for ( bool linked : { true, false } )
	{
		if ( !linked )
		{
			NGlobal::RemoveVar( "ui_consts" );
		}
		for ( const auto &part : record->pMessageParts->parts )
		{
			const auto name = NStr::ToUnicode( part.szValName );
			CObj<IML> text = CreateML();
			handler->Exec( text, nullptr, { L"val", name } );
			std::wstring expanded;
			CMLStream *stream = text->GetStream();
			while ( !stream->IsEof() )
			{
				expanded += stream->GetChar();
				stream->IncSeek();
			}
			EXPECT_EQ( expanded, NText::GetText( part.szValueTextFileRef ) ) << part.szValName;
		}
	}
}
} // namespace
