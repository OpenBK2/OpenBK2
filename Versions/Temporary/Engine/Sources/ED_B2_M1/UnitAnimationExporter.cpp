#include "StdAfx.h"

#include "UnitAnimationExporter.h"
#include "../MapEditorLib/ExporterFactory.h"
#include "../libdb/ResourceManager.h"
#include "ExporterMethods.h"
#include "../MapEditorLib/ManipulatorManager.h"
#include "AnimationMnemonics.h"
#include "WeaponMnemonics.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


REGISTER_EXPORTER_IN_DLL( InfantryRPGStats, CInfantryExporter )

static bool CopyVector2D( IManipulator *pSrc, IManipulator *pDst, const string &szSrc, const string &szDst )
{
	CVariant var;
	bool bResult = true;
	bResult = bResult && pSrc->GetValue( szSrc + ".x", &var );
	bResult = bResult && pDst->SetValue( szDst + ".x", var );
	bResult = bResult && pSrc->GetValue( szSrc + ".y", &var );
	bResult = bResult && pDst->SetValue( szDst + ".y", var );
	return bResult;
}


static bool CopyAABB2D( IManipulator *pSrc, IManipulator *pDst, const string &szSrc, const string &szDst )
{
	bool bResult = true;
	bResult = bResult && CopyVector2D( pSrc, pDst, szSrc + ".Center", szDst + ".Center" );
	bResult = bResult && CopyVector2D( pSrc, pDst, szSrc + ".HalfSize", szDst + ".HalfSize" );
	return bResult;
}

static bool IsStringEmpty( const string &szArg )
{
	return szArg.empty() || ( szArg == " " );
}

void CInfantryExporter::BuildAnimsMap()
{
	if ( !animsMap.empty() )
	{
		return;
	}
	//
	IResourceManager *pRM = Singleton<IResourceManager>();
	if ( CPtr<IManipulator> pAnimFolders = pRM->CreateFolderManipulator( "AnimB2" ) )
	{
		if ( CPtr<IManipulatorIterator> pItAnim = pAnimFolders->Iterate( true, ECT_NO_CACHE ) )
		{
			list<string> anims;
			while ( !pItAnim->IsEnd() )
			{
				string szType;
				pItAnim->GetType( &szType );
				if ( szType == "object" )
				{
					string szName;
					pItAnim->GetName( &szName );
					if ( !szName.empty() )
					{
						anims.push_back( szName );
					}
				}
				pItAnim->Next();
			}
			for ( list<string>::const_iterator it = anims.begin(); it != anims.end(); ++it )
			{
				CVariant var;
				if ( CPtr<IManipulator> pAnimMan = pRM->CreateObjectManipulator( "AnimB2", *it ) )
				{
					if ( pAnimMan->GetValue( "WeaponsToUseWith", &var ) )
					{
						DWORD dwAnimMask;
						memcpy( &dwAnimMask, var.GetPtr(), 4 );
						if ( dwAnimMask != 0 )
						{ 
							animsMap[dwAnimMask].push_back( *it );
						}
					}
				}
			}
		}
	}
}


bool CInfantryExporter::ProcessAABB( IManipulator *pMan )
{
	CPtr<IManipulator> pItVisObj = CManipulatorManager::CreateManipulatorFromReference( "visualObject", pMan, 0, 0, 0 );
	if ( pItVisObj == 0 ) 
	{
		return false;
	}
	// find first non-null model
	CPtr<IManipulator> pItModel = CreateModelManipulatorFromVisObj( pItVisObj, 0 );
	if ( pItModel == 0 ) 
	{
		return false;
	}
	//
	CPtr<IManipulator> pGeomMan = CManipulatorManager::CreateManipulatorFromReference( "Geometry", pItModel, 0, 0, 0 );
	if ( pGeomMan == 0 ) 
	{
		return false;
	}
	// find first non-null model
	CPtr<IManipulator> pAIGeomMan = CManipulatorManager::CreateManipulatorFromReference( "AIGeometry", pGeomMan, 0, 0, 0 );
	if ( pAIGeomMan == 0 ) 
	{
		return false;
	}
	// find first non-null model
	// AABB center
	CVec2 vAABBCenter;
	if ( CManipulatorManager::GetVec2<CVec2, float>( &vAABBCenter, pAIGeomMan, "AABBCenter" ) != false )
	{
		Vis2AI( &vAABBCenter );
		if ( !CManipulatorManager::SetVec2( vAABBCenter, pMan, "AABBCenter" ) )
		{
			NLog::Log( LT_ERROR, "CInfantryExporter(): Can't set vec2: AABBCenter" );
			return false;
		}
	}
	// AABB half size
	CVec2 vAABBHalfSize;
	if ( CManipulatorManager::GetVec2<CVec2, float>( &vAABBHalfSize, pAIGeomMan, "AABBHalfSize" ) != false )
	{
		Vis2AI( &vAABBHalfSize );
		if ( !CManipulatorManager::SetVec2( vAABBHalfSize, pMan, "AABBHalfSize" ) )
		{
			NLog::Log( LT_ERROR, "CInfantryExporter(): Can't set vec2: AABBHalfSize" );
			return false;
		}
	}
	return true;
}


