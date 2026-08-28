#pragma once

// A .gr2 as a set of sections, plus the fixups that turn offsets inside them
// into references between them.
//
// Two things this deliberately does not do.
//
// It does not decompress. Both Oodle codecs are still ahead, so a file with a
// compressed section is refused rather than half read. That is nearly every
// shipped file, which is why the tests drive hand authored uncompressed fixtures.
//
// It does not fix pointers up in place. A GR2 stores 32-bit pointers, and on x64
// an eight byte pointer does not fit the four byte slot: writing one would
// overwrite the field behind it. So section bytes are kept exactly as the file
// had them and the resolved fixups live beside them in a table that
// ResolvePointer answers from. x86 and x64 then run the same code, and the type
// tree walker that builds native structures later reads pointers through that
// table rather than out of the bytes. This is also the only correct reading: the
// slots themselves hold whatever the exporter left there, and the fixup array is
// what says which of them are pointers at all.

#include <gr2/granny.h>

#include <cstdint>
#include <vector>

namespace NGr2
{

// On-disk sizes, all measured over the 7,844 GR2 resources in the retail
// C:\Games\bk2\Data paks. See test/MinimalGr2.h for the census and for the
// arithmetic each size was derived from. The test fixture keeps its own copy of
// these numbers on purpose: a test that read them from here could not catch one
// of them being wrong.
constexpr uint32_t MAGIC_SIZE = 16;
constexpr uint32_t MAGIC_BLOCK_SIZE = 32;
constexpr uint32_t HEADER_OFFSET = MAGIC_BLOCK_SIZE;
constexpr uint32_t HEADER_SIZE = 56;
constexpr uint32_t PREFIX_SIZE = MAGIC_BLOCK_SIZE + HEADER_SIZE;
constexpr uint32_t SECTION_RECORD_SIZE = 44;
constexpr uint32_t POINTER_FIXUP_SIZE = 12;
constexpr uint32_t MIXED_MARSHALLING_FIXUP_SIZE = 16;

//! File Format 6, little endian, 32-bit pointers, the only dialect in this game.
extern const uint8_t FILE_MAGIC[MAGIC_SIZE];

constexpr uint32_t FILE_VERSION = 6;

//! What a section's bytes were compressed with.
//!
//! Read the numbers twice: Oodle0 is 1 and Oodle1 is 2, because Granny counts
//! from GrannyNoCompression. These are its own names and values, from
//! granny_compression_type in granny211.h, which also lists BitKnit as 3 and
//! BitKnit2 as 4. Neither BitKnit appears in this game's data, both postdating it.
enum ECompression
{
	COMPRESSION_NONE = 0,
	COMPRESSION_OODLE0 = 1,
	COMPRESSION_OODLE1 = 2,
};

//! One 44-byte entry of the section array.
struct SSection
{
	uint32_t nCompression = 0;
	uint32_t nDataOffset = 0;
	uint32_t nDataSize = 0;
	uint32_t nExpandedDataSize = 0;
	uint32_t nInternalAlignment = 0;
	uint32_t nFirst16Bit = 0;
	uint32_t nFirst8Bit = 0;
	uint32_t nPointerFixupOffset = 0;
	uint32_t nPointerFixupCount = 0;
	uint32_t nMixedMarshallingOffset = 0;
	uint32_t nMixedMarshallingCount = 0;
};

//! A place in the loaded file: which section, and how far into it.
struct SReference
{
	uint32_t nSection = 0;
	uint32_t nOffset = 0;
};

//! One 12-byte pointer fixup: the slot at nFromOffset points at nTo.
//!
//! nFromOffset is relative to the section that owns the fixup, which is why the
//! fixups are kept per section rather than in one list.
struct SPointerFixup
{
	uint32_t nFromOffset = 0;
	SReference To;
};

}

// granny.h declares granny_file as an incomplete struct and the engine only ever
// holds a pointer to one. Completing it here means the handle is the object, so
// no entry point has to cast and GrannyFreeFile is a delete. A C++ class with a
// C tag name is legal and costs the C side nothing.
struct granny_file
{
	//! Parse a whole file image. Null on anything malformed, with the reason logged.
	//!
	//! The bytes are not retained: everything needed is copied out, so the caller
	//! may free its buffer the moment this returns. That matches the engine, whose
	//! CMemoryStream in CGrannyMemFileLoader::RecalcValue dies at the end of the
	//! function that reads into it.
	static granny_file *ReadFromMemory( const void *pMemory, granny_int32x nSize );

	uint32_t SectionCount() const { return static_cast<uint32_t>( m_Sections.size() ); }
	const NGr2::SSection &Section( uint32_t nSection ) const { return m_Sections[nSection]; }

	//! A section's bytes, as long as its expanded size.
	const std::vector<uint8_t> &SectionData( uint32_t nSection ) const
	{
		return m_SectionData[nSection];
	}

	//! Where the root object is, and where its type definition is.
	const NGr2::SReference &RootObject() const { return m_RootObject; }
	const NGr2::SReference &RootObjectType() const { return m_RootObjectType; }

	//! Which of the four struct tags this file was written with.
	uint32_t TypeTag() const { return m_nTypeTag; }

	//! What the pointer slot at nSection:nOffset points at.
	//!
	//! False when no fixup covers that slot, which is how a GR2 spells a null
	//! pointer: the slot's own contents are meaningless and only the fixup array
	//! says what is a pointer and where it goes.
	bool ResolvePointer( uint32_t nSection, uint32_t nOffset, NGr2::SReference *pTarget ) const;

	//! Every fixup of a section, sorted by nFromOffset. Files store them unsorted,
	//! in 15,287 of the 15,373 arrays measured, so the sort happens on load.
	const std::vector<NGr2::SPointerFixup> &PointerFixups( uint32_t nSection ) const
	{
		return m_PointerFixups[nSection];
	}

private:
	granny_file() = default;

	std::vector<NGr2::SSection> m_Sections;
	std::vector<std::vector<uint8_t>> m_SectionData;
	std::vector<std::vector<NGr2::SPointerFixup>> m_PointerFixups;
	NGr2::SReference m_RootObject;
	NGr2::SReference m_RootObjectType;
	uint32_t m_nTypeTag = 0;
};
