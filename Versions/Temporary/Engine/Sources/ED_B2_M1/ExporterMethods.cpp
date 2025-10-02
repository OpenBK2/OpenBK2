#include "StdAfx.h"

#include "../misc/2darray.h"
#include "../3dmotor/dbscene.h"
#include "../stats_b2_m1/iconsset.h"
#include "../image/targa.h"
#include "ExporterMethods.h"
#include "SeasonMnemonics.h"
#include "../libdb/ResourceManager.h"
#include "../MapEditorLib/ManipulatorManager.h"
#include "../MapEditorLib/Interface_Logger.h"
#include "../MapEditorLib/Interface_MOD.h"
#include "../System/FileUtils.h"
#include "../System/FilePath.h"
#include "../Misc/StrProc.h"
#include "../ED_Common/TempAttributesTool.h"

#include "../SceneB2/TerraGen.h"
#include "../SceneB2/VisObjDesc.h"

#include "../System/VFSOperations.h"
#include "../libdb/ObjMan.h"
#include "../libdb/EditorDb.h"

#include <zconf.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




IManipulator* CreateModelManipulatorFromVisObj( IManipulator *pVisObjectManipulator, string *pModelName )
{
	int nModelCount = 0;
	if ( CManipulatorManager::GetValue( &nModelCount, pVisObjectManipulator, "Models" ) )
	{
		for ( int nModelIndex = 0; nModelIndex < nModelCount; ++nModelIndex )
		{
			const string szModelPath = StrFmt( "Models.[%d].", nModelIndex );
			string szSeason;
			if ( CManipulatorManager::GetValue( &szSeason, pVisObjectManipulator, szModelPath + "Season" ) &&
				( szSeason == NDB_DEFAULT_SEASON_MNEMONIC ) )
			{
				return CManipulatorManager::CreateManipulatorFromReference( szModelPath + "Model", pVisObjectManipulator, 0, pModelName, 0 );
				break;
			}
		}
	}
	return 0;
}


bool NormalizePassabilityOrigin( CVec2 *pvOrigin, const CTPoint<int> &rSize, const CVec3 &rvMin, const CVec3 &rvMax )
{
	NI_ASSERT( pvOrigin != 0, "NormalizePassabilityOrigin() pvOrigin == 0" );
	const CVec2 vSize( rSize.x * AI_TILE_SIZE * 1.0f, rSize.y * AI_TILE_SIZE * 1.0f );
	const CVec2 vRect0 = ( *pvOrigin ) * ( -1.0f );
	const CVec2 vRect1 = vRect0 + vSize;
	const CVec2 vDiff0( rvMin.x - vRect0.x, rvMin.y - vRect0.y );
	const CVec2 vDiff1( vRect1.x - rvMax.x, vRect1.y - rvMax.y );
	const CVec2 vDiff = ( vDiff0 + vDiff1 ) / 2.0f;
	( *pvOrigin ) += ( vDiff - vDiff0 );
	return true;
}


