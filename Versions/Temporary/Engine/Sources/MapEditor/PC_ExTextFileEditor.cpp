#include "stdafx.h"
#include "resourcedefines.h"
#include "commandhandlerdefines.h"
#include "pc_constants.h"
#include "pc_dblinkdialog.h"

#include "PC_ExTextFileEditor.h"
#include "..\Misc\StrProc.h"
#include "..\System\FileUtils.h"
#include "..\MapEditorLib\StringManager.h"
#include "../libdb/ResourceManager.h"
#include "..\MapEditorLib\Interface_Exporter.h"
#include "..\MapEditorLib\Interface_MainFrame.h"
#include "..\MapEditorLib\Interface_MOD.h"
#include "..\MapEditorLib\CommonEditorMethods.h"
#include "..\MapEditorLib\PCIEMnemonics.h"
#include "TextEditorDialog.h"
#include "Scripteditor.h"
#include "../System/Text.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CPCItemEditor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCExTextFileEditor::GetValue( CVariant *pValue )
{
	if ( pValue )
	{
		if ( const SPropertyDesc *pDesc = GetPropertyDesc() )
		{
			CPCString3ButtonEditor::GetValue( pValue );
			string szFilePath = pValue->GetStr();
			bool bResult = false;
			if ( !szFilePath.empty() )
			{
				SUserData::ENormalizePathType pathType = SUserData::NPT_UNKNOWN;
				if ( ( pDesc->nIntParam > SUserData::NPT_UNKNOWN ) && ( pDesc->nIntParam < SUserData::NPT_COUNT ) )
				{
					pathType = static_cast<SUserData::ENormalizePathType>( pDesc->nIntParam );
				}
				if ( ::IsValidFileName( szFilePath, false ) )
				{
					( *pValue ) = szFilePath;
					//
					// Устанавливаем каталог куда будем заглядывать при последующем вызове диалога открытия файла
					const string szFullFilePath = Singleton<IMODContainer>()->GetDataFolder( pathType ) + szFilePath;
					string szObjectNamePrefix;
					CStringManager::SplitFileName( &szObjectNamePrefix, 0, 0, szFullFilePath );
					string szMask;
					if ( !CStringManager::GetStringValueFromString( pDesc->szStringParam, PCSPL_MASK, 0, PCSP_MASK_DIVIDERS, "", &szMask ) || szMask.empty() )
					{
						szMask = "All Files (*.*)|*.*||";
					}
					//
					SUserData::CFilePathMap &rFilePathMap = Singleton<IUserDataContainer>()->Get()->filePathMap;
					rFilePathMap[szMask] = szObjectNamePrefix;
					bResult = true;
				}
			}
			else
			{
				bResult = true;
			}
			if ( !bResult )
			{
				SetDefaultValue();
				CPCString3ButtonEditor::GetValue( pValue );
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCExTextFileEditor::OnNew()
{
	if ( const SPropertyDesc *pDesc = GetPropertyDesc() )
	{
		string szValues = GetPropertyDesc()->szStringParam;
		NStr::ToLowerASCII( &szValues );
		string szEditor;
		if ( !CStringManager::GetStringValueFromString( szValues, PCSPL_EDITOR, 0, PCSP_DIVIDERS, "", &szEditor ) )
		{
			szEditor.clear();
		}
		const string szExtention = ( szEditor == "lua" ) ? ".lua" : ".txt";
		//
		string szFilePath = GetObjectSet().objectNameSet.begin()->first.ToString();
		string szObjectNamePrefix;
		CStringManager::SplitFileName( &szObjectNamePrefix, 0, 0, szFilePath );
		szFilePath = szObjectNamePrefix + GetName();
		CStringManager::ExtendFileExtention( &szFilePath, szExtention );
		//
		SBuildDataParams buildDataParams;
		buildDataParams.szObjectTypeName = "Text";
		CStringManager::SplitFileName( &( buildDataParams.szObjectNamePrefix ),
																	 &( buildDataParams.szObjectName ),
																	 &( buildDataParams.szObjectNameExtention ),
																	 szFilePath );
		buildDataParams.bNeedExport = false;
		buildDataParams.bNeedEdit = false;
		//
		if ( Singleton<IBuilderContainer>()->FillNewObjectName( &buildDataParams ) )
		{
			buildDataParams.GetObjectName( &szFilePath );
			{
				CFileStream stream( NVFS::GetMainFileCreator(), szFilePath );
			}
			SUserData::ENormalizePathType pathType = SUserData::NPT_UNKNOWN;
			if ( ( pDesc->nIntParam > SUserData::NPT_UNKNOWN ) && ( pDesc->nIntParam < SUserData::NPT_COUNT ) )
			{
				pathType = static_cast<SUserData::ENormalizePathType>( pDesc->nIntParam );
			}
			SetWindowText( szFilePath.c_str() );
			// Устанавливаем каталог куда будем заглядывать при последующем вызове диалога открытия файла
			const string szFullFilePath = Singleton<IMODContainer>()->GetDataFolder( pathType ) + szFilePath;
			string szObjectNamePrefix;
			CStringManager::SplitFileName( &szObjectNamePrefix, 0, 0, szFullFilePath );
			SUserData::CFilePathMap &rFilePathMap = Singleton<IUserDataContainer>()->Get()->filePathMap;
			string szMask;
			if ( !CStringManager::GetStringValueFromString( pDesc->szStringParam, PCSPL_MASK, 0, PCSP_MASK_DIVIDERS, "", &szMask ) || szMask.empty() )
			{
				szMask = "All Files (*.*)|*.*||";
			}
			rFilePathMap[szMask] = szObjectNamePrefix;
			//
			OnEdit();
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CPCString3ButtonEditor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPCExTextFileEditor::OnEdit()
{
	if ( const SPropertyDesc *pDesc = GetPropertyDesc() )
	{
		CVariant value;
		CPCString3ButtonEditor::GetValue( &value );
		string szFilePath = value.GetStr();
		bool bResult = false;
		if ( !szFilePath.empty() )
		{
			SUserData::ENormalizePathType pathType = SUserData::NPT_UNKNOWN;
			if ( ( pDesc->nIntParam > SUserData::NPT_UNKNOWN ) && ( pDesc->nIntParam < SUserData::NPT_COUNT ) )
			{
				pathType = static_cast<SUserData::ENormalizePathType>( pDesc->nIntParam );
			}
			if ( ::IsValidFileName( szFilePath, false ) )
			{
				string szText;
				bool bUnicode = true;
				File2String( &szText, &bUnicode, szFilePath, ::GetACP(), false );
				//
				string szValues = GetPropertyDesc()->szStringParam;
				NStr::ToLowerASCII( &szValues );
				//
				string szEditor;
				if ( !CStringManager::GetStringValueFromString( szValues, PCSPL_EDITOR, 0, PCSP_DIVIDERS, "", &szEditor ) )
				{
					szEditor.clear();
				}
				//
				string szNewText;
				bool bResult = false;
				//
				if ( szEditor == "lua" )
				{
					bUnicode = false;
					CScriptEditor scriptEditor( 0, GetTargetWindow() );
					//
					CString strTitle;
					strTitle.LoadString( IDS_PC_LUA_EDITOR_TITLE );
					scriptEditor.SetTitle( StrFmt( "%s - %s", szFilePath.c_str(), strTitle ) );
					//
					scriptEditor.SetText( szText );
					scriptEditor.EnableEdit( ( GetStyle() & ES_READONLY ) == 0 );
					if ( ( scriptEditor.DoModal() == IDOK ) && ( ( GetStyle() & ES_READONLY ) == 0 ) )
					{
						szNewText = scriptEditor.GetText();
						bResult = true;
					}
				}
				else
				{
					bUnicode = true;
					CTextEditorDialog textEditorDialog( GetTargetWindow() );
					//
					CString strTitle;
					strTitle.LoadString( IDS_PC_TXT_EDITOR_TITLE );
					textEditorDialog.SetTitle( StrFmt( "%s - %s", szFilePath.c_str(), strTitle ) );
					//
					textEditorDialog.SetType( typeTEMnemonics.Get( szEditor ) );
					textEditorDialog.SetText( szText );
					textEditorDialog.EnableEdit( ( GetStyle() & ES_READONLY ) == 0 );
					if ( ( textEditorDialog.DoModal() == IDOK ) && ( ( GetStyle() & ES_READONLY ) == 0 ) )
					{
						textEditorDialog.GetText( &szNewText );
						bResult = true;
					}
				}
				if ( bResult && ( szNewText != szText ) )
				{
					CString strMessagePattern;
					strMessagePattern.LoadString( IDS_CONFIRM_SAVE_MESSAGE_LONG );
					CString strMessage;
					strMessage.Format( strMessagePattern, szFilePath.c_str() );
					if ( ::MessageBox( Singleton<IMainFrameContainer>()->GetSECWorkbook()->GetSafeHwnd(), strMessage, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON2 ) == IDYES )
					{
						String2File( szNewText, bUnicode, szFilePath, ::GetACP(), false );
						NText::Reload( szFilePath );
					}
				}
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//"All supported Files (*.bzm; *.xml)|*.bzm; *.xml"
//"XML files (*.xml)|*.xml"
//"BZM files (*.bzm)|*.bzm"
//"All Files (*.*)|*.*"
void CPCExTextFileEditor::OnBrowse()
{
	if ( const SPropertyDesc *pDesc = GetPropertyDesc() )
	{
		CString strTitle;
		strTitle.LoadString( IDS_BROWSE_FOR_FILE_DIALOG_TITLE );
		string szTitle = StrFmt( strTitle, GetName() );
		//
		string szMask;
		if ( !CStringManager::GetStringValueFromString( pDesc->szStringParam, PCSPL_MASK, 0, PCSP_MASK_DIVIDERS, "", &szMask ) || szMask.empty() )
		{
			szMask = "All Files (*.*)|*.*||";
		}
		//
		SUserData::CFilePathMap &rFilePathMap = Singleton<IUserDataContainer>()->Get()->filePathMap;
		const string szInitialDir = rFilePathMap[szMask];
		//
		{
			NFile::CCurrDirHolder currDirHolder;
			CFileDialog fileDialog( true, "", "", OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, szMask.c_str(), GetTargetWindow() );
			
			fileDialog.m_ofn.lpstrFile = new char[0xFFFF];
			fileDialog.m_ofn.lpstrFile[0] = 0;			
			fileDialog.m_ofn.nMaxFile = 0xFFFF - 1;
			fileDialog.m_ofn.lpstrInitialDir = szInitialDir.c_str();
			fileDialog.m_ofn.lpstrTitle = szTitle.c_str();
			
			if ( ( fileDialog.DoModal() == IDOK ) && ( ( GetStyle() & ES_READONLY ) == 0 ) )
			{
				POSITION position = fileDialog.GetStartPosition();
				while ( position )
				{
					SUserData::ENormalizePathType pathType = SUserData::NPT_UNKNOWN;
					if ( ( pDesc->nIntParam > SUserData::NPT_UNKNOWN ) && ( pDesc->nIntParam < SUserData::NPT_COUNT ) )
					{
						pathType = static_cast<SUserData::ENormalizePathType>( pDesc->nIntParam );
					}
					const string szFullFilePath = fileDialog.GetNextPathName( position );
					const string szDataFolder = Singleton<IMODContainer>()->GetDataFolder( pathType );
					if ( CStringManager::Compare( szFullFilePath, szDataFolder, true, true, true ) == 0 )
					{
						string szFilePath = szFullFilePath.substr( szDataFolder.size() );
						SetWindowText( szFilePath.c_str() );
						//
						// Устанавливаем каталог куда будем заглядывать при последующем вызове диалога открытия файла
						string szObjectNamePrefix;
						CStringManager::SplitFileName( &szObjectNamePrefix, 0, 0, szFullFilePath );
						rFilePathMap[szMask] = szObjectNamePrefix;
					}
				}
			}
			delete[] fileDialog.m_ofn.lpstrFile;
			fileDialog.m_ofn.lpstrFile = 0;
		}
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CPCExTextFileEditor::GetPCItemStringValue( string *pszValue, const CVariant &rValue, const SPropertyDesc *pPropertyDesc )
{
	( *pszValue ) = rValue.GetStringRecode();
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CPCExTextFileEditor::GetPCItemValue( CVariant *pValue, const string &rszValue, const SPropertyDesc *pPropertyDesc )
{
	( *pValue ) = rszValue;
	return true;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// basement storage  
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
