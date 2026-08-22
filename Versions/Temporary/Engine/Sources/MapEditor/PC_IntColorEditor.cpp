#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"

#include "Image/ImageColor.h"
#include "PC_IntColorEditor.h"
#include "MapEditorLib/Interface_UserData.h"

// CPCItemEditor

void CPCIntColorEditor::SetValue( const CVariant &rValue )
{
	const int nValue = (int)rValue;
	const int a = ( nValue >> 24 ) & 0xFF;
	const int r = ( nValue >> 16 ) & 0xFF;
	const int g = ( nValue >> 8 ) & 0xFF;
	const int b = nValue & 0xFF;
	CVariant colorValue;
	if ( GetItemEditorType() ==  PCIE_INT_COLOR_WITH_ALPHA )
	{
		colorValue = string( StrFmt( "%d, %d, %d, %d", a, r, g, b ) );
	}
	else
	{
		colorValue = string( StrFmt( "%d, %d, %d", r, g, b ) );
	}
	CPCStringBrowseEditor::SetValue( colorValue );
}


void CPCIntColorEditor::GetValue( CVariant *pValue )
{
	if ( pValue )
	{
		CPCStringBrowseEditor::GetValue( pValue );
		int a = 255;
		int r = 0;
		int g = 0;
		int b = 0;
		bool bWrongValue = true;
		CVariant stringValue;
		CPCStringBrowseEditor::GetValue( &stringValue );
		if ( GetItemEditorType() == PCIE_INT_COLOR_WITH_ALPHA )
		{
			if ( sscanf( stringValue.GetStr(), "%d,%d,%d,%d", &a, &r, &g, &b ) == 4 )
			{
				if ( ( ( a >= 0 ) && ( a < 256 ) ) &&
						( ( r >= 0 ) && ( r < 256 ) ) &&
						( ( g >= 0 ) && ( g < 256 ) ) &&
						( ( b >= 0 ) && ( b < 256 ) ) )
				{
					bWrongValue = false;
				}
			}
		}
		else
		{
			if ( sscanf( stringValue.GetStr(), "%d,%d,%d", &r, &g, &b ) == 3 )
			{
				if ( ( ( r >= 0 ) && ( r < 256 ) ) &&
						 ( ( g >= 0 ) && ( g < 256 ) ) &&
						 ( ( b >= 0 ) && ( b < 256 ) ) )
				{
					bWrongValue = false;
				}
			}
		}
		if ( bWrongValue )
		{
			CPCStringBrowseEditor::SetDefaultValue();
			CVariant stringValue;
			CPCStringBrowseEditor::GetValue( &stringValue );
			if ( GetItemEditorType() == PCIE_INT_COLOR_WITH_ALPHA )
			{
				sscanf( stringValue.GetStr(), "%d,%d,%d,%d", &a, &r, &g, &b );
			}
			else
			{
				sscanf( stringValue.GetStr(), "%d,%d,%d", &r, &g, &b );
			}
		}
		const CVariant colorValue = (int)( ( a << 24 ) + ( r << 16 ) + ( g << 8 ) + b );
		( *pValue ) = colorValue;
	}
}


// CPCStringBrowseEditor

void CPCIntColorEditor::OnBrowse()
{
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	if( pUserData )
	{
		CVariant colorValue;
		GetValue( &colorValue );
		COLORREF startColor = GetBGRColorFromARGBColor( (int)colorValue );
		CColorDialog colorDialog( startColor, CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT, GetTargetWindow() );
		pUserData->colorList.resize( 16, 0xFFffFFff );
		colorDialog.m_cc.lpCustColors = &( pUserData->colorList[0] );
		if ( ( colorDialog.DoModal() == IDOK ) && ( ( GetStyle() & ES_READONLY ) == 0 ) )
		{
			int nColor = (int)colorValue;
			UpdateARGBColorFromBGRColor( colorDialog.GetColor(), &nColor );
			const int a = ( nColor >> 24 ) & 0xFF;
			const int r = ( nColor >> 16 ) & 0xFF;
			const int g = ( nColor >> 8 ) & 0xFF;
			const int b = nColor & 0xFF;
			if ( GetItemEditorType() == PCIE_INT_COLOR_WITH_ALPHA )
			{
				SetWindowText( StrFmt( "%d, %d, %d, %d", a, r, g, b ) );
			}
			else
			{
				SetWindowText( StrFmt( "%d, %d, %d", r, g, b ) );
			}
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
	}
}

// basement storage  


