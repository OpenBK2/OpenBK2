#include "MinimalGr2.h"

#include <cstring>

namespace NGr2Test
{

const uint8_t g_Magic[16] = { 0xb8, 0x67, 0xb0, 0xca, 0xf8, 0x6d, 0xb1, 0x0f,
                              0x84, 0x72, 0x8c, 0x7e, 0x5e, 0x19, 0x00, 0x1e };

CHeaderShapedFile::CHeaderShapedFile( uint32_t nSectionCount )
	: m_nSectionCount( nSectionCount )
{
	// Shipped files put the root object type in section 6 and the root object in
	// section 0. Copied rather than reasoned about; nothing here parses that far.
	m_nRootTypeSection = nSectionCount > 6 ? 6 : 0;
	m_SectionData.resize( nSectionCount );
	m_Fixups.resize( nSectionCount );
	Build();
}

void CHeaderShapedFile::SetSectionData( uint32_t nSection, std::vector<uint8_t> data )
{
	m_SectionData[nSection] = std::move( data );
	Build();
}

void CHeaderShapedFile::AddPointerFixup( uint32_t nSection, uint32_t nFromOffset,
                                         uint32_t nToSection, uint32_t nToOffset )
{
	m_Fixups[nSection].push_back( SFixup{ nFromOffset, nToSection, nToOffset } );
	Build();
}

void CHeaderShapedFile::SetRootObject( uint32_t nSection, uint32_t nOffset )
{
	m_nRootSection = nSection;
	m_nRootOffset = nOffset;
	Build();
}

void CHeaderShapedFile::SetTypeTag( uint32_t nTag )
{
	m_nTypeTag = nTag;
	Build();
}

void CHeaderShapedFile::Build()
{
	const uint32_t nArrayBegin = MAGIC_BLOCK_SIZE + HEADER_SIZE;
	const uint32_t nArrayEnd = nArrayBegin + SECTION_RECORD_SIZE * m_nSectionCount;

	m_Bytes.assign( nArrayEnd, 0 );
	memcpy( m_Bytes.data(), g_Magic, MAGIC_SIZE );

	// headerSize counts the magic block too, which is why it is 440 and not 408 in
	// an 8-section file. Read it as "everything before the first section".
	SetU32( OFF_HEADER_SIZE, nArrayEnd );
	SetU32( OFF_HEADER_FORMAT, 0 );
	SetU32( OFF_VERSION, 6 );
	// Left zero on purpose. Whether the real DLL checks the CRC, and over which
	// range, is unmeasured, and a made up value here would assert a fact nobody
	// established. See the RejectsWrongCrc test.
	SetU32( OFF_CRC, 0 );
	SetU32( OFF_SECTION_ARRAY_OFFSET, HEADER_SIZE );
	SetU32( OFF_SECTION_ARRAY_COUNT, m_nSectionCount );
	SetU32( OFF_ROOT_OBJECT_TYPE_SECTION, m_nRootTypeSection );
	SetU32( OFF_ROOT_OBJECT_TYPE_OFFSET, 0 );
	SetU32( OFF_ROOT_OBJECT_SECTION, m_nRootSection );
	SetU32( OFF_ROOT_OBJECT_OFFSET, m_nRootOffset );
	SetU32( OFF_TYPE_TAG, m_nTypeTag );

	// Per section, in the order shipped files use: pointer fixups, then the
	// marshalling array, then the data.
	for ( uint32_t i = 0; i < m_nSectionCount; ++i )
	{
		const std::vector<SFixup> &fixups = m_Fixups[i];
		const std::vector<uint8_t> &data = m_SectionData[i];

		const uint32_t nFixupOffset = static_cast<uint32_t>( m_Bytes.size() );
		for ( const SFixup &fixup : fixups )
		{
			const uint32_t nAt = static_cast<uint32_t>( m_Bytes.size() );
			m_Bytes.resize( nAt + POINTER_FIXUP_SIZE, 0 );
			SetU32( nAt + 0, fixup.nFromOffset );
			SetU32( nAt + 4, fixup.nToSection );
			SetU32( nAt + 8, fixup.nToOffset );
		}

		// Always empty. 170 sections out of 61,952 carry one entry and the rest
		// none, so the offset is what a shipped file has and the count is what
		// almost all of them have.
		const uint32_t nMarshallingOffset = static_cast<uint32_t>( m_Bytes.size() );

		const uint32_t nDataOffset = static_cast<uint32_t>( m_Bytes.size() );
		m_Bytes.insert( m_Bytes.end(), data.begin(), data.end() );

		SetSectionField( i, SEC_COMPRESSION, COMPRESSION_NONE );
		SetSectionField( i, SEC_DATA_OFFSET, nDataOffset );
		SetSectionField( i, SEC_DATA_SIZE, static_cast<uint32_t>( data.size() ) );
		// Equal to dataSize because nothing here is compressed. The loader checks
		// that, since an uncompressed section that claims to expand is incoherent.
		SetSectionField( i, SEC_EXPANDED_DATA_SIZE, static_cast<uint32_t>( data.size() ) );
		SetSectionField( i, SEC_INTERNAL_ALIGNMENT, 4 );
		SetSectionField( i, SEC_FIRST_16BIT, static_cast<uint32_t>( data.size() ) );
		SetSectionField( i, SEC_FIRST_8BIT, static_cast<uint32_t>( data.size() ) );
		SetSectionField( i, SEC_POINTER_FIXUP_OFFSET, nFixupOffset );
		SetSectionField( i, SEC_POINTER_FIXUP_COUNT, static_cast<uint32_t>( fixups.size() ) );
		SetSectionField( i, SEC_MIXED_MARSHALLING_OFFSET, nMarshallingOffset );
		SetSectionField( i, SEC_MIXED_MARSHALLING_COUNT, 0 );
	}

	SetU32( OFF_TOTAL_SIZE, static_cast<uint32_t>( m_Bytes.size() ) );
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

std::vector<uint8_t> Pattern( uint32_t nBytes, uint8_t nSeed )
{
	std::vector<uint8_t> bytes( nBytes );
	for ( uint32_t i = 0; i < nBytes; ++i )
	{
		bytes[i] = static_cast<uint8_t>( nSeed + i * 7u + ( i >> 5 ) );
	}
	return bytes;
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
