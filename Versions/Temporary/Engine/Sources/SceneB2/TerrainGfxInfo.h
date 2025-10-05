#pragma once

#include "B2_M1_Terrain/PatchHolder.h"

namespace NDb
{
	struct SRoadDesc;
	struct SCragDesc;
	struct SRiverDesc;
	struct STerrainSpotDesc;
	struct SMaterial;
};

struct SPrecipiceGFXInfo
{
	std::vector<NMeshData::SMeshData> patches;
	CDBPtr<NDb::SMaterial> pMaterial;
	int nID;
	//
	bool operator == ( const SPrecipiceGFXInfo &v ) const { return nID == v.nID; }
	//
	void Clear() { patches.clear(); pMaterial = 0; nID = 0; }
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &patches );
		saver.Add( 2, &pMaterial );
		saver.Add( 3, &nID );
		return 0;
	}
};

struct SRoadGFXInfo
{
	enum ERoadPart{ CENTER, LEFT_BORDER, RIGHT_BORDER };
	//
	std::vector<NMeshData::SMeshData> patches;
	CDBPtr<NDb::SRoadDesc> pDesc;
	int nID;
	ERoadPart ePart;
	//
	bool operator == ( const SRoadGFXInfo &v ) const
	{
		return nID == v.nID;
	}
	//
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &patches );
		saver.Add( 2, &pDesc );
		saver.Add( 3, &nID );
		saver.Add( 4, &ePart );
		return 0;
	}
};

struct SCragGFXInfo
{
	std::vector<NMeshData::SMeshData> patches;
	CDBPtr<NDb::SCragDesc> pDesc;
	int nID;
	//
	bool operator == ( const SCragGFXInfo &v ) const { return nID == v.nID; }
	//
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &patches );
		saver.Add( 2, &pDesc );
		saver.Add( 3, &nID );
		return 0;
	}
};

struct SPeakGFXInfo
{
	std::vector<std::vector<NMeshData::SMeshDataTex2> > patches;
	int nID;
	//
	bool operator == ( const SPeakGFXInfo &v ) const { return nID == v.nID; }
	//
	int operator &( IBinSaver &saver )
	{
		saver.Add( 1, &patches );
		saver.Add( 2, &nID );
		return 0;
	}
};

struct SFootGFXInfo
{
	std::vector<NMeshData::SMeshData> patches;
	CDBPtr<NDb::SMaterial> pMaterial;
	int nID;
	//
	int operator &( IBinSaver &saver )
	{
		saver.Add( 1, &patches );
		saver.Add( 2, &pMaterial );
		saver.Add( 3, &nID );
		return 0;
	}
};

struct SRiverGFXInfo
{
	std::vector<NMeshData::SMeshData> waterPatches;
	std::vector<NMeshData::SMeshData> bottomPatches;
	CDBPtr<NDb::SRiverDesc> pDesc;
	int nID;
	std::vector<NMeshData::SMeshData> water2Patches;
	//
	bool operator == ( const SRiverGFXInfo &v )const { return nID == v.nID; }
	//
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &waterPatches );
		saver.Add( 2, &bottomPatches );
		saver.Add( 3, &pDesc );
		saver.Add( 4, &nID );
		saver.Add( 5, &water2Patches );
		return 0;
	}
};

struct STerraSpotGFXInfo
{
	NMeshData::SMeshData data;
	CDBPtr<NDb::STerrainSpotDesc> pDesc;
	int nID;
	//
	bool operator == ( const STerraSpotGFXInfo &v ) const { return nID == v.nID; }
	//
	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &data );
		saver.Add( 2, &pDesc );
		saver.Add( 3, &nID );
		return 0;
	}
};

struct STerrainGfxInfo
{
	std::vector<std::vector<NMeshData::SMeshData> > terraPatches;
	std::vector<std::vector<NMeshData::SMeshData> > terraBorders;

	std::list<SRoadGFXInfo> roads;
	std::list<SRiverGFXInfo> rivers;
	std::list<STerraSpotGFXInfo> terraspots;
	std::list<SPrecipiceGFXInfo> precipices;
	std::list<SPeakGFXInfo> peaks;
	std::list<SFootGFXInfo> foots;
	//
	int operator&( IBinSaver &saver )
	{
		return 0;
	}
};


