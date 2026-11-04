#include "port/unicode.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

#include <gtest/gtest.h>

namespace
{
bool ConversionsMatch()
{
	// Include an embedded NUL and a supplementary character in every encoding.
	const wchar_t wideChars[] = L"A\u017d\0\u6771\U0001f600";
	const char utf8Chars[] = u8"A\u017d\0\u6771\U0001f600";
	const char utf16Chars[] = "\x41\0\x7d\x01\0\0\x71\x67\x3d\xd8\0\xde";
	const std::wstring wide( wideChars, sizeof( wideChars ) / sizeof( wchar_t ) - 1 );
	const std::string utf8( utf8Chars, sizeof( utf8Chars ) - 1 );
	const std::string utf16( utf16Chars, sizeof( utf16Chars ) - 1 );
	return WideToUTF8( wide ) == utf8 && UTF8ToWide( utf8 ) == wide && WideToUTF16LE( wide ) == utf16 &&
	       UTF16LEToWide( utf16.data(), utf16.size() ) == wide;
}

void CheckConversionsAtExit()
{
	// exit runs the main thread's TLS destructors before atexit callbacks, just
	// as it does before the console singleton dumps its persistent log lines.
	for ( int i = 0; i < 2; ++i )
	{
		if ( !ConversionsMatch() )
		{
			std::fputs( "Unicode conversion failed during shutdown\n", stderr );
			std::_Exit( 1 );
		}
	}
}
} // namespace

TEST( UnicodeDeathTest, ConvertsAtExitAfterWarmingEveryDirection )
{
	EXPECT_EXIT(
			{
				if ( std::atexit( CheckConversionsAtExit ) != 0 || !ConversionsMatch() )
				{
					std::_Exit( 2 );
				}
				std::exit( 0 );
			},
			::testing::ExitedWithCode( 0 ),
			"" );
}

TEST( UnicodeDeathTest, ConvertsForTheFirstTimeAtExit )
{
	// Re-exec on Linux too, so this child cannot inherit another test's caches.
	GTEST_FLAG_SET( death_test_style, "threadsafe" );
	EXPECT_EXIT(
			{
				if ( std::atexit( CheckConversionsAtExit ) != 0 )
				{
					std::_Exit( 2 );
				}
				std::exit( 0 );
			},
			::testing::ExitedWithCode( 0 ),
			"" );
}

TEST( Unicode, ConvertsOnIndependentWorkerThreads )
{
	std::array<bool, 4> matches{};
	std::array<std::thread, 4> workers;
	for ( size_t i = 0; i < workers.size(); ++i )
	{
		workers[i] = std::thread(
				[&, i]()
				{
					matches[i] = true;
					for ( int nRepeat = 0; nRepeat < 64; ++nRepeat )
					{
						matches[i] = ConversionsMatch() && matches[i];
					}
				} );
	}
	for ( auto &worker : workers )
	{
		worker.join();
	}
	for ( const bool match : matches )
	{
		EXPECT_TRUE( match );
	}
}