bool NormalizePassabilityArray( CArray2D<BYTE> *pDestination, CVec2 *pvOrigin )
{
	NI_ASSERT( pDestination != 0, "NormalizePassabilityArray() pDestination == 0" );
	CTPoint<int> startPoint( -1, -1 );
	CTPoint<int> finishPoint( 0, 0 );
	for ( int x = 0; x < pDestination->GetSizeX(); ++x )
	{
		for ( int y = 0; y < pDestination->GetSizeY(); ++y )
		{
			if ( ( *pDestination )[y][x] > 0 )
			{
				if ( finishPoint.x < x )
				{
					finishPoint.x = x;
				}
				//
				if ( finishPoint.y < y )
				{
					finishPoint.y = y;
				}
				//
				if ( startPoint.x < 0 )
				{
					startPoint.x = x;
				}
				else if ( startPoint.x > x )
				{
					startPoint.x = x;
				}
				//
				if ( startPoint.y < 0 )
				{
					startPoint.y = y;
				}
				else if ( startPoint.y > y )
				{
					startPoint.y = y;
				}
			}
		}
	}
	if ( ( startPoint.x >= 0 ) && ( startPoint.y >= 0 ) )
	{
		if ( ( startPoint.x != 0 ) ||
				 ( startPoint.y != 0 ) ||
				 ( finishPoint.x != ( pDestination->GetSizeX() - 1 ) ) ||
 				 ( finishPoint.x != ( pDestination->GetSizeY() - 1 ) ) )
		{
			CArray2D<BYTE> noEmptyArray;
			noEmptyArray.SetSizes( finishPoint.x - startPoint.x + 1, finishPoint.y - startPoint.y + 1 );
			for ( int x = startPoint.x; x <= finishPoint.x; ++x )
			{
				for ( int y = startPoint.y; y <= finishPoint.y; ++y )
				{
					noEmptyArray[y - startPoint.y][x - startPoint.x] = ( *pDestination )[y][x];
				}
			}
			( *pDestination ) = noEmptyArray;
		}
		pvOrigin->x -= ( startPoint.x * AI_TILE_SIZE * 1.0f );
		pvOrigin->y -= ( startPoint.y * AI_TILE_SIZE * 1.0f );
	}
	else
	{
		pDestination->Clear();
		pvOrigin->x = 0.0f;
		pvOrigin->y = 0.0f;
	}
	return true;
}


bool CreateObjectStaticDebris( const string &rszGrannyFileName, const string &rszImageFileName, CVec2 *pvOrigin, const int nSmoothRadius )
{
	NI_ASSERT( pvOrigin != 0, "CreateObjectPassability() pvOrigin = 0" );
	//
	if ( CPtr<ITerraManager> pTerraManager = MakeObject<ITerraManager>( ITerraManager::tidTypeID ) )
	{
		CArray2D<BYTE> imageArray;
		pTerraManager->CreateDebris( rszGrannyFileName, &imageArray, pvOrigin, NDebrisBuilder::MASK_STATIC, nSmoothRadius );
		CFileStream imageFileStream( NVFS::GetMainFileCreator(), rszImageFileName );
		return NImage::SaveAsTGA<BYTE>( imageArray, &imageFileStream );
	}
	return false;
}


bool CreateObjectDynamicDebris( const string &rszGrannyFileName, const string &rszImageFileName, CVec2 *pvOrigin, float fWidth )
{
	NI_ASSERT( pvOrigin != 0, "CreateObjectPassability() pvOrigin = 0" );
	//
	if ( CPtr<ITerraManager> pTerraManager = MakeObject<ITerraManager>( ITerraManager::tidTypeID ) )
	{
		CArray2D<BYTE> imageArray;
		pTerraManager->CreateDebris( rszGrannyFileName, &imageArray, pvOrigin, NDebrisBuilder::MASK_DYNAMIC, fWidth );
		CArray2D<DWORD> imageArray32;
		imageArray32.SetSizes( imageArray.GetSizeY(), imageArray.GetSizeX() );
		for ( int i = 0; i < imageArray.GetSizeX(); ++i )
		{
			for ( int j = 0; j < imageArray.GetSizeY(); ++j )
			{
				BYTE cData = imageArray[j][i];
				DWORD dwColor = cData << 24 | cData << 16 | cData << 8 | cData;
				imageArray32[i][j] = dwColor;
			}
		}
		CFileStream imageFileStream( NVFS::GetMainFileCreator(), rszImageFileName );
		return NImage::SaveAsTGA( imageArray32, &imageFileStream );
	}
	return false;
}


bool CreateObjectPassability( const string &rszGrannyFileName, CArray2D<BYTE> *pPassabilityArray, CVec2 *pvOrigin )
{
	NI_ASSERT( pPassabilityArray != 0, "CreateObjectPassability() pPassabilityArray = 0" );
	NI_ASSERT( pvOrigin != 0, "CreateObjectPassability() pvOrigin = 0" );
	//
	if ( CPtr<ITerraManager> pTerraManager = MakeObject<ITerraManager>( ITerraManager::tidTypeID ) )
	{
    pTerraManager->CreateDebris( rszGrannyFileName, pPassabilityArray, pvOrigin, NDebrisBuilder::MASK_AI_PASSABILITY, DEF_DEBRIS_SMOOTH_RADIUS );
		return true;
	}
	return false;
}


