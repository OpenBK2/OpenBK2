#include "stdafx.h"

#include "PC_Constants.h"
#include "Misc/StrProc.h"
#include "PC_IntComboEditor.h"
#include "MapEditorLib/StringManager.h"

// CPCItemEditor

bool CPCIntComboEditor::CreateEditor( const std::string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow )
{
	if ( CPCStringComboEditor::CreateEditor( rszName, _nEditorType, _pPropertyDesc, _nControlID, rObjectSet, _pwndTargetWindow ) )
	{
		SetCreateControls( true );
		ResetContent();
		//
		std::vector<std::string> stringList;
		//
		std::string szValues = GetPropertyDesc()->szStringParam;
		NStr::ToLowerASCII( &szValues );
		//
		std::string szNumbers;
		if ( !CStringManager::GetStringValueFromString( szValues, PCSPL_VALUES, 0, PCSP_STRONG_DIVIDERS, "", &szNumbers ) )
		{
			return false;
		}
		int	nStep = CStringManager::GetIntValueFromString( szValues, PCSPL_STEP, 0, PCSP_DIVIDERS, 1 );
		if ( nStep <= 0 )
		{
			nStep = 1;
		}
		//		
		int nLeftPos = szNumbers.find_first_of( PCSP_NUMBERS, 0 );
		while( nLeftPos != std::string::npos )
		{
			const int nRightPos = szNumbers.find_first_of( PCSP_SOFT_DIVIDERS, nLeftPos + 1 );
			const std::string szNumberList = szNumbers.substr( nLeftPos, nRightPos - nLeftPos );
			const int nRangePos = szNumberList.find_first_of( PCSP_RANGE_DIVIDERS );
			if ( nRangePos == std::string::npos )
			{
				int nValue = 0;
				if ( sscanf( szNumberList.c_str(), "%d", &nValue ) == 1 )
				{
					const std::string szValue = std::to_string(  nValue );
					stringList.push_back( szValue );
				}
				else
				{
					return false;
				}
			}
			else
			{
				const std::string szMinValue = szNumberList.substr( 0, nRangePos );
				const std::string szMaxValue = szNumberList.substr( nRangePos + 1 );
				int nMinValue = 0;
				int nMaxValue = 0;
				if ( ( sscanf( szMinValue.c_str(), "%d", &nMinValue ) == 1 ) &&
						 ( sscanf( szMaxValue.c_str(), "%d", &nMaxValue ) == 1 ) )
				{
					if ( nMinValue > nMaxValue )
					{
						const int nSwapValue = nMinValue;
						nMinValue = nMaxValue;
						nMaxValue = nSwapValue;
					}
					for ( int nValue = nMinValue; nValue <= nMaxValue; nValue += nStep )
					{
						const std::string szValue = std::to_string(  nValue );
						stringList.push_back( szValue );
					}
				}
			}
			if ( nRightPos != std::string::npos )
			{
				nLeftPos = szNumbers.find_first_of( PCSP_NUMBERS, nRightPos + 1 );
			}
			else
			{
				nLeftPos = std::string::npos;
			}
		}
		//
		if ( stringList.empty() )
		{
			return false;
		}
		//
		sort( stringList.begin(), stringList.end(), CPCIntComboEditorCompareItem() ); 
		for ( std::vector<std::string>::const_iterator itString = stringList.begin(); itString != stringList.end(); ++itString )
		{
			AddString( itString->c_str() );
		}
		SetCreateControls( false );
		return true;
	}
	return false;
}


void CPCIntComboEditor::SetValue( const CVariant &rValue )
{
	CVariant value = std::string( std::to_string(  (int)rValue ) );
	CPCStringComboEditor::SetValue( value );
}


void CPCIntComboEditor::GetValue( CVariant *pValue )
{
	if ( pValue )
	{
		CVariant value;
		CPCStringComboEditor::GetValue( &value );
		int nValue = 0;
		bool bWrongValue = true;
		if ( sscanf( value.GetStr(), "%d", &nValue ) == 1 )
		{
			bWrongValue = false;
		}
		if ( bWrongValue )
		{
			CPCStringComboEditor::SetDefaultValue();
			CPCStringComboEditor::GetValue( &value );
			sscanf( value.GetStr(), "%d", &nValue );
		}
		*pValue = nValue;
	}
}

// basement storage  


