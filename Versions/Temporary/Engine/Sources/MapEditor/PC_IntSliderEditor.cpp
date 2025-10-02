#include "stdafx.h"

#include "pc_constants.h"
#include "..\Misc\StrProc.h"
#include "..\MapEditorLib\StringManager.h"
#include "PC_IntSliderEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CPCIntSliderEditor::CPCIntSliderEditor() : nStep( 1 )
{
}


BEGIN_MESSAGE_MAP(CPCIntSliderEditor, CPCStringSliderEditor)
END_MESSAGE_MAP()


void CPCIntSliderEditor::OnChangeEditBox()
{
	CVariant value;
	CPCStringSliderEditor::GetValue( &value );
	int nValue = 0;
	if ( sscanf( value.GetStr(), "%d", &nValue ) == 1 )
	{
		SetValueChanged();
		SetCreateControls( true );
		GetSlider()->SetPos( nValue );
		SetCreateControls( false );
	}
}


// CPCItemEditor

bool CPCIntSliderEditor::CreateEditor( const string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow )
{
	if ( CPCStringSliderEditor::CreateEditor( rszName, _nEditorType, _pPropertyDesc, _nControlID, rObjectSet, _pwndTargetWindow ) )
	{
		SetCreateControls( true );
		if ( GetPropertyDesc() )
		{
			string szValues = GetPropertyDesc()->szStringParam;
			NStr::ToLowerASCII( &szValues );
			//
			int nMin = 0;
			int nMax = 0;
			int nPage = 0;
			//
			nMin = CStringManager::GetIntValueFromString( szValues, PCSPL_MIN, 0, PCSP_DIVIDERS, nMin );
			nMax = CStringManager::GetIntValueFromString( szValues, PCSPL_MAX, 0, PCSP_DIVIDERS, nMax );
			nStep = CStringManager::GetIntValueFromString( szValues, PCSPL_STEP, 0, PCSP_DIVIDERS, nStep );
			nPage = CStringManager::GetIntValueFromString( szValues, PCSPL_PAGE, 0, PCSP_DIVIDERS, nPage );

			if ( nMin == nMax )
			{
				return false;
			}
			else if ( nMin > nMax )
			{
				const int nSwapValue = nMin;
				nMin = nMax;
				nMax = nSwapValue;
			}
			//
			if ( nStep == 0 )
			{
				nStep = 1;
			}
			else if ( nStep < 0 )
			{
				nStep *= (-1);
			}
			//
			if ( nPage == 0 )
			{
				nPage = ( nMax - nMin ) / 10;
			}
			else if ( nPage < 0 )
			{
				nPage *= (-1);
			}
			//
			GetSlider()->SetRange( nMin, nMax );
			GetSlider()->SetLineSize( nStep );
			GetSlider()->SetPageSize( nPage );
			GetSlider()->SetTicFreq( nPage );
			SetCreateControls( false );
			return true;
		}
	}
	return false;
}


void CPCIntSliderEditor::SetValue( const CVariant &rValue )
{
	const int nValue = CStringManager::NormalizeValue( (int)rValue, nStep );
	CVariant value = string( StrFmt( "%d", nValue ) ); 
	CPCStringSliderEditor::SetValue( value );
	GetSlider()->SetPos( nValue );
}


void CPCIntSliderEditor::GetValue( CVariant *pValue )
{
	if ( pValue )
	{
		CVariant value;
		CPCStringSliderEditor::GetValue( &value );
		int nValue = 0;
		bool bWrongValue = true;
		if ( sscanf( value.GetStr(), "%d", &nValue ) == 1 )
		{
			if ( ( nValue <= GetSlider()->GetRangeMax() ) &&
					 ( nValue >= GetSlider()->GetRangeMin() ) )
			{
				bWrongValue = false;
			}
		}
		if ( bWrongValue )
		{
			CPCStringSliderEditor::SetDefaultValue();
			CPCStringSliderEditor::GetValue( &value );
			sscanf( value.GetStr(), "%d", &nValue );
		}
		nValue = CStringManager::NormalizeValue( nValue, nStep );
		*pValue = nValue;
	}
}


// CPCStringBrowseEditor

void CPCIntSliderEditor::OnChangePos( int nPos )
{
	int nSliderPos = CStringManager::NormalizeValue( GetSlider()->GetPos(), nStep );
	SetWindowText( StrFmt( "%d", nSliderPos ) );
}

// basement storage  


