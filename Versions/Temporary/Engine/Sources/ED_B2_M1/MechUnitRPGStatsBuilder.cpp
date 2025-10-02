#include "StdAfx.h"

#include "../misc/strproc.h"
#include "../misc/2darray.h"
#include "MechUnitRPGStatsBuilder.h"
#include "../MapEditorLib/BuilderFactory.h"
//
#include "../libdb/ResourceManager.h"
//
#include "../MapEditorLib/DefaultView.h"
#include "../MapEditorLib/ObjectController.h"

#include "ExporterMethods.h"
#include "../ED_Common/TempAttributesTool.h"
#include "../MapEditorLib/StringManager.h"

#include <zconf.h>

REGISTER_BUILDER_IN_DLL( MechUnitRPGStats, CMechUnitRPGStatsBuilder )


const string CMechUnitRPGStatsBuilder::BUILD_DATA_TYPE_NAME = "MechUnitRPGStatsBuilder";


struct SPropellerInfo
{
	string szLocatorName;
	int nLocatorIndex;
	int nScaledStart;
	int nScaledEnd;
	int nDynamicStart;
	int nDynamicEnd;
	float fScaledSpeed;
	float fDynamicSpeed;
	bool bScaledDefined;
	bool bDynamicDefined;

	SPropellerInfo() : szLocatorName( "" ), nLocatorIndex( -1 ), nScaledStart( -1 ), nScaledEnd( -1 ), nDynamicStart( -1 ),
		nDynamicEnd( -1 ), fScaledSpeed( 0.0f ), fDynamicSpeed( 0.0f ), bScaledDefined( false ), bDynamicDefined( false ) {}

	SPropellerInfo( const int _nLocatorIndex, const string &_szLocatorName ) : szLocatorName( _szLocatorName ), nLocatorIndex( _nLocatorIndex ), nScaledStart( -1 ), nScaledEnd( -1 ), nDynamicStart( -1 ),
		nDynamicEnd( -1 ), fScaledSpeed( 0.0f ), fDynamicSpeed( 0.0f ), bScaledDefined( false ), bDynamicDefined( false ) {}

	const bool IsDefined() const { return bScaledDefined && bDynamicDefined; } 
};

