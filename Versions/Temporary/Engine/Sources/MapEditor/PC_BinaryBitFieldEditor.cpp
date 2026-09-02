#include "stdafx.h"
#include <fmt/format.h>
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"

#include "PC_BinaryBitFieldEditor.h"
#include "BinaryBitFieldDialog.h"

#include "MapEditorLib/Interface_UserData.h"

#include <cstdint>

bool CPCBinaryBitFieldEditor::GetPCItemStringValue( std::string *pszValue, const CVariant &rValue, const SPropertyDesc *pPropertyDesc )
{
	NI_ASSERT( pszValue != 0, "CPCBinaryBitFieldEditor::GetPCItemStringValue() pszValue == 0" );
	pszValue->clear();
	if ( rValue.GetType() == CVariant::VT_POINTER )
	{
		const uint8_t *pValues = static_cast<const uint8_t*>( rValue.GetPtr() );
		for ( int nByteIndex = 0; nByteIndex < pPropertyDesc->nSize; ++nByteIndex )
			*pszValue += fmt::format( "{:02X}", pValues[nByteIndex] );
	}
	else if ( rValue.GetType() == CVariant::VT_INT )
	{
		const int nValue = (int)rValue;
		const uint8_t *pValues = reinterpret_cast<const uint8_t *>( &nValue );
		for ( int i = 0; i < 4; ++i )
			*pszValue += fmt::format( "{:02X}", pValues[i] );
	}
	else
	{
		NI_ASSERT( false, fmt::format("Can't convert type {} to bitfield", rValue.GetType()) );
	}
	return true;
}


bool CPCBinaryBitFieldEditor::GetPCItemValue( CVariant *pValue, const std::string &rszValue, const SPropertyDesc *pPropertyDesc )
{
	NI_ASSERT( pValue != 0, "CPCBinaryBitFieldEditor::GetPCItemValue() pValue == 0" );
	( *pValue ) = CVariant();
	uint8_t * pData = new uint8_t[pPropertyDesc->nSize];
	memset( pData, 0, pPropertyDesc->nSize );
	{
		uint8_t nHighByte = 0;
		uint8_t nLowByte = 0;
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
	std::string szValue;
	GetPCItemStringValue( &szValue, rValue, GetPropertyDesc() );
	CPCStringBrowseEditor::SetValue( szValue );
}


void CPCBinaryBitFieldEditor::GetValue( CVariant *pValue )
{
	if ( pValue )
	{
		CPCStringBrowseEditor::GetValue( pValue );
		const std::string szValue = pValue->GetStr();
		GetPCItemValue( pValue, szValue, GetPropertyDesc() );
	}
}

void CPCBinaryBitFieldEditor::OnBrowse()
{
	CVariant value;
	GetValue( &value );
	CBinaryBitFieldDialog binaryBitFieldDialog( Singleton<IUserDataContainer>()->Get()->constUserData.szStartFolder + GetPropertyDesc()->szStringParam, static_cast<const uint8_t*>( value.GetPtr() ), GetPropertyDesc()->nSize, GetTargetWindow() );
	if ( ( binaryBitFieldDialog.DoModal() == IDOK ) && ( ( GetStyle() & ES_READONLY ) == 0 ) )
	{
		std::string szValue;
		GetPCItemStringValue( &szValue, value, GetPropertyDesc() );
		SetWindowText( szValue.c_str() );
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
}


