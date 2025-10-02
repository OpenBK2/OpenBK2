#include "stdafx.h"
#include "../mapeditorlib/resourcedefines.h"
#include "../mapeditorlib/commandhandlerdefines.h"
#include "pc_constants.h"
#include "pc_dblinkdialog.h"

#include "PC_StringNewRefEditor.h"
#include "../Misc/StrProc.h"
#include "../MapEditorLib/StringManager.h"
#include "../libdb/ResourceManager.h"
#include "../MapEditorLib/Interface_Exporter.h"
#include "../MapEditorLib/CommonEditorMethods.h"
#include "../MapEditorLib/PCIEMnemonics.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CPCItemEditor

void CPCStringNewRefEditor::SetValue( const CVariant &rValue )
{
	if ( rValue.GetType() == CVariant::VT_NULL )
	{
		CVariant nulRefValue = string();
		CPCStringNewBrowseEditor::SetValue( nulRefValue );
	}
	else
	{
		CPCStringNewBrowseEditor::SetValue( rValue );
	}
}


void CPCStringNewRefEditor::GetValue( CVariant *pValue )
{
	if ( pValue )
	{
		CPCStringNewBrowseEditor::GetValue( pValue );
		if ( string( pValue->GetStr() ).empty() )
		{
			( *pValue ) = CVariant();
		}
	}
}


// CPCStringNewBrowseEditor

void CPCStringNewRefEditor::OnNew()
{
	if ( !GetPropertyDesc()->refTypes.empty() )
	{
		// Получаем списко возможных типов объекта
		CString strText;
		GetWindowText( strText );
		string szObjectName = strText;
		//
		string szDefaultObjectTypeName;
		CStringManager::GetTypeAndNameFromRefValue( &szDefaultObjectTypeName, 0, szObjectName, TYPE_SEPARATOR_CHAR, GetPropertyDesc()->refTypes.begin()->first );
		string szObjectTypeName;
		for ( SPropertyDesc::CTypesMap::const_iterator itType = GetPropertyDesc()->refTypes.begin();
					itType != GetPropertyDesc()->refTypes.end();
					++itType )
		{
			if ( szObjectTypeName.empty() )
			{
				szObjectTypeName = itType->first;	
			}
			else
			{
				if ( itType->first == szDefaultObjectTypeName )
				{
					szObjectTypeName = string( StrFmt( "%s%c", itType->first.c_str(), TYPE_SEPARATOR_CHAR ) ) + szObjectTypeName;	
				}
				else
				{
					szObjectTypeName += StrFmt( "%c%s", TYPE_SEPARATOR_CHAR, itType->first.c_str() );	
				}
			}
		}
		// получаем имя объекта
		//CString strNewName;
		//strNewName.LoadString( IDS_TREE_GDB_BROWSE_NEW_RESOURCE );
		//
		szObjectName = GetObjectSet().objectNameSet.begin()->first.ToString();
		string szObjectNamePrefix;
		CStringManager::SplitFileName( &szObjectNamePrefix, 0, 0, szObjectName );
		szObjectName = szObjectNamePrefix + GetName();
		CStringManager::ExtendFileExtention( &szObjectName, ".xdb" );
		//
		bool bCanChangeObjectName = true;
		bool bNeedEdit = true;
		bool bNeedExport = false;
		Singleton<IFolderCallback>()->ClearUndoData();
		if ( Singleton<IBuilderContainer>()->InsertObject( &szObjectTypeName,
																											 &szObjectName,
																											 false,
																											 &bCanChangeObjectName,
 																											 &bNeedExport,
																											 &bNeedEdit ) )
		{
			if ( bNeedExport )
			{
				Singleton<IExporterContainer>()->StartExport( szObjectTypeName, FORCE_EXPORT, START_EXPORT_TOOLS, EXPORT_REFERENCES );
				if ( CPtr<IManipulator> pObjectManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( szObjectTypeName, szObjectName ) )
				{
					bool bForceExport = true;
					Singleton<IExporterContainer>()->ExportObject( pObjectManipulator,
																												 szObjectTypeName,
																												 szObjectName,
 																												 bForceExport,
																												 EXPORT_REFERENCES );
				}
				Singleton<IExporterContainer>()->FinishExport( szObjectTypeName, FORCE_EXPORT, FINISH_EXPORT_TOOLS, EXPORT_REFERENCES );
			}
			Singleton<IFolderCallback>()->ClearUndoData();
			SetWindowTextByTypeAndName( szObjectTypeName, szObjectName );
			RedrawWindow();
		}
	}
}


void CPCStringNewRefEditor::OnBrowse()
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
			//
			if ( pcDBLinkDialog.DoModal() == IDOK )
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


void CPCStringNewRefEditor::SetWindowTextByTypeAndName( const string &szTableName, const string &szObjectName )
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


