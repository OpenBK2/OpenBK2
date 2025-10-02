#include "StdAfx.h"
#include "../mapeditorlib/commandhandlerdefines.h"
#include "../mapeditorlib/resourcedefines.h"
#include "../misc/2darray.h"
#include "CommandHandlerDefines.h"
#include "ResourceDefines.h"

#include "ExporterMethods.h"
#include "AnimationBuilder.h"
#include "WeaponMnemonics.h"
#include "AnimationMnemonics.h"
#include "../MapEditorLib/BuilderFactory.h"
#include "../MapEditorLib/Interface_Exporter.h"
#include "../MapEditorLib/Interface_Logger.h"
#include "../MapEditorLib/Tools_HashSet.h"
#include "../MapEditorLib/ManipulatorManager.h"
#include "../MapEditorLib/Interface_MOD.h"
#include "../System/FilePath.h"
#include "../System/FileUtils.h"
#include "../Misc/StrProc.h"
#include "../libdb/ResourceManager.h"

#include <zconf.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


REGISTER_BUILDER_IN_DLL( AnimB2, CAnimationBuilder )


CAnimationBuilder::CAnimationBuilder()
{
	Singleton<ICommandHandlerContainer>()->Set( CHID_ANIMATION_BUILDER, this );
	Singleton<ICommandHandlerContainer>()->Register( CHID_ANIMATION_BUILDER, ID_TOOLS_CREATE_INF_ANIMS, ID_TOOLS_CREATE_INF_ANIMS );
}


CAnimationBuilder::~CAnimationBuilder()
{
	Singleton<ICommandHandlerContainer>()->UnRegister( CHID_ANIMATION_BUILDER );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_ANIMATION_BUILDER );
}


//CRAP{ PLAIN_TEXT
bool CAnimationBuilder::IsValidBuildData( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView )
{
	NI_ASSERT( pBuildDataManipulator != 0, "CMapInfoBuilder::IsValidBuildData() pBuildDataManipulator == 0" );
	NI_ASSERT( pszDescription != 0, "CMapInfoBuilder::IsValidBuildData() pszDescription == 0" );
	pszDescription->clear();	
	// Считываем данные
	string szSrcName;
	if ( !CManipulatorManager::GetValue( &szSrcName, pBuildDataManipulator, "SrcName" ) || szSrcName.empty() )
	{
		( *pszDescription ) = "<SrcName> must be filled.";
		return false;
	}
	if ( !NFile::DoesFileExist( ( Singleton<IUserDataContainer>()->Get()->constUserData.szExportSourceFolder + szSrcName ) ) )
	{
		( *pszDescription ) = "<SrcName> is invalid file name. Can't find file.";
		return false;
	}
	string szRootJoint;
	if ( !CManipulatorManager::GetValue( &szRootJoint, pBuildDataManipulator, "RootJoint" ) || szRootJoint.empty() )
	{
		( *pszDescription ) = "<RootJoint> must be filled.";
		return false;
	}
	return true;
}
//CRAP} PLAIN_TEXT


bool CAnimationBuilder::IsUniqueObjectName( const string &szObjectType, const string &szObjectName )
{
	return Singleton<IFolderCallback>()->IsUniqueName( szObjectType, szObjectName );
}


