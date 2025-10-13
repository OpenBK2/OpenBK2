#include "stdafx.h"
#include "mapeditorlib/resourcedefines.h"
#include "mapeditorlib/commandhandlerdefines.h"
#include "ResourceDefines.h"

#include "PC_StringDirRefEditor.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/Interface_MOD.h"

#include <cstdint>

const char CPCStringDirRefEditor::FOLDER_PATH_LABEL[] = "_FOLDER_";


static int CALLBACK CPCStringDirRefEditor_BrowseForFolderProc( HWND hwnd, UINT nCode, LPARAM lParam, LPARAM pData )
{
	//BFFM_ENABLEOK:
	//BFFM_SETEXPANDED:
	//BFFM_SETOKTEXT:
	//BFFM_SETSELECTION:
	//BFFM_SETSTATUSTEXT:
	
	//TCHAR pBuffer[] = "Select Folder";
	switch ( nCode )
	{
		case BFFM_INITIALIZED:
			::SendMessage( hwnd, BFFM_SETSELECTION, (WPARAM)0, pData );
			break;	
		case BFFM_SELCHANGED:
			break;	
	}
	return 0;
}


CPCStringDirRefEditor::CPCStringDirRefEditor( const string &rszObjectTypeName ) : szObjectTypeName( rszObjectTypeName )
{
}


// CPCItemEditor

void CPCStringDirRefEditor::GetValue( CVariant *pValue )
{
	if ( pValue )
	{
		if ( const SPropertyDesc *pDesc = GetPropertyDesc() )
		{
			CPCStringBrowseEditor::GetValue( pValue );
			string szPath = pValue->GetStr();
			bool bResult = false;
			if ( !szPath.empty() )
			{
				SUserData::ENormalizePathType pathType = SUserData::NPT_UNKNOWN;
				if ( ( pDesc->nIntParam > SUserData::NPT_UNKNOWN ) && ( pDesc->nIntParam < SUserData::NPT_COUNT ) )
				{
					pathType = static_cast<SUserData::ENormalizePathType>( pDesc->nIntParam );
				}
				if ( ::IsValidFileName( szPath, false ) )
				{
					( *pValue ) = szPath;
					//
					// Устанавливаем каталог куда будем заглядывать при последующем вызове диалога открытия файла
					const string szFullPath = Singleton<IMODContainer>()->GetDataFolder( pathType ) + szPath;
					string szObjectNamePrefix;
					CStringManager::SplitFileName( &szObjectNamePrefix, 0, 0, szFullPath );
					SUserData::CFilePathMap &rFilePathMap = Singleton<IUserDataContainer>()->Get()->filePathMap;
					rFilePathMap[FOLDER_PATH_LABEL] = szFullPath;
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
				CPCStringBrowseEditor::GetValue( pValue );
			}
		}
	}
}


// CPCStringBrowseEditor

void CPCStringDirRefEditor::OnBrowse()
{
	if ( const SPropertyDesc *pDesc = GetPropertyDesc() )
	{
		SUserData::CFilePathMap &rFilePathMap = Singleton<IUserDataContainer>()->Get()->filePathMap;
		const string szInitialDir = rFilePathMap[FOLDER_PATH_LABEL];
		//
		CString strTitle;
		strTitle.LoadString( IDS_BROWSE_FOR_FOLDER_DIALOG_TITLE );
		string szTitle = StrFmt( strTitle, GetName() );

		//return value...assume failure...
		bool bResult = true;
		string szPath;

		//Have to get the Shell's Memory Allocator
		LPMALLOC pMalloc = 0;
		HRESULT hResult = ::SHGetMalloc( &pMalloc );
		ASSERT( SUCCEEDED( hResult ) );

		//Sanity check for Release builds
		if ( SUCCEEDED( hResult ) )
		{
			LPSHELLFOLDER pShellFolder = 0;

			hResult = ::SHGetDesktopFolder( &pShellFolder );
	
			if ( SUCCEEDED( hResult ) )
			{
				OLECHAR olePath[_MAX_PATH];
				::ZeroMemory( olePath, sizeof( olePath ) );

				LPITEMIDLIST pidl = NULL;
				uint32_t dwEaten   = 0;
				uint32_t dwAttribs = 0;

				::MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szInitialDir.c_str(), -1, olePath, _MAX_PATH );

				hResult = pShellFolder->ParseDisplayName( NULL, NULL, olePath, &dwEaten, &pidl, &dwAttribs );

				if( SUCCEEDED( hResult ) )
				{
					TCHAR pBuffer[_MAX_PATH];
					::ZeroMemory( pBuffer, sizeof( pBuffer ) );

					BROWSEINFO bi;
					::ZeroMemory( &bi, sizeof( bi ) );

					bi.hwndOwner = AfxGetMainWnd()->m_hWnd;
					bi.pidlRoot = 0;
					bi.pszDisplayName = pBuffer;
					bi.lpszTitle = szTitle.c_str();
					bi.ulFlags = BIF_USENEWUI;
					bi.lpfn = CPCStringDirRefEditor_BrowseForFolderProc;
					bi.lParam = ( LPARAM )pidl;

					LPITEMIDLIST pidlPath = ::SHBrowseForFolder( &bi );

					if ( pidlPath != NULL )
					{
						if( ::SHGetPathFromIDList( pidlPath, pBuffer ) )
						{
							bResult = true;
							szPath = pBuffer;
							if ( ( !szPath.empty() ) &&
									 ( szPath[ szPath.size() - 1] != '\\' ) )
							{
								szPath += "\\";
							}
						}
						pMalloc->Free( pidlPath );
					}
					pMalloc->Free( pidl );
				}
				pShellFolder->Release();
				pShellFolder = NULL;
			}

			pMalloc->Release();
			pMalloc = NULL;
		}
		if ( bResult && ( ( GetStyle() & ES_READONLY ) == 0 ) )
		{
			if ( !szPath.empty() )
			{
				SUserData::ENormalizePathType pathType = SUserData::NPT_UNKNOWN;
				if ( ( pDesc->nIntParam > SUserData::NPT_UNKNOWN ) && ( pDesc->nIntParam < SUserData::NPT_COUNT ) )
				{
					pathType = static_cast<SUserData::ENormalizePathType>( pDesc->nIntParam );
				}
				const string szFullPath = szPath;
				const string szDataFolder = Singleton<IMODContainer>()->GetDataFolder( pathType );
				if ( CStringManager::Compare( szFullPath, szDataFolder, true, true, true ) == 0 )
				{
					szPath = szFullPath.substr( szDataFolder.size() );
					SetWindowText( szPath.c_str() );
					// Устанавливаем каталог куда будем заглядывать при последующем вызове диалога открытия файла
					rFilePathMap[FOLDER_PATH_LABEL] = szFullPath;
				}
			}
		}
		//
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
	}
	/**/
}

// basement storage  