//
//
//
//
//							LOCATORS / MODEL SPECIAL POINTS
//
//
//
//

void GetModelLocators(	vector<SSkeletonLocatorInfo> *pLocatorsInfo, 
											IManipulator* pRPGStatsManipulator, 
											const char *pszModelSrcField )
{
	const char SKELETON_FIELD_IN_VISOBJ[] = "Skeleton";

	CPtr<IManipulator> pItVisObj = 
		CManipulatorManager::CreateManipulatorFromReference( pszModelSrcField, pRPGStatsManipulator, 0, 0, 0 );
	if ( !pItVisObj )
		return;

	CPtr<IManipulator> pItModel = CreateModelManipulatorFromVisObj( pItVisObj, 0 );
	if ( !pItModel )
		return;

	CPtr<IManipulator> pSkelManinpulator = 
		CManipulatorManager::CreateManipulatorFromReference( SKELETON_FIELD_IN_VISOBJ, pItModel, 0, 0, 0 );
	if ( !pSkelManinpulator )
		return;

	GetSkeletonLocatorsInfo( pLocatorsInfo, pSkelManinpulator->GetDBID() );
}

void GetSkeletonLocatorsInfo( vector<SSkeletonLocatorInfo> *pLocatorsInfo, const CDBID &dbidSkeleton )
{
	const char SKELETONS_BIN_PATH[] = "bin\\Skeletons\\";

	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	if ( !pUserData )
		return;
	const string szSkeletonsFolder = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + SKELETONS_BIN_PATH;


	// создаем аниматор для вычисления положения костей в мировых координатах
	NAnimation::SGrannySkeletonHandle handle;
	handle.nModelInFile = 0;
	handle.pSkeleton = 0;
	if ( CObj<NDb::IObjMan> pSkeletonObjMan = NDb::GetManipulator( dbidSkeleton ) )
		handle.pSkeleton = dynamic_cast<const NDb::SSkeleton *>( pSkeletonObjMan->GetObject() );
	//
	CObj<CCSTime> pCSTime = new CCSTime();
	pCSTime->Set( 0 );
	// 
	/*
	NAnimation::CGrannyFileInfo *pSkeletonFileInfo = NAnimation::GetSkeletonFileInfo( handle.nFileName )->GetValue();
	if ( !pSkeletonFileInfo || !GetSkeleton( pSkeletonFileInfo, handle.nModelInFile ) )
	{
		ILogger *pLogger = NLog::GetLogger();
		pLogger->Log( LT_ERROR, "Bad or missing skeleton file while reading locators info. Check 'RootJoint' field.\n" );
		pLogger->Log( LT_ERROR, StrFmt("\tFile name: %s%d\n", szSkeletonsFolder.c_str(), handle.nFileName) );
		return;
	}
	*/
	CDGPtr< NAnimation::ISkeletonAnimator > pAnimator(NAnimation::CreateSkeletonAnimator( handle, pCSTime ));
	if ( !pAnimator )
	{
		ILogger *pLogger = NLog::GetLogger();
		pLogger->Log( LT_ERROR, "Can't create animator from skeleton while reading locators info.\n" );
		pLogger->Log( LT_ERROR, StrFmt("\tFile name: %s:%s\n", szSkeletonsFolder.c_str(), NDb::GetResName(handle.pSkeleton)) );
		return;
	}
	//

	SHMatrix zeroPos = SHMatrix( CVec3(0.0f, 0.0f, 0.0f), CQuat( 0.0f, V3_AXIS_Z/*CVec3( 0.0f, 1.0f, 0.0f )*/ ) );
	pAnimator->SetGlobalPosition( zeroPos ); 
	pAnimator.Refresh(); // CSkeletonAnimator::Recalc()


	// читаем файл скелета для получения имен костей (локаторов)
	const NDb::SSkeleton *pDBSkeleton = NDb::Get<NDb::SSkeleton>( dbidSkeleton );
	string szFilePath = NBinResources::GetExistentBinaryFileName( szSkeletonsFolder, pDBSkeleton->GetRecordID(), pDBSkeleton->uid );

	WaitForFile( szFilePath, 10000 );
	granny_file *pFile = GrannyReadEntireFile( szFilePath.c_str() );
	if ( !pFile )
		return;

	granny_file_info *pInfo = GrannyGetFileInfo( pFile );				
	if ( !pInfo )
		return;

	hash_map<int,int> potentialParents;

	// вычисляем положение костей
	granny_skeleton *pSkeleton = pInfo->Skeletons[0];
	for ( int nBoneIndex = 0; nBoneIndex < pSkeleton->BoneCount; ++nBoneIndex )
	{
		granny_bone *pBone = &pSkeleton->Bones[nBoneIndex];

		CVec3 bonePos = VNULL3;
		SHMatrix mtx;
		if ( pAnimator->GetBonePosition( pBone->Name, &bonePos ) && 
			 pAnimator->GetBonePosition( pBone->Name, &mtx ) )
		{
			SSkeletonLocatorInfo inf;
			inf.szName = pBone->Name;
			Vis2AI( &bonePos );
			inf.vPos = bonePos;
			inf.mtx = mtx;
			inf.nParentIdx = pBone->ParentIndex;
			pLocatorsInfo->push_back( inf ); 
			potentialParents[nBoneIndex] = (pLocatorsInfo->size() - 1);
		}
	}

	// вычисляем иерархию локаторов
	for ( CLocatorInfoIter i = pLocatorsInfo->begin(); i != pLocatorsInfo->end(); ++i )
	{
		int nGrannyParentIdx = i->nParentIdx;
		if ( nGrannyParentIdx == -1 )
			i->nParentIdx = -1;
		else
			i->nParentIdx = potentialParents[nGrannyParentIdx];
	}

	GrannyFreeFile( pFile );
}

