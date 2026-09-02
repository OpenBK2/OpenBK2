// Reading the chain of type definitions that describes a file's structures.

#include "TypeTree.h"

#include "Structures.h"
#include "Trace.h"

#include <cstring>

namespace NGr2
{

namespace
{

//! A pointer in a file is four bytes whatever the host is.
constexpr uint32_t DISK_POINTER_SIZE = 4;

//! What one member occupies on disk, before its array width multiplies it.
//!
//! Inline is absent because its size is its referenced type's, which takes a
//! lookup and so cannot be a constant.
uint32_t ScalarDiskSize( uint32_t nType )
{
	switch ( nType )
	{
		case MEMBER_REFERENCE:
		case MEMBER_STRING:
		case MEMBER_EMPTY_REFERENCE:
			return DISK_POINTER_SIZE;

		case MEMBER_REFERENCE_TO_ARRAY:
		case MEMBER_ARRAY_OF_REFERENCES:
			// A count and a pointer, which is the pair that becomes two separate
			// members once marshalled into the shape granny211.h describes.
			return 4 + DISK_POINTER_SIZE;

		case MEMBER_VARIANT_REFERENCE:
			return 2 * DISK_POINTER_SIZE;

		case MEMBER_REFERENCE_TO_VARIANT_ARRAY:
			return 2 * DISK_POINTER_SIZE + 4;

		case MEMBER_TRANSFORM:
			// Flags, Position[3], Orientation[4], ScaleShear[3][3].
			return 4 + 12 + 16 + 36;

		case MEMBER_REAL32:
		case MEMBER_INT32:
		case MEMBER_UINT32:
			return 4;

		case MEMBER_INT16:
		case MEMBER_UINT16:
		case MEMBER_BINORMAL_INT16:
		case MEMBER_NORMAL_UINT16:
		case MEMBER_REAL16:
			return 2;

		case MEMBER_INT8:
		case MEMBER_UINT8:
		case MEMBER_BINORMAL_INT8:
		case MEMBER_NORMAL_UINT8:
			return 1;

		default:
			return 0;
	}
}

//! A guard against a type tree that refers to itself through Inline members.
//!
//! No shipped file does, and a file that did would recurse until the stack ran
//! out, which is the one failure mode a loader cannot report.
constexpr uint32_t MAX_TYPE_DEPTH = 32;

//! No shipped structure has more than about thirty members.
constexpr uint32_t MAX_MEMBERS = 1024;

const std::vector<SMember> *ReadTypeAt( granny_file &file, const SReference &typeRef,
                                        uint32_t nDepth );

uint32_t ObjectSizeAt( granny_file &file, const SReference &typeRef, uint32_t nDepth )
{
	const std::vector<SMember> *pMembers = ReadTypeAt( file, typeRef, nDepth );
	if ( pMembers == nullptr || pMembers->empty() )
	{
		return 0;
	}
	return pMembers->back().nOffset + pMembers->back().nSize;
}

const std::vector<SMember> *ReadTypeAt( granny_file &file, const SReference &typeRef,
                                        uint32_t nDepth )
{
	const auto cached = file.m_Types.find( typeRef );
	if ( cached != file.m_Types.end() )
	{
		return cached->second.empty() ? nullptr : &cached->second;
	}
	if ( nDepth > MAX_TYPE_DEPTH )
	{
		Logger().warn( "type tree at {}:{} nests deeper than {}", typeRef.nSection,
		               typeRef.nOffset, MAX_TYPE_DEPTH );
		return nullptr;
	}

	// An empty entry marks both "malformed" and "being read", so a type that
	// reaches itself finds the empty vector rather than recursing.
	std::vector<SMember> &members = file.m_Types[typeRef];

	uint32_t nAt = typeRef.nOffset;
	uint32_t nOffset = 0;
	for ( ;; )
	{
		if ( members.size() > MAX_MEMBERS )
		{
			Logger().warn( "type at {}:{} has no End marker in {} members",
			               typeRef.nSection, typeRef.nOffset, MAX_MEMBERS );
			members.clear();
			return nullptr;
		}

		uint32_t nType = 0;
		if ( !file.ReadU32( typeRef.nSection, nAt, &nType ) )
		{
			Logger().warn( "type at {}:{} runs past its section", typeRef.nSection,
			               typeRef.nOffset );
			members.clear();
			return nullptr;
		}
		if ( nType == MEMBER_END )
		{
			break;
		}
		if ( nType >= MEMBER_ONE_PAST_LAST )
		{
			Logger().warn( "member type {} at {}:{} is not one Granny defines", nType,
			               typeRef.nSection, nAt );
			members.clear();
			return nullptr;
		}

		SMember member;
		member.nType = nType;
		member.pszName = file.ReadString( typeRef.nSection, nAt + 4 );
		member.bHasReferenceType =
			file.ResolvePointer( typeRef.nSection, nAt + 8, &member.ReferenceType );

		int32_t nArrayWidth = 0;
		if ( !file.ReadI32( typeRef.nSection, nAt + 12, &nArrayWidth ) )
		{
			members.clear();
			return nullptr;
		}
		member.nArrayWidth = nArrayWidth;

		const uint32_t nWidth = nArrayWidth > 0 ? static_cast<uint32_t>( nArrayWidth ) : 1;
		if ( nType == MEMBER_INLINE )
		{
			if ( !member.bHasReferenceType )
			{
				Logger().warn( "inline member {} at {}:{} names no type",
				               member.pszName ? member.pszName : "(unnamed)",
				               typeRef.nSection, nAt );
				members.clear();
				return nullptr;
			}
			member.nSize = ObjectSizeAt( file, member.ReferenceType, nDepth + 1 ) * nWidth;
		}
		else
		{
			member.nSize = ScalarDiskSize( nType ) * nWidth;
		}
		if ( member.nSize == 0 )
		{
			Logger().warn( "member {} at {}:{} has no size",
			               member.pszName ? member.pszName : "(unnamed)", typeRef.nSection,
			               nAt );
			members.clear();
			return nullptr;
		}

		member.nOffset = nOffset;
		nOffset += member.nSize;
		members.push_back( member );

		nAt += TYPE_DEFINITION_DISK_SIZE;
	}

	return members.empty() ? nullptr : &members;
}

}

const std::vector<SMember> *ReadType( granny_file &file, const SReference &typeRef )
{
	return ReadTypeAt( file, typeRef, 0 );
}

uint32_t DiskObjectSize( granny_file &file, const SReference &typeRef )
{
	return ObjectSizeAt( file, typeRef, 0 );
}

const SMember *FindMember( const std::vector<SMember> &members, const char *pszName )
{
	for ( const SMember &member : members )
	{
		if ( member.pszName != nullptr && strcmp( member.pszName, pszName ) == 0 )
		{
			return &member;
		}
	}
	return nullptr;
}

}

