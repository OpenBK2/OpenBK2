#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "PC_Constants.h"


#include "PC_StringBigInputEditor.h"
#include "TextEditorView.h"
#include "Misc/StrProc.h"
#include "MapEditorLib/MfcWidget.h"
#include "MapEditorLib/StringManager.h"

// CPCItemEditor

bool CPCStringBigInputEditor::CreateEditor( const std::string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow )
{
	bool bResult = CPCStringBrowseEditor::CreateEditor( rszName, _nEditorType, _pPropertyDesc, _nControlID, rObjectSet, _pwndTargetWindow );
	if ( bResult )
	{
		SetMultiLine( true );
	}
	return bResult;
}


// CPCStringBrowseEditor

void CPCStringBigInputEditor::OnBrowse()
{
	CVariant value;
	CPCStringBrowseEditor::GetValue( &value );

	std::string szValues = GetPropertyDesc()->szStringParam;
	NStr::ToLowerASCII( &szValues );
	//
	std::string szEditor;
	if ( !CStringManager::GetStringValueFromString( szValues, PCSPL_EDITOR, 0,  PCSP_DIVIDERS, "", &szEditor ) )
	{
		szEditor.clear();
	}
	//
	if ( szEditor == "lua" )
	{
		// редактор LUA-скриптов
		CWndWidget ownerWidget( GetTargetWindow() );
		std::string szValue;
		if ( NTextEditor::RunScript( &ownerWidget, std::string(), value.GetStringRecode(),
																 ( GetStyle() & ES_READONLY ) == 0, &szValue ) )
		{
			SetWindowText( szValue.c_str() );
		}
	}
	else
	{
		CWndWidget ownerWidget( GetTargetWindow() );
		std::string szValue;
		if ( NTextEditor::RunText( &ownerWidget, std::string(), szEditor, value.GetStringRecode(),
															 ( GetStyle() & ES_READONLY ) == 0, &szValue ) )
		{
			SetWindowText( szValue.c_str() );
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
	}
}

// basement storage  