int PatMat(const char *raw,const char *pat)
{
	//
	// http://www.proglib.ru/
	//
	int  i, slraw;

	if ((*pat == '\0') && (*raw == '\0'))
		return( 1 ) ;
	if (*pat == '\0')
		return( 0 ) ;
	if (*pat == '*')
	{ 
		if (*(pat+1) == '\0')
			return( 1 ) ;
		for(i=0,slraw=strlen(raw);i<=slraw;i++)
			if ((*(raw+i) == *(pat+1)) ||
				(*(pat+1) == '?'))
				if (PatMat(raw+i+1,pat+2) == 1)
					return( 1 ) ;              
	}
	else
	{ 
		if (*raw == '\0')                    
			return( 0 ) ;                      
		if ((*pat == '?') || (*pat == *raw))
			if (PatMat(raw+1,pat+1) == 1)   
				return( 1 ) ;
	}
	return( 0 ) ;                       
}

bool IsNameMatchPattern( int *pQIdx, const char *pszName, const char *pszPattern )
{
	if ( !PatMat( pszName, pszPattern ) )	
		return false;

	const char *pp = pszName;
	
	while ( *pp && ! isdigit(*pp) )
		++pp;
	
	if ( *pp )
	{
		if ( pp[1] && isdigit(pp[1]) )
		{
			char pszIndex[3]={0};
			pszIndex[0] = pp[0];
			pszIndex[1] = pp[1];
			*pQIdx = atoi(pszIndex);
			return true;
		}
		else
		{
			++pp;
		}
	}

	//имя не содержит числового индекса
	*pQIdx = -1;
	return true;
}

void SearchLocators(	vector<SLocatorQInfo> *pQLocInfo, 
										const vector<SSkeletonLocatorInfo> &rLocatorsInfo,
										const char *pszLocatorNamePattern )
{
	pQLocInfo->clear();

	for ( CLocatorInfoConstIter i = rLocatorsInfo.begin(); i != rLocatorsInfo.end(); ++i )
	{
		int nQIdx;
		if ( !IsNameMatchPattern( &nQIdx, i->szName.c_str(), pszLocatorNamePattern ) )
			continue;

		SLocatorQInfo qinf;
		qinf.inf = *i;
		qinf.nQIdx = nQIdx;

		pQLocInfo->push_back( qinf ); 
	}

	sort( pQLocInfo->begin(), pQLocInfo->end() ); 
}

