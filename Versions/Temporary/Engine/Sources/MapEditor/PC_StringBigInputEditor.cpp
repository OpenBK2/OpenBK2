#include "stdafx.h"
#include "..\mapeditorlib\resourcedefines.h"
#include "..\mapeditorlib\commandhandlerdefines.h"
#include "pc_constants.h"


#include "PC_StringBigInputEditor.h"
#include "TextEditorDialog.h"
#include "..\Misc\StrProc.h"
#include "..\MapEditorLib\StringManager.h"
#include "Scripteditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CPCItemEditor

bool CPCStringBigInputEditor::CreateEditor( const string &rszName, EPCIEType _nEditorType, const SPropertyDesc* _pPropertyDesc, int _nControlID, const SObjectSet &rObjectSet, CWnd *_pwndTargetWindow )
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

	string szValues = GetPropertyDesc()->szStringParam;
	NStr::ToLowerASCII( &szValues );
	//
	string szEditor;
	if ( !CStringManager::GetStringValueFromString( szValues, PCSPL_EDITOR, 0,  PCSP_DIVIDERS, "", &szEditor ) )
	{
		szEditor.clear();
	}
	//
	if ( szEditor == "lua" )
	{
		// редактор LUA-скриптов
		CScriptEditor scriptEditor( 0, GetTargetWindow() );
		scriptEditor.SetText( value.GetStringRecode() );
		scriptEditor.EnableEdit( ( GetStyle() & ES_READONLY ) == 0 );
		if ( ( scriptEditor.DoModal() == IDOK ) && ( ( GetStyle() & ES_READONLY ) == 0 ) )
		{
			string szValue = scriptEditor.GetText();
			SetWindowText( szValue.c_str() );
		}
	}
	else
	{
		CTextEditorDialog textEditorDialog( GetTargetWindow() );
		textEditorDialog.SetType( typeTEMnemonics.Get( szEditor ) );
		textEditorDialog.SetText( value.GetStringRecode() );
		textEditorDialog.EnableEdit( ( GetStyle() & ES_READONLY ) == 0 );
		if ( ( textEditorDialog.DoModal() == IDOK ) && ( ( GetStyle() & ES_READONLY ) == 0 ) )
		{
			string szValue;
			textEditorDialog.GetText( &szValue );
			SetWindowText( szValue.c_str() );
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
	}
}

// basement storage  


