#include "StdAfx.h"

#include "SkeletonExporter.h"
#include "../MapEditorLib/ExporterFactory.h"
#include "../libdb/ResourceManager.h"
#include "../MapEditorLib/StringManager.h"
#include "../MapEditorLib/Tools_HashSet.h"
#include "../MapEditorLib/ManipulatorManager.h"
#include "../ED_Common/TempAttributesTool.h"

#include "ExporterMethods.h"
#include "AnimationMnemonics.h"
#include "../Misc/StrProc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


REGISTER_EXPORTER_IN_DLL( Skeleton, CSkeletonExporter )


static const char PARAMS_EXT[] = ".params";
const char *CSkeletonExporter::GetAddPath() const
{
	return "bin\\skeletons\\";
}

bool CSkeletonExporter::FormScript( string *pScriptText,
																		const string &szTypeName,
																		const string &szObjName, 
                                    const string &szDstPath,
																		const string &szSrcPath,
                                    IManipulator *pManipulator )
{
	const string szSettingsFileName = GetGrannyExportSettingsFileName( szTypeName );
	if ( szSettingsFileName.empty() )
	{
		NLog::Log( LT_ERROR, "Granny exporter settings file is not specified\n" );
		NLog::Log( LT_ERROR, "Check ConstUserData.xml in \"MayaExport\" section\n" );
		NLog::Log( LT_ERROR, "\tExport type: %s\n", szTypeName.c_str() );
		return false;
	}
	//
	string szRootJoint;
	if ( CManipulatorManager::GetValue( &szRootJoint, pManipulator, "RootJoint" ) == false )
	{
		szRootJoint.clear();
	}
	// main script - export skeleton
	string szScriptTemplate = GetScriptTemplate( "ExportSkeleton" );
	*pScriptText = StrFmt( szScriptTemplate.c_str(),
		szDstPath.c_str(), szSrcPath.c_str(),
		"", szRootJoint.c_str(),
		szSettingsFileName.c_str() );
	*pScriptText += ";\n";
	// store all existed animations and remove references
	if ( PatMat( szRootJoint.c_str(), "*section??" ) )
		return true;
	animations.clear();
	int nRefCount = 0;
	CManipulatorManager::GetValue( &nRefCount, pManipulator, "Animations" );
	for ( int nRefIndex = 0; nRefIndex < nRefCount; ++nRefIndex )
	{
		string szRef;
		CManipulatorManager::GetValue( &szRef, pManipulator, StrFmt("Animations.[%d]", nRefIndex) );
		if ( !szRef.empty() )
			animations[szRef] = 1;
	}
	pManipulator->RemoveNode( "Animations" );
	//
	return true;
}

