#include "stdafx.h"
#include "MapEditorLib/Resources.h"
#include "MapEditorLib/MessageBoxes.h"
#include <fmt/format.h>
#include <fmt/printf.h>
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "ResourceDefines.h"
#include "PC_Constants.h"
#include "DBLinkView.h"
#include "FileDialogs.h"

#include "PropertyButtons.h"
#include "BitFieldView.h"
#include "PropertyValues.h"
#include "TextEditorView.h"

#include "Image/ImageColor.h"
#include "Misc/StrProc.h"
#include "System/FileUtils.h"
#include "System/Text.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/CommonEditorMethods.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/Interface_Exporter.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/Interface_MOD.h"
#include "MapEditorLib/Interface_UserData.h"
#include "MapEditorLib/PCIEMnemonics.h"
#include "MapEditorLib/StringManager.h"

#include "port/unicode.h"

#include <cstring>

// What the buttons beside a property's value do, moved here from the MFC item
// editors' OnBrowse, OnNew and OnEdit with their behaviour kept. Where the
// editors differed only in how they read or wrote their own edit box, that is
// now the caller's text in and text out. The colour picker is wx's
// (PropertyButtonsWx.cpp). The file and folder browsers are still MFC's and
// the shell's.

namespace
{
	std::string LoadResourceString( UINT nID )
	{
		return NResources::GetString( nID );
	}


