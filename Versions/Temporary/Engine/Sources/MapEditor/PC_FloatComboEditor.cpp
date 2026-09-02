#include "stdafx.h"
#include <fmt/format.h>

#include "PC_Constants.h"
#include "Misc/StrProc.h"
#include "PC_FloatComboEditor.h"
#include "MapEditorLib/StringManager.h"

CPCFloatComboEditor::CPCFloatComboEditor() : nPrecision( PCSV_DEFAULT_RECISION )
{
}


// CPCItemEditor

bool CPCFloatComboEditor::CreateEditor( const std::string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow )
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
		float	fStep = CStringManager::GetFloatValueFromString( szValues, PCSPL_STEP, 0, PCSP_DIVIDERS, 1 );
		nPrecision = CStringManager::GetIntValueFromString( szValues, PCSPL_PRECISION, 0, PCSP_DIVIDERS, nPrecision );
		if ( fStep <= 0.0f )
		{
			fStep = 1.0f;
		}
		if ( nPrecision > PCSV_MAX_RECISION )
		{
			nPrecision = PCSV_DEFAULT_RECISION;
		}
		else if ( nPrecision < 0 )
		{
			nPrecision = PCSV_DEFAULT_RECISION;
		}
		//		
//		const std::string szFormat = fmt::format( "%.{}f", nPrecision );
		int nLeftPos = szNumbers.find_first_of( PCSP_NUMBERS, 0 );
		while( nLeftPos != std::string::npos )
		{
			const int nRightPos = szNumbers.find_first_of( PCSP_SOFT_DIVIDERS, nLeftPos + 1 );
			const std::string szNumberList = szNumbers.substr( nLeftPos, nRightPos - nLeftPos );
			const int nRangePos = szNumberList.find_first_of( PCSP_RANGE_DIVIDERS );
			if ( nRangePos == std::string::npos )
			{
				float fValue = 0.0f;
				if ( sscanf( szNumberList.c_str(), "%g", &fValue ) == 1 )
				{
					const std::string szValue = CStringManager::GetFloatStringWithPrecision( fValue, nPrecision ); //StrFmt( szFormat.c_str(), fValue );
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
				float fMinValue = 0.0f;
				float fMaxValue = 0.0f;
				if ( ( sscanf( szMinValue.c_str(), "%g", &fMinValue ) == 1 ) &&
						 ( sscanf( szMaxValue.c_str(), "%g", &fMaxValue ) == 1 ) )
				{
					if ( fMinValue > fMaxValue )
					{
						const float fSwapValue = fMinValue;
						fMinValue = fMaxValue;
						fMaxValue = fSwapValue;
					}
					for ( float fValue = fMinValue; fValue <= fMaxValue; fValue += fStep )
					{
						const std::string szValue = CStringManager::GetFloatStringWithPrecision( fValue, nPrecision );//StrFmt( szFormat.c_str(), fValue );
						stringList.push_back( szValue.c_str() );
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
		sort( stringList.begin(), stringList.end(), CPCFloatComboEditorCompareItem() ); 
		for ( std::vector<std::string>::const_iterator itString = stringList.begin(); itString != stringList.end(); ++itString )
		{
			AddString( itString->c_str() );
		}
		SetCreateControls( false );
		return true;
	}
	return false;
}


void CPCFloatComboEditor::SetValue( const CVariant &rValue )
{
//	const std::string szFormat = fmt::format( "%.{}f", nPrecision );
	CVariant value = CStringManager::GetFloatStringWithPrecision( (float)rValue, nPrecision );//StrFmt( szFormat.c_str(), (float)rValue ) );
	CPCStringComboEditor::SetValue( value );
}


void CPCFloatComboEditor::GetValue( CVariant *pValue )
{
	if ( pValue )
	{
		CVariant value;
		CPCStringComboEditor::GetValue( &value );
		float fValue = 0.0f;
		bool bWrongValue = true;
		if ( sscanf( value.GetStr(), "%g", &fValue ) == 1 )
		{
			bWrongValue = false;
		}
		if ( bWrongValue )
		{
			CPCStringComboEditor::SetDefaultValue();
			CPCStringComboEditor::GetValue( &value );
			sscanf( value.GetStr(), "%g", &fValue );
		}
		*pValue = fValue;
	}
}

// basement storage  


