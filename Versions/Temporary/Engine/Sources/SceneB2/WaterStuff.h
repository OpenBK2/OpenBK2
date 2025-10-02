#pragma once

#include "../B2_M1_Terrain/DBWater.h"
#include "../B2_M1_Terrain/DBTerrain.h"

namespace NWaterStuff
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SSurfBorder
	{
		vector<NDb::SVSOPoint> points;
		CDBPtr<NDb::SMaterial> pSurfMaterial;
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SWaterParams
	{
		bool bUseWaves;
		float fHorDeformMinRadius;
		float fHorDeformMaxRadius;
		float fHorDeformRadiusSpeed;
		float fHorDeformRotationSpeedMin;
		float fHorDeformRotationSpeedVariation;
		CDBPtr<NDb::SMaterial> pMaterial;
		CDBPtr<NDb::STwoSidedLight> pLight;
		int nNumFramesX;
		int nNumFramesY;
		string szNoiseFileName;
		float fNoiseCoeff;
		int nSeaMapIndex; // filled during water initialization
		NDb::SWater::EWaterType eWaterType;
		//
		SWaterParams& operator = ( const NDb::SWater &v )
		{
			bUseWaves = v.bUseWaves;
			fHorDeformMinRadius = v.fHorDeformMinRadius;
			fHorDeformMaxRadius = v.fHorDeformMaxRadius;
			fHorDeformRadiusSpeed = v.fHorDeformRadiusSpeed;
			fHorDeformRotationSpeedMin = v.fHorDeformRotationSpeedMin;
			fHorDeformRotationSpeedVariation = v.fHorDeformRotationSpeedVariation;
			pMaterial = v.pWaterSet->water.pMaterial;
			pLight = v.pLight;
			nNumFramesX = v.pWaterSet->water.nNumFramesX;
			nNumFramesY = v.pWaterSet->water.nNumFramesY;
			szNoiseFileName = v.pDepthNoise->szFileName;
			fNoiseCoeff = v.fDepthNoiseCoeff;
			eWaterType = v.eWaterType;
			return *this;
		}
		//
		bool operator == ( const NDb::SWater &v ) const
		{
			return ( pMaterial == v.pWaterSet[0].water.pMaterial ) &&
				( bUseWaves == v.bUseWaves ) &&
				( fHorDeformMinRadius == v.fHorDeformMinRadius ) &&
				( fHorDeformMaxRadius == v.fHorDeformMaxRadius ) &&
				( fHorDeformRadiusSpeed == v.fHorDeformRadiusSpeed ) &&
				( fHorDeformRotationSpeedMin == v.fHorDeformRotationSpeedMin ) &&
				( fHorDeformRotationSpeedVariation == v.fHorDeformRotationSpeedVariation ) &&
				( pLight == v.pLight ) &&
				( nNumFramesX == v.pWaterSet->water.nNumFramesX ) &&
				( nNumFramesY == v.pWaterSet->water.nNumFramesY ) &&
				( szNoiseFileName == v.pDepthNoise->szFileName ) &&
				( fNoiseCoeff == v.fDepthNoiseCoeff ) &&
				( eWaterType == v.eWaterType);
		}
		//
		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &bUseWaves );
			saver.Add( 2, &fHorDeformMinRadius );
			saver.Add( 3, &fHorDeformMaxRadius );
			saver.Add( 4, &fHorDeformRadiusSpeed );
			saver.Add( 5, &fHorDeformRotationSpeedMin );
			saver.Add( 6, &fHorDeformRotationSpeedVariation );
			saver.Add( 7, &pMaterial );
			saver.Add( 8, &pLight );
			saver.Add( 9, &nNumFramesX );
			saver.Add( 10, &nNumFramesY );
			saver.Add( 11, &szNoiseFileName );
			saver.Add( 12, &fNoiseCoeff );
			saver.Add( 13, &nSeaMapIndex );
			return 0;
		}
	};
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
};

