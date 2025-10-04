#pragma once

// automatically generated file, don't change manually!

#include "dbterrainspot.h"
#include "DBVSO.h"
#include "System/FilePath.h"

#include "B2_M1_Terrain_export.h"


struct IXmlSaver;

namespace NDb
{
	struct STGTerraType;
	struct SWeatherDesc;
	struct SAmbientLight;
	struct SLightInstance;
	struct STGTerraSet;
	struct SPreLight;
	enum EWeatherType;
	struct SComplexSoundDesc;
	struct SWater;

	struct STGNoise : public CResource
	{
		OBJECT_BASIC_METHODS( STGNoise )
	public:
		enum { typeID = 0x10081500 };
		NFile::CFilePath szFileName;

		STGNoise() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct STGTerraType : public CResource
	{
		OBJECT_BASIC_METHODS( STGTerraType )
	public:
		enum { typeID = 0x13121B41 };
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SMaterial > pMaterial;
		STerrainAIProperties aIProperty;
		int nColor;
		CDBPtr< SMaterial > pPeakMaterial;
		float fScaleCoeff;
		CDBPtr< SComplexSoundDesc > pSound;
		CDBPtr< SComplexSoundDesc > pCycledSound;

		STGTerraType() :
			__dwCheckSum( 0 ),
			nColor( 0 ),
			fScaleCoeff( 1 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct STGTerraSet : public CResource
	{
		OBJECT_BASIC_METHODS( STGTerraSet )
	public:
		enum { typeID = 0x13121B01 };
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< CDBPtr< STGTerraType > > terraTypes;
		bool bWrapTexture;

		STGTerraSet() :
			__dwCheckSum( 0 ),
			bWrapTexture( false )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	enum EWeatherType
	{
		WEATHER_RAIN = 0,
		WEATHER_SNOW = 1,
		WEATHER_SANDSTORM = 2,
	};

	struct SWeather
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nWindDirection;
		int nWindForce;
		float fWeatherPeriod;
		float fWeatherPeriodRandom;
		CDBPtr< SWeatherDesc > pVisuals;

		SWeather() :
			__dwCheckSum( 0 ),
			nWindDirection( 0 ),
			nWindForce( 0 ),
			fWeatherPeriod( 0.0f ),
			fWeatherPeriodRandom( 0.0f )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SWeatherDesc : public CResource
	{
		OBJECT_BASIC_METHODS( SWeatherDesc )
	public:
		enum { typeID = 0x1918BBC0 };
	private:
		mutable DWORD __dwCheckSum;
	public:

		struct SAmbientSoundDescr
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CDBPtr< SComplexSoundDesc > pAmbientSound;
			float fSoundLength;

			SAmbientSoundDescr() :
				__dwCheckSum( 0 ),
				fSoundLength( 0.0f )
			{ }
			//
			void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		EWeatherType eType;
		CDBPtr< SMaterial > pPartMaterial;
		vector< CDBPtr< SMaterial > > partMaterials;
		float fPartSize;
		float fFallHeight;
		float fSpeed;
		float fTrajectoryParameter;
		int nIntensity;
		bool bWindAffected;
		CDBPtr< SAmbientLight > pWeatherLight;
		vector< CDBPtr< SLightInstance > > lightnings;
		float fLightningsPerMinute;
		float fLightningsRandom;
		CDBPtr< SWater > pWater;
		vector< SAmbientSoundDescr > ambientSound;
		vector< CDBPtr< SComplexSoundDesc > > randomSounds;

		SWeatherDesc() :
			__dwCheckSum( 0 ),
			eType( WEATHER_RAIN ),
			fPartSize( 0.0f ),
			fFallHeight( 0.0f ),
			fSpeed( 0.0f ),
			fTrajectoryParameter( 0.0f ),
			nIntensity( 1 ),
			bWindAffected( true ),
			fLightningsPerMinute( 1 ),
			fLightningsRandom( 0 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct B2_M1_TERRAIN_EXPORT STerrain : public CResource
	{
	public:
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nNumPatchesX;
		int nNumPatchesY;
		CDBPtr< STGTerraSet > pTerraSet;
		string szMapFilesPath;
		CDBPtr< SAmbientLight > pLight;
		CDBPtr< SPreLight > pPreLight;
		SWeather weather;
		CDBPtr< SWater > pOceanWater;
		vector< SVSOInstance > roads;
		vector< SVSOInstance > rivers;
		vector< SVSOInstance > crags;
		vector< STerrainSpotInstance > spots;
		vector< SVSOInstance > lakes;
		bool bHasCoast;
		SVSOInstance coast;
		CVec3 vCoastMidPoint;
		GUID uid;

		STerrain() :
			__dwCheckSum( 0 ),
			nNumPatchesX( 0 ),
			nNumPatchesY( 0 ),
			bHasCoast( false ),
			vCoastMidPoint( VNULL3 )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};
}

namespace NDb
{
	string EnumToString( NDb::EWeatherType eValue );
	EWeatherType StringToEnum_NDb_EWeatherType( const string &szValue );
}

template <>
struct SKnownEnum<NDb::EWeatherType>
{
	enum { isKnown = 1 };
	static string ToString( NDb::EWeatherType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EWeatherType ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_EWeatherType( szValue ); }
};

