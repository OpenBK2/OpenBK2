#include "MapEditorLib/stdafx.h"
#include "MapEditorLib/Tools_Resources.h"
#include "System/Text.h"
#include "port/unicode.h"

#include <gtest/gtest.h>

namespace
{
const std::string text = u8"Boj \u010c\u4e2d\U0001f680";
// An independent UTF-16LE fixture: BOM, ASCII, two BMP characters, a surrogate pair.
const std::vector<uint8_t> encoded = {
	0xff, 0xfe, 'B', 0, 'o', 0, 'j', 0, ' ', 0, 0x0c, 0x01, 0x2d, 0x4e, 0x3d, 0xd8, 0x80, 0xde
};
}

TEST( TextResourceEncoding, EditorWritesGameCompatibleUtf16FromBothStringTypes )
{
	std::vector<uint8_t> bytes;
	String2File( &bytes, text, true, false );
	EXPECT_EQ( bytes, encoded );
	String2File( &bytes, UTF8ToWide(text), false );
	EXPECT_EQ( bytes, encoded );
	CMemoryStream stream;
	stream.Write( bytes.data(), bytes.size() );
	stream.Seek( 0 );
	std::wstring gameText;
	ASSERT_TRUE( NText::LoadUnicodeText( &gameText, &stream ) );
	EXPECT_EQ( gameText, UTF8ToWide(text) );
}

TEST( TextResourceEncoding, ReadsExistingWindowsFilesAndSingleCharacterFiles )
{
	std::string result;
	std::wstring wide;
	bool unicode = false, repair = true;
	File2String( &result, &unicode, encoded, false, &repair );
	EXPECT_EQ( result, text );
	EXPECT_TRUE( unicode );
	EXPECT_FALSE( repair );
	File2String( &wide, encoded, false );
	EXPECT_EQ( wide, UTF8ToWide(text) );
	File2String( &result, &unicode, std::vector<uint8_t>{0xff, 0xfe, 'B', 0}, false );
	EXPECT_EQ( result, "B" );
}

TEST( TextResourceEncoding, EmptyUnicodeFileRetainsItsBomAndEncoding )
{
	std::vector<uint8_t> bytes;
	String2File( &bytes, std::string(), true, false );
	EXPECT_EQ( bytes, (std::vector<uint8_t>{0xff, 0xfe}) );
	std::string result = "old";
	bool unicode = false, repair = true;
	File2String( &result, &unicode, bytes, false, &repair );
	EXPECT_TRUE( result.empty() );
	EXPECT_TRUE( unicode );
	EXPECT_FALSE( repair );
	File2String( &result, &unicode, std::vector<uint8_t>(), false );
	EXPECT_TRUE( result.empty() );
	EXPECT_FALSE( unicode );
}

TEST( TextResourceEncoding, RecoversFilesWrittenWithLinuxWcharWidth )
{
	// The broken writer used a two-byte BOM followed by four-byte scalars.
	const std::vector<uint8_t> broken = {
		0xff, 0xfe, 'B', 0, 0, 0, 'o', 0, 0, 0, 'j', 0, 0, 0, ' ', 0, 0, 0,
		0x0c, 0x01, 0, 0, 0x2d, 0x4e, 0, 0, 0x80, 0xf6, 0x01, 0
	};
	std::string result;
	std::wstring wide;
	bool unicode = false, repair = false;
	File2String( &result, &unicode, broken, false, &repair );
	EXPECT_EQ( result, text );
	EXPECT_TRUE( unicode );
	EXPECT_TRUE( repair );
	File2String( &wide, broken, false );
	EXPECT_EQ( wide, UTF8ToWide(text) );
	std::vector<uint8_t> saved;
	String2File( &saved, result, true, false );
	EXPECT_EQ( saved, encoded );
	File2String( &result, &unicode, saved, false, &repair );
	EXPECT_FALSE( repair );
}

TEST( TextResourceEncoding, KeepsLuaUtf8AndExistingLineEndingOptions )
{
	std::vector<uint8_t> bytes;
	String2File( &bytes, std::string("a\nb\r\nc\n"), false, true );
	EXPECT_EQ( std::string(bytes.begin(), bytes.end()), "a\r\nb\r\nc" );
	String2File( &bytes, text, false, false );
	EXPECT_EQ( std::string(bytes.begin(), bytes.end()), text );
	std::string result;
	bool unicode = true;
	File2String( &result, &unicode, bytes, false );
	EXPECT_EQ( result, text );
	EXPECT_FALSE( unicode );
	String2File( &bytes, std::wstring(L"a\nb\r\nc\n"), true );
	File2String( &result, &unicode, bytes, false );
	EXPECT_EQ( result, "a\r\nb\r\nc" );
	File2String( &result, &unicode, bytes, true );
	EXPECT_EQ( result, "a\nb\nc" );
}
