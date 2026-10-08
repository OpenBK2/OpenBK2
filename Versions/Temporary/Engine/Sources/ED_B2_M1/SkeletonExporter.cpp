#include "stdafx.h"
#include "System/FileUtils.h"
#include <fmt/format.h>
#include <fmt/printf.h>

#include "SkeletonExporter.h"
#include "ED_Common/GltfExporter.h"
#include "3Dmotor/GltfAnimation.h"
#include "libdb/ObjMan.h"
#include "System/VFSOperations.h"
#include <limits>
#include <set>
#include "MapEditorLib/ExporterFactory.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/StringManager.h"
#include "MapEditorLib/Tools_HashSet.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "ED_Common/TempAttributesTool.h"

#include "ExporterMethods.h"
#include "AnimationMnemonics.h"
#include "Misc/StrProc.h"

#include <cstdint>

REGISTER_EXPORTER_IN_DLL( Skeleton, CSkeletonExporter )


static const char PARAMS_EXT[] = ".params";
const char *CSkeletonExporter::GetAddPath() const
{
	return "bin\\skeletons\\";
}

bool CSkeletonExporter::FormScript( std::string *pScriptText,
																		const std::string &szTypeName,
																		const std::string &szObjName, 
                                    const std::string &szDstPath,
																		const std::string &szSrcPath,
                                    IManipulator *pManipulator )
{
	const std::string szSettingsFileName = GetGrannyExportSettingsFileName( szTypeName );
	if ( szSettingsFileName.empty() )
	{
		NLog::Log( LT_ERROR, "Granny exporter settings file is not specified\n" );
		NLog::Log( LT_ERROR, "Check ConstUserData.xml in \"MayaExport\" section\n" );
		NLog::Log( LT_ERROR, "\tExport type: %s\n", szTypeName.c_str() );
		return false;
	}
	//
	std::string szRootJoint;
	if ( CManipulatorManager::GetValue( &szRootJoint, pManipulator, "RootJoint" ) == false )
	{
		szRootJoint.clear();
	}
	// main script - export skeleton
	std::string szScriptTemplate = GetScriptTemplate( "ExportSkeleton" );
	*pScriptText = fmt::sprintf( szScriptTemplate.c_str(),
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
		std::string szRef;
		CManipulatorManager::GetValue( &szRef, pManipulator, fmt::format("Animations.[{}]", nRefIndex) );
		if ( !szRef.empty() )
			animations[szRef] = 1;
	}
	pManipulator->RemoveNode( "Animations" );
	//
	return true;
}

