#pragma once

// The shape of a .gr2 file, as measured, plus a builder that produces one.
//
// The corpus cannot be committed: 83,184 files, 15.5 GB, and it is Nival's
// copyrighted data. So the fixtures are hand authored, and every constant below
// was read out of shipped files rather than out of a specification. The census
// that produced them ran over the 7,844 GR2 resources in the retail
// C:\Games\bk2\Data paks on 2026-08-28:
//
//   magic         b867b0ca f86db10f 84728c7e 5e19001e, in all 7,844
//   headerFormat  0, in all 7,844
//   version       6, in all 7,844
//   totalSize     equal to the file's length, in all 7,844
//   headerSize    equal to the end of the section array, in all 7,844
//   sectionArrayOffset  56, in all 7,844
//   sectionArrayCount   8, or 6 in the 0x8000000f files
//   typeTag       0x80000010, 0x80000013, 0x8000000f
//
// Three sizes were confirmed by arithmetic that only closes if they are right.
// headerSize is 440 in a file with 8 sections, and 32 + 56 + 44 * 8 = 440
// exactly, which fixes the magic block at 32 bytes, the header at 56 and a
// section record at 44. A pointer fixup entry is 12 bytes because section 0 of
// one file puts 110 of them at offset 440 and its data at 1760, and
// 440 + 110 * 12 = 1760; section 6 of the same file agrees independently,
// 6632 + 191 * 12 = 8924. A mixed marshalling entry is 16 bytes because section 3
// of bin/AIGeometries/2B95A3C1-314F-4721-9A9F-D37390307B86 puts one at 2008 and
// its data at 2024.
//
// The layout the builder emits follows the same files: the magic block, the
// header, the section array, then per section its pointer fixups, its marshalling
// fixups and its data, in that order.
//
// These constants are deliberately a second copy of the ones in src/File.h. A
// test that read the implementation's own numbers could not catch one of them
// being wrong.

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
constexpr uint32_t MIXED_MARSHALLING_FIXUP_SIZE = 16;

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
// which is Oodle1 is not established, so the names here say the number.
constexpr uint32_t COMPRESSION_NONE = 0;
constexpr uint32_t COMPRESSION_OODLE_1 = 1;
constexpr uint32_t COMPRESSION_OODLE_2 = 2;

//! A buffer laid out the way a shipped .gr2 is.
//!
//! Fresh from the constructor it has the requested number of empty uncompressed
//! sections, and nothing but a header and a section array. Sections gain content
//! through SetSectionData and references through AddPointerFixup, each of which
//! lays the file out again; after that a test can reach in with SetU32 and break
//! one field, which is what the rejection tests do.
class CHeaderShapedFile
{
public:
	//! \param nSectionCount 8 in almost every shipped file, 6 in the 0x8000000f ones.
	explicit CHeaderShapedFile( uint32_t nSectionCount = 8 );

	//! Uncompressed content for one section.
	void SetSectionData( uint32_t nSection, std::vector<uint8_t> data );

	//! A four byte pointer slot at nSection:nFromOffset, pointing at nTo.
	//!
	//! What the slot itself contains is left alone, since that is exactly what a
	//! reader must not depend on: the fixup array is what says which words are
	//! pointers and where they lead.
	void AddPointerFixup( uint32_t nSection, uint32_t nFromOffset, uint32_t nToSection,
	                      uint32_t nToOffset );

	void SetRootObject( uint32_t nSection, uint32_t nOffset );
	void SetTypeTag( uint32_t nTag );

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
	void Build();

	struct SFixup
	{
		uint32_t nFromOffset;
		uint32_t nToSection;
		uint32_t nToOffset;
	};

	uint32_t m_nSectionCount;
	uint32_t m_nRootSection = 0;
	uint32_t m_nRootOffset = 0;
	uint32_t m_nRootTypeSection = 0;
	uint32_t m_nTypeTag = TYPE_TAG_13;
	std::vector<std::vector<uint8_t>> m_SectionData;
	std::vector<std::vector<SFixup>> m_Fixups;
	std::vector<uint8_t> m_Bytes;
};

//! Bytes with some structure to them, so a wrong offset shows up as wrong content.
std::vector<uint8_t> Pattern( uint32_t nBytes, uint8_t nSeed = 0 );

//! A buffer with a magic that is not Granny's at all.
//!
//! Real, not invented: entries under bin/Geometries/ and its siblings are
//! addressed by extensionless GUID and a handful of them are not GR2 files.
//! This one starts "[Localiz", 636F4C5B 7A696C61. Per port/PORT_ROADMAP.md a
//! failed resource load currently crashes the engine, so rejecting this
//! gracefully is a live requirement rather than a theoretical one.
std::vector<uint8_t> ForeignMagicFile();

}
