#pragma once

// automatically generated file, don't change manually!


struct IXmlSaver;

namespace NDb
{
	struct SWaterSet;
	struct STwoSidedLight;
	struct STGNoise;
	struct SMaterial;

	struct SAnimatedTexture
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SMaterial > pMaterial;
		int nNumFramesX;
		int nNumFramesY;
		int nUseFrames;

		#include "include_animatedtexture.h"

		SAnimatedTexture() :
			__dwCheckSum( 0 ),
			nNumFramesX( 0 ),
			nNumFramesY( 0 ),
			nUseFrames( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SWaterSet : public CResource
	{
		OBJECT_BASIC_METHODS( SWaterSet )
	public:
		enum { typeID = 0x10084340 };
		SAnimatedTexture water;
		SAnimatedTexture whiteHorses;
		CDBPtr< SMaterial > pSurf;

		SWaterSet() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SWater : public CResource
	{
		OBJECT_BASIC_METHODS( SWater )
	public:
		enum { typeID = 0x10084341 };

		enum EWaterType
		{
			WT_OCEAN = 0,
			WT_LAKE = 1,
			WT_RIVER = 2,
			WT_SWAMP = 3,
			WT_OTHER = 4,
		};

		struct SWaterWaveType
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			float fAmplitude;
			float fPeriod;
			float fPeriodVariation;
			float fInvPeriod;
			float fDeepWaveNumber;
			float fPhaseOffset;

			#include "include_waterwavetype.h"

			SWaterWaveType() :
				__dwCheckSum( 0 ),
				fAmplitude( 1.4000f ),
				fPeriod( 0.3000f ),
				fPeriodVariation( 0.1500f ),
				fInvPeriod( 0.0f ),
				fDeepWaveNumber( 0.1000f ),
				fPhaseOffset( 0.0f )
			{ }
			//
			void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		vector< SWaterWaveType > waves;
		CDBPtr< SWaterSet > pWaterSet;
		CDBPtr< STwoSidedLight > pLight;
		CDBPtr< STGNoise > pDepthNoise;
		float fDepthNoiseCoeff;
		int nTilesNumPerWaterTexture;
		bool bUseWaves;
		float fHorDeformMinRadius;
		float fHorDeformMaxRadius;
		float fHorDeformRadiusSpeed;
		float fHorDeformRotationSpeedMin;
		float fHorDeformRotationSpeedVariation;
		EWaterType eWaterType;

		#include "include_water.h"

		SWater() :
			fDepthNoiseCoeff( 0.0f ),
			nTilesNumPerWaterTexture( 6 ),
			bUseWaves( true ),
			fHorDeformMinRadius( 0.0000f ),
			fHorDeformMaxRadius( 0.5000f ),
			fHorDeformRadiusSpeed( 0.0005f ),
			fHorDeformRotationSpeedMin( 0.0009f ),
			fHorDeformRotationSpeedVariation( 0.0026f ),
			eWaterType( WT_OTHER )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};
}

namespace NDb
{
	string EnumToString( NDb::SWater::EWaterType eValue );
	SWater::EWaterType StringToEnum_NDb_SWater_EWaterType( const string &szValue );
}

template <>
struct SKnownEnum<NDb::SWater::EWaterType>
{
	enum { isKnown = 1 };
	static string ToString( NDb::SWater::EWaterType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SWater::EWaterType ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_SWater_EWaterType( szValue ); }
};

