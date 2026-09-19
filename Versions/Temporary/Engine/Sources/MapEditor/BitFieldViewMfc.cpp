#include "stdafx.h"
#include "MapEditorLib/ToolkitChoice.h"

#include "BitFieldView.h"
#include "BinaryBitFieldDialog.h"

#include "MapEditorLib/MfcWidget.h"
#include "System/VFSOperations.h"
#include "System/XmlSaver.h"

#include <fmt/format.h>

#include <cstdlib>
#include <cstring>

// The bit field editor as it has always been, CBinaryBitFieldDialog, behind the
// boundary; the dispatcher in front of both implementations; and the reading
// and writing of the bits, which both dialogs call.

namespace
{
	// The same flag every migrated dialog follows, so a session runs either the
	// MFC set or the wx set.
	bool UseWx()
	{
		return NToolkit::UseWxViews();
	}
}


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


	bool RunMfc( IWidget *pParent, const std::string &rszFieldsFile, uint8_t *pData, int nSize )
	{
		CBinaryBitFieldDialog dialog( rszFieldsFile, pData, nSize, ToCWnd( pParent ) );
		return dialog.DoModal() == IDOK;
	}


	bool Run( IWidget *pParent, const std::string &rszFieldsFile, uint8_t *pData, int nSize )
	{
		if ( UseWx() )
		{
			return RunWx( pParent, rszFieldsFile, pData, nSize );
		}
		return RunMfc( pParent, rszFieldsFile, pData, nSize );
	}
}