bool CInfantryExporter::ProcessShootPoint( IManipulator *pMan )
{
	CPtr<IManipulator> pGunMan = CManipulatorManager::CreateManipulatorFromReference( "guns.[0].Weapon", pMan, 0, 0, 0 );
	if ( !IsValid( pGunMan ) )
	{
		return false;
	}
	CPtr<IManipulator> pGunVOMan = CManipulatorManager::CreateManipulatorFromReference( "VisObj", pGunMan, 0, 0, 0 );
	if ( !IsValid( pGunVOMan ) )
	{
		return false;
	}
	CPtr<IManipulator> pGunModelMan = CreateModelManipulatorFromVisObj( pGunVOMan, 0 );
	if ( !IsValid( pGunModelMan ) )
	{
		return false;
	}
	CPtr<IManipulator> pGeomModelMan = CManipulatorManager::CreateManipulatorFromReference( "Geometry", pGunModelMan, 0, 0, 0 );
	if ( !IsValid( pGeomModelMan ) )
	{
		return false;
	}
	// Get all attributes from model
	CVec3 vOffset = VNULL3;
	CGrannyBoneAttributesList attribs;
	if ( GetGeometryAttributes( pGeomModelMan, &attribs ) )
	{
		for ( CGrannyBoneAttributesList::const_iterator it = attribs.begin(); it != attribs.end(); ++it ) 
		{
			if ( PatMat( it->szBoneName.c_str(), "lshootpoint" ) )			// Found the shoot point
			{
				it->GetAttribute( "translatex", &vOffset.x );
				it->GetAttribute( "translatey", &vOffset.y );
				it->GetAttribute( "translatez", &vOffset.z );
				break;
			}
		}
	}
	// Store the data
	const string szPrefix = "guns.[0].ShootPointOffset";
	CManipulatorManager::SetVec3( vOffset, pMan, szPrefix ); 
	return true;
}


EXPORT_RESULT CInfantryExporter::ExportObject( IManipulator* pManipulator,
																							const string &rszObjectTypeName,
																							const string &rszObjectName,
																							bool bForce,
																							EXPORT_TYPE exportType )
{
	if ( exportType == ET_BEFORE_REF )
	{
		return ER_SUCCESS;
	}
	// process animations
	if ( ProcessMechUnitLikeAnimations(pManipulator) == false )
	{
		ProcessInfantrySpecificAnimations( pManipulator );
	}
	// AABB information about this unit (from AI geometry)
	ProcessAABB( pManipulator );
	// shoot point information
	ProcessShootPoint( pManipulator );
	return ER_SUCCESS;
}


void CInfantryExporter::FinishExport( const string &rszObjectTypeName, bool bForce )
{
	CBasicExporter::FinishExport( rszObjectTypeName, bForce );
	animsMap.clear();
}