bool CSkeletonExporter::ImportInfoToDBBeforeRefs( const string &szObjName, 
																									const string &szSrcScenePath,
																									const string &szDstFileName, 
																									IManipulator *pManipulator )
{
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	bool bResult = true;
	const string szAnimationTypeName = "AnimB2";
	CPtr<IManipulator> pFolderManipulator = Singleton<IResourceManager>()->CreateFolderManipulator( "Skeleton" );
	if ( !pFolderManipulator )
	{
		return false;
	}
	CPtr<IManipulator> pAnimationFolderManipulator = Singleton<IResourceManager>()->CreateFolderManipulator( szAnimationTypeName );
	if ( !pAnimationFolderManipulator )
	{
		return false;
	}
	//
	// 
	string szSrcName;
	string szRootJoint;
	CManipulatorManager::GetValue( &szSrcName, pManipulator, "SrcName" );
	CManipulatorManager::GetValue( &szRootJoint, pManipulator, "RootJoint" );
	//
	if ( PatMat( szRootJoint.c_str(), "*section??" ) )
		return true;
	//
	string szAnimationNamePrefix = NFile::CutFileExt( szObjName, 0 );
	const string szSkeletonPostfix = "_skeleton";
	if ( NFile::ComparePathEq(szAnimationNamePrefix.size() - szSkeletonPostfix.size(), szSkeletonPostfix.size(), 
		                        szAnimationNamePrefix, 0, szSkeletonPostfix.size(), szSkeletonPostfix) != false )
	{
		szAnimationNamePrefix = szAnimationNamePrefix.substr( 0, szAnimationNamePrefix.size() - szSkeletonPostfix.size() ) + "_";
	}
	//
	const SUserData *pUD = Singleton<IUserDataContainer>()->Get();
	string szSrcFileName = pUD->constUserData.szExportSourceFolder + szSrcName;
	NStr::ReplaceAllChars( &szSrcFileName, '\\', '/' );
	try
	{
		// add new animations
		CGrannyBoneAttributesList attributesList;
		granny_file_info *pGFI = NMEGeomAttribs::GetAttribs( szSrcFileName, "", ANIMATIONS_ROOT_JOINT );
		ReadAttributes( &attributesList, pGFI, ANIMATIONS_ROOT_JOINT, false );
		int nAnimationRefCount = 0;
		for ( CGrannyBoneAttributesList::const_iterator itAttribute = attributesList.begin(); 
			    itAttribute != attributesList.end(); ++itAttribute )
		{
			bool bResult = true;
			//
			string szAnimationName;
			//
			string szAnimationType;
			int nFirstFrame = -1;
			int nLastFrame = -1;
			string szAABBAName;
			string szAABBDName;
			DWORD dwWeaponBits = 0;
			bool bLooped = false;
			float fSpeed = 1.0f;
			int nActionFrame = 0;
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
						szAnimationName = StrFmt( "%s_%02d", szMnemonic.c_str(), nNumber ); 
					else
						szAnimationName = szMnemonic;
					bResult = bResult && itAttribute->GetAttribute( "starttime", &nFirstFrame );
					bResult = bResult && itAttribute->GetAttribute( "endtime", &nLastFrame );
					if ( itAttribute->GetAttribute( "speed", &fSpeed ) == false )
						fSpeed = 1.0f;
					//
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
					// optional attribs
					itAttribute->GetAttribute( "looped", &bLooped );
					itAttribute->GetAttribute( "actiontime", &nActionFrame );
					nActionFrame -= nFirstFrame;
					if ( nActionFrame < 0 )
						nActionFrame = 0;
				}
			}
			szAnimationName = szAnimationNamePrefix + szAnimationName + "_animb2.xdb";
			if ( bResult )
			{
				if ( animations.find(szAnimationName) == animations.end() && 
					   pFolderCallback->IsUniqueName(szAnimationTypeName,  szAnimationName) )
				{
					pFolderCallback->InsertObject( szAnimationTypeName, szAnimationName );
					NLog::Log( LT_IMPORTANT, "Adding new animation \"%s\"\n", szAnimationName.c_str() );
				}
				//
				if ( CPtr<IManipulator> pAnimationManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( szAnimationTypeName, szAnimationName ) ) 
				{
					// Удалим анимацию из списка несуществующих анимаций
					string szTypeAndName;
					CStringManager::GetRefValueFromTypeAndName( &szTypeAndName, szAnimationTypeName, szAnimationName, TYPE_SEPARATOR_CHAR );
					CAnimationRefMap::iterator posAnimationName = animations.find( szAnimationName );
					if ( posAnimationName != animations.end() )
						animations.erase( posAnimationName );
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
					bResult = bResult && pAnimationManipulator->SetValue( "ActionFrame", nActionFrame );
					bResult = bResult && pAnimationManipulator->SetValue( "MoveSpeed", fSpeed );
					// Добавим анимацию в скелет
					bResult = bResult && pManipulator->InsertNode( "Animations" );
					bResult = bResult && pManipulator->SetValue( StrFmt( "Animations.[%d]", nAnimationRefCount ), szTypeAndName );
				}
			}
			if ( bResult )
				++nAnimationRefCount;
		}
		//
		// remove left animations
		for ( CAnimationRefMap::iterator itAnimationName = animations.begin(); itAnimationName != animations.end(); ++itAnimationName ) 
		{
			pFolderCallback->RemoveObject( "AnimB2", itAnimationName->first.ToString(),  false );
			const string szFileName = NDb::GetFileName( itAnimationName->first );
			::DeleteFile( (pUD->constUserData.szDataStorageFolder + szFileName).c_str() );
			//
			NLog::Log( LT_IMPORTANT, "Removing old animation: %s\n", szFileName.c_str() );
		}
		animations.clear();
//		if ( nAnimationRefCount == 0 )
//			pFolderCallback->RemoveObject( szAnimationTypeName, szAnimationFolder, false );
	}
	catch ( ... ) 
	{
		NLog::Log( LT_ERROR, "Can't get params file to read animations\n" );
		bResult = false;
	}
	//	
	return bResult;
}

EXPORT_RESULT CSkeletonExporter::CustomCheck( const string &szTypeName,
																							const string &szObjName, 
																							const string &szSrcScenePath,
																							const string &szDestinationPath, 
																							IManipulator *pManipulator )
{
	CGrannyFileInfoGuard fileInfo( szDestinationPath );
	// check for number of skeletons in file
	if ( fileInfo->SkeletonCount != 1 )
	{
		NLog::Log( LT_ERROR, "Incorrect number of skeletons in file\n" );
		NLog::Log( LT_ERROR, "\tSkeleton: %s\n", szObjName.c_str() );
		NLog::Log( LT_ERROR, "\tSkeletons count: %d\n", fileInfo->SkeletonCount );
		NLog::Log( LT_ERROR, "\tSource file: %s\n", szSrcScenePath.c_str() );
		return ER_FAIL;
	}
	// check for number of bones
	if ( fileInfo->Skeletons[0]->BoneCount == 0 ) 
	{
		NLog::Log( LT_ERROR, "Empty bones list in file - check \"RootJoint\"\n" );
		NLog::Log( LT_ERROR, "\tSkeleton: %s\n", szObjName.c_str() );
		NLog::Log( LT_ERROR, "\tSource file: %s\n", szSrcScenePath.c_str() );
		return ER_FAIL;
	}
	//
	return ER_SUCCESS;
}

// basement storage  