// сделать копию модели, если szOldModelName и szNewModelName совпадают, просто очистить модель от анимации, прописать новые RootMesh и RootJoint, поправить текстуры
static bool CopyModel( const string &szOldModelName, const string &szNewName, const string &szRootMesh, const int nStartFrame, const int nEndFrame, const float fSpeed )
{
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	//
	if ( szOldModelName != szNewName && !pFolderCallback->CopyObject( "Model", szNewName, szOldModelName ) )
	{
		return false;
	}
	CPtr<IManipulator> pModel = Singleton<IResourceManager>()->CreateObjectManipulator( "Model", szNewName );
	if ( !pModel )
	{
		return false;
	}
	//удалить анимацию
	pModel->RemoveNode( "Animations", NODE_REMOVEALL_INDEX );
	
	//копировать геометрию
	string szGeometry = "";
	string szGeometryType = "";
	if ( !CManipulatorManager::GetParamsFromReference( "Geometry", pModel, &szGeometryType, &szGeometry, 0 ) )
		return false;
	if ( szNewName != szGeometry && !pFolderCallback->CopyObject( szGeometryType, szNewName, szGeometry ) )
		return false;
	CPtr<IManipulator> pGeometry = Singleton<IResourceManager>()->CreateObjectManipulator( szGeometryType, szNewName );
	if ( !pGeometry )
		return false;
	if ( !CManipulatorManager::SetValue( szRootMesh, pGeometry, "RootMesh", true ) )
		return false;
	if ( !CManipulatorManager::SetValue( szRootMesh, pGeometry, "RootJoint", true ) )
		return false;
	if ( !CManipulatorManager::SetValue( "", pGeometry, "AIGeometry", true ) )
		return false;
	if ( !CManipulatorManager::SetValue( szNewName, pModel, "Geometry", true ) )
		return false;

	//копировать скелет
	string szSkeleton = "";
	string szSkeletonType = "";
	if ( !CManipulatorManager::GetParamsFromReference( "Skeleton", pModel, &szSkeletonType, &szSkeleton, 0 ) )
		return false;
	if ( szNewName != szSkeleton && !pFolderCallback->CopyObject( szSkeletonType, szNewName, szSkeleton ) )
		return false;
	CPtr<IManipulator> pSkeleton = Singleton<IResourceManager>()->CreateObjectManipulator( szSkeletonType, szNewName );
	if ( !pSkeleton )
		return false;
	if ( !CManipulatorManager::SetValue( szRootMesh, pSkeleton, "RootJoint", true ) )
		return false;
	if ( !CManipulatorManager::SetValue( szNewName, pModel, "Skeleton", true ) )
		return false;
	pSkeleton->RemoveNode( "Animations", NODE_REMOVEALL_INDEX );

	//добавляем новую анимацию
	const string szAnimationName = szNewName + " Rotation";
	if ( pFolderCallback->IsUniqueName( "AnimB2", szAnimationName ) && !pFolderCallback->InsertObject( "AnimB2", szAnimationName ) )
		return false;
	CPtr<IManipulator> pAnimation = Singleton<IResourceManager>()->CreateObjectManipulator( "AnimB2", szAnimationName );
	if ( !pAnimation )
		return false;

	string szSrcName = "";
	CManipulatorManager::GetValue( &szSrcName, pSkeleton, "SrcName" );

	CManipulatorManager::SetValue( szSrcName, pAnimation, "SrcName", false );
	CManipulatorManager::SetValue( szRootMesh, pAnimation, "RootJoint", false );
	CManipulatorManager::SetValue( "ANIMATION_MOVE", pAnimation, "Type", false );
	CManipulatorManager::SetValue( 0, pAnimation, "Action" );
	CManipulatorManager::SetValue( true, pAnimation, "Looped" );
	CManipulatorManager::SetValue( nStartFrame, pAnimation, "FirstFrame" );
	CManipulatorManager::SetValue( nEndFrame, pAnimation, "LastFrame" );
	CManipulatorManager::SetValue( "", pAnimation, "AABBAName", false );
	CManipulatorManager::SetValue( "", pAnimation, "AABBDName", false );
	CManipulatorManager::SetValue( fSpeed, pAnimation, "MoveSpeed" );

	pModel->InsertNode( "Animations", NODE_ADD_INDEX );
	string szTypeAndName;
	CStringManager::GetRefValueFromTypeAndName( &szTypeAndName, "AnimB2", szAnimationName, TYPE_SEPARATOR_CHAR );
	if ( !CManipulatorManager::SetValue( szTypeAndName, pModel, "Animations.[0]", true ) )
		return false;

	//выставляем материал
	CPtr<IManipulator> pOldModel = Singleton<IResourceManager>()->CreateObjectManipulator( "Model", szOldModelName );
	if ( !pOldModel )
		return false;
  
	string szMaterialName;
	string szTextureName;
	int nMaterialsCount = 0;
	if ( !CManipulatorManager::GetValue( &nMaterialsCount, pOldModel, "Materials" ) )
		return false;

	bool bFound = false;
	bool bNeedCopy = false;

	for ( int i = 0; i < nMaterialsCount; ++i )
	{
		string szMaterial;
		CPtr<IManipulator> pMaterial = CManipulatorManager::CreateManipulatorFromReference( StrFmt( "Materials.[%d]", i ), pOldModel, 0, &szMaterial, 0 );
		if ( !pMaterial )
			return false;
		string szTexture;
		CPtr<IManipulator> pTexture = CManipulatorManager::CreateManipulatorFromReference( "Texture", pMaterial, 0, &szTexture, 0 );
		if ( !pTexture )
			return false;
    string szFileName;
		if ( !CManipulatorManager::GetValue( &szFileName, pTexture, "SrcName" ) )
			return false;
		if ( stricmp( szFileName.substr( szFileName.length()-5, 5 ).c_str(), "1.tga" ) == 0 )
		{
			//наш файл, смотрим параметр прозрачности
			if ( !bFound )
			{
				szMaterialName = szMaterial;
				szTextureName = szTexture;
			}
			bFound = true;
      string szAlphaMode;
			if ( !CManipulatorManager::GetValue( &szAlphaMode, pMaterial, "AlphaMode" ) )
				return false;
			bNeedCopy = szAlphaMode != "AM_TRANSPARENT";
			if ( !bNeedCopy )
			{
				szMaterialName = szMaterial;
				szTextureName = szTexture;
				break;
			}
		}
	}

	if ( bNeedCopy )
	{
		string szOldMaterialName = szMaterialName;
		szMaterialName = szMaterialName + "_propeller";
		if ( pFolderCallback->IsUniqueName( "Material", szMaterialName ) )
		{
			if ( !pFolderCallback->CopyObject( "Material", szMaterialName, szOldMaterialName ) )
				return false;
			CPtr<IManipulator> pMaterial = Singleton<IResourceManager>()->CreateObjectManipulator( "Material", szMaterialName );
			if ( !pMaterial )
				return false;
			if ( !CManipulatorManager::SetValue( szTextureName, pMaterial, "Texture", true ) )
				return false;
			if ( !CManipulatorManager::SetValue( "AM_TRANSPARENT", pMaterial, "AlphaMode", false ) )
				return false;
		}
	}

	pModel->RemoveNode( "Materials", NODE_REMOVEALL_INDEX );
	if ( bFound )
	{
		pModel->InsertNode( "Materials", NODE_ADD_INDEX );
		if ( !CManipulatorManager::SetValue( szMaterialName, pModel, "Materials.[0]", true ) )
			return false;
	}

	return true;
}