bool SearchLocator(	SSkeletonLocatorInfo *pLocInfo,
									 const vector<SSkeletonLocatorInfo> &rLocatorsInfo,
									 const char *pszLocatorName )
{
	for ( CLocatorInfoConstIter i = rLocatorsInfo.begin(); i != rLocatorsInfo.end(); ++i )
	{
		if ( i->szName != pszLocatorName )
			continue;

		*pLocInfo = *i;
		return true;
	}
	return false;
}

void SetPointValueForVec2Field(	IManipulator* pManipulator, 
															 const char *pszLocatorName,
															 const char *pszFieldName,
															 const vector<SSkeletonLocatorInfo> &rLocatorsInfo )
{
	SSkeletonLocatorInfo inf;
	if ( SearchLocator( &inf, rLocatorsInfo, pszLocatorName ) )
	{
		CVec2 vPoint = CVec2( inf.vPos.x, inf.vPos.y );
		CManipulatorManager::SetVec2( vPoint, pManipulator, pszFieldName ); 
	}
}

void SetPointValueForStringField(	IManipulator* pManipulator, 
																 const char *pszLocatorName,
																 const char *pszFieldName,
																 const vector<SSkeletonLocatorInfo> &rLocatorsInfo )
{
	SSkeletonLocatorInfo inf;
	if ( SearchLocator( &inf, rLocatorsInfo, pszLocatorName ) )
		pManipulator->SetValue( pszFieldName, inf.szName );
}

void SetPointsValuesForVec2Array(	IManipulator* pManipulator, 
																 const char *pszLocatorNamePattern,
																 const char *pszFieldName,
																 const vector<SSkeletonLocatorInfo> &rLocatorsInfo )
{
	pManipulator->RemoveNode( pszFieldName, NODE_REMOVEALL_INDEX );	

	vector<SLocatorQInfo> lc;
	SearchLocators( &lc, rLocatorsInfo, pszLocatorNamePattern ); 

	int nIdx = 0;
	for ( CLocatorQInfoConstIter i = lc.begin(); i != lc.end(); ++i, ++nIdx )
	{
		pManipulator->InsertNode( pszFieldName );

		char pszAddr[256] = {0};
		sprintf( pszAddr, "%s.[%d]", pszFieldName, nIdx );

		CVec2 vPoint = CVec2( i->inf.vPos.x, i->inf.vPos.y ); 
		CManipulatorManager::SetVec2( vPoint, pManipulator, pszAddr ); 
	}
}

void SetPointsValuesForStringArray(	IManipulator* pManipulator, 
																	 const char *pszLocatorNamePattern,
																	 const char *pszFieldName,
																	 const vector<SSkeletonLocatorInfo> &rLocatorsInfo )
{
	pManipulator->RemoveNode( pszFieldName, NODE_REMOVEALL_INDEX );	

	vector<SLocatorQInfo> lc;
	SearchLocators( &lc, rLocatorsInfo, pszLocatorNamePattern ); 

	int nIdx = 0;
	for ( CLocatorQInfoConstIter i = lc.begin(); i != lc.end(); ++i, ++nIdx )
	{
		pManipulator->InsertNode( pszFieldName );

		char pszAddr[256] = {0};
		sprintf( pszAddr, "%s.[%d]", pszFieldName, nIdx );

		pManipulator->SetValue( pszAddr, i->inf.szName );
	}
}

