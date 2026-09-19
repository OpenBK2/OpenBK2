#include "stdafx.h"

#include <fmt/format.h>
#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/Interface_Progress.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/MfcWidget.h"
#include "MapEditorLib/ResourceDefines.h"
#include "ResourceDefines.h"
#include "PC_Constants.h"

#include "System/FileUtils.h"
#include <HtmlHelp.h>

#include "libdb/ResourceManager.h"
#include "MainFrameShared.h"
#include "AboutView.h"
#include "MapEditorLib/Interface_Editor.h"
#include "Misc/StrProc.h"
#include "MapEditorLib/StringManager.h"
#include "DBLinkView.h"
#include "FileDialogs.h"
#include "MapEditorLib/CommonEditorMethods.h"
#include "MapEditorLib/Tools_HashSet.h"
#include "MapEditorLib/Tools_Resources.h"
#include "Main/MODs.h"
#include "Main/MainLoop.h"

#include "libdb/Db.h"

#include <filesystem>
#include <atlbase.h>
#include <msxml6.h>
#include "libdb/EditorDb.h"
#include "libdb/TypeDef.h"
#include "MapEditorLib/Interface_MOD.h"
#include "System/VFS.h"

// Moved out of MainFrame.cpp unchanged but for two things: a message box or a
// dialog is owned by MainFrameWnd() where CMainFrame passed itself, which is the
// same window there and the wx frame's window in the wx frame; and the frame's
// own ReloadData is passed in rather than called.

namespace NMainFrameShared
{
	bool UpdateTitle( SSWTParams *pCurrent, const SSWTParams &rSWTParams, std::string *pszTitle )
	{
		SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
		SSWTParams &currentSWTParams = *pCurrent;
		//
		const int MAX_NAME_SIZE = 133;
		//
		bool bModified = false;
		if ( rSWTParams.dwFlags & SWT_MOD )
		{
			std::string szMOD = rSWTParams.szMOD;
			if ( rSWTParams.bFillMODFromBase )
			{
				NMOD::SMOD mod;
				NMOD::GetAttachedMOD( &mod );
				if ( mod.wszName.empty() )
				{
					szMOD.clear();
				}
				else
				{
					CString strMOD;
					Unicode2MBSC( &strMOD, mod.wszName, ::GetACP() );
					szMOD = strMOD;
				}
			}
			if ( currentSWTParams.szMOD != szMOD )
			{
				currentSWTParams.szMOD = szMOD;
				bModified = true;
			}
		}
		if ( rSWTParams.dwFlags & SWT_TYPE )
		{
			if ( currentSWTParams.szType.empty() )
			{
				currentSWTParams.szType = pUserData->constUserData.szMainObjectType;
			}
			if ( currentSWTParams.szType != rSWTParams.szType )
			{
				currentSWTParams.szType = rSWTParams.szType;
				bModified = true;
			}
			if ( currentSWTParams.szType == pUserData->constUserData.szMainObjectType )
			{
				currentSWTParams.szType.clear();
			}
		}
		if ( rSWTParams.dwFlags & SWT_OBJECT )
		{
			if ( currentSWTParams.szObject != rSWTParams.szObject )
			{
				currentSWTParams.szObject = rSWTParams.szObject;
				bModified = true;
			}
		}
		if ( rSWTParams.dwFlags & SWT_PARAMS )
		{
			if ( currentSWTParams.szParams != rSWTParams.szParams )
			{
				currentSWTParams.szParams = rSWTParams.szParams;
				bModified = true;
			}
		}
		if ( rSWTParams.dwFlags & SWT_MODIFIED )
		{
			if ( currentSWTParams.bModified != rSWTParams.bModified )
			{
				currentSWTParams.bModified = rSWTParams.bModified;
				bModified = true;
			}
		}
		if ( !bModified )
		{
			return false;
		}
		std::string szFilePath;
		std::string szFileName;
		std::string szFileExtention;
		CStringManager::SplitFileName( &szFilePath, &szFileName, &szFileExtention, currentSWTParams.szObject );
		//
		std::string szTitle = szFilePath + szFileName;
		std::string szExtention = szFileExtention;
		//
		if ( !currentSWTParams.szType.empty() )
		{
			szTitle = currentSWTParams.szType + ":" + szTitle;
		}
		if ( currentSWTParams.bModified )
		{
			szExtention += "*";
		}
		if ( !currentSWTParams.szParams.empty() )
		{
			szExtention += " " + currentSWTParams.szParams;
		}
		if ( !currentSWTParams.szMOD.empty() )
		{
			szExtention += " MOD: " + currentSWTParams.szMOD;
		}
		//
		if ( ( szTitle.size() + szExtention.size() ) > MAX_NAME_SIZE )
		{
			if ( ( MAX_NAME_SIZE - szExtention.size() - 4 ) >= 0 )
			{
				szTitle = szTitle.substr( 0, MAX_NAME_SIZE - szExtention.size() - 4 ) + "..." + szTitle.substr( szTitle.size() - 1 );
			}
		}
		if ( szTitle.empty() && szExtention.empty() )
		{
			szTitle = pUserData->constUserData.szApplicationTitle;
		}
		else
		{
			szTitle = pUserData->constUserData.szApplicationTitle + " - [" + szTitle + szExtention + "]";
		}
		( *pszTitle ) = szTitle;
		return true;
	}