using namespace NGr2;

namespace
{

//! What one member occupies in memory, which is what the entry points below mean.
//!
//! Different from ScalarDiskSize wherever a pointer is involved, since these run
//! over the converted structures rather than the file. The engine uses them for
//! one thing, walking a vertex type to find the offset of a named component, and
//! vertex types contain no pointers, so the two agree everywhere it matters. A
//! vertex type that did contain one would make the engine's stride disagree with
//! the file's, which is why Convert.cpp says so if it ever meets one.
uint32_t NativeMemberSize( const SDataTypeDefinition *pMember )
{
	const uint32_t nWidth =
		pMember->nArrayWidth > 0 ? static_cast<uint32_t>( pMember->nArrayWidth ) : 1;

	switch ( pMember->nType )
	{
		case MEMBER_INLINE:
		{
			if ( pMember->pReferenceType == nullptr )
			{
				return 0;
			}
			uint32_t nTotal = 0;
			for ( const SDataTypeDefinition *p = pMember->pReferenceType;
			      p != nullptr && p->nType != MEMBER_END; ++p )
			{
				nTotal += NativeMemberSize( p );
			}
			return nTotal * nWidth;
		}

		case MEMBER_REFERENCE:
		case MEMBER_STRING:
		case MEMBER_EMPTY_REFERENCE:
			return static_cast<uint32_t>( sizeof( void * ) ) * nWidth;

		case MEMBER_REFERENCE_TO_ARRAY:
		case MEMBER_ARRAY_OF_REFERENCES:
			return static_cast<uint32_t>( 4 + sizeof( void * ) ) * nWidth;

		case MEMBER_VARIANT_REFERENCE:
			return static_cast<uint32_t>( sizeof( SVariant ) ) * nWidth;

		case MEMBER_REFERENCE_TO_VARIANT_ARRAY:
			return static_cast<uint32_t>( 4 + 2 * sizeof( void * ) ) * nWidth;

		case MEMBER_TRANSFORM:
			return static_cast<uint32_t>( sizeof( STransform ) ) * nWidth;

		default:
			return ScalarDiskSize( pMember->nType ) * nWidth;
	}
}

}

