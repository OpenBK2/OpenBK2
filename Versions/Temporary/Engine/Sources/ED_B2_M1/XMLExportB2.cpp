#include "StdAfx.h"
#include "XMLExport.h"
#include "../libdb/ResourceManager.h"
#include "../Misc/StrProc.h"

namespace NXMLExport
{

void MakePrefixAndPostfix( string *pszPrefix, string *pszPostfix, const string &szTypeName )
{
	if ( szTypeName == "MapInfo" )
	{
		*pszPrefix = "";
		*pszPostfix = '/' + szTypeName + ".xdb";
	}
	else if ( szTypeName == "MechUnitRPGStats" || szTypeName == "InfantryRPGStats" )
	{
		*pszPrefix = "";
		*pszPostfix = '/' + szTypeName + ".xdb";
	}
	else if ( szTypeName == "SquadRPGStats" )
	{
		*pszPrefix = "";
		*pszPostfix = '/' + szTypeName + ".xdb";
	}
	else if ( szTypeName == "ObjectRPGStats" )
	{
		*pszPrefix = "";
		*pszPostfix = '/' + szTypeName + ".xdb";
	}
	else if ( szTypeName == "FenceRPGStats" )
	{
		*pszPrefix = "";
		*pszPostfix = '/' + szTypeName + ".xdb";
	}
	else if ( szTypeName == "BridgeRPGStats" )
	{
		*pszPrefix = "";
		*pszPostfix = '/' + szTypeName + ".xdb";
	}
	else if ( szTypeName == "BuildingRPGStats" )
	{
		*pszPrefix = "";
		*pszPostfix = '/' + szTypeName + ".xdb";
	}
	else if ( szTypeName == "TerrainSpotDesc" )
	{
		*pszPrefix = "";
		*pszPostfix = '/' + szTypeName + ".xdb";
	}
	else if ( szTypeName == "EntrenchmentRPGStats" )
	{
		*pszPrefix = "";
		*pszPostfix = '/' + szTypeName + ".xdb";
	}
	else if ( szTypeName == "WeaponRPGStats" )
	{
		*pszPrefix = "";
		*pszPostfix = '/' + szTypeName + ".xdb";
	}
	else if ( szTypeName == "MineRPGStats" )
	{
		*pszPrefix = "Mines/";
		*pszPostfix = '/' + szTypeName + ".xdb";
	}
	else if ( ( szTypeName == "ComplexSoundDesc" ) || 
		( szTypeName == "Composition" ) ||
		( szTypeName == "MusicTrack" ) || 
		( szTypeName == "PlayList" ) ||
		( szTypeName == "PlayTime" ) ||
		( szTypeName == "SoundDesc" ) ||
		( szTypeName == "MapMusic" ) )
	{
		*pszPrefix = "SoundAndMusic/";
		*pszPostfix = '_' + szTypeName + ".xdb";
	}
	else if ( ( szTypeName == "AIGameConsts" ) || 
		( szTypeName == "CameraLimits" ) ||
		( szTypeName == "ClientGameConsts" ) || 
		( szTypeName == "GameConsts" ) ||
		( szTypeName == "MultiplayerConsts" ) ||
		( szTypeName == "NetGameConsts" ) ||
		( szTypeName == "SceneConsts" ) ||
		( szTypeName == "UIConstsB2" ) )
	{
		*pszPrefix = "Consts/";
		*pszPostfix = '_' + szTypeName + ".xdb";
	}
	else if ( ( szTypeName.size() > 6 && szTypeName.compare(0, 6, "Window") == 0 ) ||
		( szTypeName.size() > 2 && szTypeName.compare(0, 2, "AR") == 0 ) ||
		( szTypeName.size() > 2 && szTypeName.compare(0, 2, "UI") == 0 ) ||
		( szTypeName == "BackgroundSimpleScallingTexture" ) ||
		( szTypeName == "BackgroundSimpleTexture" ) ||
		( szTypeName == "BackgroundTiledTexture" ) ||
		( szTypeName == "BackgroundFrameSequence" ) ||
		( szTypeName == "CheckIsTabActive" ) ||
		( szTypeName == "CheckIsWindowEnabled" ) ||
		( szTypeName == "CheckIsWindowVisible" ) ||
		( szTypeName == "CheckPreprogrammed" ) ||
		( szTypeName == "ForegroundTextString" ) ||
		( szTypeName == "ForegroundTextStringShared" ) ||
		( szTypeName == "TextFormat" ) ||
		( szTypeName == "TooltipContext" ) || 
		( szTypeName == "MessageReactionComplex" )  )
	{
		*pszPrefix = "UI/";
		*pszPostfix = '_' + szTypeName + ".xdb";
	}
	// SCENE{
	else if ( ( szTypeName == "AnimLight" ) || 
		( szTypeName == "LightInstance" ) ||
		( szTypeName == "Particle" ) || 
		( szTypeName == "ParticleInstance" ) ||
		( szTypeName == "ComplexEffect" ) ||
		( szTypeName == "ComplexSeasonedEffect" ) ||
		( szTypeName == "Effect" ) )
	{
		*pszPrefix = "Scene/Effects/";
		*pszPostfix = '_' + szTypeName + ".xdb";
	}
	else if ( ( szTypeName == "TwoSidedLight" ) ||
		( szTypeName == "AmbientLight" ) )
	{
		*pszPrefix = "Scene/Lights/";
		*pszPostfix = '_' + szTypeName + ".xdb";
	}
	else if ( ( szTypeName == "Material" ) || 
		        ( szTypeName == "Texture" ) ||
						( szTypeName == "CubeTexture" ) )
	{
		*pszPrefix = "Scene/TexAndMats/";
		*pszPostfix = '_' + szTypeName + ".xdb";
	}
	else if ( ( szTypeName == "AIGeometry" ) || 
		( szTypeName == "Geometry" ) ||
		( szTypeName == "Skeleton" ) ||
		( szTypeName == "Model" ) ||
		( szTypeName == "AnimB2" ) ||
		( szTypeName == "VisObj" ) )
	{
		*pszPrefix = "Scene/Geoms/";
		*pszPostfix = '_' + szTypeName + ".xdb";
	}
	// SCENE}
	else if ( ( szTypeName == "Campaign" ) ||
		( szTypeName == "Chapter" ) ||
		( szTypeName == "ChapterBonus" ) ||
		( szTypeName == "PartyDependentInfo" ) ||
		( szTypeName == "PlayerRank" ) ||
		( szTypeName == "DifficultyLevel" ) ||
		( szTypeName == "GameRoot" )  )
	{
		*pszPrefix = "Scenario/" + szTypeName + '/';
		*pszPostfix = '_' + szTypeName + ".xdb";
	}
	else if ( ( szTypeName == "CragDesc" ) ||
		( szTypeName == "CoastDesc" ) ||
		( szTypeName == "LakeDesc" ) ||
		( szTypeName == "RiverDesc" ) ||
		( szTypeName == "RoadDesc" ) ||
		( szTypeName == "TerraObjSetRPGStats" ) ||
		( szTypeName == "TGNoise" ) ||
		( szTypeName == "PreLight" ) ||
		( szTypeName == "TGTerraSet" ) ||
		( szTypeName == "TGTerraType" ) ||
		( szTypeName == "Water" ) ||
		( szTypeName == "WaterSet" ) ||
		( szTypeName == "WeatherDesc" )  )
	{
		*pszPrefix = "Terrain/" + szTypeName + '/';
		*pszPostfix = '_' + szTypeName + ".xdb";
	}
	else if ( 
		( szTypeName == "DeployTemplate" ) ||
		( szTypeName == "Reinforcement" ) ||
		( szTypeName == "ReinforcementChange" ) ||
		( szTypeName == "ReinforcementDisable" ) ||
		( szTypeName == "ReinforcementEnable" ) ||
		( szTypeName == "ReinforcementTypes" ) )
	{
		*pszPrefix = "Reinforcements/" + szTypeName + '/';
		*pszPostfix = '_' + szTypeName + ".xdb";
	}
	else 
	{
		*pszPrefix = "Other/" + szTypeName + '/';
		*pszPostfix = '/' + szTypeName + ".xdb";
	}
}

// ************************************************************************************************************************ //
// **
// ** 
// **
// **
// **
// ************************************************************************************************************************ //



// ************************************************************************************************************************ //
// **
// ** 
// **
// **
// **
// ************************************************************************************************************************ //

class CXmlExporterB2 : public CXmlExporter
{
	typedef hash_map<string, string> CNamesMap;
	CNamesMap namesMap;
	//
	string MakePathNameOther( const string &szObjectName, const string &szClassTypeName, const string &szFieldName );
	//
	string MakePathName( const string &szObjectName, const string &szClassTypeName, const string &szFieldName );
};

string CXmlExporterB2::MakePathName( const string &szObjectName, const string &szClassTypeName, const string &szFieldName )
{
	string szFullName = szClassTypeName + ':' + szObjectName;// + ':' + szFieldName;
	NStr::ToLowerASCII( &szFullName );
	CNamesMap::const_iterator pos = namesMap.find( szFullName );
	if ( pos == namesMap.end() )
	{
		string szFullPathName = MakePathNameOther( szObjectName, szClassTypeName, szFieldName );
		NStr::ReplaceAllChars( &szFullPathName, ' ', '_' );
		NStr::ReplaceAllChars( &szFullPathName, '\\', '/' );
		// correct non-english file names
		for ( int i = 0; i < szFullPathName.size(); ++i )
		{
			if ( ((szFullPathName[i] & 0x80) != 0) || (szFullPathName[i] < 0x20) )
				szFullPathName[i] = '_';
		}
		//
		namesMap[szFullName] = szFullPathName;
		return szFullPathName;
	}
	else
		return pos->second;
}

string CXmlExporterB2::MakePathNameOther( const string &szObjectName, const string &szClassTypeName, const string &szFieldName )
{
	string szPrefix, szPostfix;
	MakePrefixAndPostfix( &szPrefix, &szPostfix, szClassTypeName );
	return szPrefix + szObjectName + szPostfix;
}

static CXmlExporterB2 theExporter;
CXmlExporter *GetExporter() { return &theExporter; }

}