	bool SaveChanges( bool bShowConfirmDialog, const std::function<void()> &rReloadData )
	{
		if ( IEditorContainer *pEditorContainer = Singleton<IEditorContainer>() )
		{
			bool bModified = ( Singleton<IEditorContainer>()->IsModified() || Singleton<IResourceManager>()->CanSyncDB() );
			bShowConfirmDialog = bShowConfirmDialog && bModified;
			bool bConfirmed = true;
			if ( bShowConfirmDialog	)
			{
				if ( pEditorContainer->GetActiveEditor() )
				{
					SObjectSet objectSet;
					pEditorContainer->GetActiveEditor()->GetView()->GetObjectSet( &objectSet );
					//
					CString strMessagePattern;
					strMessagePattern.LoadString( IDS_CONFIRM_SAVE_MESSAGE_LONG );
					std::string szName;
					{
						CStringManager::GetRefValueFromTypeAndName( &szName, objectSet.szObjectTypeName, objectSet.objectNameSet.begin()->first.ToString(), TYPE_SEPARATOR_CHAR );
					}
					CString strMessage;
					strMessage.Format( strMessagePattern, szName.c_str() );
					const int nButtonPressed = ::MessageBox( MainFrameWnd()->GetSafeHwnd(), strMessage, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON2 );
					if ( nButtonPressed == IDCANCEL )
					{
						return false;
					}
					bConfirmed = ( nButtonPressed == IDYES );
				}
				else
				{
					CString strMessagePattern;
					strMessagePattern.LoadString( IDS_CONFIRM_SAVE_MESSAGE_SHORT );
					const int nButtonPressed = ::MessageBox( MainFrameWnd()->GetSafeHwnd(), strMessagePattern, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON2 );
					if ( nButtonPressed == IDCANCEL )
					{
						return false;
					}
					bConfirmed = ( nButtonPressed == IDYES );
				}
			}
			//
			if ( bModified && bConfirmed )
			{
				NProgress::Create( true );
				CString strPM;
				strPM.LoadString( IDS_PM_SAVE );
				NProgress::SetMessage( std::string( strPM ) );
				NProgress::SetRange( 0, pEditorContainer->GetActiveEditor() ? 2 : 1 );
			}
			CWaitCursor waitCursor;
			if ( pEditorContainer->GetActiveEditor() )
			{
				SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
				SObjectSet objectSet;
				pEditorContainer->GetActiveEditor()->GetView()->GetObjectSet( &objectSet );
				const bool bMainObject = ( objectSet.szObjectTypeName == pUserData->constUserData.szMainObjectType );
				std::string szName;
				CStringManager::CreateRecentListName( &szName, objectSet, bMainObject );
				CStringManager::AddToRecentList( szName, bMainObject );
			}
			//
			if ( bModified && bConfirmed )
			{
				if ( pEditorContainer->GetActiveEditor() )
				{
					pEditorContainer->Save( true );
					NProgress::SetPosition( 1 );
				}
				Singleton<IResourceManager>()->SyncDB();
			}
			else if ( bModified && !bConfirmed )
			{
				if ( pEditorContainer->GetActiveEditor() )
				{
					pEditorContainer->Save( false );
					NProgress::SetPosition( 2 );
				}
				Singleton<IResourceManager>()->ResetCache();
				rReloadData();
			}
			if ( bModified && bConfirmed )
			{
				NProgress::Destroy();
			}
			//
			{
				bModified = ( Singleton<IEditorContainer>()->IsModified() || Singleton<IResourceManager>()->CanSyncDB() );
				//
				SSWTParams swtParams;
				swtParams.dwFlags = SWT_MODIFIED;
				swtParams.bModified = bModified;
				Singleton<IMainFrameContainer>()->Get()->SetWindowTitle( swtParams );
			}
		}
		return true;
	}