// создать новый VisObj на основе уже существующего, в новом объекте будут новые модели (изменены корневые кости)
static bool CreateVisObj( IManipulator* pManipulator, const string &szObjectName, const string &szRootMesh, const int nStartFrame, const int nEndFrame, const float fSpeed )
{
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	string szName;
	if ( !CManipulatorManager::GetParamsFromReference( "visualObject", pManipulator, 0, &szName, 0 ) )
	{
		return false;
	}
	if ( !pFolderCallback->CopyObject( "VisObj", szObjectName, szName ) )
	{
		return false;
	}
	CPtr<IManipulator> pVisObj = Singleton<IResourceManager>()->CreateObjectManipulator( "VisObj", szObjectName );
	if ( !pVisObj )
		return false;

	//делаем копии моделей, если в оригинальном объекте модель для сезона новая, в новом объекте также будет новая
	int nModelCount = 0;
	if ( !CManipulatorManager::GetValue( &nModelCount, pVisObj, "Models" ) )
		return false;

	hash_map<string, int> models; // ID модели - на индекс имени в names
	vector<string> names; // новые имена моделей

	for ( int i = 0; i < nModelCount; ++i )
	{
		const string szModelPath = StrFmt( "Models.[%d].", i );
		string szModelName;
		if ( !CManipulatorManager::GetParamsFromReference( szModelPath + "Model", pVisObj, 0, &szModelName, 0 ) )
			continue;
		hash_map<string, int>::const_iterator pos = models.find( szModelName );
		int nNameIndex = -1;
		if ( pos == models.end() )
		{
			string szSeason;
			if ( !CManipulatorManager::GetValue( &szSeason, pVisObj, szModelPath + "Season" ) )
				continue;
			const string szNewModelName = szObjectName + " (" + szSeason + ")";

			if ( !CopyModel( szModelName, szNewModelName, szRootMesh, nStartFrame, nEndFrame, fSpeed ) )
				continue;

			nNameIndex = names.size();
			names.push_back( szNewModelName );
			models[szModelName] = nNameIndex;
		}
		else
			nNameIndex = pos->second;

		CManipulatorManager::SetValue( names[nNameIndex], pVisObj, szModelPath + "Model", true );
		CManipulatorManager::SetValue( true, pVisObj, "ForceAnimated" );
	}

	return true;
}

