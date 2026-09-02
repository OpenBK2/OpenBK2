#include "stdafx.h"
#include <fmt/format.h>
#include "ResourceDefines.h"
#include "CommandHandlerDefines.h"
#include "PC_Constants.h"
#include "PC_DBLinkDialog.h"

#include "PC_TextFileEditor.h"
#include "Misc/StrProc.h"
#include "System/FileUtils.h"
#include "MapEditorLib/StringManager.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/Interface_Exporter.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/CommonEditorMethods.h"
#include "MapEditorLib/PCIEMnemonics.h"
#include "MapEditorLib/Interface_MOD.h"
#include "TextEditorDialog.h"
#include "ScriptEditor.h"
#include "System/Text.h"

// CPCItemEditor

void CPCTextFileEditor::GetValue( CVariant *pValue )
{
	if ( pValue )
	{
		if ( const SPropertyDesc *pDesc = GetPropertyDesc() )
		{
			CPCStringNewBrowseEditor::GetValue( pValue );
			std::string szFilePath = pValue->GetStr();
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
					const std::string szFullFilePath = Singleton<IMODContainer>()->GetDataFolder( pathType ) + szFilePath;
					std::string szObjectNamePrefix;
					CStringManager::SplitFileName( &szObjectNamePrefix, 0, 0, szFullFilePath );
					std::string szMask;
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
				CPCStringNewBrowseEditor::GetValue( pValue );
			}
		}
	}
}


// CPCStringNewBrowseEditor

void CPCTextFileEditor::OnNew()
{
	if ( const SPropertyDesc *pDesc = GetPropertyDesc() )
	{
		CVariant value;
		CPCStringNewBrowseEditor::GetValue( &value );
		std::string szFilePath = value.GetStr();
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
				std::string szText;
				bool bUnicode = true;
				File2String( &szText, &bUnicode, szFilePath, ::GetACP(), false );
				//
				std::string szValues = GetPropertyDesc()->szStringParam;
				NStr::ToLowerASCII( &szValues );
				//
				std::string szEditor;
				if ( !CStringManager::GetStringValueFromString( szValues, PCSPL_EDITOR, 0, PCSP_DIVIDERS, "", &szEditor ) )
				{
					szEditor.clear();
				}
				//
				std::string szNewText;
				bool bResult = false;
				//
				if ( szEditor == "lua" )
				{
					bUnicode = false;
					CScriptEditor scriptEditor( 0, GetTargetWindow() );
					//
					CString strTitle;
					strTitle.LoadString( IDS_PC_LUA_EDITOR_TITLE );
					scriptEditor.SetTitle( fmt::format( "{} - {}", szFilePath.c_str(), strTitle ) );
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
					textEditorDialog.SetTitle( fmt::format( "{} - {}", szFilePath.c_str(), strTitle ) );
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


//"All supported Files (*.bzm; *.xml)|*.bzm; *.xml"
//"XML files (*.xml)|*.xml"
//"BZM files (*.bzm)|*.bzm"
//"All Files (*.*)|*.*"
void CPCTextFileEditor::OnBrowse()
{
	if ( const SPropertyDesc *pDesc = GetPropertyDesc() )
	{
		CString strTitle;
		strTitle.LoadString( IDS_BROWSE_FOR_FILE_DIALOG_TITLE );
		std::string szTitle = StrFmt( strTitle, GetName() );
		//
		std::string szMask;
		if ( !CStringManager::GetStringValueFromString( pDesc->szStringParam, PCSPL_MASK, 0, PCSP_MASK_DIVIDERS, "", &szMask ) || szMask.empty() )
		{
			szMask = "All Files (*.*)|*.*||";
		}
		//
		SUserData::CFilePathMap &rFilePathMap = Singleton<IUserDataContainer>()->Get()->filePathMap;
		const std::string szInitialDir = rFilePathMap[szMask];
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
					const std::string szFullFilePath = fileDialog.GetNextPathName( position );
					const std::string szDataFolder = Singleton<IMODContainer>()->GetDataFolder( pathType );
					if ( CStringManager::Compare( szFullFilePath, szDataFolder, true, true, true ) == 0 )
					{
						std::string szFilePath = szFullFilePath.substr( szDataFolder.size() );
						SetWindowText( szFilePath.c_str() );
						//
						// Устанавливаем каталог куда будем заглядывать при последующем вызове диалога открытия файла
						std::string szObjectNamePrefix;
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


bool CPCTextFileEditor::GetPCItemStringValue( std::string *pszValue, const CVariant &rValue, const SPropertyDesc *pPropertyDesc )
{
	( *pszValue ) = rValue.GetStringRecode();
	return true;
}


bool CPCTextFileEditor::GetPCItemValue( CVariant *pValue, const std::string &rszValue, const SPropertyDesc *pPropertyDesc )
{
	( *pValue ) = rszValue;
	return true;
}

// basement storage  