	// Every editor ended its dialog with this, so the scene drops whatever input
	// state the click on the button started.
	void RemoveSceneInput()
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
	}


	// nIntParam, when it names one of the data folders.
	SUserData::ENormalizePathType GetPathType( const SPropertyDesc *pDesc )
	{
		if ( ( pDesc->nIntParam > SUserData::NPT_UNKNOWN ) && ( pDesc->nIntParam < SUserData::NPT_COUNT ) )
		{
			return static_cast<SUserData::ENormalizePathType>( pDesc->nIntParam );
		}
		return SUserData::NPT_UNKNOWN;
	}


	// The "editor:" parameter, lower case; empty when there is none.
	std::string GetEditorParameter( const SPropertyDesc *pDesc )
	{
		std::string szValues = pDesc->szStringParam;
		NStr::ToLowerASCII( &szValues );
		std::string szEditor;
		if ( !CStringManager::GetStringValueFromString( szValues, PCSPL_EDITOR, 0, PCSP_DIVIDERS, "", &szEditor ) )
		{
			szEditor.clear();
		}
		return szEditor;
	}


	// A file reference's whole string parameter is its mask; the text files
	// name theirs with "mask:". It is also the key the last folder is kept under.
	std::string GetFileMask( EPCIEType nType, const SPropertyDesc *pDesc )
	{
		std::string szMask;
		if ( nType == PCIE_STRING_FILE_REF )
		{
			szMask = pDesc->szStringParam;
		}
		else if ( !CStringManager::GetStringValueFromString( pDesc->szStringParam, PCSPL_MASK, 0, PCSP_MASK_DIVIDERS, "", &szMask ) )
		{
			szMask.clear();
		}
		if ( szMask.empty() )
		{
			szMask = "All Files (*.*)|*.*||";
		}
		return szMask;
	}


	// SetWindowTextByTypeAndName, of both reference editors: "Table:Object" for
	// a reference that may point into several tables, the object alone otherwise.
	std::string GetRefText( EPCIEType nType, const std::string &rszTableName, const std::string &rszObjectName )
	{
		if ( typePCIEMnemonics.IsMultiRef( nType ) )
		{
			if ( rszTableName.empty() && rszObjectName.empty() )
			{
				return std::string();
			}
			return fmt::format( "{}{:c}{}", rszTableName, TYPE_SEPARATOR_CHAR, rszObjectName );
		}
		return rszObjectName;
	}


	// CPCStringRefEditor::OnBrowse and CPCStringNewRefEditor::OnBrowse. They
	// differed in one line, the first telling the dialog whether it may edit;
	// neither box was ever read-only, so both asked for the same thing.
	bool BrowseRef( const NPropertyButton::SContext &rContext, const std::string &rszText, std::string *pszNewText )
	{
		const SPropertyDesc *const pDesc = rContext.pDesc;
		if ( pDesc->refTypes.empty() )
		{
			return false;
		}
		std::string szValues = pDesc->szStringParam;
		NStr::ToLowerASCII( &szValues );
		//
		const int	nWidth = CStringManager::GetIntValueFromString( szValues, PCSPL_WIDTH, 0, PCSP_DIVIDERS, 0 );
		const int	nHeight = CStringManager::GetIntValueFromString( szValues, PCSPL_HEIGHT, 0, PCSP_DIVIDERS, 0 );
		const bool bTextEditor = CStringManager::GetBoolValueFromString( szValues, PCSPL_EDITOR, 0, PCSP_DIVIDERS, false );
		const bool bMultiRef = typePCIEMnemonics.IsMultiRef( rContext.nType );
		//
		NDBLink::SRequest request;
		request.eType = NDBLink::TYPE_LINK;
		request.bMultiRef = bMultiRef;
		request.bTextEditor = bTextEditor;
		request.nFixedWidth = nWidth;
		request.nFixedHeight = nHeight;
		request.bEnableEdit = rContext.bEditable;
		request.selectedTables = pDesc->refTypes;
		//
		std::string szTableName;
		std::string szObjectName = rszText;
		if ( bMultiRef )
		{
			CStringManager::GetTypeAndNameFromRefValue( &szTableName, &szObjectName, rszText, TYPE_SEPARATOR_CHAR, pDesc->refTypes.begin()->first );
		}
		else
		{
			szTableName = pDesc->refTypes.begin()->first;
		}
		//
		// An empty box opens where the last pick for the same set of tables was.
		SUserData::CRefPathMap &rRefPathMap = Singleton<IUserDataContainer>()->Get()->refPathMap;
		std::string szRefKey;
		CreateRefKey( &szRefKey, pDesc );
		if ( szObjectName.empty() )
		{
			std::string szRefValue = rRefPathMap[szRefKey];
			std::string szLocalTableName;
			CStringManager::GetTypeAndNameFromRefValue( &szLocalTableName, &szObjectName, szRefValue, TYPE_SEPARATOR_CHAR, szTableName );
			if ( !szLocalTableName.empty() )
			{
				szTableName = szLocalTableName;
			}
		}
		//
		request.szTable = szTableName;
		request.szObject = szObjectName;
		//
		bool bResult = false;
		NDBLink::SResult result;
		if ( NDBLink::Run( rContext.pOwner, request, &result ) && rContext.bEditable )
		{
			szTableName = result.szTable;
			szObjectName = result.szObject;
			//
			std::string szRefValue;
			CStringManager::GetRefValueFromTypeAndName( &szRefValue, szTableName, szObjectName, TYPE_SEPARATOR_CHAR );
			rRefPathMap[szRefKey] = szRefValue;
			//
			if ( result.bEmpty )
			{
				szTableName.clear();
				szObjectName.clear();
			}
			( *pszNewText ) = GetRefText( rContext.nType, szTableName, szObjectName );
			bResult = true;
		}
		RemoveSceneInput();
		return bResult;
	}


	// CPCStringNewRefEditor::OnNew: a new object, of a type the reference may
	// point at, named after the first object shown and the property, made by
	// the builder -- which asks for the name and type -- and exported if it
	// says so.
	bool NewRef( const NPropertyButton::SContext &rContext, const std::string &rszText, std::string *pszNewText )
	{
		const SPropertyDesc *const pDesc = rContext.pDesc;
		// The editor took the first object without looking; an empty set is
		// refused here instead.
		if ( pDesc->refTypes.empty() || ( rContext.pObjectSet == 0 ) || rContext.pObjectSet->objectNameSet.empty() )
		{
			return false;
		}
		// The types the new object may have, the one the box names first.
		std::string szDefaultObjectTypeName;
		CStringManager::GetTypeAndNameFromRefValue( &szDefaultObjectTypeName, 0, rszText, TYPE_SEPARATOR_CHAR, pDesc->refTypes.begin()->first );
		std::string szObjectTypeName;
		for ( SPropertyDesc::CTypesMap::const_iterator itType = pDesc->refTypes.begin(); itType != pDesc->refTypes.end(); ++itType )
		{
			if ( szObjectTypeName.empty() )
			{
				szObjectTypeName = itType->first;
			}
			else
			{
				if ( itType->first == szDefaultObjectTypeName )
				{
					szObjectTypeName = std::string( fmt::format( "{}{:c}", itType->first, TYPE_SEPARATOR_CHAR ) ) + szObjectTypeName;
				}
				else
				{
					szObjectTypeName += fmt::format( "{:c}{}", TYPE_SEPARATOR_CHAR, itType->first );
				}
			}
		}
		//
		std::string szObjectName = rContext.pObjectSet->objectNameSet.begin()->first.ToString();
		std::string szObjectNamePrefix;
		CStringManager::SplitFileName( &szObjectNamePrefix, 0, 0, szObjectName );
		szObjectName = szObjectNamePrefix + rContext.szName;
		CStringManager::ExtendFileExtention( &szObjectName, ".xdb" );
		//
		bool bCanChangeObjectName = true;
		bool bNeedEdit = true;
		bool bNeedExport = false;
		Singleton<IFolderCallback>()->ClearUndoData();
		if ( !Singleton<IBuilderContainer>()->InsertObject( &szObjectTypeName,
																											 &szObjectName,
																											 false,
																											 &bCanChangeObjectName,
																											 &bNeedExport,
																											 &bNeedEdit ) )
		{
			return false;
		}
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
		( *pszNewText ) = GetRefText( rContext.nType, szObjectTypeName, szObjectName );
		return true;
	}


	// OnBrowse of the file reference and both text files: a file under the
	// property's data folder, as a path relative to it. A file anywhere else is
	// not taken. The folder it was picked in is where the next picker with the
	// same mask opens.
	bool BrowseFile( const NPropertyButton::SContext &rContext, std::string *pszNewText )
	{
		const SPropertyDesc *const pDesc = rContext.pDesc;
		const std::string szTitle = fmt::sprintf( LoadResourceString( IDS_BROWSE_FOR_FILE_DIALOG_TITLE ), rContext.szName );
		const std::string szMask = GetFileMask( rContext.nType, pDesc );
		//
		SUserData::CFilePathMap &rFilePathMap = Singleton<IUserDataContainer>()->Get()->filePathMap;
		const std::string szInitialDir = rFilePathMap[szMask];
		//
		bool bResult = false;
		{
			NFile::CCurrDirHolder currDirHolder;
			std::string szFullFilePath;
			if ( NFileDialog::OpenFile( rContext.pOwner, szTitle, szMask, szInitialDir, &szFullFilePath ) && rContext.bEditable )
			{
				const std::string szDataFolder = Singleton<IMODContainer>()->GetDataFolder( GetPathType( pDesc ) );
				if ( CStringManager::Compare( szFullFilePath, szDataFolder, true, true, true ) == 0 )
				{
					( *pszNewText ) = szFullFilePath.substr( szDataFolder.size() );
					std::string szObjectNamePrefix;
					CStringManager::SplitFileName( &szObjectNamePrefix, 0, 0, szFullFilePath );
					rFilePathMap[szMask] = szObjectNamePrefix;
					bResult = true;
				}
			}
		}
		RemoveSceneInput();
		return bResult;
	}


	// CPCStringDirRefEditor::OnBrowse: a folder under the property's data
	// folder, relative to it, with a trailing backslash.
	//
	// The picker used to open only when the remembered folder parsed as a
	// shell path, so with nothing remembered yet no picker appeared at all. It
	// always opens now, on the remembered folder when there is one.
	bool BrowseFolder( const NPropertyButton::SContext &rContext, std::string *pszNewText )
	{
		const SPropertyDesc *const pDesc = rContext.pDesc;
		SUserData::CFilePathMap &rFilePathMap = Singleton<IUserDataContainer>()->Get()->filePathMap;
		const std::string szInitialDir = rFilePathMap[NPropertyButton::PSZ_FOLDER_PATH_LABEL];
		const std::string szTitle = fmt::sprintf( LoadResourceString( IDS_BROWSE_FOR_FOLDER_DIALOG_TITLE ), rContext.szName );

		std::string szPath;
		if ( NFileDialog::ChooseFolder( rContext.pOwner, szTitle, szInitialDir, &szPath ) &&
				 ( !szPath.empty() ) && ( szPath[szPath.size() - 1] != '\\' ) )
		{
			szPath += "\\";
		}
		bool bResult = false;
		if ( rContext.bEditable && !szPath.empty() )
		{
			const std::string szFullPath = szPath;
			const std::string szDataFolder = Singleton<IMODContainer>()->GetDataFolder( GetPathType( pDesc ) );
			if ( CStringManager::Compare( szFullPath, szDataFolder, true, true, true ) == 0 )
			{
				( *pszNewText ) = szFullPath.substr( szDataFolder.size() );
				rFilePathMap[NPropertyButton::PSZ_FOLDER_PATH_LABEL] = szFullPath;
				bResult = true;
			}
		}
		RemoveSceneInput();
		return bResult;
	}


	// CPCTextFileEditor::OnNew and CPCExTextFileEditor::OnEdit, which were the
	// same code: the file the box names, in the Lua editor or the text editor,
	// and written back only if the user says yes to saving it.
	void EditTextFile( const NPropertyButton::SContext &rContext, const std::string &rszFilePath )
	{
		if ( rszFilePath.empty() || !::IsValidFileName( rszFilePath, false ) )
		{
			return;
		}
		std::string szText;
		bool bUnicode = true;
		File2String( &szText, &bUnicode, rszFilePath, false );
		//
		const std::string szEditor = GetEditorParameter( rContext.pDesc );
		std::string szNewText;
		bool bResult = false;
		if ( szEditor == "lua" )
		{
			bUnicode = false;
			bResult = NTextEditor::RunScript( rContext.pOwner, fmt::format( "{} - {}", rszFilePath, LoadResourceString( IDS_PC_LUA_EDITOR_TITLE ) ),
																				szText, rContext.bEditable, &szNewText );
		}
		else
		{
			bUnicode = true;
			bResult = NTextEditor::RunText( rContext.pOwner, fmt::format( "{} - {}", rszFilePath, LoadResourceString( IDS_PC_TXT_EDITOR_TITLE ) ),
																			szEditor, szText, rContext.bEditable, &szNewText );
		}
		if ( bResult && ( szNewText != szText ) )
		{
			std::string strMessagePattern = NResources::GetString( IDS_CONFIRM_SAVE_MESSAGE_LONG );
			const std::string strMessage = fmt::sprintf( strMessagePattern.c_str(), rszFilePath.c_str() );
			// Three-way as it was, though only Yes is acted on: No and Cancel both
			// leave the edited text unsaved.
			if ( NMessage::AskYesNoCancel( strMessage ) == NMessage::ANSWER_YES )
			{
				String2File( szNewText, bUnicode, rszFilePath, false );
				NText::Reload( rszFilePath );
			}
		}
		RemoveSceneInput();
	}


	// CPCExTextFileEditor::OnNew: an empty file beside the first object shown,
	// named after the property -- the builder may rename it -- and then opened
	// for editing.
	bool NewTextFile( const NPropertyButton::SContext &rContext, std::string *pszNewText )
	{
		const SPropertyDesc *const pDesc = rContext.pDesc;
		if ( ( rContext.pObjectSet == 0 ) || rContext.pObjectSet->objectNameSet.empty() )
		{
			return false;
		}
		const std::string szExtention = ( GetEditorParameter( pDesc ) == "lua" ) ? ".lua" : ".txt";
		//
		std::string szFilePath = rContext.pObjectSet->objectNameSet.begin()->first.ToString();
		std::string szObjectNamePrefix;
		CStringManager::SplitFileName( &szObjectNamePrefix, 0, 0, szFilePath );
		szFilePath = szObjectNamePrefix + rContext.szName;
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
		if ( !Singleton<IBuilderContainer>()->FillNewObjectName( &buildDataParams ) )
		{
			return false;
		}
		buildDataParams.GetObjectName( &szFilePath );
		{
			CFileStream stream( NVFS::GetMainFileCreator(), szFilePath );
		}
		const std::string szFullFilePath = Singleton<IMODContainer>()->GetDataFolder( GetPathType( pDesc ) ) + szFilePath;
		std::string szFolder;
		CStringManager::SplitFileName( &szFolder, 0, 0, szFullFilePath );
		Singleton<IUserDataContainer>()->Get()->filePathMap[GetFileMask( rContext.nType, pDesc )] = szFolder;
		//
		( *pszNewText ) = szFilePath;
		EditTextFile( rContext, szFilePath );
		return true;
	}


	// CPCStringBigInputEditor::OnBrowse: the text itself, in the Lua editor or
	// the text editor.
	bool EditBigString( const NPropertyButton::SContext &rContext, const std::string &rszText, std::string *pszNewText )
	{
		const std::string szEditor = GetEditorParameter( rContext.pDesc );
		if ( szEditor == "lua" )
		{
			return NTextEditor::RunScript( rContext.pOwner, std::string(), rszText, rContext.bEditable, pszNewText );
		}
		const bool bResult = NTextEditor::RunText( rContext.pOwner, std::string(), szEditor, rszText, rContext.bEditable, pszNewText );
		RemoveSceneInput();
		return bResult;
	}


	// CPCBinaryBitFieldEditor::OnBrowse: the box's hex as bytes, a check per
	// named bit, and the bytes as hex again.
	bool EditBitField( const NPropertyButton::SContext &rContext, const std::string &rszText, std::string *pszNewText )
	{
		const SPropertyDesc *const pDesc = rContext.pDesc;
		CVariant value;
		NPropertyValues::BitFieldValue( &value, rszText, pDesc );
		// OK writes the flags into the variant's own buffer.
		const bool bResult = NBitField::Run( rContext.pOwner,
																				 Singleton<IUserDataContainer>()->Get()->constUserData.szStartFolder + pDesc->szStringParam,
																				 const_cast<uint8_t*>( static_cast<const uint8_t*>( value.GetPtr() ) ), pDesc->nSize ) &&
												 rContext.bEditable;
		if ( bResult )
		{
			NPropertyValues::BitFieldString( pszNewText, value, pDesc );
		}
		RemoveSceneInput();
		return bResult;
	}


	// OnBrowse of the int colour and vec3 colour editors: the colour picker,
	// alpha kept as it was.
	bool PickColourValue( const NPropertyButton::SContext &rContext, const std::string &rszText, std::string *pszNewText )
	{
		CVariant value;
		if ( !GetPCItemValue( &value, rszText, CVariant(), rContext.nType, rContext.pDesc ) )
		{
			return false;
		}
		int nColor = (int)value;
		uint32_t nChosen = 0;
		const bool bResult = NPropertyButton::PickColour( rContext.pOwner, GetBGRColorFromARGBColor( nColor ), &nChosen ) &&
												 rContext.bEditable;
		if ( bResult )
		{
			UpdateARGBColorFromBGRColor( nChosen, &nColor );
			const CVariant colorValue = nColor;
			GetPCItemStringValue( pszNewText, colorValue, std::string(), rContext.nType, rContext.pDesc, false );
		}
		RemoveSceneInput();
		return bResult;
	}
}