bool CAnimationBuilder::UpdateAminations( const string &rszAnimationFolder )
{
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	//
	string szAnimationTypeName = "AnimB2";
	string szSrcName;
	string szRootJoint;
	CPtr<IManipulator> pAnimationFolderManipulator = Singleton<IResourceManager>()->CreateFolderManipulator( szAnimationTypeName );
	if ( !pAnimationFolderManipulator )
	{
		return false;
	}
	SBuildDataParams buildDataParams;
	buildDataParams.nFlags = BDF_CHECK_PROPERTIES;
	buildDataParams.szObjectTypeName = szAnimationTypeName;
	buildDataParams.szObjectName = rszAnimationFolder;
	buildDataParams.bNeedExport = false;
	buildDataParams.bNeedEdit = false;
	//
	string szBuildDataTypeName = "AnimB2Builder";
	string szBuildDataName;
	if ( Singleton<IBuilderContainer>()->FillBuildData( &szBuildDataTypeName, &szBuildDataName, &buildDataParams, this ) )
	{
		if ( CPtr<IManipulator> pBuildDataManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( szBuildDataTypeName, szBuildDataName ) )
		{
			string szDescription;
			if ( IsValidBuildData( pBuildDataManipulator, &szDescription, 0 ) )
			{
				if ( !CManipulatorManager::GetValue( &szSrcName, pBuildDataManipulator, "SrcName" ) )
				{
					return false;
				}
				if ( !CManipulatorManager::GetValue( &szRootJoint, pBuildDataManipulator, "RootJoint" ) )
				{
					return false;
				}
			}
		}
	}
	bool bResult = true;
	// Формируем файл с атрибутами анимаций
	if ( !szSrcName.empty() && !szRootJoint.empty() )
	{
		string szScriptText;
		const string szSource = pUserData->constUserData.szExportSourceFolder + szSrcName;
		const string szAnimParamsFolder = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + "bin\\Animations\\Params\\";
		const string szAnimParamsDestination = szAnimParamsFolder + "FFFFFFFF";
		NFile::CreatePath( szAnimParamsFolder.c_str() );
		//
		MEStartScript( &szScriptText, false );
		szScriptText += StrFmt( "sysFile -del \"%s\";\r\n", szAnimParamsDestination.c_str() );
		szScriptText += StrFmt( "file -o -f \"%s\";\r\n", szSource.c_str() );
		szScriptText += StrFmt( "triangulateAll;\r\n" );
		szScriptText += StrFmt( "string $AnimParamsMeshFFFFFFFF[] = `ls -dag -tr \"%s\"`;\r\n", ANIMATIONS_ROOT_JOINT );
		szScriptText += StrFmt( "select -r $AnimParamsMeshFFFFFFFF;\r\n" );
		szScriptText += StrFmt( "GrannyLoadSettings \"%s\";\r\n", GetGrannyExportSettingsFileName("Attribs").c_str() );
		szScriptText += StrFmt( "GrannyExport -s on \"%s\";\r\n", szAnimParamsDestination.c_str() );
		MEFinishScript( &szScriptText, false );
		bResult = MERunScript( szScriptText, "Skeleton", true, false );
		if ( bResult )
		{
			// CRAP{ HASH_SET
			typedef hash_map<string, int> CAnimationMap;
			CAnimationMap animationMap;
			// CRAP} HASH_SET
			// Находим все анимации в текущем каталоге
			{
				CPtr<IManipulatorIterator> pIterator = pAnimationFolderManipulator->Iterate( true, ECT_CACHE_LOCAL );
				const int nSize = rszAnimationFolder.size();
				bool bFound = false;
				while( !pIterator->IsEnd() )
				{
					string szName;
					if ( pIterator->GetName( &szName ) )
					{
						if ( szName.compare( 0, rszAnimationFolder.size(), rszAnimationFolder ) == 0 )
						{
							bFound = true;
							if ( ( szName != rszAnimationFolder ) &&
									 ( szName.find( '\\', nSize ) == string::npos ) )
							{
								InsertHashSetElement( &animationMap, szName );
								DebugTrace( "CAnimationBuilder::UpdateAminations() found animation: <%s>", szName.c_str() );
							}
						}
						else if ( bFound )
						{
							break;
						}
					}
					pIterator->Next();
				}
				pAnimationFolderManipulator->ClearCache();
			}
			// Читаем набор анимаций из файла с атрибутами
			if ( bResult )
			{
				if ( !Singleton<IExporterContainer>()->StartExport( szAnimationTypeName, FORCE_EXPORT, START_EXPORT_TOOLS, NOT_EXPORT_REFERENCES ) )
				{
					return false;
				}
				try
				{
					CGrannyBoneAttributesList attributesList;
					if ( ReadAttributes( &attributesList, szAnimParamsDestination, ANIMATIONS_ROOT_JOINT, false ) == false )
					{
						const string szError = StrFmt( "Can't open animation params file \"%s\" to build/update animation\n", szAnimParamsDestination.c_str() );
						NLog::GetLogger()->Log( LT_ERROR, szError );
						return false;
					}
					//
					int nAnimationRefCount = 0;
					for ( CGrannyBoneAttributesList::const_iterator itAttribute = attributesList.begin(); itAttribute != attributesList.end(); ++itAttribute )
					{
						string szAnimationName;
						//
						string szAnimationType;
						int nFirstFrame = -1;
						int nLastFrame = -1;
						string szAABBAName;
						string szAABBDName;
						DWORD dwWeaponBits = 0;
						bool bLooped = 0;
						int nActionFrame = 0;
						float fSpeed = 1.0f;
						{
							string szBoneName = itAttribute->szBoneName;
							NStr::ToUpper( &szBoneName );
							UINT nNumber = INVALID_NODE_ID;
							NDb::EAnimationType animationType = typeMayaAnimationMnemonics.Get( szBoneName, 0, &nNumber );
							bResult = ( animationType != NDb::ANIMATION_UNKNOWN );
							if ( bResult )
							{
								szAnimationType = typeAnimationMnemonics.GetMnemonic( animationType );
								string szMnemonic = typeMayaAnimationMnemonics.GetMnemonic( animationType );
								NStr::ToLowerASCII( &szMnemonic );
								if ( nNumber != INVALID_NODE_ID )
								{
									szAnimationName = StrFmt( "%s_%02d", szMnemonic.c_str(), nNumber ); 
								}
								else
								{
									szAnimationName = szMnemonic;
								}
								bResult = bResult && itAttribute->GetAttribute( "starttime", &nFirstFrame );
								bResult = bResult && itAttribute->GetAttribute( "endtime", &nLastFrame );
								if ( bResult )
								{
									int nAABBIndex = INVALID_NODE_ID;
									itAttribute->GetAttribute( "aabbindex", &nAABBIndex );
									if ( nAABBIndex != INVALID_NODE_ID )
									{
										szAABBAName = StrFmt( "AABB_A%02d", nAABBIndex );
										szAABBDName = StrFmt( "AABB_D%02d", nAABBIndex );
									}
								}
								if ( bResult )
								{
									dwWeaponBits = GetWeaponBits( *itAttribute );
									itAttribute->GetAttribute( "looped", &bLooped );
									itAttribute->GetAttribute( "speed", &fSpeed );
									itAttribute->GetAttribute( "actiontime", &nActionFrame );
									nActionFrame -= nFirstFrame;
									if ( nActionFrame < 0 )
										nActionFrame = 0;
								}
							}
						}
						szAnimationName = rszAnimationFolder + szAnimationName;
						DebugTrace( "CAnimationBuilder::UpdateAminations() add animation: %s, Name:<%s>, RootJoint:<%s>, type:<%s>, frame:[%d,%d], aabb_a:<%s>, aabb_d:<%s>, bits: 0x%X, looped: %s",
												bResult ? "true" : "false",
												szAnimationName.c_str(),
												szRootJoint.c_str(),
												szAnimationType.c_str(),
												nFirstFrame,
												nLastFrame,
												szAABBAName.c_str(),
												szAABBDName.c_str(),
												dwWeaponBits,
												bLooped ? "true" : "false" );
						if ( bResult )
						{
							if ( pFolderCallback->IsUniqueName( szAnimationTypeName,  szAnimationName ) )
							{
								pFolderCallback->InsertObject( szAnimationTypeName, szAnimationName );
							}
							if ( CPtr<IManipulator> pAnimationManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( szAnimationTypeName, szAnimationName ) ) 
							{
								
								// Удалим анимацию из списка несуществующих анимаций
								CAnimationMap::iterator posAnimation = animationMap.find( szAnimationName );
								if ( posAnimation != animationMap.end() )
								{
									animationMap.erase( posAnimation );
								}
								// Запишем свойства инимации
								bResult = bResult && pAnimationManipulator->SetValue( "SrcName", szSrcName );
								bResult = bResult && pAnimationManipulator->SetValue( "RootJoint", szRootJoint );
								bResult = bResult && pAnimationManipulator->SetValue( "Type", szAnimationType );
								bResult = bResult && pAnimationManipulator->SetValue( "FirstFrame", nFirstFrame );
								bResult = bResult && pAnimationManipulator->SetValue( "LastFrame", nLastFrame );
								bResult = bResult && pAnimationManipulator->SetValue( "AABBAName", szAABBAName );
								bResult = bResult && pAnimationManipulator->SetValue( "AABBDName", szAABBDName );
								bResult = bResult && pAnimationManipulator->SetValue( "WeaponsToUseWith", CVariant( &dwWeaponBits, sizeof(dwWeaponBits) ) );
								bResult = bResult && pAnimationManipulator->SetValue( "Looped", bLooped );
								bResult = bResult && pAnimationManipulator->SetValue( "MoveSpeed", fSpeed );
								bResult = bResult && pAnimationManipulator->SetValue( "ActionFrame", nActionFrame );
								
								if ( bResult )
								{
									Singleton<IExporterContainer>()->ExportObject( pAnimationManipulator,
																																 szAnimationTypeName,
																																 szAnimationName,
																																 FORCE_EXPORT,
																																 NOT_EXPORT_REFERENCES );
								}
							}
						}
						if ( bResult )
						{
							++nAnimationRefCount;
						}
					}
					DebugTrace( "CAnimationBuilder::UpdateAminations(): SrcName:<%s>, animations added: %d",
											szSrcName.c_str(),
											nAnimationRefCount );
				}
				catch ( ... ) 
				{
					const string szError = StrFmt( "General error during build/update animation(s) from params file \"%s\"\n", szAnimParamsDestination.c_str() );
					NLog::GetLogger()->Log( LT_ERROR, szError );
					bResult = false;
				}
				::DeleteFile( szAnimParamsDestination.c_str() );
				Singleton<IExporterContainer>()->FinishExport( szAnimationTypeName, FORCE_EXPORT, FINISH_EXPORT_TOOLS, NOT_EXPORT_REFERENCES );
			}
			// Удаляем лишние анимации:
			if ( bResult )
			{
				for ( CAnimationMap::iterator itAnimation = animationMap.begin(); itAnimation != animationMap.end(); ++itAnimation ) 
				{
					bResult = pFolderCallback->RemoveObject( szAnimationTypeName, itAnimation->first, false );
					DebugTrace( "CAnimationBuilder::UpdateAminations() delete animation: %s <%s>",
											bResult ? "true" : "false",
											itAnimation->first.c_str() );
					if ( !bResult )
					{
						if ( CPtr<IManipulator> pAminationanipulator = Singleton<IResourceManager>()->CreateObjectManipulator( szAnimationTypeName, itAnimation->first ) )
						{
							CManipulatorManager::SetValue( typeAnimationMnemonics.GetMnemonic( NDb::ANIMATION_UNKNOWN ), pAminationanipulator, "Type" );
						}
						bResult = true;
					}
				}
			}
		}
	}
	return bResult;
}