void SetPointsValuesForVec2StructArray(	IManipulator* pManipulator, 
																			 const char *pszLocatorNamePattern,
																			 const char *pszArrayName,
																			 const char *pszStructFieldName,
																			 const vector<SSkeletonLocatorInfo> &rLocatorsInfo )
{
	// есть массив структур
	// ф-ция устанавливает значение точек для поля типа Vec2 в структуре
	// если точек больше чем элементов в массиве будет добавлено нужное число пустых структур

	vector<SLocatorQInfo> lc;
	SearchLocators( &lc, rLocatorsInfo, pszLocatorNamePattern ); 

	CVariant v;
	pManipulator->GetValue( pszArrayName, &v );
	int nArraySize = v;
	for ( int i = 0; i < (nArraySize - lc.size()); ++i )
		pManipulator->InsertNode( pszArrayName );

	int nIdx = 0;
	for ( CLocatorQInfoConstIter i = lc.begin(); i != lc.end(); ++i, ++nIdx )
	{
		char pszDBA[256] = {0};
		sprintf( pszDBA, "%s.[%d].%s", pszArrayName, nIdx, pszStructFieldName );

		CVec2 vPoint = CVec2( i->inf.vPos.x, i->inf.vPos.y ); 
		CManipulatorManager::SetVec2( vPoint, pManipulator, pszDBA ); 
	}
}

void SetPointsValuesForVec3StructArray(	IManipulator* pManipulator, 
																			 const char *pszLocatorNamePattern,
																			 const char *pszArrayName,
																			 const char *pszStructFieldName,
																			 const vector<SSkeletonLocatorInfo> &rLocatorsInfo,
																			 bool bClear )
{
	// есть массив структур
	// ф-ция устанавливает значение точек для поля типа Vec3 в структуре
	// если точек больше чем элементов в массиве будет добавлено нужное число пустых структур
	// если bClear, то все уже существующие элементы массива будут удалены

	vector<SLocatorQInfo> lc;
	SearchLocators( &lc, rLocatorsInfo, pszLocatorNamePattern ); 

	if ( bClear )
	{
		pManipulator->RemoveNode( pszArrayName, NODE_REMOVEALL_INDEX );	
		for ( int i = 0; i < lc.size(); ++i )
			pManipulator->InsertNode( pszArrayName );
	}
	else
	{
		CVariant v;
		pManipulator->GetValue( pszArrayName, &v );
		int nArraySize = v;
		for ( int i = 0; i < (lc.size() - nArraySize); ++i )
			pManipulator->InsertNode( pszArrayName );
	}

	int nIdx = 0;
	for ( CLocatorQInfoConstIter i = lc.begin(); i != lc.end(); ++i, ++nIdx )
	{
		char pszDBA[256] = {0};
		sprintf( pszDBA, "%s.[%d].%s", pszArrayName, nIdx, pszStructFieldName );

		CManipulatorManager::SetVec3( i->inf.vPos, pManipulator, pszDBA ); 
	}
}

void GetPassability( CArray2D<BYTE> *pPassability, IManipulator *pBuildingRPGStatsManipulator )
{
	CVariant v;
	pBuildingRPGStatsManipulator->GetValue( "passability.data", &v ); 
	int nSizeY = v;
	int nMaxSizeX = 0;
	vector< vector<int> > data;
	data.resize( nSizeY );
	for ( int y = 0; y < nSizeY; ++y )
	{
		char pszDBARow[64]={0};
		sprintf( pszDBARow, "passability.data.[%d].data", y );
		CVariant vElemNum;
		if ( ! pBuildingRPGStatsManipulator->GetValue( pszDBARow, &vElemNum ) )
			continue;
		int nElemNum = vElemNum;
		if ( nElemNum == 0 )
			continue;
		if ( nElemNum > nMaxSizeX )
			nMaxSizeX = nElemNum;
		data[y].resize( nElemNum );
		for ( int x = 0; x < nElemNum; ++x )
		{
			char pszDBAElem[64]={0};
			sprintf( pszDBAElem, "passability.data.[%d].data.[%d]", y, x );
			CVariant elem;
			if ( ! pBuildingRPGStatsManipulator->GetValue( pszDBAElem, &elem ) )
			{
				data[y][x] = -1;
			}
			else
			{
				data[y][x] = elem;
			}
		}
	}

	pPassability->SetSizes( nMaxSizeX, nSizeY );
	for ( int y = 0; y < nSizeY; ++y )
	{
		int nVecRealSize = data[y].size();
		for ( int x = 0; x < nMaxSizeX; ++x )
		{
			if ( x >= nVecRealSize )
				(*pPassability)[y][x] = -1;
			else
				(*pPassability)[y][x] = data[y][x];
		}
	}
}