static void TransferAnimInfo2Unit( IManipulator *pUnitMan, IManipulator *pAnimMan, const string &szAnimName, int *pnAnimCounter )
{
	CVariant var;
	// get type and create entry
	bool bResult = true;
	bResult = bResult && pAnimMan->GetValue( "Type", &var );
	if ( !bResult )
	{
		NLog::Log( LT_ERROR, "TransferAnimInfo2Unit(): Can't get Type" );
		return;
	}
	int nAnimType = typeAnimationMnemonics.GetValue( (string)var.GetStr() );
	string szDescName = StrFmt( "animdescs.[%d].anims", nAnimType );
	bResult = bResult && pUnitMan->InsertNode( szDescName );
	bResult = bResult && pUnitMan->GetValue( szDescName, &var );
	if ( !bResult )
	{
		NLog::Log( LT_ERROR, "TransferAnimInfo2Unit(): Can't insert animdescs.[%d].anims", nAnimType );
		return;
	}
	szDescName += StrFmt( ".[%d]", ((int)var) - 1 );
	//create aabb's and write indexes
	bResult = bResult && pAnimMan->GetValue( "AABBAName", &var );
	if ( !bResult )
	{
		NLog::Log( LT_ERROR, "TransferAnimInfo2Unit(): Can't get AABBAName" );
		return;
	}
	string szAABBAName = var.GetStr();
	bResult = bResult && pAnimMan->GetValue( "AABBDName", &var );
	if ( !bResult )
	{
		NLog::Log( LT_ERROR, "TransferAnimInfo2Unit(): Can't get AABBDName" );
		return;
	}
	string szAABBDName = var.GetStr();
	if ( !IsStringEmpty( szAABBAName ) || ! IsStringEmpty( szAABBDName ) )
	{
		bResult = bResult && pUnitMan->InsertNode( "aabb_as" );
		bResult = bResult && pUnitMan->InsertNode( "aabb_ds" );
		bResult = bResult && CopyAABB2D( pAnimMan, pUnitMan, "aabb_a", StrFmt( "aabb_as.[%d]", (*pnAnimCounter) ) );
		bResult = bResult && CopyAABB2D( pAnimMan, pUnitMan, "aabb_d", StrFmt( "aabb_ds.[%d]", (*pnAnimCounter) ) );
		if ( !bResult )
		{
			NLog::Log( LT_ERROR, "TransferAnimInfo2Unit(): Can't insert aabb_as & aabb_ds" );
			return;
		}
		var = CVariant( (*pnAnimCounter) );
		bResult = bResult && pUnitMan->SetValue( szDescName + ".AABB_A", var );
		bResult = bResult && pUnitMan->SetValue( szDescName + ".AABB_D", var );
		if ( !bResult )
		{
			NLog::Log( LT_ERROR, "TransferAnimInfo2Unit(): Can't set AABB_A & AABB_D" );
			return;
		}
		++(*pnAnimCounter);
	}
	else
	{
		var = CVariant( -1 );
		bResult = bResult && pUnitMan->SetValue( szDescName + ".AABB_A", var );
		bResult = bResult && pUnitMan->SetValue( szDescName + ".AABB_D", var );
		if ( !bResult )
		{
			NLog::Log( LT_ERROR, "TransferAnimInfo2Unit(): Can't set AABB_A & AABB_D" );
			return;
		}
	}
	//read and write other data
	bResult = bResult && pAnimMan->GetValue( "Length", &var );
	bResult = bResult && pUnitMan->SetValue( szDescName + ".Length", var );
	bResult = bResult && pAnimMan->GetValue( "Action", &var );
	bResult = bResult && pUnitMan->SetValue( szDescName + ".Action", var );
	var = CVariant( "AnimB2:" + szAnimName );
	bResult = bResult && pUnitMan->SetValue( szDescName + ".Animation", var );
	if ( !bResult )
	{
		NLog::Log( LT_ERROR, "TransferAnimInfo2Unit(): Can't set other data" );
		return;
	}
}