static bool TryBuildHelicopter( const string &rszObjectName, IManipulator* pManipulator, IManipulator* pSrcManipulator )
{
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	// исходный манипулятор должен указывать на данные для вертолета (или не должен быть вообще)
	/*
	if ( pSrcManipulator )
	{
		string szObjectType;
		CManipulatorManager::GetParamsFromReference( "M1UnitSpecific", pSrcManipulator, &szObjectType, 0, 0, 0, 0 );
		if ( szObjectType != "M1UnitHelicopter" )
			return false;
	}
	*/
	
	// получаем манипулятор на visObj
	string szVisObjName;
	CPtr<IManipulator> pVisObjectManipulator = CManipulatorManager::CreateManipulatorFromReference( "visualObject", pManipulator, 0, &szVisObjName, 0 );
	if ( !pVisObjectManipulator )
		return false;
	const int nPathSeparator = szVisObjName.rfind( PATH_SEPARATOR_CHAR );
	if ( nPathSeparator != string::npos )
		szVisObjName = szVisObjName.substr( 0, nPathSeparator );

	// получает манипулятор на model
	CPtr<IManipulator> pItModel = CreateModelManipulatorFromVisObj( pVisObjectManipulator, 0 );
	if ( !pItModel )
		return false;

	// получаем манипулятор на geometry
	CPtr<IManipulator> pGeomManipulator = CManipulatorManager::CreateManipulatorFromReference( "Geometry", pItModel, 0, 0, 0 );
	if ( !pGeomManipulator )
		return false;

	// вытаскиваем информацию о пропеллерах
	CGrannyBoneAttributesList attribs;
	hash_map<int, SPropellerInfo> propellers;
	int nPropellersCount = 0;
	if ( GetGeometryAttributes( pGeomManipulator, &attribs ) )
	{
		for ( CGrannyBoneAttributesList::const_iterator it = attribs.begin(); it != attribs.end(); ++it ) 
		{
			if ( PatMat( it->szBoneName.c_str(), "laxis??" ) )
			{
				if ( NStr::IsDecDigit( it->szBoneName[5] ) && NStr::IsDecDigit( it->szBoneName[6] ) )
				{
					const int nLocatorIndex = NStr::ToInt( it->szBoneName.substr( 5, 2 ) );
					propellers[nLocatorIndex] = SPropellerInfo( nLocatorIndex, it->szBoneName );
					++nPropellersCount;
				}
			}
		}
	}
	if ( propellers.empty() )
		return false;

	// перебираем всю анимацию, смотрим какая из них для пропеллера
	attribs.clear();
	string szModelFileName;
	if ( !CManipulatorManager::GetValue( &szModelFileName, pGeomManipulator, "SrcName" ) )
		return false;
	const SUserData *pUD = Singleton<IUserDataContainer>()->Get();
	if ( !ReadAttributes( &attribs, NMEGeomAttribs::GetAttribs( pUD->constUserData.szExportSourceFolder + szModelFileName, "Animations", "" ), "", true ) )
		return false;
	for ( CGrannyBoneAttributesList::const_iterator it = attribs.begin(); it != attribs.end(); ++it )
	{
		if ( PatMat( it->szBoneName.c_str(), "rotate_axis??_scaled" ) )
		{
			if ( NStr::IsDecDigit( it->szBoneName[11] ) && NStr::IsDecDigit( it->szBoneName[12] ) )
			{
				const int nIndex = NStr::ToInt( it->szBoneName.substr( 11, 2 ) );
				hash_map<int, SPropellerInfo>::iterator pos = propellers.find( nIndex );
				if ( pos == propellers.end() )
					return false;
				it->GetAttribute( "starttime", &( pos->second.nScaledStart) );
				it->GetAttribute( "endtime", &( pos->second.nScaledEnd) );
				it->GetAttribute( "speed", &( pos->second.fScaledSpeed) );
				pos->second.bScaledDefined = true;
			}
		}
		else if ( PatMat( it->szBoneName.c_str(), "rotate_axis??_dynamic" ) )
		{
			if ( NStr::IsDecDigit( it->szBoneName[11] ) && NStr::IsDecDigit( it->szBoneName[12] ) )
			{
				const int nIndex = NStr::ToInt( it->szBoneName.substr( 11, 2 ) );
				hash_map<int, SPropellerInfo>::iterator pos = propellers.find( nIndex );
				if ( pos == propellers.end() )
					return false;
				it->GetAttribute( "starttime", &( pos->second.nDynamicStart) );
				it->GetAttribute( "endtime", &( pos->second.nDynamicEnd) );
				it->GetAttribute( "speed", &( pos->second.fDynamicSpeed) );
				pos->second.bDynamicDefined = true;
			}
		}
	}

	// проверка что все пропеллеры есть
	for ( int i = 1; i <= nPropellersCount; ++i )
	{
		hash_map<int, SPropellerInfo>::const_iterator pos = propellers.find( i );
		if ( pos == propellers.end() )
			return false;
		if ( !pos->second.IsDefined() )
			return false;
	}

	// похоже что моделька от вертолета ...
	// создаем специфический тип вертолета (M1UnitHelicopter)
	if ( pFolderCallback->IsUniqueName( "M1UnitHelicopter", rszObjectName ) && !pFolderCallback->InsertObject( "M1UnitHelicopter", rszObjectName ) )
	{
		return false;
	}
	CPtr<IManipulator> pHelicopterStats = Singleton<IResourceManager>()->CreateObjectManipulator( "M1UnitHelicopter", rszObjectName );
	if ( !pHelicopterStats )
	{
		return false;
	}
	pHelicopterStats->RemoveNode( "Axes", NODE_REMOVEALL_INDEX );
	
	for ( int i = 1; i <= nPropellersCount; ++i )
	{
		hash_map<int, SPropellerInfo>::const_iterator pos = propellers.find( i );
		pHelicopterStats->InsertNode( "Axes", NODE_ADD_INDEX );
		const string szNodePrefix = StrFmt( "Axes.[%d].", i-1 );
		pHelicopterStats->SetValue( szNodePrefix + "LocatorName", pos->second.szLocatorName );
		const string szScaledVisObj = szVisObjName + PATH_SEPARATOR_CHAR + StrFmt( "Axis%02d_Scaled", i );
		if ( !CreateVisObj( pManipulator, szScaledVisObj, StrFmt( "Axis%02d_Scaled", i ), pos->second.nScaledStart, pos->second.nScaledEnd, pos->second.fScaledSpeed ) )
			return false;
		CManipulatorManager::SetValue( szScaledVisObj, pHelicopterStats, szNodePrefix + "Scaled", true );
		const string szDynamicVisObj = szVisObjName + PATH_SEPARATOR_CHAR + StrFmt( "Axis%02d_Dynamic", i );
		if ( !CreateVisObj( pManipulator, szDynamicVisObj, StrFmt( "Axis%02d_Dynamic", i ), pos->second.nDynamicStart, pos->second.nDynamicEnd, pos->second.fDynamicSpeed ) )
			return false;
		CManipulatorManager::SetValue( szDynamicVisObj, pHelicopterStats, szNodePrefix + "Dynamic", true );

		CManipulatorManager::SetValue( 0.4f, pHelicopterStats, szNodePrefix + "StartScaleSpeed" );
		CManipulatorManager::SetValue( 0.9f, pHelicopterStats, szNodePrefix + "HideStaticSpeed" );
	}
	CManipulatorManager::SetValue( 3.0f, pHelicopterStats, "FullSpinTime" );

	string szTypeAndName;
	CStringManager::GetRefValueFromTypeAndName( &szTypeAndName, "M1UnitHelicopter", rszObjectName, TYPE_SEPARATOR_CHAR );
	CManipulatorManager::SetValue( szTypeAndName, pManipulator, "M1UnitSpecific", true );

	return true;
}