static const int FindLocator( vector<SLocatorQInfo> &lc, const string &szName )
{
	for ( int i = 0; i < lc.size(); ++i )
	{
		if ( lc[i].inf.szName == szName )
			return i;
	}

	return -1;
}

static void FixLocators( IManipulator *pManipulator, const string &szLocatorNamePattern, const string &szArrayName, const vector<SSkeletonLocatorInfo> &rLocatorsInfo )
{
	vector<SLocatorQInfo> lc;
	SearchLocators( &lc, rLocatorsInfo, szLocatorNamePattern.c_str() ); 
	vector<bool> locatorFound( lc.size(), false );
	int nLocators = 0;

	CVariant v;
	if ( pManipulator->GetValue( szArrayName, &v ) )
	{
		const int nArraySize = v;

		list<int> nodesToRemove;
		for ( int i = 0; i < nArraySize; ++i )
		{
			CVariant vLocatorName;
			const string szNodeName = szArrayName + StrFmt( ".[%d].Locator", i );

			if ( pManipulator->GetValue( szNodeName, &vLocatorName ) )
			{
				int k = FindLocator( lc, string(vLocatorName.GetStr()) );
				if ( k == -1 || locatorFound[k] )
					nodesToRemove.push_front( i );
				else
				{
					locatorFound[k] = true;
					++nLocators;

					const string szVecNodeName = szArrayName + StrFmt( ".[%d].AIRelativePos", i );
					CManipulatorManager::SetVec3( lc[k].inf.vPos, pManipulator, szVecNodeName );
				}
			}
			else
			{
				ILogger *pLogger = NLog::GetLogger();
				pLogger->Log( LT_ERROR, StrFmt("Can't find manipulator value: %s\n", szNodeName.c_str()) );
				return;
			}
		}

		for ( list<int>::iterator iter = nodesToRemove.begin(); iter != nodesToRemove.end(); ++iter )
			pManipulator->RemoveNode( szArrayName, *iter );

		for ( int i = 0; i < lc.size(); ++i )
		{
			if ( !locatorFound[i] )
			{
				pManipulator->InsertNode( szArrayName );
				const string szLocatorNodeName = szArrayName + StrFmt( ".[%d].Locator", nLocators );
				pManipulator->SetValue( szLocatorNodeName, CVariant( lc[i].inf.szName ) );

				const string szVecNodeName = szArrayName + StrFmt( ".[%d].AIRelativePos", nLocators );
				CManipulatorManager::SetVec3( lc[i].inf.vPos, pManipulator, szVecNodeName );

				++nLocators;
			}
		}
	}
	else
	{
		ILogger *pLogger = NLog::GetLogger();
		pLogger->Log( LT_ERROR, StrFmt("Can't find manipulator value: %s\n", szArrayName.c_str()) );
		return;
	}
}

bool FixLocators( const SObjectSet &objectSet, const string &szLocatorNamePattern, const string &szArrayName )
{
	for ( CObjectNameSet::const_iterator itObjectName = objectSet.objectNameSet.begin(); itObjectName != objectSet.objectNameSet.end(); ++itObjectName )
	{
		// Получаем манипулятор на объект
		const string szCurObjectTypeName = objectSet.szObjectTypeName;
		CPtr<IManipulator> pManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( szCurObjectTypeName, itObjectName->first );
		if ( !pManipulator )
			return false;

		// Получаем манипулятор на VisObject
		CPtr<IManipulator> pVisObjectManipulator = CManipulatorManager::CreateManipulatorFromReference( "visualObject", pManipulator, 0, 0, 0 );
		if ( pVisObjectManipulator )
		{
			vector<SSkeletonLocatorInfo> visualObjectLocators;
			GetModelLocators( &visualObjectLocators, pManipulator, "visualObject" );

			FixLocators( pManipulator, szLocatorNamePattern, szArrayName, visualObjectLocators );
		}
	}

	return true;
}

