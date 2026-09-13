#include "stdafx.h"
#include <fmt/format.h>
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"

#include "Image/ImageColor.h"
#include "PC_Vec3ColorEditor.h"
#include "MapEditorLib/Interface_UserData.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/ObjectBaseController.h"

bool CPCVec3ColorEditor::GetColorValue( int *pnColor, IManipulator *pManipulator, const std::string &rszName )
{
	if ( pnColor && pManipulator )
	{
		bool bResult = true;
		float fR = 0.0f;
		float fG = 0.0f;
		float fB = 0.0f;
		//
		bResult = bResult && CManipulatorManager::GetValue( &fR, pManipulator, rszName + ".x" );
		bResult = bResult && CManipulatorManager::GetValue( &fG, pManipulator, rszName + ".y" );
		bResult = bResult && CManipulatorManager::GetValue( &fB, pManipulator, rszName + ".z" );
		//
		if ( bResult )
		{
			const int r = Clamp<int>( Clamp<float>( fR, 0.0f, 1.0f ) * 256.0f, 0, 255 );
			const int g = Clamp<int>( Clamp<float>( fG, 0.0f, 1.0f ) * 256.0f, 0, 255 );
			const int b = Clamp<int>( Clamp<float>( fB, 0.0f, 1.0f ) * 256.0f, 0, 255 );
			( *pnColor ) = (int)( ( 255 << 24 ) + ( r << 16 ) + ( g << 8 ) + b );
		}
		return bResult;
	}
	return false;
}


bool CPCVec3ColorEditor::AddChangeOperation( const std::string &rszName,const int nColor, CObjectBaseController *pObjectController, IManipulator *pManipulator )
{
	if ( pObjectController && pManipulator )
	{
		const int r = ( nColor >> 16 ) & 0xFF;
		const int g = ( nColor >> 8 ) & 0xFF;
		const int b = nColor & 0xFF;
		//
		bool bResult = true;
		//
		float fR = Clamp<float>( Clamp<int>( r, 0, 255 ) / 255.0f, 0.0f, 1.0f );
		float fG = Clamp<float>( Clamp<int>( g, 0, 255 ) / 255.0f, 0.0f, 1.0f );
		float fB = Clamp<float>( Clamp<int>( b, 0, 255 ) / 255.0f, 0.0f, 1.0f );
		//	
		bResult = bResult && pObjectController->AddChangeOperation( rszName + ".x", fR, pManipulator );
		bResult = bResult && pObjectController->AddChangeOperation( rszName + ".y", fG, pManipulator );
		bResult = bResult && pObjectController->AddChangeOperation( rszName + ".z", fB, pManipulator );
		//
		return bResult;
	}
	return false;
}


// CPCItemEditor

void CPCVec3ColorEditor::SetValue( const CVariant &rValue )
{
	const int nValue = (int)rValue;
	const int a = 255;
	const int r = ( nValue >> 16 ) & 0xFF;
	const int g = ( nValue >> 8 ) & 0xFF;
	const int b = nValue & 0xFF;
	CVariant colorValue = std::string( fmt::format( "{}, {}, {}", r, g, b ) );
	CPCStringBrowseEditor::SetValue( colorValue );
}


void CPCVec3ColorEditor::GetValue( CVariant *pValue )
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
		if ( sscanf( stringValue.GetStr(), "%d,%d,%d", &r, &g, &b ) == 3 )
		{
			if ( ( ( r >= 0 ) && ( r < 256 ) ) &&
					 ( ( g >= 0 ) && ( g < 256 ) ) &&
					 ( ( b >= 0 ) && ( b < 256 ) ) )
			{
				bWrongValue = false;
			}
		}
		if ( bWrongValue )
		{
			CPCStringBrowseEditor::SetDefaultValue();
			CVariant stringValue;
			CPCStringBrowseEditor::GetValue( &stringValue );
			sscanf( stringValue.GetStr(), "%d,%d,%d", &r, &g, &b );
		}
		const CVariant colorValue = (int)( ( a << 24 ) + ( r << 16 ) + ( g << 8 ) + b );
		( *pValue ) = colorValue;
	}
}


// CPCStringBrowseEditor

// The colour picker, in NPropertyButton. GetValue first, for what it does to
// the box: text that is not a colour is put back to the stored one, which is
// where the picker then starts.
void CPCVec3ColorEditor::OnBrowse()
{
	CVariant colorValue;
	GetValue( &colorValue );
	PressButton( NPropertyButton::BUTTON_BROWSE );
}

// basement storage  


