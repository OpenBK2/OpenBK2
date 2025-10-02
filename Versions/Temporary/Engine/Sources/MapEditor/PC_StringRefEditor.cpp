#include "stdafx.h"
#include "..\mapeditorlib\resourcedefines.h"
#include "..\mapeditorlib\commandhandlerdefines.h"
#include "pc_constants.h"
#include "pc_dblinkdialog.h"

#include "PC_StringRefEditor.h"
#include "..\Misc\StrProc.h"
#include "..\MapEditorLib\StringManager.h"
#include "..\MapEditorLib\PCIEMnemonics.h"
#include "..\MapEditorLib\CommonEditorMethods.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CPCItemEditor

void CPCStringRefEditor::SetValue( const CVariant &rValue )
{
	if ( rValue.GetType() == CVariant::VT_NULL )
	{
		CVariant nulRefValue = string();
		CPCStringBrowseEditor::SetValue( nulRefValue );
	}
	else
	{
		CPCStringBrowseEditor::SetValue( rValue );
	}
}


void CPCStringRefEditor::GetValue( CVariant *pValue )
{
	if ( pValue )
	{
		CPCStringBrowseEditor::GetValue( pValue );
		if ( pValue->GetStringRecode().empty() )
		{
			( *pValue ) = CVariant();
		}
	}
}


// CPCStringBrowseEditor

void CPCStringRefEditor::OnBrowse()
{
	if ( GetPropertyDesc() )
	{
		string szValues = GetPropertyDesc()->szStringParam;
		NStr::ToLowerASCII( &szValues );
		//
		const int	nWidth = CStringManager::GetIntValueFromString( szValues, PCSPL_WIDTH, 0, PCSP_DIVIDERS, 0 );
		const int	nHeight = CStringManager::GetIntValueFromString( szValues, PCSPL_HEIGHT, 0, PCSP_DIVIDERS, 0 );
		const bool bTextEditor = CStringManager::GetBoolValueFromString( szValues, PCSPL_EDITOR, 0, PCSP_DIVIDERS, false );
		//
		CPCDBLinkDialog pcDBLinkDialog( CPCDBLinkDialog::TYPE_LINK, typePCIEMnemonics.IsMultiRef( GetItemEditorType() ), bTextEditor, nWidth, nHeight, GetTargetWindow() );
		pcDBLinkDialog.SetSelectedTables( GetPropertyDesc()->refTypes );
		if ( !GetPropertyDesc()->refTypes.empty() )
		{
			CString strText;
			GetWindowText( strText );
			//
			string szTableName;
			string szObjectName = strText;
			if ( typePCIEMnemonics.IsMultiRef( GetItemEditorType() ) )
			{
				CStringManager::GetTypeAndNameFromRefValue( &szTableName, &szObjectName, szObjectName, TYPE_SEPARATOR_CHAR, GetPropertyDesc()->refTypes.begin()->first );
			}
			else
			{
				szTableName = GetPropertyDesc()->refTypes.begin()->first;
			}
			//
			SUserData::CRefPathMap &rRefPathMap = Singleton<IUserDataContainer>()->Get()->refPathMap;
			string szRefKey;
			CreateRefKey( &szRefKey, GetPropertyDesc() );
			//
			if ( szObjectName.empty() )
			{
				string szRefValue = rRefPathMap[szRefKey];
				string szLocalTableName;
				CStringManager::GetTypeAndNameFromRefValue( &szLocalTableName, &szObjectName, szRefValue, TYPE_SEPARATOR_CHAR, szTableName );
				if ( !szLocalTableName.empty() )
				{
					szTableName = szLocalTableName;
				}
			}
			//
			pcDBLinkDialog.SetCurrentTable( szTableName );
			pcDBLinkDialog.SetCurrentObject( szObjectName );
			pcDBLinkDialog.EnableEdit( ( GetStyle() & ES_READONLY ) == 0 );
			//
			if ( ( pcDBLinkDialog.DoModal() == IDOK ) && ( ( GetStyle() & ES_READONLY ) == 0 ) )
			{
				pcDBLinkDialog.GetCurrentTable( &szTableName );
				pcDBLinkDialog.GetCurrentObject( &szObjectName );
				//
				string szRefValue;
				CStringManager::GetRefValueFromTypeAndName( &szRefValue, szTableName, szObjectName, TYPE_SEPARATOR_CHAR );
				rRefPathMap[szRefKey] = szRefValue;
				//
				if ( pcDBLinkDialog.IsEmpty() )
				{
					szTableName.clear();
					szObjectName.clear();
				}
				SetWindowTextByTypeAndName( szTableName, szObjectName );
			}
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
		}
	}
}


void CPCStringRefEditor::SetWindowTextByTypeAndName( const string &szTableName, const string &szObjectName )
{
	if ( typePCIEMnemonics.IsMultiRef( GetItemEditorType() ) )
	{
		if ( szTableName.empty() && szObjectName.empty() )
		{
			SetWindowText( "" );
		}
		else
		{
			SetWindowText( StrFmt( "%s%c%s", szTableName.c_str(), TYPE_SEPARATOR_CHAR, szObjectName.c_str() ) );
		}
	}
	else
	{
		SetWindowText( szObjectName.c_str() );
	}
}

// basement storage  


