#include "StdAfx.h"
#include "BinaryBitFieldDialog.h"
#include "BinaryBitFieldDialog.h"
#include "System/VFSOperations.h"
#include "System/XmlSaver.h"

CBinaryBitFieldDialog::CBinaryBitFieldDialog( const string &_szFileName, const BYTE *_pData, const int _nSize, CWnd *pwndParent )
	: CResizeDialog( CBinaryBitFieldDialog::IDD, pwndParent ),
		bCreateControls( true ),
		szFileName( _szFileName ),
		pData( _pData ),
		nSize( _nSize )
{
	SetControlStyle( IDC_CT_FIELDS, ANCHORE_LEFT_TOP | RESIZE_HOR_VER );
	SetControlStyle( IDOK, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDCANCEL, ANCHORE_RIGHT_BOTTOM );
}


CBinaryBitFieldDialog::~CBinaryBitFieldDialog()
{
}


void CBinaryBitFieldDialog::DoDataExchange( CDataExchange* pDX )
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_CT_FIELDS, wndTablesList );
}


BEGIN_MESSAGE_MAP(CBinaryBitFieldDialog, CResizeDialog)
END_MESSAGE_MAP()


struct SBinaryBitField
{
	string szName;
	int nValue;

	SBinaryBitField() : szName( "" ), nValue( -1 ) {}
	SBinaryBitField( const string &_szName, const int _nValue )	: szName( _szName ), nValue( _nValue ) {}
	SBinaryBitField( const char *_szName, const int _nValue )	: nValue( _nValue )
	{
		szName.assign( _szName );
	}

	int operator&( IXmlSaver &saver )
	{
		saver.Add( "Name", &szName );
		saver.Add( "Value", &nValue );

		return 0;
	}
};

BOOL CBinaryBitFieldDialog::OnInitDialog() 
{
	bCreateControls = true;

	CResizeDialog::OnInitDialog();

	vector<SBinaryBitField> fields;
	CFileStream stream( NVFS::GetMainVFS(), szFileName.c_str() );
	CPtr<IXmlSaver> pXS = CreateXmlSaver( &stream, SAVER_MODE_READ );
	if ( pXS == 0 ) 
	{
		NI_ASSERT( pXS != 0, StrFmt("Can't open stream \"%s\" to read bit fields", szFileName.c_str()) );
		return FALSE;
	}
	pXS->Add( "Fields", &fields );

	for ( int i = 0; i < fields.size(); ++i )
	{
		name2value[fields[i].szName] = fields[i].nValue;
		value2name[fields[i].nValue] = fields[i].szName;
		wndTablesList.AddString( fields[i].szName.c_str() );
	}

	for ( int nByteIndex = 0; nByteIndex < nSize; ++nByteIndex )
	{
		BYTE bMask = 1;
		for ( int nBitIndex = 0; nBitIndex < 8; ++nBitIndex )
		{
			if ( pData[nByteIndex] & bMask )
			{
				const int index = wndTablesList.FindStringExact( 0, value2name[nByteIndex * 8 + nBitIndex].c_str() );
				if ( index != LB_ERR )
				{
					wndTablesList.SetCheck( index, 1 );
				}
			}
			bMask <<= 1;
		}
	}
	bCreateControls = false;
	return true;
}


void CBinaryBitFieldDialog::OnOK()
{
	CResizeDialog::OnOK();

	BYTE *pNewData = const_cast<BYTE *>( pData );
	memset( pNewData, 0, nSize );
	CString strString;
	for ( int nStringIndex = 0; nStringIndex < wndTablesList.GetCount(); ++nStringIndex )
	{
		if ( wndTablesList.GetCheck( nStringIndex ) )
		{
			wndTablesList.GetText( nStringIndex, strString );
			hash_map<string, int>::const_iterator pos = name2value.find( string( strString ) );
			if ( pos != name2value.end() ) 
			{
				const int nValue = pos->second;
				const int nIndex = nValue / 8;
				pNewData[nIndex] |= ( 1UL << ( nValue - nIndex * 8 ) );
			}
		}
	}
}