bool CAnimationBuilder::HandleCommand( UINT nCommandID, DWORD dwData )
{
	switch( nCommandID )
	{
		case ID_TOOLS_CREATE_INF_ANIMS:
		{	
			SSelectionSet selectionSet;
			bool bResult = Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_SELECTION, reinterpret_cast<DWORD>( &selectionSet ) );
			const string szObjectTypeName = selectionSet.szObjectTypeName;
			bResult = bResult && ( szObjectTypeName == "AnimB2" );
			bResult = bResult && ( !selectionSet.objectNameList.empty() );
			if ( bResult )
			{
				const string szObjectName = selectionSet.objectNameList.front().ToString();
				bResult = bResult && ( szObjectName )[szObjectName.size() - 1] == PATH_SEPARATOR_CHAR;
				bResult = bResult && UpdateAminations( szObjectName );
			}
			return bResult;
		}
		default:
			return false;
	}
	return false;
}


bool CAnimationBuilder::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CAnimationBuilder::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CAnimationBuilder::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_TOOLS_CREATE_INF_ANIMS:
		{
			SSelectionSet selectionSet;
			bool bResult = Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_SELECTION, reinterpret_cast<DWORD>( &selectionSet ) );
			const string szObjectTypeName = selectionSet.szObjectTypeName;
			bResult = bResult && ( szObjectTypeName == "AnimB2" );
			bResult = bResult && ( !selectionSet.objectNameList.empty() );
			if ( bResult )
			{
				const string szObjectName = selectionSet.objectNameList.front().ToString();
				bResult = bResult && ( szObjectName )[szObjectName.size() - 1] == PATH_SEPARATOR_CHAR;
				( *pbEnable ) = bResult;
				( *pbCheck ) = false;
			}
			return true;
		}
		default:
			return false;
	}
	return false;
}


DWORD CAnimationBuilder::GetWeaponBits( const SGrannyBoneAttributes & gba ) const
{
	DWORD dwWeaponBits = 0;
	for ( UINT nWeaponTypeIndex = NDb::SWeaponRPGStats::WEAPON_PISTOL; nWeaponTypeIndex <= NDb::SWeaponRPGStats::_WEAPON_COUNTER; ++nWeaponTypeIndex )
	{
		string szMnemonic = typeMayaWeaponMnemonics.GetMnemonic( nWeaponTypeIndex );
		NStr::ToLowerASCII( &szMnemonic );
		float fAttributeValue = 0;
		if ( gba.GetAttribute( szMnemonic, &fAttributeValue ) && ( fAttributeValue > 0.0f ) )
		{
			dwWeaponBits += ( 1 << nWeaponTypeIndex );
		}
	}
	return dwWeaponBits;
}


// basement storage  