float GetLocatorDirection( const SSkeletonLocatorInfo *pLocInfo, bool bGetInRadian )
{
	float fDirection = 0.0f;
	SHMatrix mtx = pLocInfo->mtx;
	CVec3 vec;   
	mtx.RotateVector( &vec, V3_AXIS_Z ); 
	float fAngle = atan2( vec.y, vec.x ); // -pi...+pi

	// угол возвращается в AI системе отчсчета (отсчитывается против часовой стрелки от (0,1,0))
	if ( fAngle < 0 ) // 0..2pi
		fAngle += FP_2PI;

	if ( fAngle <= FP_PI2 )
	{
		const float F_3PI2 = 3 * FP_PI2;
		fAngle += F_3PI2;
	}
	else if ( fAngle > FP_PI2 )
	{
		fAngle -= FP_PI2;
	}

	if ( bGetInRadian )
		return fAngle;

	// в AI еденицах
	const float F_EPS_ANGLE = 0.01f;
	if ( fabs(fAngle) < F_EPS_ANGLE )
	{
		fDirection = 0xFFFF;
	}
	else
	{
		fDirection = (fAngle / FP_2PI)*0xFFFF;
	}

	return fDirection;			
}

// Acquire attributes for a given model
bool GetGeometryAttributes( IManipulator* pGeomMan, CGrannyBoneAttributesList *pAttributeList )
{
	return ReadAttributes( pAttributeList, NMEGeomAttribs::GetAttribsByGeometry(pGeomMan), "", true );
}

namespace NExportFilesList
{
	struct SEntry
	{
		string szFileName;
		//
		int operator&( IXmlSaver &saver )
		{
			saver.Add( "FileName", &szFileName );
			//
			if ( saver.IsReading() )
			{
				if ( NFile::GetFileExt(szFileName).empty() )
					szFileName += ".bik";
			}
			//
			return 0;
		}
	};
	//
	bool LoadFilesList( vector<SEntry> *pFiles, const string &szFileName, const char *pszBase )
	{
		CFileStream stream( NVFS::GetMainVFS(), szFileName );
		CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ );
		if ( pSaver == 0 )
			return false;
		pSaver->Add( pszBase, pFiles );
		return true;
	}
};

bool ExportFilesList( const string &szFilesListFileName, bool bForce, const char *pszBase )
{
	ILogger *pLogger = NLog::GetLogger();
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	if ( CheckFilesUpdated(pUserData->constUserData.szExportSourceFolder + szFilesListFileName, 
		                     Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + szFilesListFileName, bForce) == false )
	{
		// first, copy XML file
		if ( NFile::CopyFile( pUserData->constUserData.szExportSourceFolder + szFilesListFileName, 
			                    Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + szFilesListFileName ) == false )
		{
			pLogger->Log( LT_ERROR, "Can't copy files list\n" );
			pLogger->Log( LT_ERROR, StrFmt("\tFiles list: %s\n", szFilesListFileName.c_str()) );
			return false;
		}
		// then, load XML file and copy each movie
		vector<NExportFilesList::SEntry> files;
		if ( NExportFilesList::LoadFilesList(&files, pUserData->constUserData.szExportSourceFolder + szFilesListFileName, pszBase) != false )
		{
			for ( vector<NExportFilesList::SEntry>::const_iterator it = files.begin(); it != files.end(); ++it )
			{
				const string szSrcFileName = pUserData->constUserData.szExportSourceFolder + it->szFileName;
				const string szDstFileName = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + it->szFileName;
				if ( CheckFilesUpdated(szSrcFileName, szDstFileName, bForce) == false )
				{
					if ( NFile::CopyFile(szSrcFileName, szDstFileName) == false )
					{
						pLogger->Log( LT_ERROR, "Can't copy file\n" );
						pLogger->Log( LT_ERROR, StrFmt("\tFile: %s\n", it->szFileName.c_str()) );
					}
				}
			}
		}
		else
		{
			pLogger->Log( LT_ERROR, "Can't load files list to export\n" );
			pLogger->Log( LT_ERROR, StrFmt("\tFiles list: %s\n", szFilesListFileName.c_str()) );
			return false;
		}
	}
	return true;
}