bool CMechUnitRPGStatsBuilder::IsValidBuildData( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView )
{
	NI_ASSERT( pBuildDataManipulator != 0, "CMechUnitRPGStatsBuilder::IsValidBuildData() pBuildDataManipulator == 0" );
	NI_ASSERT( pszDescription != 0, "CMechUnitRPGStatsBuilder::IsValidBuildData() pszDescription == 0" );
	pszDescription->clear();	
	
	// Считываем данные
	string szVisualObject;
	if ( !CManipulatorManager::GetValue( &szVisualObject, pBuildDataManipulator, "VisualObject" ) || szVisualObject.empty() )
	{
		( *pszDescription ) = "<VisualObject> must be filled.";
		return false;
	}
	string szDBType;
	if ( !CManipulatorManager::GetValue( &szDBType, pBuildDataManipulator, "Type" ) || szDBType.empty() )
	{
		( *pszDescription ) = "<Type> must be filled.";
		return false;
	}
	string szSource;
	CManipulatorManager::GetValue( &szSource, pBuildDataManipulator, "Source" );
	if ( szPreviousDBType != szDBType )
	{
		szPreviousDBType = szDBType;
		const SUserData::CObjectDBTypeMap &rMechUnitDBTypeMap = Singleton<IUserDataContainer>()->Get()->mechUnitDBTypeMap;
		SUserData::CObjectDBTypeMap::const_iterator posMechUnitDBType = rMechUnitDBTypeMap.find( szPreviousDBType );
		if ( ( posMechUnitDBType != rMechUnitDBTypeMap.end() ) && ( !posMechUnitDBType->second.empty() ) )
		{
			if ( pBuildDataView )
			{
				CPtr<CObjectBaseController> pObjectController = dynamic_cast<CDefaultView*>(pBuildDataView)->CDefaultView::CreateController<CObjectController>( static_cast<CObjectController*>( 0 ) );
				if ( pObjectController->AddChangeOperation( "Source", posMechUnitDBType->second, pBuildDataManipulator ) )
				{
					pObjectController->Redo( false, true, 0 );
					Singleton<IControllerContainer>()->Add( pObjectController );
				}
			}
		}
	}
	if ( !CManipulatorManager::GetValue( &szSource, pBuildDataManipulator, "Source" ) || szSource.empty() )
	{
		( *pszDescription ) = "<Source> must be filled.";
		return false;
	}
	return true;
}