bool CInfantryExporter::ProcessInfantrySpecificAnimations( IManipulator *pItUnit )
{
	BuildAnimsMap();
	//
	IResourceManager *pRM = Singleton<IResourceManager>();
	CVariant var;
	bool bResult = true;
	bResult = bResult && pItUnit->RemoveNode( "aabb_as" );
	bResult = bResult && pItUnit->RemoveNode( "aabb_ds" );
	bResult = bResult && pItUnit->GetValue( "animdescs", &var );
	if ( !bResult )
	{
		NLog::Log( LT_ERROR, "ProcessInfantrySpecificAnimations(): Can't remove aabb_as, aabb_ds" );
		return false;
	}
	int nNumDescs = var;
	for ( int i = 0; i < nNumDescs; ++i )
	{
		bResult = bResult && pItUnit->RemoveNode( StrFmt( "animdescs.[%d].anims", i ) );
		if ( !bResult )
		{
			break;
		}
	}
	bResult = bResult && pItUnit->RemoveNode( "animdescs" );
	if ( !bResult )
	{
		NLog::Log( LT_ERROR, "ProcessInfantrySpecificAnimations(): Can't remove animdescs" );
		return false;
	}
	for ( int i = 0; i < NDb::__ANIMATION_TYPE_COUNTER - 1; ++i )
	{
		bResult = bResult && pItUnit->InsertNode( "animdescs" );
		if ( !bResult )
		{
			NLog::Log( LT_ERROR, "ProcessInfantrySpecificAnimations(): Can't insert animdesc[%d]", i );
			return false;
		}
	}
	//collect weapon type
	bResult = bResult && pItUnit->GetValue( "guns", &var );
	if ( !bResult || ( int( var ) == 0 ) )
	{
		NLog::Log( LT_ERROR, "ProcessInfantrySpecificAnimations(): guns count is zero" );
		return false;
	}
	bResult = bResult && pItUnit->GetValue( "guns.[0].Weapon", &var );
	if ( !bResult || ( var.GetType() == CVariant::VT_NULL ) )
	{
		NLog::Log( LT_ERROR, "ProcessInfantrySpecificAnimations(): guns.[0].Weapon is zero" );
		return false;
	}
	bool bAnimationsAssigned = false;
	if ( CPtr<IManipulator> pItWeapon = pRM->CreateObjectManipulator( "WeaponRPGStats", string( var.GetStr() ) ) )
	{
		bResult = bResult && pItWeapon->GetValue( "WeaponType", &var );
		if ( bResult )
		{
			DWORD dwWeaponMask = 0x1 << typeWeaponMnemonics.GetValue( string( var.GetStr() ) );

			//write animation data to stats
			int nAnimCounter = 0;
			for ( hash_map<DWORD, list<string> >::const_iterator itAnimList = animsMap.begin(); itAnimList != animsMap.end(); ++itAnimList )
			{
				const DWORD dwAnimMask = itAnimList->first;
				if ( dwAnimMask & dwWeaponMask )
				{
					for ( list<string>::const_iterator itAnim = itAnimList->second.begin(); itAnim != itAnimList->second.end(); ++itAnim )
					{
						if ( CPtr<IManipulator> pAnimMan = pRM->CreateObjectManipulator( "AnimB2", *itAnim ) )
						{
							TransferAnimInfo2Unit( pItUnit, pAnimMan, *itAnim, &nAnimCounter );
							bAnimationsAssigned = true;
						}
					}
				}
			}	
		}
	}
	return bAnimationsAssigned;
}


