#pragma once

// A .gr2 with a hand authored type tree, for the tests that walk one.
//
// MinimalGr2.h builds the container: a header, sections, fixups. This builds
// what goes inside it. A GR2 describes its own structures, so the only way to
// author a file whose root is a granny_file_info, or whose track group holds a
// curve, is to write the type definitions that say so. That makes these fixtures
// a real exercise of the walker rather than a mock of it.
//
// Shared between FileInfo.cpp and Animation.cpp, which build files out of the
// same pieces and would otherwise carry two copies of this that could drift.

#include "MinimalGr2.h"

#include <gr2/granny.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace NGr2Test
{

// The member type numbers the fixtures use, from granny_member_type. A second
// copy of the enum in src/File.h on purpose: a fixture that read the
// implementation's own numbers could not catch one of them being wrong.
constexpr uint32_t T_INLINE = 1;
constexpr uint32_t T_REFERENCE = 2;
constexpr uint32_t T_REFERENCE_TO_ARRAY = 3;
constexpr uint32_t T_ARRAY_OF_REFERENCES = 4;
constexpr uint32_t T_STRING = 8;
constexpr uint32_t T_TRANSFORM = 9;
constexpr uint32_t T_REAL32 = 10;
constexpr uint32_t T_INT32 = 19;

//! A file with a hand authored type tree, so the walker has something to walk.
//!
//! Section 0 holds objects, section 1 strings, section 2 type definitions. That
//! is not how a real file is arranged, and deliberately so: nothing in the reader
//! may assume a particular section holds a particular kind of thing.
class CTypedFile
{
public:
	static constexpr uint32_t OBJECTS = 0;
	static constexpr uint32_t STRINGS = 1;
	static constexpr uint32_t TYPES = 2;

	//! Add a string, and hand back where it landed.
	uint32_t AddString( const std::string &s )
	{
		const uint32_t nAt = static_cast<uint32_t>( m_Strings.size() );
		m_Strings.insert( m_Strings.end(), s.begin(), s.end() );
		m_Strings.push_back( 0 );
		return nAt;
	}

	//! Reserve nBytes of object space, zeroed, and hand back where it starts.
	uint32_t AddObject( uint32_t nBytes )
	{
		const uint32_t nAt = static_cast<uint32_t>( m_Objects.size() );
		m_Objects.resize( m_Objects.size() + nBytes, 0 );
		return nAt;
	}

	void PutU32( uint32_t nAt, uint32_t nValue )
	{
		memcpy( m_Objects.data() + nAt, &nValue, sizeof( nValue ) );
	}

	void PutI32( uint32_t nAt, int32_t nValue )
	{
		memcpy( m_Objects.data() + nAt, &nValue, sizeof( nValue ) );
	}

	void PutReal32( uint32_t nAt, float fValue )
	{
		memcpy( m_Objects.data() + nAt, &fValue, sizeof( fValue ) );
	}

	//! One member of a type definition. Written in order; End is added for you.
	struct SMemberSpec
	{
		const char *pszName;
		uint32_t nType;
		int32_t nArrayWidth;
		//! Where the referenced type is, in the types section. 0 means none, which
		//! is why no type is ever placed at offset 0.
		uint32_t nReferenceType;
	};

	//! Write a type definition, and hand back where it starts.
	uint32_t AddType( const std::vector<SMemberSpec> &members )
	{
		if ( m_Types.empty() )
		{
			// Nothing at offset 0, so that 0 can mean "no referenced type".
			m_Types.resize( 32, 0 );
		}
		const uint32_t nAt = static_cast<uint32_t>( m_Types.size() );
		m_Types.resize( m_Types.size() + 32 * ( members.size() + 1 ), 0 );

		for ( size_t i = 0; i < members.size(); ++i )
		{
			const uint32_t nEntry = nAt + static_cast<uint32_t>( i ) * 32;
			PutTypeU32( nEntry, members[i].nType );
			PutTypeI32( nEntry + 12, members[i].nArrayWidth );

			m_Fixups.push_back( SFixup{ TYPES, nEntry + 4, STRINGS,
			                            AddString( members[i].pszName ) } );
			if ( members[i].nReferenceType != 0 )
			{
				m_Fixups.push_back(
					SFixup{ TYPES, nEntry + 8, TYPES, members[i].nReferenceType } );
			}
		}
		// The trailing entry is zeroed, and End is 0.
		return nAt;
	}

	//! A pointer at nFrom in the objects section, leading to nTo.
	void Point( uint32_t nFrom, uint32_t nToSection, uint32_t nToOffset )
	{
		m_Fixups.push_back( SFixup{ OBJECTS, nFrom, nToSection, nToOffset } );
	}

	//! A string member: writes the fixup that makes it point at the text.
	void PointAtString( uint32_t nFrom, const std::string &s )
	{
		m_Fixups.push_back( SFixup{ OBJECTS, nFrom, STRINGS, AddString( s ) } );
	}

	void SetRoot( uint32_t nTypeOffset, uint32_t nObjectOffset )
	{
		m_nRootType = nTypeOffset;
		m_nRootObject = nObjectOffset;
	}

	//! Assemble, and load through the public entry point.
	granny_file *Load()
	{
		CHeaderShapedFile file( 3 );
		file.SetSectionData( OBJECTS, m_Objects );
		file.SetSectionData( STRINGS, m_Strings );
		file.SetSectionData( TYPES, m_Types );
		for ( const SFixup &fixup : m_Fixups )
		{
			file.AddPointerFixup( fixup.nFromSection, fixup.nFromOffset, fixup.nToSection,
			                      fixup.nToOffset );
		}
		file.SetRootObject( OBJECTS, m_nRootObject );
		file.SetU32( OFF_ROOT_OBJECT_TYPE_SECTION, TYPES );
		file.SetU32( OFF_ROOT_OBJECT_TYPE_OFFSET, m_nRootType );

		m_Bytes = file.Bytes();
		return GrannyReadEntireFileFromMemory( static_cast<granny_int32x>( m_Bytes.size() ),
		                                       m_Bytes.data() );
	}

private:
	struct SFixup
	{
		uint32_t nFromSection;
		uint32_t nFromOffset;
		uint32_t nToSection;
		uint32_t nToOffset;
	};

	void PutTypeU32( uint32_t nAt, uint32_t nValue )
	{
		memcpy( m_Types.data() + nAt, &nValue, sizeof( nValue ) );
	}
	void PutTypeI32( uint32_t nAt, int32_t nValue )
	{
		memcpy( m_Types.data() + nAt, &nValue, sizeof( nValue ) );
	}

	std::vector<uint8_t> m_Objects;
	std::vector<uint8_t> m_Strings;
	std::vector<uint8_t> m_Types;
	std::vector<SFixup> m_Fixups;
	std::vector<uint8_t> m_Bytes;
	uint32_t m_nRootType = 0;
	uint32_t m_nRootObject = 0;
};

}
