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
#include <map>
#include <memory>
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

	//! Ordered so that a reference can key a map, which is how types and
	//! converted objects are cached.
	bool operator<( const SReference &other ) const
	{
		return nSection != other.nSection ? nSection < other.nSection
		                                  : nOffset < other.nOffset;
	}
	bool operator==( const SReference &other ) const
	{
		return nSection == other.nSection && nOffset == other.nOffset;
	}
};

//! Memory that lives exactly as long as the file it was converted out of.
//!
//! Everything GrannyGetFileInfo hands back points into here, so GrannyFreeFile
//! is a delete of the file and nothing else has to be tracked. Blocks are never
//! resized, because the whole point is that the pointers stay put.
class CArena
{
public:
	//! Zeroed, and aligned enough for anything in Structures.h. Null on failure.
	void *Alloc( size_t nBytes );

private:
	static constexpr size_t BLOCK_SIZE = 64 * 1024;
	static constexpr size_t ALIGNMENT = 8;

	std::vector<std::unique_ptr<uint8_t[]>> m_Blocks;
	uint8_t *m_pNext = nullptr;
	size_t m_nLeft = 0;
};

//! granny_member_type, from granny211.h, in the enum's own order.
enum EMemberType
{
	MEMBER_END = 0,
	MEMBER_INLINE,
	MEMBER_REFERENCE,
	MEMBER_REFERENCE_TO_ARRAY,
	MEMBER_ARRAY_OF_REFERENCES,
	MEMBER_VARIANT_REFERENCE,
	MEMBER_UNSUPPORTED_REMOVE,
	MEMBER_REFERENCE_TO_VARIANT_ARRAY,
	MEMBER_STRING,
	MEMBER_TRANSFORM,
	MEMBER_REAL32,
	MEMBER_INT8,
	MEMBER_UINT8,
	MEMBER_BINORMAL_INT8,
	MEMBER_NORMAL_UINT8,
	MEMBER_INT16,
	MEMBER_UINT16,
	MEMBER_BINORMAL_INT16,
	MEMBER_NORMAL_UINT16,
	MEMBER_INT32,
	MEMBER_UINT32,
	MEMBER_REAL16,
	MEMBER_EMPTY_REFERENCE,
	MEMBER_ONE_PAST_LAST,
};

//! One 32-byte type definition entry, plus where its member sits in the object.
struct SMember
{
	//! Points into the file's own bytes, which outlive every use of it.
	const char *pszName = nullptr;
	uint32_t nType = MEMBER_END;
	//! Where the referenced type is, meaningful when bHasReferenceType.
	SReference ReferenceType;
	bool bHasReferenceType = false;
	int32_t nArrayWidth = 0;
	//! Both on disk, so both with 32-bit pointers.
	uint32_t nOffset = 0;
	uint32_t nSize = 0;
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

	//! Reading primitives, bounds checked against the section they name.
	//!
	//! False means the read would have run off the end, which on a file this has
	//! already accepted means a structure pointing somewhere a fixup did not.
	bool ReadU32( uint32_t nSection, uint32_t nOffset, uint32_t *pnValue ) const;
	bool ReadI32( uint32_t nSection, uint32_t nOffset, int32_t *pnValue ) const;
	bool ReadReal32( uint32_t nSection, uint32_t nOffset, float *pfValue ) const;
	bool ReadBytes( const NGr2::SReference &at, void *pDest, uint32_t nBytes ) const;

	//! The string the pointer slot at nSection:nOffset leads to, or null.
	//!
	//! Points into the file's own section bytes, which live as long as this
	//! object, so the converted structures can hold it without copying. That is
	//! also why nothing here ever reallocates a section.
	const char *ReadString( uint32_t nSection, uint32_t nOffset ) const;

	//! A pointer into a section's bytes, for arrays the engine reads in place.
	//!
	//! Indices and vertices need no conversion, being plain integers and floats,
	//! so the engine can be handed the file's own bytes rather than a copy.
	const uint8_t *Raw( const NGr2::SReference &at, uint32_t nBytes ) const;

	// Internal state, public because this header is internal and the type tree
	// walker and the converter are as much a part of this object as its methods.

	//! Type definitions already read, keyed by where they are. Also the guard
	//! against a type tree that reaches itself.
	std::map<NGr2::SReference, std::vector<NGr2::SMember>> m_Types;

	//! Objects already converted, keyed by where they came from.
	//!
	//! Identity matters, not just content: the engine finds a mesh's model by
	//! comparing MeshBindings[i].Mesh against a mesh pointer it already has, so
	//! one file object has to become exactly one native object.
	std::map<NGr2::SReference, void *> m_Converted;

	//! Everything the conversion allocated.
	NGr2::CArena m_Arena;

	//! The converted root, built on the first GrannyGetFileInfo and kept.
	void *m_pFileInfo = nullptr;
	bool m_bConversionFailed = false;

private:
	granny_file() = default;

	std::vector<NGr2::SSection> m_Sections;
	std::vector<std::vector<uint8_t>> m_SectionData;
	std::vector<std::vector<NGr2::SPointerFixup>> m_PointerFixups;
	NGr2::SReference m_RootObject;
	NGr2::SReference m_RootObjectType;
	uint32_t m_nTypeTag = 0;
};