extern "C"
{

GR2_API( granny_int32x )
	GrannyGetMemberTypeSize( granny_data_type_definition const *MemberType )
{
	GR2_TRACE( "MemberType={}", MemberType );

	if ( MemberType == nullptr )
	{
		return 0;
	}
	return static_cast<granny_int32x>(
		NativeMemberSize( reinterpret_cast<const SDataTypeDefinition *>( MemberType ) ) );
}

GR2_API( granny_int32x )
	GrannyGetTotalObjectSize( granny_data_type_definition const *TypeDefinition )
{
	GR2_TRACE( "TypeDefinition={}", TypeDefinition );

	if ( TypeDefinition == nullptr )
	{
		return 0;
	}

	// Every member up to the End marker. This is the vertex stride when the type
	// is a vertex type, which is the one thing the engine calls it for.
	uint32_t nTotal = 0;
	for ( const SDataTypeDefinition *p =
			  reinterpret_cast<const SDataTypeDefinition *>( TypeDefinition );
	      p != nullptr && p->nType != MEMBER_END; ++p )
	{
		nTotal += NativeMemberSize( p );
	}
	return static_cast<granny_int32x>( nTotal );
}

// Copy one object from one type definition to another, member by member and by
// name. Not written. Returns without touching DestObject.
//
// The map editor is the only caller in the tree: GetAttributesFromBone in
// ED_Common/TempAttributesTool.cpp builds a definition of GrannyReal32Member
// fields named after Maya attributes and asks for a bone's ExtendedData to be
// read into them. Nothing in the game reaches this.
//
// Doing nothing is the right stub for the data we have. Real 2.11 leaves
// ExtendedData null on file_info, model, skeleton, bone and animation for every
// file in this corpus, measured against the DLL and written up in
// docs/GrannyReplacement.md, so SourceType and SourceObject are both null here
// and there is nothing to read whatever this function does.
//
// Two things have to be settled before it can be written, and neither can be
// guessed: what 2.11 does with a destination member that has no match in the
// source, leave it or zero it, and whether it converts between member types or
// only copies matching ones. GetAttributesFromBone does not initialise its
// destination, so the first answer decides whether the editor reads its own
// stack. Both are measurable against the vendored DLL the same way the rest of
// the conversion was.
//
// The signature returns void, so there is no failure to report. The warn-once
// line GR2_STUB emits is the whole signal that this was reached.
GR2_API( void ) GrannyConvertSingleObject( granny_data_type_definition const *SourceType,
                                           void const *SourceObject,
                                           granny_data_type_definition const *DestType,
                                           void *DestObject,
                                           granny_conversion_handler *OverrideHandler )
{
	GR2_STUB( "SourceType={} SourceObject={} DestType={} DestObject={} OverrideHandler={}",
	          SourceType, SourceObject, DestType, DestObject, OverrideHandler );
}

}