bool CSkeletonExporter::ImportInfoToDBBeforeRefs( const std::string &szObjName, 
																									const std::string &szSrcScenePath,
																									const std::string &szDstFileName, 
																									IManipulator *pManipulator )
{
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	bool bResult = true;
	const std::string szAnimationTypeName = "AnimB2";
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
	std::string szSrcName;
	std::string szRootJoint;
	CManipulatorManager::GetValue( &szSrcName, pManipulator, "SrcName" );
	CManipulatorManager::GetValue( &szRootJoint, pManipulator, "RootJoint" );
	//
	if ( PatMat( szRootJoint.c_str(), "*section??" ) )
		return true;
	//
	std::string szAnimationNamePrefix = NFile::CutFileExt( szObjName, 0 );
	const std::string szSkeletonPostfix = "_skeleton";
	if ( NFile::ComparePathEq(szAnimationNamePrefix.size() - szSkeletonPostfix.size(), szSkeletonPostfix.size(), 
		                        szAnimationNamePrefix, 0, szSkeletonPostfix.size(), szSkeletonPostfix) != false )
	{
		szAnimationNamePrefix = szAnimationNamePrefix.substr( 0, szAnimationNamePrefix.size() - szSkeletonPostfix.size() ) + "_";
	}
	//
	const SUserData *pUD = Singleton<IUserDataContainer>()->Get();
	std::string szSrcFileName = pUD->constUserData.szExportSourceFolder + szSrcName;
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
			std::string szAnimationName;
			//
			std::string szAnimationType;
			int nFirstFrame = -1;
			int nLastFrame = -1;
			std::string szAABBAName;
			std::string szAABBDName;
			uint32_t dwWeaponBits = 0;
			bool bLooped = false;
			float fSpeed = 1.0f;
			int nActionFrame = 0;
			{
				std::string szBoneName = itAttribute->szBoneName;
				NStr::ToUpper( &szBoneName );
				unsigned nNumber = INVALID_NODE_ID;
				NDb::EAnimationType animationType = typeMayaAnimationMnemonics.Get( szBoneName, 0, &nNumber );
				bResult = ( animationType != NDb::ANIMATION_UNKNOWN );
				if ( bResult )
				{
					szAnimationType = typeAnimationMnemonics.GetMnemonic( animationType );
					std::string szMnemonic = typeMayaAnimationMnemonics.GetMnemonic( animationType );
					NStr::ToLowerASCII( &szMnemonic );
					if ( nNumber != INVALID_NODE_ID )
						szAnimationName = fmt::format( "{}_{:02d}", szMnemonic.c_str(), nNumber ); 
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
							szAABBAName = fmt::format( "AABB_A{:02d}", nAABBIndex );
							szAABBDName = fmt::format( "AABB_D{:02d}", nAABBIndex );
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
					std::string szTypeAndName;
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
					bResult = bResult && pManipulator->SetValue( fmt::format( "Animations.[{}]", nAnimationRefCount ), szTypeAndName );
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
			const std::string szFileName = NDb::GetFileName( itAnimationName->first );
			NFile::RemoveFile( (pUD->constUserData.szDataStorageFolder + szFileName).c_str() );
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

EXPORT_RESULT CSkeletonExporter::CustomCheck( const std::string &szTypeName,
																							const std::string &szObjName, 
																							const std::string &szSrcScenePath,
																							const std::string &szDestinationPath, 
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



namespace
{
struct SGltfAnimationMarker
{
	std::string name, type, attackBox, defenceBox;
	int first = 0, last = 0, action = 0;
	bool looped = false;
	float speed = 1.0f;
};

bool GetFrame( const SGrannyBoneAttributes &attributes, const char *key, int *frame )
{
	float value;
	if ( !attributes.GetAttribute(key, &value) || !std::isfinite(value) || value < 0 ||
		static_cast<double>(value) > (std::numeric_limits<int>::max)() || std::floor(value) != value )
		return false;
	*frame = static_cast<int>(value);
	return true;
}

CPtr<IManipulator> PrepareAnimation( const std::string &name )
{
	IFolderCallback *folder = Singleton<IFolderCallback>();
	IResourceManager *manager = Singleton<IResourceManager>();
	if ( !folder->IsUniqueName("AnimB2", name) )
	{
		CPtr<IManipulator> animation = manager->CreateObjectManipulator("AnimB2", name);
		// DB/VFS existence lookups can retain deleted files, so check their backing stats.
		NVFS::SFileStats stats;
		const bool exists = NVFS::GetMainVFS()->GetFileStats(&stats, NDb::GetFileName(CDBID(name)));
		if ( animation )
		{
			if ( !exists ) animation->GetObjMan()->SetChanged();
			return animation;
		}
		if ( exists || !folder->RemoveObject("AnimB2", name, false) ) return nullptr;
	}
	if ( !folder->InsertObject("AnimB2", name) ) return nullptr;
	return manager->CreateObjectManipulator("AnimB2", name);
}

bool ImportGltfMarkers( IManipulator *resource, const NGltf::TGltfFilePtr &file, size_t markerRoot )
{
	CGrannyBoneAttributesList attributes;
	// Without a root filter, ReadAttributes preserves the document's node indices.
	if ( !NEditorGltf::ReadAttributes(resource, &attributes) || attributes.size() != file->asset.nodes.size() )
		return false;
	std::string prefix = NFile::CutFileExt(NDb::GetFileName(resource->GetDBID()), 0);
	std::string lowerPrefix = prefix;
	NStr::ToLowerASCII(&lowerPrefix);
	const std::string suffix = "_skeleton";
	if ( lowerPrefix.size() >= suffix.size() && lowerPrefix.compare(lowerPrefix.size() - suffix.size(), suffix.size(), suffix) == 0 )
		prefix.resize(prefix.size() - suffix.size());
	prefix += "_";

	std::vector<SGltfAnimationMarker> markers;
	std::set<std::string> names;
	// Validate every marker before creating files or changing existing references.
	// These empty objects describe slices across all the object's Blender actions.
	for ( size_t index : file->asset.nodes[markerRoot].children )
	{
		const auto &entry = attributes[index];
		std::string mnemonic = entry.szRealName;
		NStr::ToUpper(&mnemonic);
		unsigned number = INVALID_NODE_ID;
		const auto type = typeMayaAnimationMnemonics.Get(mnemonic, nullptr, &number);
		if ( type == NDb::ANIMATION_UNKNOWN )
		{
			NLog::Log(LT_ERROR, "GLTF animation marker '%s' has an unknown animation type.\n", entry.szRealName.c_str());
			return false;
		}
		SGltfAnimationMarker marker;
		if ( !GetFrame(entry, "starttime", &marker.first) || !GetFrame(entry, "endtime", &marker.last) )
		{
			NLog::Log(LT_ERROR, "GLTF animation marker '%s' needs integer StartTime and EndTime custom properties. Include custom properties when exporting the GLB/GLTF.\n", entry.szRealName.c_str());
			return false;
		}
		float duration = 0;
		if ( marker.last <= marker.first || !NAnimation::CGltfSkeletonAnimator::GetSourceDuration(file, "", marker.first, marker.last, &duration) )
		{
			NLog::Log(LT_ERROR, "GLTF animation marker '%s' has an invalid frame range %d..%d; use increasing frames within the sampled timeline.\n", entry.szRealName.c_str(), marker.first, marker.last);
			return false;
		}
		mnemonic = typeMayaAnimationMnemonics.GetMnemonic(type);
		NStr::ToLowerASCII(&mnemonic);
		marker.name = prefix + mnemonic;
		if ( number != INVALID_NODE_ID ) marker.name += fmt::format("_{:02d}", number);
		marker.name += "_animb2.xdb";
		if ( !names.insert(marker.name).second )
		{
			NLog::Log(LT_ERROR, "GLTF animation markers resolve to the same resource '%s'.\n", marker.name.c_str());
			return false;
		}
		marker.type = typeAnimationMnemonics.GetMnemonic(type);
		int actionTime = 0, boxIndex = 0;
		if ( entry.attributeMap.count("actiontime") && !GetFrame(entry, "actiontime", &actionTime) )
		{
			NLog::Log(LT_ERROR, "GLTF animation marker '%s' has an invalid ActionTime.\n", entry.szRealName.c_str());
			return false;
		}
		marker.action = (std::max)(0, actionTime - marker.first);
		entry.GetAttribute("looped", &marker.looped);
		entry.GetAttribute("speed", &marker.speed);
		if ( !std::isfinite(marker.speed) || marker.speed <= 0 )
		{
			NLog::Log(LT_ERROR, "GLTF animation marker '%s' needs a positive Speed.\n", entry.szRealName.c_str());
			return false;
		}
		if ( entry.attributeMap.count("aabbindex") )
		{
			if ( !GetFrame(entry, "aabbindex", &boxIndex) )
			{
				NLog::Log(LT_ERROR, "GLTF animation marker '%s' has an invalid AABBIndex.\n", entry.szRealName.c_str());
				return false;
			}
			marker.attackBox = fmt::format("AABB_A{:02d}", boxIndex);
			marker.defenceBox = fmt::format("AABB_D{:02d}", boxIndex);
		}
		markers.push_back(std::move(marker));
	}

	std::string reference, root;
	CManipulatorManager::GetValue(&reference, resource, "ModelFileRef");
	CManipulatorManager::GetValue(&root, resource, "RootJoint");
	int count = 0;
	if ( !CManipulatorManager::GetValue(&count, resource, "Animations") ) return false;
	std::set<CDBID> linked;
	for ( int i = 0; i < count; ++i )
	{
		std::string name;
		CManipulatorManager::GetValue(&name, resource, fmt::format("Animations.[{}]", i));
		if ( !name.empty() ) linked.insert(CDBID(name));
	}
	for ( const auto &marker : markers )
	{
		CPtr<IManipulator> animation = PrepareAnimation(marker.name);
		if ( !animation )
		{
			NLog::Log(LT_ERROR, "Cannot create GLTF animation resource '%s'.\n", marker.name.c_str());
			return false;
		}
		// A nonempty ClipName would override frame slicing and select just one action.
		if ( !animation->SetValue("ModelFileRef", reference) || !animation->SetValue("RootJoint", root) ||
			!animation->SetValue("ClipName", "") || !animation->SetValue("Type", marker.type) ||
			!animation->SetValue("FirstFrame", marker.first) || !animation->SetValue("LastFrame", marker.last) ||
			!animation->SetValue("ActionFrame", marker.action) || !animation->SetValue("Looped", marker.looped) ||
			!animation->SetValue("MoveSpeed", marker.speed) || !animation->SetValue("AABBAName", marker.attackBox) ||
			!animation->SetValue("AABBDName", marker.defenceBox) || !NEditorGltf::Export(animation, "AnimB2", true) ) return false;
		// Keep manually assigned animations and make repeated exports idempotent.
		if ( linked.insert(CDBID(marker.name)).second )
		{
			if ( !resource->InsertNode("Animations") || !resource->SetValue(fmt::format("Animations.[{}]", count), marker.name) ) return false;
			++count;
		}
		NLog::Log(LT_IMPORTANT, "Imported GLTF animation '%s' (frames %d..%d).\n", marker.name.c_str(), marker.first, marker.last);
	}
	return true;
}
}

// Marker objects retain the Maya frame-slice convention. Without an Animations
// node, named glTF clips use the same animation mnemonics as the old source scenes.
// The named-clip fallback preserves an already populated AnimB2 list.
bool CSkeletonExporter::ImportGltfInfo( IManipulator *resource )
{
	const auto file = NEditorGltf::Load(resource);
	if ( !file ) return false;
	int markerRoot = -1;
	for ( size_t i = 0; i < file->asset.nodes.size(); ++i )
	{
		if ( std::string(file->asset.nodes[i].name) != ANIMATIONS_ROOT_JOINT ) continue;
		if ( markerRoot >= 0 )
		{
			NLog::Log(LT_ERROR, "GLTF has more than one Animations node.\n");
			return false;
		}
		markerRoot = static_cast<int>(i);
	}
	if ( markerRoot >= 0 )
	{
		std::string root;
		CManipulatorManager::GetValue(&root, resource, "RootJoint");
		// Building sections keep their separately generated damage-stage animations.
		if ( PatMat(root.c_str(), "*section??") ) return true;
		return ImportGltfMarkers(resource, file, markerRoot);
	}
	int count = 0;
	CManipulatorManager::GetValue(&count, resource, "Animations");
	// A populated list may deliberately use frame slices rather than named clips.
	if ( count != 0 ) return true;
	std::string reference, root;
	CManipulatorManager::GetValue(&reference, resource, "ModelFileRef");
	CManipulatorManager::GetValue(&root, resource, "RootJoint");
	std::set<std::string> added;
	for ( const auto &clip : file->asset.animations )
	{
		const std::string name(clip.name);
		if ( name.empty() || !added.insert(name).second ) continue;
		std::string mnemonic = name;
		NStr::ToUpper(&mnemonic);
		unsigned number = INVALID_NODE_ID;
		const auto type = typeMayaAnimationMnemonics.Get(mnemonic, nullptr, &number);
		if ( type == NDb::ANIMATION_UNKNOWN )
		{
			NLog::Log(LT_IMPORTANT, "GLTF clip '%s' needs an AnimB2 reference with an assigned Type.\n", name.c_str());
			continue;
		}
		std::string safeName = name;
		for ( char &c : safeName )
			if ( !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') ) c = '_';
		const std::string animationName = NFile::CutFileExt(NDb::GetFileName(resource->GetDBID()), 0) +
			"_" + safeName + "_" + std::to_string(count) + "_animb2.xdb";
		IFolderCallback *folder = Singleton<IFolderCallback>();
		if ( folder->IsUniqueName("AnimB2", animationName) && !folder->InsertObject("AnimB2", animationName) )
			return false;
		CPtr<IManipulator> animation = Singleton<IResourceManager>()->CreateObjectManipulator("AnimB2", animationName);
		if ( !animation ) return false;
		if ( !CManipulatorManager::SetValue(reference, animation, "ModelFileRef") ||
			!CManipulatorManager::SetValue(root, animation, "RootJoint") ||
			!CManipulatorManager::SetValue(name, animation, "ClipName") ||
			!CManipulatorManager::SetValue(typeAnimationMnemonics.GetMnemonic(type), animation, "Type") ||
			!NEditorGltf::Export(animation, "AnimB2", true) ) return false;
		if ( !resource->InsertNode("Animations") ||
			!CManipulatorManager::SetValue(animationName, resource, fmt::format("Animations.[{}]", count), true) )
			return false;
		++count;
	}
	return true;
}
