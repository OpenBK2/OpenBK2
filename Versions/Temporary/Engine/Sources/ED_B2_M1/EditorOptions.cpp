#include "StdAfx.h"
#include "EditorOptions.h"

#include "..\MapEditorLib\Interface_UserData.h"
#include "..\Misc\StrProc.h"
#include "../libdb/ResourceManager.h"
#include "..\MapEditorLib\ManipulatorManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace NEditorOptions
{

static const char MAIN_EDITOR_OPTIONS_TYPE_NAME[] = "EditorOptions";

// ************************************************************************************************************************ //
// **
// ** editor options manipulator
// **
// **
// **
// ************************************************************************************************************************ //

IManipulator* CreateOptionsManipulator()
{
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	return Singleton<IResourceManager>()->CreateObjectManipulator( MAIN_EDITOR_OPTIONS_TYPE_NAME, pUserData->dbidMainOptions );
}

// ************************************************************************************************************************ //
// **
// ** data retrieving
// **
// **
// **
// ************************************************************************************************************************ //

int FindBySeason( const string &szDesiredSeason, const string &szName, IManipulator *pOptsMan )
{
	int nAmount = 0;
	CManipulatorManager::GetValue( &nAmount, pOptsMan, szName );
	if ( nAmount == 0 ) 
		return -1;
	//
	if ( szDesiredSeason == "SEASON_ANY" ) 
		return 0;
	//
	for ( int i = 0; i < nAmount; ++i ) 
	{
		string szSeason;
		if ( CManipulatorManager::GetValue( &szSeason, pOptsMan, StrFmt("%s.[%d].Season", szName.c_str(), i) ) &&
			   szDesiredSeason == szSeason ) 
		{
			return i;
		}
	}
	return -1;
}

pair<int, int> FindBySeasonAndTime( const string &szDesiredSeason, const string &szDesiredTime, const string &szName, IManipulator *pOptsMan )
{
	const int nSeason = FindBySeason( szDesiredSeason, szName, pOptsMan );
	if ( nSeason == -1 ) 
		return pair<int, int>( -1, -1 );
	const string szAddName = StrFmt( "%s.[%d].DayTimes", szName.c_str(), nSeason );
		//
	int nAmount = 0;
	CManipulatorManager::GetValue( &nAmount, pOptsMan, szAddName );
	if ( nAmount == 0 ) 
		return pair<int, int>( nSeason, -1 );
	//
	if ( szDesiredTime == "DAY_ANY" ) 
		return pair<int, int>( nSeason, 0 );
	//
	for ( int i = 0; i < nAmount; ++i ) 
	{
		string szDayTime;
		if ( CManipulatorManager::GetValue( &szDayTime, pOptsMan, StrFmt("%s.[%d].DayTime", szAddName.c_str(), i) ) &&
			szDesiredTime == szDayTime ) 
		{
			return pair<int, int>( nSeason, i );
		}
	}
	return pair<int, int>( nSeason, -1 );
}

template <class TYPE>
bool GetFromOptionsForSeason( TYPE *pRes, const string &szPreName, const string &szPostName, 
															const string &szDesiredSeason, IManipulator *pOptsMan )
{
	const int nIndex = FindBySeason( szDesiredSeason, szPreName, pOptsMan );
	if ( nIndex == -1 ) 
		return false;
	if ( CManipulatorManager::GetValue( pRes, pOptsMan, StrFmt("%s.[%d].%s", szPreName.c_str(), nIndex, szPostName.c_str()) ) )
		return true;
	return false;
}

template <class TYPE>
bool GetFromOptionsForSeasonAndTime( TYPE *pRes, const string &szPreName, const string &szPostName, 
														         const string &szDesiredSeason, const string &szDesiredTime, 
																		 IManipulator *pOptsMan )
{
	const pair<int, int> seasonTime = FindBySeasonAndTime( szDesiredSeason, szDesiredTime, szPreName, pOptsMan );
	if ( seasonTime.first == -1 || seasonTime.second == -1 ) 
		return false;
	if ( CManipulatorManager::GetValue( pRes, pOptsMan, StrFmt("%s.[%d].DayTimes.[%d].%s", szPreName.c_str(), seasonTime.first, seasonTime.second, szPostName.c_str()) ) )
		return true;
	return false;
}

template <class TYPE>
bool GetFromOptions( TYPE *pRes, const string &szPreName, const string &szPostName, const string &szDesiredSeason )
{
	if ( CPtr<IManipulator> pOptsMan = CreateOptionsManipulator() ) 
	{
		if ( GetFromOptionsForSeason( pRes, szPreName, szPostName, szDesiredSeason, pOptsMan ) == true )
			return true;
		if ( GetFromOptionsForSeason( pRes, szPreName, szPostName, "SEASON_SUMMER", pOptsMan ) == true )
			return true;
		return GetFromOptionsForSeason( pRes, szPreName, szPostName, "SEASON_ANY", pOptsMan );
	}
	return false;
}

template <class TYPE>
bool GetFromOptions( TYPE *pRes, const string &szPreName, const string &szPostName, 
										 const string &szDesiredSeason, const string &szDesiredTime )
{
	if ( CPtr<IManipulator> pOptsMan = CreateOptionsManipulator() ) 
	{
		if ( GetFromOptionsForSeasonAndTime( pRes, szPreName, szPostName, szDesiredSeason, szDesiredTime, pOptsMan ) == true )
			return true;
		if ( GetFromOptionsForSeasonAndTime( pRes, szPreName, szPostName, "SEASON_SUMMER", szDesiredTime, pOptsMan ) == true )
			return true;
		return GetFromOptionsForSeasonAndTime( pRes, szPreName, szPostName, "SEASON_ANY", "DAY_ANY", pOptsMan );
	}
	return false;
}

string GetStringFromOptions( const string &szPreName, const string &szPostName, 
														 const string &szDesiredSeason, const string &szDesiredDayTime )
{
	string szValue;
	if ( GetFromOptions( &szValue, szPreName, szPostName, szDesiredSeason, szDesiredDayTime ) )
		return szValue;
	return "";
}

string GetStringFromOptions( const string &szPreName, const string &szPostName, const string &szDesiredSeason )
{
	string szValue;
	if ( GetFromOptions( &szValue, szPreName, szPostName, szDesiredSeason ) )
		return szValue;
	return "";
}

int GetIntFromOptions( const string &szPreName, const string &szPostName, const string &szDesiredSeason )
{
	int nValue;
	if ( GetFromOptions( &nValue, szPreName, szPostName, szDesiredSeason ) )
		return nValue;
	return 0;
}

float GetFloatFromOptions( const string &szPreName, const string &szPostName, const string &szDesiredSeason )
{
	float fValue;
	if ( GetFromOptions( &fValue, szPreName, szPostName, szDesiredSeason ) )
		return fValue;
	return 0.0f;
}

CVec2 GetVec2FromOptions( const string &szPreName, const string &szPostName, const string &szDesiredSeason )
{
	CVec2 vValue;
	if ( GetFromOptions( &vValue, szPreName, szPostName, szDesiredSeason ) )
		return vValue;
	return VNULL2;
}

CVec3 GetVec3FromOptions( const string &szPreName, const string &szPostName, const string &szDesiredSeason )
{
	CVec3 vValue;
	if ( GetFromOptions( &vValue, szPreName, szPostName, szDesiredSeason ) )
		return vValue;
	return VNULL3;
}

// ************************************************************************************************************************ //
// **
// ** MapInfo data
// **
// **
// **
// ************************************************************************************************************************ //

string GetPeakMaskTexture( const string &szDesiredSeason )
{
	return GetStringFromOptions( "MapInfo", "PeakMask", szDesiredSeason );
}

string GetTileset( const string &szDesiredSeason )
{
	return GetStringFromOptions( "MapInfo", "Tileset", szDesiredSeason );
}

string GetOceanWater( const string &szDesiredSeason )
{
	return GetStringFromOptions( "MapInfo", "OceanWater", szDesiredSeason );
}

string GetMinimap( const string &szDesiredSeason )
{
	return GetStringFromOptions( "MapInfo", "Minimap", szDesiredSeason );
}

string GetLight( const string &szDesiredSeason, const string &szDayTime )
{
	return GetStringFromOptions( "MapInfo", "Light", szDesiredSeason, szDayTime );
}

string GetPreLight( const string &szDesiredSeason, const string &szDayTime )
{
	return GetStringFromOptions( "MapInfo", "PreLight", szDesiredSeason, szDayTime );
}

// ************************************************************************************************************************ //
// **
// ** backgrounds
// **
// **
// **
// ************************************************************************************************************************ //

string GetBgMap( const string &szDesiredSeason )
{
	return GetStringFromOptions( "Backgrounds", "Map", szDesiredSeason );
}

CVec3 GetBgMapAnchor( const string &szDesiredSeason )
{
	return GetVec3FromOptions( "Backgrounds", "Anchor", szDesiredSeason );
}

string GetMiscString( const string &szName )
{
	CPtr<IManipulator> pMan = CreateOptionsManipulator();
	string szValue;
	if ( CManipulatorManager::GetValue(&szValue, pMan, "Misc." + szName) )
		return szValue;
	else
		return "";
}

}

