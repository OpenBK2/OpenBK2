#include "MinimalGr2.h"

#include <cstring>

namespace NGr2Test
{

const uint8_t g_Magic[16] = { 0xb8, 0x67, 0xb0, 0xca, 0xf8, 0x6d, 0xb1, 0x0f,
                              0x84, 0x72, 0x8c, 0x7e, 0x5e, 0x19, 0x00, 0x1e };

CHeaderShapedFile::CHeaderShapedFile( uint32_t nSectionCount )
{
	const uint32_t nTotal =
		MAGIC_BLOCK_SIZE + HEADER_SIZE + SECTION_RECORD_SIZE * nSectionCount;
	m_Bytes.assign( nTotal, 0 );

	memcpy( m_Bytes.data(), g_Magic, MAGIC_SIZE );

	// headerSize counts the magic block too, which is why it is 440 and not 408
	// in an 8-section file. Read as "everything before the first section's data".
	SetU32( OFF_HEADER_SIZE, nTotal );
	SetU32( OFF_HEADER_FORMAT, 0 );
	SetU32( OFF_VERSION, 6 );
	SetU32( OFF_TOTAL_SIZE, nTotal );
	// Left zero on purpose. Whether the real DLL checks the CRC, and over which
	// range, is unmeasured; putting a made up value here would assert a fact
	// nobody established. See the RejectsWrongCrc test.
	SetU32( OFF_CRC, 0 );
	SetU32( OFF_SECTION_ARRAY_OFFSET, HEADER_SIZE );
	SetU32( OFF_SECTION_ARRAY_COUNT, nSectionCount );
	// Shipped files point the root object type at section 6 and the root object
	// at section 0, offset 0. Copied rather than reasoned about: this fixture is
	// not parsed past the section array yet.
	SetU32( OFF_ROOT_OBJECT_TYPE_SECTION, 6 );
	SetU32( OFF_ROOT_OBJECT_TYPE_OFFSET, 0 );
	SetU32( OFF_ROOT_OBJECT_SECTION, 0 );
	SetU32( OFF_ROOT_OBJECT_OFFSET, 0 );
	SetU32( OFF_TYPE_TAG, TYPE_TAG_13 );

	for ( uint32_t i = 0; i < nSectionCount; ++i )
	{
		// Empty and uncompressed, all pointing at the end of the section array,
		// which is where a real file's first section data starts.
		SetSectionField( i, SEC_COMPRESSION, COMPRESSION_NONE );
		SetSectionField( i, SEC_DATA_OFFSET, nTotal );
		SetSectionField( i, SEC_INTERNAL_ALIGNMENT, 4 );
		SetSectionField( i, SEC_POINTER_FIXUP_OFFSET, nTotal );
		SetSectionField( i, SEC_MIXED_MARSHALLING_OFFSET, nTotal );
	}
}

uint32_t CHeaderShapedFile::GetU32( uint32_t nOffset ) const
{
	uint32_t nValue = 0;
	memcpy( &nValue, m_Bytes.data() + nOffset, sizeof( nValue ) );
	return nValue;
}

void CHeaderShapedFile::SetU32( uint32_t nOffset, uint32_t nValue )
{
	memcpy( m_Bytes.data() + nOffset, &nValue, sizeof( nValue ) );
}

uint32_t CHeaderShapedFile::SectionArrayOffset() const
{
	return HEADER_OFFSET + GetU32( OFF_SECTION_ARRAY_OFFSET );
}

uint32_t CHeaderShapedFile::GetSectionField( uint32_t nSection, uint32_t nFieldOffset ) const
{
	return GetU32( SectionArrayOffset() + SECTION_RECORD_SIZE * nSection + nFieldOffset );
}

void CHeaderShapedFile::SetSectionField( uint32_t nSection, uint32_t nFieldOffset,
                                         uint32_t nValue )
{
	SetU32( SectionArrayOffset() + SECTION_RECORD_SIZE * nSection + nFieldOffset, nValue );
}

void CHeaderShapedFile::TruncateTo( uint32_t nSize )
{
	if ( nSize < m_Bytes.size() )
	{
		m_Bytes.resize( nSize );
	}
}

std::vector<uint8_t> ForeignMagicFile()
{
	// Long enough that nothing rejects it for being short, so that the magic is
	// the only reason left.
	std::vector<uint8_t> bytes( 512, 0 );
	const char szHead[] = "[Localization]\r\nLanguage=Russian\r\n";
	memcpy( bytes.data(), szHead, sizeof( szHead ) - 1 );
	return bytes;
}

}
