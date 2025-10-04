#include "stdafx.h"
#include "mapeditorlib/resourcedefines.h"
#include "mapeditorlib/commandhandlerdefines.h"

#include "PC_BinaryBitFieldEditor.h"
#include "BinaryBitFieldDialog.h"

#include "MapEditorLib/Interface_UserData.h"

bool CPCBinaryBitFieldEditor::GetPCItemStringValue( string *pszValue, const CVariant &rValue, const SPropertyDesc *pPropertyDesc )
{
	NI_ASSERT( pszValue != 0, "CPCBinaryBitFieldEditor::GetPCItemStringValue() pszValue == 0" );
	pszValue->clear();
	if ( rValue.GetType() == CVariant::VT_POINTER )
	{
		const BYTE *pValues = static_cast<const BYTE*>( rValue.GetPtr() );
		for ( int nByteIndex = 0; nByteIndex < pPropertyDesc->nSize; ++nByteIndex )
			*pszValue += StrFmt( "%02X", pValues[nByteIndex] );
	}
	else if ( rValue.GetType() == CVariant::VT_INT )
	{
		const int nValue = (int)rValue;
		const BYTE *pValues = reinterpret_cast<const BYTE *>( &nValue );
		for ( int i = 0; i < 4; ++i )
			*pszValue += StrFmt( "%02X", pValues[i] );
	}
	else
	{
		NI_ASSERT( false, StrFmt("Can't convert type %d to bitfield", rValue.GetType()) );
	}
	return true;
}


bool CPCBinaryBitFieldEditor::GetPCItemValue( CVariant *pValue, const string &rszValue, const SPropertyDesc *pPropertyDesc )
{
	NI_ASSERT( pValue != 0, "CPCBinaryBitFieldEditor::GetPCItemValue() pValue == 0" );
	( *pValue ) = CVariant();
	BYTE * pData = new BYTE[pPropertyDesc->nSize];
	memset( pData, 0, pPropertyDesc->nSize );
	{
		BYTE nHighByte = 0;
		BYTE nLowByte = 0;
		int nByteIndex = 0;
		bool bLowByteAcquired = false;
		for ( int nCharIndex = 0; nCharIndex < rszValue.length(); ++nCharIndex ) 
		{
			if ( ( rszValue[nCharIndex] >= '0' ) && ( rszValue[nCharIndex] <= '9' ) )
			{
				nLowByte = rszValue[nCharIndex] - '0';
			}
			else if ( ( rszValue[nCharIndex] >= 'A' ) && ( rszValue[nCharIndex] <= 'F' ) )
			{
				nLowByte = rszValue[nCharIndex] - 'A' + 10;
			}
			else if ( ( rszValue[nCharIndex] >= 'a' ) && ( rszValue[nCharIndex] <= 'f' ) )
			{
				nLowByte = rszValue[nCharIndex] - 'a' + 10;
			}
			else
			{
				continue;
			}
			if ( bLowByteAcquired )
			{
				pData[nByteIndex] = ( nHighByte << 4 ) + nLowByte;
				++nByteIndex;
				if ( nByteIndex >= pPropertyDesc->nSize ) 
				{
					break;
				}
			}
			else
			{
				nHighByte = nLowByte;
			}
			bLowByteAcquired = !bLowByteAcquired;
		}
	}
	( *pValue ) = CVariant( static_cast<void*>( pData ), pPropertyDesc->nSize );
	pValue->SetDestructorDeleted( ( pPropertyDesc->nSize > 0 ), pPropertyDesc->nSize );
	return true;
}


void CPCBinaryBitFieldEditor::SetValue( const CVariant &rValue )
{
	string szValue;
	GetPCItemStringValue( &szValue, rValue, GetPropertyDesc() );
	CPCStringBrowseEditor::SetValue( szValue );
}


void CPCBinaryBitFieldEditor::GetValue( CVariant *pValue )
{
	if ( pValue )
	{
		CPCStringBrowseEditor::GetValue( pValue );
		const string szValue = pValue->GetStr();
		GetPCItemValue( pValue, szValue, GetPropertyDesc() );
	}
}

void CPCBinaryBitFieldEditor::OnBrowse()
{
	CVariant value;
	GetValue( &value );
	CBinaryBitFieldDialog binaryBitFieldDialog( Singleton<IUserDataContainer>()->Get()->constUserData.szStartFolder + GetPropertyDesc()->szStringParam, static_cast<const BYTE*>( value.GetPtr() ), GetPropertyDesc()->nSize, GetTargetWindow() );
	if ( ( binaryBitFieldDialog.DoModal() == IDOK ) && ( ( GetStyle() & ES_READONLY ) == 0 ) )
	{
		string szValue;
		GetPCItemStringValue( &szValue, value, GetPropertyDesc() );
		SetWindowText( szValue.c_str() );
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
}