namespace NPropertyButton
{
	void GetButtons( EPCIEType nType, std::vector<EButton> *pButtons )
	{
		if ( pButtons == 0 )
		{
			return;
		}
		pButtons->clear();
		switch ( nType )
		{
			case PCIE_INT_COLOR:
			case PCIE_INT_COLOR_WITH_ALPHA:
			case PCIE_VEC3_COLOR:
			case PCIE_STRING_REF:
			case PCIE_STRING_MULTI_REF:
			case PCIE_STRING_FILE_REF:
			case PCIE_STRING_DIR_REF:
			case PCIE_STRING_BIG_INPUT:
			case PCIE_BINARY_BIT_FIELD:
				pButtons->push_back( BUTTON_BROWSE );
				break;
			case PCIE_STRING_NEW_REF:
			case PCIE_STRING_NEW_MULTI_REF:
				pButtons->push_back( BUTTON_BROWSE );
				pButtons->push_back( BUTTON_NEW );
				break;
			// CPCTextFileEditor's second button was its "New" slot, captioned
			// "Edit", and edited.
			case PCIE_TEXT_FILE:
				pButtons->push_back( BUTTON_BROWSE );
				pButtons->push_back( BUTTON_EDIT );
				break;
			case PCIE_NEW_TEXT_FILE:
				pButtons->push_back( BUTTON_BROWSE );
				pButtons->push_back( BUTTON_NEW );
				pButtons->push_back( BUTTON_EDIT );
				break;
			default:
				break;
		}
	}