	// CMainFrame's version also sent WM_TREE_GDB_BROWSER to the frame's parent
	// window. A main frame has none, so that never happened, and it is left out.
	void OpenResource( const std::string &rszResourceName )
	{
		if ( SUserData *pUserData = Singleton<IUserDataContainer>()->Get() )
		{
			std::string szResourceName = rszResourceName;
			NStr::TrimBoth( szResourceName, '"' );
			if ( pUserData->NormalizePath( &szResourceName, true, true, false, SUserData::NPT_DATA_STORAGE, 0 ) )
			{
				std::string szExtention;
				CStringManager::SplitFileName( 0, 0, &szExtention, szResourceName );
				NStr::ToLower( &szExtention );
				if ( szExtention == ".xdb" )
				{
					if ( IResourceManager *pResourceManager = Singleton<IResourceManager>() )
					{
						CDBID resourceDBID = CDBID( szResourceName );
						SObjectSet objectSet;
						objectSet.szObjectTypeName = NDb::GetClassTypeName( resourceDBID );
						InsertHashSetElement( &( objectSet.objectNameSet ), resourceDBID );
						if ( CPtr<IManipulator> pObjectManipulator = CManipulatorManager::CreateObectSetManipulator( objectSet ) )
						{
							const bool bMainObject = ( objectSet.szObjectTypeName == pUserData->constUserData.szMainObjectType );
							std::string szName;
							CStringManager::CreateRecentListName( &szName, objectSet, bMainObject );
							CStringManager::AddToRecentList( szName, bMainObject );
							//
							IView *pView = 0;
							Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_GET_VIEW, reinterpret_cast<uintptr_t>( &pView ) );
							if ( pView != 0 )
							{
								pView->SetViewManipulator( pObjectManipulator, objectSet, std::string() );
								Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_CREATE_TREE, 0 );
							}
							if ( CPtr<IManipulator> pEditorManipulator = pResourceManager->CreateObjectManipulator( objectSet.szObjectTypeName, objectSet.objectNameSet.begin()->first ) )
							{
								Singleton<IEditorContainer>()->Create( pEditorManipulator, objectSet );
							}
						}
					}
				}
			}
		}
	}


	bool BrowseLink( std::string *pszResult, const std::string &rszInitialValue, const SPropertyDesc* pPropertyDesc, bool bMultiRef, bool bEnableEdit )
	{
		NI_ASSERT( pPropertyDesc != 0, "CMainFrame::CreateToolBar() pPropertyDesc == 0" );
		//
		std::string szValues = pPropertyDesc->szStringParam;
		NStr::ToLowerASCII( &szValues );
		//
		const int	nWidth = CStringManager::GetIntValueFromString( szValues, PCSPL_WIDTH, 0, PCSP_DIVIDERS, 0 );
		const int	nHeight = CStringManager::GetIntValueFromString( szValues, PCSPL_HEIGHT, 0, PCSP_DIVIDERS, 0 );
		const bool bTextEditor = CStringManager::GetBoolValueFromString( szValues, PCSPL_EDITOR, 0, PCSP_DIVIDERS, false );
		//
		NDBLink::SRequest request;
		request.eType = NDBLink::TYPE_LINK;
		request.bMultiRef = bMultiRef;
		request.bTextEditor = bTextEditor;
		request.nFixedWidth = nWidth;
		request.nFixedHeight = nHeight;
		request.bEnableEdit = bEnableEdit;
		request.selectedTables = pPropertyDesc->refTypes;
		if ( !pPropertyDesc->refTypes.empty() )
		{
			std::string szTableName;
			std::string szObjectName;
			const int nPos = rszInitialValue.find( TYPE_SEPARATOR_CHAR );
			if ( nPos >= 0 )
			{
				szTableName = rszInitialValue.substr( 0, nPos );
				szObjectName = rszInitialValue.substr( nPos + 1 );
			}
			else
			{
				szTableName = pPropertyDesc->refTypes.begin()->first;
				szObjectName = rszInitialValue;
			}
			//
			SUserData::CRefPathMap &rRefPathMap = Singleton<IUserDataContainer>()->Get()->refPathMap;
			std::string szRefKey;
			CreateRefKey( &szRefKey, pPropertyDesc );
			//
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
			NDBLink::SResult result;
			if ( NDBLink::Run( Singleton<IMainFrameContainer>()->GetMainWindow(), request, &result ) && bEnableEdit && ( pszResult != 0 ) )
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
					pszResult->clear();
				}
				else
				{
					( *pszResult ) = szRefValue;
				}
				return true;
			}
		}
		return false;
	}


	bool BrowseForObject( CDBID *pObjectDBID, std::string *pszObjectTypeName, bool bEnableEdit, bool bEnableEmpty )
	{
		const std::string szObjectTypeName = ( pszObjectTypeName != 0 ) ? ( *pszObjectTypeName ) : std::string();
		//
		IResourceManager *pResourceManager = Singleton<IResourceManager>();
		IEditorContainer *pEditorContainer = Singleton<IEditorContainer>();
		SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
		//
		SUserData::CRefPathMap &rRefPathMap = pUserData->refPathMap;
		const std::string szRefKey = fmt::format( "_OPEN:{}", szObjectTypeName.c_str() );
		//
		NDBLink::SRequest request;
		request.eType = bEnableEmpty ? NDBLink::TYPE_LINK : NDBLink::TYPE_OPEN;
		CTableSet tableSet;
		if ( szObjectTypeName.empty() )
		{
			if ( CPtr<IManipulator> pTableManipulator = pResourceManager->CreateTableManipulator() )
			{
				if ( CPtr<IManipulatorIterator> pTableManipulatorIterator = pTableManipulator->Iterate( true, ECT_NO_CACHE ) )
				{
					std::string szName;
					while ( !pTableManipulatorIterator->IsEnd() )
					{
						pTableManipulatorIterator->GetName( &szName );
						if ( pEditorContainer->CanCreate( szName ) )
						{
							InsertHashSetElement( &tableSet, szName );
						}
						pTableManipulatorIterator->Next();
					}
				}
			}
		}
		else
		{
			InsertHashSetElement( &tableSet, szObjectTypeName );
		}
		request.selectedTables = tableSet;
		request.szTable = szObjectTypeName;
		{
			std::string szRefValue = rRefPathMap[szRefKey];
			std::string szTableName;
			std::string szObjectName;
			CStringManager::GetTypeAndNameFromRefValue( &szTableName, &szObjectName, szRefValue, TYPE_SEPARATOR_CHAR, szTableName );
			if ( !szTableName.empty() )
			{
				request.szTable = szTableName;
			}
			if ( !szObjectName.empty() )
			{
				request.szObject = szObjectName;
			}
		}
		request.bEnableEdit = bEnableEdit;
		NDBLink::SResult result;
		const bool bResult = NDBLink::Run( Singleton<IMainFrameContainer>()->GetMainWindow(), request, &result );
		if ( bResult )
		{
			const std::string szTableName = result.szTable;
			const std::string szObjectName = result.szObject;
			if ( !szObjectName.empty() )
			{
				std::string szRefValue;
				CStringManager::GetRefValueFromTypeAndName( &szRefValue, szTableName, szObjectName, TYPE_SEPARATOR_CHAR );
				rRefPathMap[szRefKey] = szRefValue;
			}
			if ( pObjectDBID != 0 )
			{
				if ( !result.bEmpty )
				{
					( *pObjectDBID ) = CDBID( szObjectName );
				}
				else
				{
					pObjectDBID->Clear();
				}
			}
			if ( pszObjectTypeName != 0 )
			{
				( *pszObjectTypeName ) = szTableName;
			}
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
		return bResult;
	}


	namespace
	{
	// The old lightweight XML readers stop at the root header and do not report
	// malformed documents reliably. Validate the entire selected file with MSXML
	// before letting the database add its header to index.bin.
	bool ValidateXDB( const std::string &dbPath, std::string *pTypeName, std::string *pError )
	{
		struct CComScope
		{
			HRESULT result = ::CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED );
			~CComScope() { if ( SUCCEEDED(result) ) ::CoUninitialize(); }
		} com;
		if ( FAILED(com.result) && com.result != RPC_E_CHANGED_MODE )
		{
			*pError = "Cannot initialize XML validation.";
			return false;
		}
		ATL::CComPtr<IXMLDOMDocument2> document;
		if ( FAILED(document.CoCreateInstance(__uuidof(DOMDocument60))) )
		{
			*pError = "Cannot create the MSXML 6 XML validator.";
			return false;
		}
		document->put_async( VARIANT_FALSE );
		document->put_validateOnParse( VARIANT_FALSE );
		document->put_resolveExternals( VARIANT_FALSE );
		document->setProperty( ATL::CComBSTR(L"ProhibitDTD"), ATL::CComVariant(true) );
		// Validate the bytes the engine will read, including the mounted mod's
		// precedence. XDB strings are UTF-8; MSXML alone would also accept UTF-16.
		CFileStream stream( NVFS::GetMainVFS(), dbPath );
		if ( !stream.IsOk() || stream.GetSize() == 0 )
		{
			*pError = "The XDB is empty or cannot be read from the current database.";
			return false;
		}
		const char *data = reinterpret_cast<const char*>(stream.GetBuffer());
		int size = stream.GetSize();
		// loadXML takes Unicode text, so the byte-order mark must be removed before conversion.
		if ( size >= 3 && memcmp(data, "\xef\xbb\xbf", 3) == 0 )
		{
			data += 3;
			size -= 3;
		}
		const int length = ::MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, data, size, nullptr, 0 );
		if ( length == 0 || memchr(data, 0, size) != nullptr )
		{
			*pError = "XDB files must use UTF-8 text encoding.";
			return false;
		}
		ATL::CComBSTR xml( length );
		::MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, data, size, xml.m_str, length );
		VARIANT_BOOL loaded = VARIANT_FALSE;
		const HRESULT result = document->loadXML( xml, &loaded );
		if ( FAILED(result) || loaded != VARIANT_TRUE )
		{
			ATL::CComPtr<IXMLDOMParseError> error;
			ATL::CComBSTR reason;
			long line = 0;
			document->get_parseError( &error );
			if ( error )
			{
				error->get_reason( &reason );
				error->get_line( &line );
			}
			*pError = fmt::format("Invalid XDB/XML at line {}: {}", line,
				reason ? NStr::ToMBCS(std::wstring(reason, reason.Length())) : "Unable to read the document.");
			return false;
		}
		ATL::CComPtr<IXMLDOMElement> root;
		ATL::CComBSTR name;
		if ( FAILED(document->get_documentElement(&root)) || !root || FAILED(root->get_tagName(&name)) || !name )
		{
			*pError = "The XDB has no resource root element.";
			return false;
		}
		*pTypeName = NStr::ToMBCS( std::wstring(name, name.Length()) );
		std::vector<NDb::NTypeDef::STypeClass*> types;
		NDb::GetClassesList( &types );
		for ( const auto *pType : types )
		{
			if ( pType && pType->nClassTypeID != -1 && pType->szTypeName == *pTypeName &&
					 NObjectFactory::IsRegistered(pType->nClassTypeID) )
				return true;
		}
		*pError = "Unknown game resource type <" + *pTypeName + ">. The file was not registered.";
		return false;
	}
	}


	void RegisterXDB( const std::function<void()> &rReloadData )
	{
		const HWND hwndOwner = MainFrameWnd()->GetSafeHwnd();
		const auto ReportError = [hwndOwner]( const std::string &message ) {
			::MessageBox( hwndOwner, message.c_str(), "Register XDB", MB_OK | MB_ICONERROR );
		};
		// Attaching a mod changes the writable folder, but the base Data folder is
		// still mounted. Accept both roots and use the same base path as MODs.cpp.
		const std::string baseDataFolder = NFile::JoinPath( NMainLoop::GetBaseDir(), NFile::DIR_DATA );
		std::vector<std::filesystem::path> dataRoots;
		for ( const std::string &folder : {
			Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_DATA_STORAGE ), baseDataFolder } )
		{
			std::error_code error;
			auto rootPath = std::filesystem::canonical( std::filesystem::u8path(folder), error );
			if ( error )
			{
				ReportError( "Cannot open the game/mod data folder:\n" + folder );
				return;
			}
			dataRoots.push_back( rootPath.make_preferred() );
		}
		// Give the shell a canonical Windows path, without the mixed trailing
		// separators used internally by the VFS. Always start in the game's Data.
		const std::string initialFolder = dataRoots.back().string();
		std::string szChosen;
		if ( !NFileDialog::OpenFile( Singleton<IMainFrameContainer>()->GetMainWindow(), "Register XDB in the current game or mod database",
																 "Game database files (*.xdb)|*.xdb||", initialFolder, &szChosen ) )
			return;
		std::error_code error;
		const auto filePath = std::filesystem::canonical( std::filesystem::u8path( szChosen ), error );
		if ( error || _wcsicmp(filePath.extension().c_str(), L".xdb") != 0 )
		{
			ReportError( "Select an existing .xdb file." );
			return;
		}
		std::string dbPath;
		const std::wstring fullPath = filePath.wstring();
		// Try the active mod first; the resulting DBID is relative to its mounted
		// root, never prefixed with Data/ or Mods/<name>/. Compare whole directories.
		for ( const auto &rootPath : dataRoots )
		{
			std::wstring rootPrefix = rootPath.wstring();
			if ( rootPrefix.back() != L'\\' )
				rootPrefix += L'\\';
			if ( fullPath.size() > rootPrefix.size() &&
					 _wcsnicmp(fullPath.c_str(), rootPrefix.c_str(), rootPrefix.size()) == 0 )
			{
				dbPath = std::filesystem::path(fullPath.substr(rootPrefix.size())).generic_u8string();
				break;
			}
		}
		if ( dbPath.empty() )
		{
			std::string message = "Select an XDB inside the game's Data folder:\n" + initialFolder;
			if ( NMOD::DoesAnyMODAttached() )
				message += "\n\nOr inside the active mod folder:\n" + dataRoots.front().string();
			ReportError( message );
			return;
		}
		std::string typeName, validationError;
		if ( !ValidateXDB(dbPath, &typeName, &validationError) )
		{
			ReportError( validationError );
			return;
		}
		const bool bAlreadyRegistered = NDb::IsFileRegistered( dbPath );
		if ( bAlreadyRegistered && NDb::GetClassTypeName(CDBID(dbPath)) != typeName )
		{
			ReportError( "This database path is already registered with a different resource type. Use a new filename." );
			return;
		}
		if ( !NDb::RegisterResourceFile(dbPath) )
		{
			ReportError( "The database could not read the selected XDB. It was not registered." );
			return;
		}
		// Persist the combined database index in the active writable layer (the mod
		// when attached). Base-data references are valid there too; do not copy XDBs
		// or save unrelated resource edits.
		const bool bSaved = NDb::SaveChangedIndex();
		rReloadData();
		if ( !bSaved )
		{
			ReportError( "The resource is registered for this session, but index.bin could not be saved.\nCheck data-folder permissions, then retry registration." );
			return;
		}
		Singleton<IMainFrameContainer>()->Get()->Log( LT_NORMAL, fmt::format("Registered {} ({})\n", dbPath, typeName) );
		::MessageBox( hwndOwner, fmt::format("{}\nType: {}\n\n{}", dbPath, typeName,
			bAlreadyRegistered ? "This resource was already registered." : "Registered and saved to the database index.").c_str(),
			"Register XDB", MB_OK | MB_ICONINFORMATION );
	}


	std::string GetHelpFilePath()
	{
		char pBuffer[0xFFF + 1];
		::GetCurrentDirectory( 0xFFF, pBuffer );
		CString strHelpFileName;
		strHelpFileName.LoadString( IDS_HELP_FILE_NAME );
		return std::string( CString( pBuffer ) + CString( "\\" ) + strHelpFileName );
	}


	bool HasHelpFile( const std::string &rszHelpFilePath )
	{
		return NFile::DoesFileExist( rszHelpFilePath.c_str() );
	}


	void ShowHelpContents( const std::string &rszHelpFilePath )
	{
		if ( HasHelpFile( rszHelpFilePath ) )
		{
			::HtmlHelp( ::GetDesktopWindow(), rszHelpFilePath.c_str(), HH_DISPLAY_TOPIC, 0 );
		}
		else
		{
			CString strMessagePattern;
			CString strMessage;
			strMessagePattern.LoadString( IDS_NO_HELP_FILE_MESSAGE );
			strMessage.Format( strMessagePattern, rszHelpFilePath.c_str() );
			::MessageBox( MainFrameWnd()->GetSafeHwnd(), strMessage, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONERROR | MB_OK );
		}
	}


	void ShowAbout()
	{
		// Which dialog answers is NAbout's business, not the frame's.
		NAbout::Run( Singleton<IMainFrameContainer>()->GetMainWindow() );
	}


	const SUserData::CRecentList* GetRecentList( unsigned nCommandID, unsigned *pnFirstID )
	{
		if ( nCommandID == ID_MAIN_RECENT_0 )
		{
			( *pnFirstID ) = ID_MAIN_RECENT_0;
			return &( Singleton<IUserDataContainer>()->Get()->recentList );
		}
		if ( nCommandID == ID_MAIN_RECENT_RESOURCE_0 )
		{
			( *pnFirstID ) = ID_MAIN_RECENT_RESOURCE_0;
			return &( Singleton<IUserDataContainer>()->Get()->recentResourceList );
		}
		return 0;
	}


	std::string GetRecentEmptyLabel()
	{
		CString strMenuLabel;
		strMenuLabel.LoadString( IDS_RECENT_EMPTY );
		return std::string( strMenuLabel );
	}


	void RunUserCommand( unsigned nCommandID )
	{
		bool bEnable = false;
		bool bChecked = false;
		if ( Singleton<ICommandHandlerContainer>()->UpdateCommand( nCommandID, &bEnable, &bChecked ) && bEnable  )
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( nCommandID, 0 );
		}
	}


	void UpdateUserCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
	{
		( *pbEnable ) = false;
		( *pbCheck ) = false;
		if ( nCommandID == ID_MAIN_REGISTER_XDB )
		{
			( *pbEnable ) = ( NVFS::GetMainVFS() != 0 );
			return;
		}
		unsigned nFirstID = 0;
		if ( const SUserData::CRecentList *pRecentList = GetRecentList( nCommandID, &nFirstID ) )
		{
			( *pbEnable ) = !( pRecentList->empty() );
			return;
		}
		if ( !Singleton<ICommandHandlerContainer>()->UpdateCommand( nCommandID, pbEnable, pbCheck ) )
		{
			( *pbEnable ) = false;
			( *pbCheck ) = false;
		}
	}


	CProgressHost::~CProgressHost()
	{
		delete pView;
		pView = 0;
	}


	void CProgressHost::Create( IWidget *pOwner )
	{
		hwndPreviousFocus = ::GetFocus();
		if ( pView == 0 )
		{
			// Which toolkit draws it is NProgressView's business.
			pView = NProgressView::Create();
		}
		if ( pView->IsCreated() )
		{
			pView->Show();
		}
		else
		{
			// The frame itself, not a temporary: the view outlives this call, and
			// the frame outlives the view.
			pView->Create( pOwner );
		}
	}


	void CProgressHost::Destroy()
	{
		if ( pView != 0 )
		{
			pView->Destroy();
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
		if ( ::IsWindow( hwndPreviousFocus ) )
		{
			::SetFocus( hwndPreviousFocus );
		}
	}


	void CProgressHost::SetTitle( const std::string &rszTitle )
	{
		if ( pView != 0 )
		{
			pView->SetTitle( rszTitle );
		}
	}


	void CProgressHost::SetMessage( const std::string &rszMessage )
	{
		if ( pView != 0 )
		{
			pView->SetMessage( rszMessage );
		}
	}


	void CProgressHost::SetRange( int nStart, int nFinish )
	{
		if ( pView != 0 )
		{
			pView->SetRange( nStart, nFinish );
		}
	}


	void CProgressHost::SetPosition( int nPosition )
	{
		if ( pView != 0 )
		{
			pView->SetPosition( nPosition );
		}
	}


	void CProgressHost::IteratePosition()
	{
		if ( pView != 0 )
		{
			pView->IteratePosition();
		}
	}
}