bool CInfantryExporter::ProcessMechUnitLikeAnimations( IManipulator *pItUnit )
{
	IResourceManager *pRM = Singleton<IResourceManager>();
	CVariant var;
	bool bResult = true;
	bResult = bResult && pItUnit->RemoveNode( "aabb_as" );
	bResult = bResult && pItUnit->RemoveNode( "aabb_ds" );
	bResult = bResult && pItUnit->GetValue( "animdescs", &var );
	if (! bResult )
	{
		NLog::Log( LT_ERROR, "ProcessMechUnitLikeAnimations(): cant't remove aabb_as or aabb_ds" );
		return false;
	}
	int nNumDescs = var;
	for ( int i = 0; i < nNumDescs; ++i )
	{
		bResult = bResult && pItUnit->RemoveNode( StrFmt( "animdescs.[%d].anims", i ) );
		if ( !bResult )
		{
			break;
		}
	}
	bResult = bResult && pItUnit->RemoveNode( "animdescs" );
	if ( !bResult )
	{
		NLog::Log( LT_ERROR, "ProcessMechUnitLikeAnimations(): cant't remove animdescs" );
		return false;
	}
	for ( int i = 0; i < NDb::__ANIMATION_TYPE_COUNTER; ++i )
	{
		bResult = bResult && pItUnit->InsertNode( "animdescs" );
		if ( !bResult )
		{
			NLog::Log( LT_ERROR, "ProcessMechUnitLikeAnimations(): Can't insert animdesc[%d]", i );
			return false;
		}
	}
	// for infantry unit we can use animations only from main visual object
	list<string> visObjects;
	if ( pItUnit->GetValue( "visualObject", &var ) )
	{
		if ( !IsDBIDEmpty(var) )
		{
			visObjects.push_back( var.GetStr() );
		}
	}
	//collect skeletons
	typedef hash_map<string, bool> CSkeletonsMap;
	CSkeletonsMap skeletons;
	for ( list<string>::const_iterator it = visObjects.begin(); it != visObjects.end(); ++it )
	{
		if ( CPtr<IManipulator> pItVisObj = pRM->CreateObjectManipulator( "VisObj", *it ) )
		{
			if ( CPtr<IManipulator> pItModel = CreateModelManipulatorFromVisObj( pItVisObj, 0 ) )
			{
				if ( pItModel != 0 ) 
				{
					if ( pItModel->GetValue( "Skeleton", &var ) && !IsDBIDEmpty(var) )
					{
						skeletons[var.GetStr()] = true;
					}
				}
			}
		}
	}
	if ( skeletons.empty() )
	{
		NLog::Log( LT_ERROR, "ProcessMechUnitLikeAnimations(): Can't find any sceletons" );
		return false;
	}

	// convert animations from anims array to RPG stats structures
	int nAnimCounter = 0;
	bool bAnimationsAssigned = false;
	for ( CSkeletonsMap::const_iterator itSkeleton = skeletons.begin(); itSkeleton != skeletons.end(); ++itSkeleton )
	{
		if ( CPtr<IManipulator> pItSkeleton = pRM->CreateObjectManipulator( "Skeleton", itSkeleton->first ) )
		{
			// retrieve animation information
			bResult = bResult && pItSkeleton->GetValue( "Animations", &var );
			int nNumAnimations = var;
			for ( int j = 0; j < nNumAnimations; ++j )
			{
				if ( pItSkeleton->GetValue( StrFmt( "Animations.[%d]", j ), &var ) && !IsDBIDEmpty(var) )
				{
					string szAnimName = var.GetStr();
					int nTypeSepPos = szAnimName.find( ':' );
					if ( nTypeSepPos != string::npos )
					{
						szAnimName = szAnimName.substr( nTypeSepPos + 1 );
					}
					if ( CPtr<IManipulator> pAnimMan = pRM->CreateObjectManipulator( "AnimB2", szAnimName ) )
					{
						TransferAnimInfo2Unit( pItUnit, pAnimMan, szAnimName, &nAnimCounter );
						bAnimationsAssigned = true;
					}
				}
			}
		}
	}
	return bAnimationsAssigned;
}


EXPORT_RESULT CInfantryExporter::CheckObject( IManipulator* pManipulator,
																							const string &rszObjectTypeName,
																							const string &rszObjectName,
																							bool bExport,
																							EXPORT_TYPE exportType )
{
	if ( exportType == ET_BEFORE_REF ) 
		return ER_SUCCESS;
	//
	ILogger *pLogger = NLog::GetLogger();
	string szArmJointName;
	if ( CManipulatorManager::GetValue( &szArmJointName, pManipulator, "GunBoneName" ) == false )
	{
		pLogger->Log( LT_ERROR, StrFmt("Can't extract 'GunBoneName' from RPG stats\n") );
		pLogger->Log( LT_ERROR, StrFmt("\tStats type: %s\n", rszObjectTypeName.c_str()) );
		pLogger->Log( LT_ERROR, StrFmt("\tUnit: %s\n", rszObjectName.c_str()) );
		return ER_FAIL;
	}
	//
	if ( szArmJointName.empty() || szArmJointName == " " ) 
	{
		pLogger->Log( LT_ERROR, StrFmt("Empty 'GunBoneName' - infantry unit will be without weapon in hands!\n") );
		pLogger->Log( LT_ERROR, StrFmt("\tStats type: %s\n", rszObjectTypeName.c_str()) );
		pLogger->Log( LT_ERROR, StrFmt("\tUnit: %s\n", rszObjectName.c_str()) );
		return ER_FAIL;
	}
	return ER_SUCCESS;
}

