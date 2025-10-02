#pragma once

#include "../misc/2darray.h"
#include "../zlib/zconf.h"
#include "../stats_b2_m1/rpgstats.h"
#include "../MapEditorLib/CommonExporterMethods.h"
#include "../ED_Common/Tools_Granny.h"

namespace NDb
{
	struct SPassProfile;
}

#define ANIMATIONS_ROOT_JOINT "Animations"

bool CreateObjectStaticDebris( const string &rszGrannyFileName, const string &rszImageFileName, CVec2 *pvOrigin, const int nSmoothRadius );
bool CreateObjectDynamicDebris( const string &rszGrannyFileName, const string &rszImageFileName, CVec2 *pvOrigin, float fWidth );
bool CreateObjectPassability( const string &rszGrannyFileName, CArray2D<BYTE> *pPassabilityArray, CVec2 *pvOrigin );
bool CreateObjectPassabilityProfile( const string &szGrannyFileName, const float fZEps, NDb::SPassProfile *pPassProfile );
void SavePassProfile( const NDb::SPassProfile &profile, const string &szPrefix, const string &szFieldName, IManipulator *pManipulator );

// Правильное расположение bounding box
bool NormalizePassabilityOrigin( CVec2 *pvOrigin, const CTPoint<int> &rSize, const CVec3 &rvMin, const CVec3 &rvMax );
// Правильные размеры bounding box
bool NormalizePassabilityArray( CArray2D<BYTE> *pDestination, CVec2 *pvOrigin );


//
//
//
//
//							LOCATORS / MODEL SPECIAL POINTS
//
//
//
//

struct SSkeletonLocatorInfo
{
	string szName;
	int nParentIdx;
	CVec3 vPos;
	//
	SHMatrix mtx;
	//
};
struct SLocatorQInfo
{
	SSkeletonLocatorInfo inf;
	int nQIdx;

	bool operator<( const SLocatorQInfo &rOther ) const
	{
		return nQIdx < rOther.nQIdx; 
	}
};
typedef vector<SSkeletonLocatorInfo>::const_iterator CLocatorInfoConstIter; 
typedef vector<SSkeletonLocatorInfo>::iterator CLocatorInfoIter; 
typedef vector<SLocatorQInfo>::const_iterator CLocatorQInfoConstIter; 
struct IManipulator;
void GetModelLocators(	vector<SSkeletonLocatorInfo> *pLocatorsInfo, 
						IManipulator* pRPGStatsManipulator, 
						const char *pszModelSrcField );
void GetSkeletonLocatorsInfo( vector<SSkeletonLocatorInfo> *pLocatorsInfo, const CDBID &dbidSkeleton );
int PatMat(const char *raw,const char *pat);
bool IsNameMatchPattern( int *pQIdx, const char *pszName, const char *pszPattern );
void SearchLocators(	vector<SLocatorQInfo> *pQLocInfo, 
						const vector<SSkeletonLocatorInfo> &rLocatorsInfo,
						const char *pszLocatorNamePattern );
bool SearchLocator(	SSkeletonLocatorInfo *pLocInfo,
					const vector<SSkeletonLocatorInfo> &rLocatorsInfo,
					const char *pszLocatorName );
float GetLocatorDirection( const SSkeletonLocatorInfo *pLocInfo, bool bGetInRadian );
void SetPointValueForVec2Field(	IManipulator* pManipulator, 
								const char *pszLocatorName,
								const char *pszFieldName,
								const vector<SSkeletonLocatorInfo> &rLocatorsInfo );
void SetPointValueForStringField(	IManipulator* pManipulator, 
									const char *pszLocatorName,
									const char *pszFieldName,
									const vector<SSkeletonLocatorInfo> &rLocatorsInfo );
void SetPointsValuesForVec2Array(	IManipulator* pManipulator, 
									const char *pszLocatorNamePattern,
									const char *pszFieldName,
									const vector<SSkeletonLocatorInfo> &rLocatorsInfo );
void SetPointsValuesForStringArray(	IManipulator* pManipulator, 
									const char *pszLocatorNamePattern,
									const char *pszFieldName,
									const vector<SSkeletonLocatorInfo> &rLocatorsInfo );
void SetPointsValuesForVec2StructArray(	IManipulator* pManipulator, 
										const char *pszLocatorNamePattern,
										const char *pszArrayName,
										const char *pszStructFieldName,
										const vector<SSkeletonLocatorInfo> &rLocatorsInfo );
void SetPointsValuesForVec3StructArray(	IManipulator* pManipulator, 
										const char *pszLocatorNamePattern,
										const char *pszArrayName,
										const char *pszStructFieldName,
										const vector<SSkeletonLocatorInfo> &rLocatorsInfo,
										bool bClear );
//
void GetPassability( CArray2D<BYTE> *pPassability, IManipulator *pBuildingRPGStatsManipulator );
//
// fix locators of object set
bool FixLocators( const struct SObjectSet &objectSet, const string &szLocatorNamePattern, const string &szArrayName );

// Acquire attributes for a given model
bool GetGeometryAttributes( IManipulator* pGeomMan, CGrannyBoneAttributesList *pAttributeList );

struct IManipulator* CreateModelManipulatorFromVisObj( struct IManipulator *pVisObjectManipulator, string *pModelName );

template<class TValue> 
bool GetSeasonedValue( TValue *pData, struct IManipulator *pManipulator, const string &rszName, const string &rszSeasonArrayName, NDb::ESeason eSeason, string *pszDataPrefix )
{
	NI_ASSERT( pData != 0, "CManipulatorManager::GetValue(): pData == 0" );
	NI_ASSERT( pManipulator != 0, "CManipulatorManager::GetValue(): pManipulator == 0" );
	int nSeasonCount = 0;
	bool bResult = CManipulatorManager::GetValue( &nSeasonCount, pManipulator, rszSeasonArrayName );
	if ( bResult )
	{
		bool bSeasonMissed = true;
		string szDataPrefix;
		do
		{
			for ( int nSeasonIndex = 0; nSeasonIndex < nSeasonCount; ++nSeasonIndex )
			{
				szDataPrefix = StrFmt( "%s%c%c%d%c", rszSeasonArrayName.c_str(), LEVEL_SEPARATOR_CHAR, ARRAY_NODE_START_CHAR, nSeasonIndex, ARRAY_NODE_END_CHAR );
				string szSeasonName;
				bResult = bResult && CManipulatorManager::GetValue( &szSeasonName, pManipulator, szDataPrefix + LEVEL_SEPARATOR_CHAR + "Season" );
				if ( !bResult )
					break;
				if ( eSeason == typeSeasonMnemonics.GetValue( szSeasonName ) )
				{
					bSeasonMissed = false;
					break;
				}
			}
			//
			if ( !bResult )
				break;
			//
			if ( bSeasonMissed )
				eSeason = NDB_DEFAULT_SEASON;
			else
				break;
		} while( eSeason != NDB_DEFAULT_SEASON );
		bResult = bResult && ( !bSeasonMissed );
		bResult = bResult && CManipulatorManager::GetValue( pData, pManipulator, szDataPrefix + LEVEL_SEPARATOR_CHAR + rszName );
		if ( bResult && ( pszDataPrefix != 0 ) )
			( *pszDataPrefix ) = szDataPrefix;
	}
	return bResult;
}

bool ExportFilesList( const string &szFilesListFileName, bool bForce, const char *pszBase );


