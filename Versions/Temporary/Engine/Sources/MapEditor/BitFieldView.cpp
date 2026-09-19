#include "stdafx.h"

#include "BitFieldView.h"

#include "System/VFSOperations.h"
#include "System/XmlSaver.h"

#include <fmt/format.h>

#include <cstring>

// The reading and writing of the bits, which the dialog (BitFieldViewWx.cpp)
// calls.

namespace NBitField
{
	int SField::operator&( IXmlSaver &saver )
	{
		saver.Add( "Name", &szName );
		saver.Add( "Value", &nValue );
		return 0;
	}


	bool LoadFields( const std::string &rszFieldsFile, std::vector<SField> *pFields )
	{
		if ( pFields == 0 )
		{
			return false;
		}
		pFields->clear();
		CFileStream stream( NVFS::GetMainVFS(), rszFieldsFile.c_str() );
		CPtr<IXmlSaver> pXS = CreateXmlSaver( &stream, SAVER_MODE_READ );
		if ( pXS == 0 )
		{
			NI_ASSERT( pXS != 0, fmt::format( "Can't open stream \"{}\" to read bit fields", rszFieldsFile.c_str() ) );
			return false;
		}
		pXS->Add( "Fields", pFields );
		return true;
	}


	bool IsSet( const uint8_t *pData, int nSize, int nValue )
	{
		if ( pData == 0 || nValue < 0 || nValue >= nSize * 8 )
		{
			return false;
		}
		return ( pData[nValue / 8] & ( 1 << ( nValue % 8 ) ) ) != 0;
	}


	void Store( const std::vector<SField> &rFields, const std::vector<bool> &rChecked, uint8_t *pData, int nSize )
	{
		if ( pData == 0 || nSize <= 0 )
		{
			return;
		}
		memset( pData, 0, nSize );
		for ( size_t nField = 0; nField < rFields.size() && nField < rChecked.size(); ++nField )
		{
			const int nValue = rFields[nField].nValue;
			if ( rChecked[nField] && nValue >= 0 && nValue < nSize * 8 )
			{
				pData[nValue / 8] |= static_cast<uint8_t>( 1 << ( nValue % 8 ) );
			}
		}
	}
}