bool CMechUnitRPGStatsBuilder::InternalInsertObject( string *pszObjectTypeName,
																										 string *pszUniqueObjectName,
																										 bool bFromMainMenu,
																										 bool *pbCanChangeObjectName,
																										 bool *pbNeedExport,
																										 bool *pbNeedEdit,
																										 IManipulator *pBuildDataManipulator )
{
	NI_ASSERT( pszObjectTypeName != 0, "CMechUnitRPGStatsBuilder::InternalInsertObject() pszObjectTypeName == 0" );
	NI_ASSERT( pszUniqueObjectName != 0, "CMechUnitRPGStatsBuilder::InternalInsertObject() pszUniqueObjectName == 0" );
	NI_ASSERT( pBuildDataManipulator != 0, "CMechUnitRPGStatsBuilder::InternalInsertObject() pBuildDataManipulator == 0" );
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	//
	string szDescription;
	if ( !IsValidBuildData( pBuildDataManipulator, &szDescription, 0 ) )
	{
		return false;
	}
	// Считываем данные
	string szVisualObject;
	string szDBType;
	string szSource;
	CManipulatorManager::GetValue( &szVisualObject, pBuildDataManipulator, "VisualObject" );
	CManipulatorManager::GetValue( &szDBType, pBuildDataManipulator, "Type" );
	CManipulatorManager::GetValue( &szSource, pBuildDataManipulator, "Source" );
	//
	bool bResult = pFolderCallback->InsertObject( *pszObjectTypeName, *pszUniqueObjectName );
	if ( bResult )
	{
		CPtr<IManipulator> pMechUnitRPGStatsManipulator = pResourceManager->CreateObjectManipulator( *pszObjectTypeName, *pszUniqueObjectName );
		CPtr<IManipulator> pSourceMechUnitRPGStatsManipulator = 0;
		NI_ASSERT( pMechUnitRPGStatsManipulator != 0, "CMechUnitRPGStatsBuilder::InternalInsertObject() pMechUnitRPGStatsManipulator == 0" );
		if ( !szSource.empty() )
		{
			pSourceMechUnitRPGStatsManipulator = pResourceManager->CreateObjectManipulator( *pszObjectTypeName, szSource );
			if ( pSourceMechUnitRPGStatsManipulator )
			{
				CManipulatorManager::CloneDBManipulator( pMechUnitRPGStatsManipulator, pSourceMechUnitRPGStatsManipulator, true );
			}
		}
		// Проставляем основные параметры
		bResult = bResult && pMechUnitRPGStatsManipulator->SetValue( "GameType", string( "SGVOGT_UNIT" ) );
		bResult = bResult && pMechUnitRPGStatsManipulator->SetValue( "DBtype", szDBType );
		bResult = bResult && pMechUnitRPGStatsManipulator->SetValue( "visualObject", szVisualObject );

		// Может это вертолет ???
		TryBuildHelicopter( *pszUniqueObjectName, pMechUnitRPGStatsManipulator, pSourceMechUnitRPGStatsManipulator );
	}
	return bResult;
}

// basement storage  


