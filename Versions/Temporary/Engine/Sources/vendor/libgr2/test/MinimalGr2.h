#pragma once

// The shape of a .gr2 file, as measured, plus a builder that produces one.
//
// The corpus cannot be committed: 83,184 files, 15.5 GB, and it is Nival's
// copyrighted data. So the fixtures are hand authored, and every constant below
// was read out of shipped files rather than out of a specification. The census
// that produced them ran over the 7,442 GR2 resources in
// C:\Games\bk2\Data\data.pak on 2026-08-28:
//
//   magic         b867b0ca f86db10f 84728c7e 5e19001e, in all 7,442
//   headerFormat  0, in all 7,442
//   version       6, in all 7,442
//   sectionArrayOffset  56, in all 7,442
//   sectionArrayCount   8 in 7,127, 6 in 315
//   typeTag       0x80000010 in 5,944, 0x80000013 in 1,183, 0x8000000f in 315
//
// The two derived sizes were confirmed by arithmetic that only closes if they
// are right. headerSize is 440 in a file with 8 sections, and
// 32 + 56 + 44 * 8 = 440 exactly, which fixes the magic block at 32 bytes, the
// header at 56 and a section record at 44. A pointer fixup entry is 12 bytes
// because section 0 of one file puts 110 of them at offset 440 and its data at
// 1760, and 440 + 110 * 12 = 1760; section 6 of the same file agrees
// independently, 6632 + 191 * 12 = 8924.
//
// What this builder does NOT claim is that its output is a file the real
// granny2.dll would accept. It reproduces the layout up to the end of the
// section array, which is all a rejection test needs, since the mutation under
// test is then the only thing wrong with the buffer. Anything past that, a CRC
// that validates, a type section that parses, a root object, is left for M1 and
// for fixtures generated against the oracle.

#include <gr2/granny.h>

#include <cstdint>
#include <vector>

namespace NGr2Test
{

//! File Format 6, little endian, 32-bit pointers. The one dialect this game uses.
extern const uint8_t g_Magic[16];

constexpr uint32_t MAGIC_SIZE = 16;
constexpr uint32_t MAGIC_BLOCK_SIZE = 32;
constexpr uint32_t HEADER_OFFSET = MAGIC_BLOCK_SIZE;
constexpr uint32_t HEADER_SIZE = 56;
constexpr uint32_t SECTION_RECORD_SIZE = 44;
constexpr uint32_t POINTER_FIXUP_SIZE = 12;

// Absolute byte offsets of the fields, magic block then header. Absolute rather
// than relative to each struct because that is how a test reaches them, and
// because the one field that is itself relative, sectionArrayOffset, is counted
// from HEADER_OFFSET and mixing the two is exactly the mistake to avoid.
constexpr uint32_t OFF_HEADER_SIZE = 16;
constexpr uint32_t OFF_HEADER_FORMAT = 20;
constexpr uint32_t OFF_VERSION = 32;
constexpr uint32_t OFF_TOTAL_SIZE = 36;
constexpr uint32_t OFF_CRC = 40;
constexpr uint32_t OFF_SECTION_ARRAY_OFFSET = 44;
constexpr uint32_t OFF_SECTION_ARRAY_COUNT = 48;
constexpr uint32_t OFF_ROOT_OBJECT_TYPE_SECTION = 52;
constexpr uint32_t OFF_ROOT_OBJECT_TYPE_OFFSET = 56;
constexpr uint32_t OFF_ROOT_OBJECT_SECTION = 60;
constexpr uint32_t OFF_ROOT_OBJECT_OFFSET = 64;
constexpr uint32_t OFF_TYPE_TAG = 68;
constexpr uint32_t OFF_EXTRA_TAGS = 72;

// Field offsets within one section record.
constexpr uint32_t SEC_COMPRESSION = 0;
constexpr uint32_t SEC_DATA_OFFSET = 4;
constexpr uint32_t SEC_DATA_SIZE = 8;
constexpr uint32_t SEC_EXPANDED_DATA_SIZE = 12;
constexpr uint32_t SEC_INTERNAL_ALIGNMENT = 16;
constexpr uint32_t SEC_FIRST_16BIT = 20;
constexpr uint32_t SEC_FIRST_8BIT = 24;
constexpr uint32_t SEC_POINTER_FIXUP_OFFSET = 28;
constexpr uint32_t SEC_POINTER_FIXUP_COUNT = 32;
constexpr uint32_t SEC_MIXED_MARSHALLING_OFFSET = 36;
constexpr uint32_t SEC_MIXED_MARSHALLING_COUNT = 40;

// The four struct tags seen in the wild. 0x80000011 appears only in Total
// Conversion mod content, three files out of 83,184, and is listed so that a
// parser written against the other three is at least on notice.
constexpr uint32_t TYPE_TAG_0F = 0x8000000fu;
constexpr uint32_t TYPE_TAG_10 = 0x80000010u;
constexpr uint32_t TYPE_TAG_11 = 0x80000011u;
constexpr uint32_t TYPE_TAG_13 = 0x80000013u;

// Per section compression. 0, 1 and 2 all occur; which of 1 and 2 is Oodle0 and
// which is Oodle1 is an M1 question and deliberately not asserted here. What is
// asserted is that a value outside this set is not a thing the loader may
// silently accept.
constexpr uint32_t COMPRESSION_NONE = 0;
constexpr uint32_t COMPRESSION_MAX_KNOWN = 2;

//! A buffer laid out like a shipped .gr2 up to the end of the section array.
//!
//! Every section is empty and uncompressed, so the file ends where the section
//! array does. Tests take a copy and break one thing about it.
class CHeaderShapedFile
{
public:
	//! \param nSectionCount 8 in almost every shipped file, 6 in the 0x8000000f ones.
	explicit CHeaderShapedFile( uint32_t nSectionCount = 8 );

	const void *Data() const { return m_Bytes.data(); }
	granny_int32x Size() const { return static_cast<granny_int32x>( m_Bytes.size() ); }
	const std::vector<uint8_t> &Bytes() const { return m_Bytes; }
	std::vector<uint8_t> &Bytes() { return m_Bytes; }

	uint32_t GetU32( uint32_t nOffset ) const;
	void SetU32( uint32_t nOffset, uint32_t nValue );

	uint32_t SectionArrayOffset() const;
	uint32_t GetSectionField( uint32_t nSection, uint32_t nFieldOffset ) const;
	void SetSectionField( uint32_t nSection, uint32_t nFieldOffset, uint32_t nValue );

	//! Drop everything past nSize, and do not touch totalSize.
	//!
	//! A truncated file in the wild is truncated, not rewritten, so the header
	//! keeps claiming the original length. That disagreement is the thing under
	//! test and repairing it would remove it.
	void TruncateTo( uint32_t nSize );

private:
	std::vector<uint8_t> m_Bytes;
};

//! A buffer with a magic that is not Granny's at all.
//!
//! Real, not invented: entries under bin/Geometries/ and its siblings are
//! addressed by extensionless GUID and a handful of them are not GR2 files.
//! This one starts "[Localiz", 636F4C5B 7A696C61. Per port/PORT_ROADMAP.md a
//! failed resource load currently crashes the engine, so rejecting this
//! gracefully is a live requirement rather than a theoretical one.
std::vector<uint8_t> ForeignMagicFile();

}
