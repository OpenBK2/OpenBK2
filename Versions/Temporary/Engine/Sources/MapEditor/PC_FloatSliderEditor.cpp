#include "stdafx.h"
#include <fmt/format.h>

#include "PC_Constants.h"
#include "Misc/StrProc.h"
#include "MapEditorLib/StringManager.h"
#include "PC_FloatSliderEditor.h"

CPCFloatSliderEditor::CPCFloatSliderEditor() : fStep( 1.0f ), nPrecision( 4 ), nPowerPrecision( 10000 )
{
}


BEGIN_MESSAGE_MAP(CPCFloatSliderEditor, CPCStringSliderEditor)
END_MESSAGE_MAP()


void CPCFloatSliderEditor::OnChangeEditBox()
{
	CVariant value;
	CPCStringSliderEditor::GetValue( &value );
	float fValue = 0;
	if ( sscanf( value.GetStr(), "%g", &fValue ) == 1 )
	{
		SetValueChanged();
		GetSlider()->SetPos( (int)( fValue * nPowerPrecision  + 0.5f ) );
	}
}


// CPCItemEditor

bool CPCFloatSliderEditor::CreateEditor( const std::string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow )
{
	if ( CPCStringSliderEditor::CreateEditor( rszName, _nEditorType, _pPropertyDesc, _nControlID, rObjectSet, _pwndTargetWindow ) )
	{
		SetCreateControls( true );
		if ( GetPropertyDesc() )
		{
			std::string szValues = GetPropertyDesc()->szStringParam;
			NStr::ToLower( &szValues );
			//
			float fMin = 0.0f;
			float fMax = 0.0f;
			float fPage = 0.0f;
			//
			fMin = CStringManager::GetFloatValueFromString( szValues, PCSPL_MIN, 0, PCSP_DIVIDERS, fMin );
			fMax = CStringManager::GetFloatValueFromString( szValues, PCSPL_MAX, 0, PCSP_DIVIDERS, fMax );
			fStep = CStringManager::GetFloatValueFromString( szValues, PCSPL_STEP, 0, PCSP_DIVIDERS, fStep );
			fPage = CStringManager::GetFloatValueFromString( szValues, PCSPL_PAGE, 0, PCSP_DIVIDERS, fPage );
			nPrecision = CStringManager::GetIntValueFromString( szValues, PCSPL_PRECISION, 0, PCSP_DIVIDERS, nPrecision );

			if ( fMin == fMax )
			{
				return false;
			}
			else if ( fMin > fMax )
			{
				const float fSwapValue = fMin;
				fMin = fMax;
				fMax = fSwapValue;
			}
			//
			if ( fStep == 0.0f )
			{
				fStep = 1.0f;
			}
			else if ( fStep < 0.0f )
			{
				fStep *= (-1.0f);
			}
			//
			if ( fPage == 0.0f )
			{
				fPage = ( fMax - fMin ) / 10.0f;
			}
			else if ( fPage < 0.0f )
			{
				fPage *= (-1.0f);
			}
			//
			if ( nPrecision > 4 )
			{
				nPrecision = 4;
			}
			else if ( nPrecision < 0 )
			{
				nPrecision = 4;
			}
			//
			nPowerPrecision = CStringManager::GetPowerPrecision( nPrecision );
			//
			GetSlider()->SetRange( (int)( fMin * nPowerPrecision ), (int)( fMax * nPowerPrecision ) );
			GetSlider()->SetLineSize( (int)( fStep * nPowerPrecision ) );
			GetSlider()->SetPageSize( (int)( fPage * nPowerPrecision ) );
			GetSlider()->SetTicFreq( (int)( fPage * nPowerPrecision ) );
			SetCreateControls( false );
			return true;
		}
	}
	return false;
}


void CPCFloatSliderEditor::SetValue( const CVariant &rValue )
{
	const float fValue = ( 1.0f * CStringManager::NormalizeValue( (int)( (float)rValue * nPowerPrecision + 0.5f ),
																																(int)( fStep * nPowerPrecision ) ) ) /
											 ( 1.0f * nPowerPrecision );
//	const std::string szFormat = fmt::format( "%.{}f", nPrecision );
//	CVariant value = std::string( StrFmt( szFormat.c_str(), fValue ) ); 
	CVariant value = CStringManager::GetFloatStringWithPrecision( fValue, nPrecision );
	CPCStringSliderEditor::SetValue( value );
	GetSlider()->SetPos( (int)( fValue * nPowerPrecision  + 0.5f ) );
}


void CPCFloatSliderEditor::GetValue( CVariant *pValue )
{
	if ( pValue )
	{
		CVariant value;
		CPCStringSliderEditor::GetValue( &value );
		float fValue = 0.0f;
		bool bWrongValue = true;
		if ( sscanf( value.GetStr(), "%g", &fValue ) == 1 )
		{
			if ( ( fValue <= ( ( 1.0f * GetSlider()->GetRangeMax() ) / ( 1.0f * nPowerPrecision ) ) ) &&
					 ( fValue >= ( ( 1.0f * GetSlider()->GetRangeMin() ) / ( 1.0f * nPowerPrecision ) ) ) )
			{
				bWrongValue = false;
			}
		}
		if ( bWrongValue )
		{
			CPCStringSliderEditor::SetDefaultValue();
			CPCStringSliderEditor::GetValue( &value );
			sscanf( value.GetStr(), "%g", &fValue );
		}
		fValue = ( 1.0f * CStringManager::NormalizeValue( (int)( fValue * nPowerPrecision + 0.5f ),
																											(int)( fStep * nPowerPrecision ) ) ) /
						 ( 1.0f * nPowerPrecision );
		*pValue = fValue;
	}
}


// CPCStringBrowseEditor

void CPCFloatSliderEditor::OnChangePos( int nPos )
{
	float fSliderPos = ( 1.0f * CStringManager::NormalizeValue( GetSlider()->GetPos(),
																															(int)( fStep * nPowerPrecision ) ) ) /
										 ( 1.0f * nPowerPrecision );
//	const std::string szFormat = fmt::format( "%.{}f", nPrecision );
//	SetWindowText( StrFmt( szFormat.c_str(), fSliderPos ) );
	SetWindowText( CStringManager::GetFloatStringWithPrecision(fSliderPos, nPrecision).c_str() );
}

// basement storage  