	std::string GetTitle( EButton eButton )
	{
		switch ( eButton )
		{
			case BUTTON_BROWSE:
				return LoadResourceString( IDS_BROWSE_BUTTON_TITLE );
			case BUTTON_NEW:
				return LoadResourceString( IDS_NEW_BUTTON_TITLE );
			case BUTTON_EDIT:
				return LoadResourceString( IDS_EDIT_BUTTON_TITLE );
			default:
				return std::string();
		}
	}


	bool Press( EButton eButton, const SContext &rContext, const std::string &rszText, std::string *pszNewText )
	{
		if ( ( rContext.pDesc == 0 ) || ( pszNewText == 0 ) )
		{
			return false;
		}
		switch ( eButton )
		{
			case BUTTON_BROWSE:
				switch ( rContext.nType )
				{
					case PCIE_STRING_REF:
					case PCIE_STRING_MULTI_REF:
					case PCIE_STRING_NEW_REF:
					case PCIE_STRING_NEW_MULTI_REF:
						return BrowseRef( rContext, rszText, pszNewText );
					case PCIE_STRING_FILE_REF:
					case PCIE_TEXT_FILE:
					case PCIE_NEW_TEXT_FILE:
						return BrowseFile( rContext, pszNewText );
					case PCIE_STRING_DIR_REF:
						return BrowseFolder( rContext, pszNewText );
					case PCIE_STRING_BIG_INPUT:
						return EditBigString( rContext, rszText, pszNewText );
					case PCIE_BINARY_BIT_FIELD:
						return EditBitField( rContext, rszText, pszNewText );
					case PCIE_INT_COLOR:
					case PCIE_INT_COLOR_WITH_ALPHA:
					case PCIE_VEC3_COLOR:
						return PickColourValue( rContext, rszText, pszNewText );
					default:
						return false;
				}
			case BUTTON_NEW:
				switch ( rContext.nType )
				{
					case PCIE_STRING_NEW_REF:
					case PCIE_STRING_NEW_MULTI_REF:
						return NewRef( rContext, rszText, pszNewText );
					case PCIE_NEW_TEXT_FILE:
						return NewTextFile( rContext, pszNewText );
					default:
						return false;
				}
			case BUTTON_EDIT:
				if ( ( rContext.nType == PCIE_TEXT_FILE ) || ( rContext.nType == PCIE_NEW_TEXT_FILE ) )
				{
					EditTextFile( rContext, rszText );
				}
				return false;
			default:
				return false;
		}
	}
}
