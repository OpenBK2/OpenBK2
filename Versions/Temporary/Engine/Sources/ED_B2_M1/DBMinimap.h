#pragma once

// automatically generated file, don't change manually!

#include "stats_b2_m1/season.h"
#include "system/filepath.h"

struct IXmlSaver;

namespace NDb
{
	enum EImageScaleMethod;
	enum EMinimapLayerType;

	enum EMinimapLayerType
	{
		LAYER_UNKNOWN = 0,
		LAYER_BRIDGE = 1,
		LAYER_BUILDING = 2,
		LAYER_RIVER = 3,
		LAYER_RAILOAD = 4,
		LAYER_ROAD = 5,
		LAYER_FLORA = 6,
		LAYER_GRAG = 7,
		LAYER_SWAMP = 8,
		LAYER_LAKE = 9,
		LAYER_OCEAN = 10,
		LAYER_TERRAIN = 11,
	};

	enum EImageScaleMethod
	{
		IMAGE_SCALE_METHOD_DEFAULT = 0,
		IMAGE_SCALE_METHOD_FILTER = 1,
		IMAGE_SCALE_METHOD_BOX = 2,
		IMAGE_SCALE_METHOD_TRIANGLE = 3,
		IMAGE_SCALE_METHOD_BELL = 4,
		IMAGE_SCALE_METHOD_BSPLINE = 5,
		IMAGE_SCALE_METHOD_LANCZOS3 = 6,
		IMAGE_SCALE_METHOD_MITCHELL = 7,
	};

	struct SShadowPoint
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nx;
		int ny;

		SShadowPoint() :
			__dwCheckSum( 0 ),
			nx( 0 ),
			ny( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SEmbossPoint
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nx;
		int ny;

		SEmbossPoint() :
			__dwCheckSum( 0 ),
			nx( 0 ),
			ny( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SMinimapLayer
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		EMinimapLayerType eType;
		int nEmbossFilterSize;
		bool bScaleNoise;
		int nColor;
		int nBorderColor;
		int nBorderWidth;
		SShadowPoint shadowPoint;
		SEmbossPoint embossPoint;
		NFile::CFilePath szNoiseImage;
		EImageScaleMethod eScaleMethod;

		SMinimapLayer() :
			__dwCheckSum( 0 ),
			eType( LAYER_UNKNOWN ),
			nEmbossFilterSize( 3 ),
			bScaleNoise( false ),
			nColor( 0 ),
			nBorderColor( 0 ),
			nBorderWidth( 1 ),
			eScaleMethod( IMAGE_SCALE_METHOD_DEFAULT )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SMinimap : public CResource
	{
		OBJECT_BASIC_METHODS( SMinimap )
	public:
		enum { typeID = 0x1414DB40 };
		int nWoodRadius;
		int nTerrainShadeRatio;
		bool bShowAllBuildingsPassability;
		bool bShowTerrainShades;
		int nMinAlpha;
		vector< SMinimapLayer > layers;

		SMinimap() :
			nWoodRadius( 5 ),
			nTerrainShadeRatio( 100 ),
			bShowAllBuildingsPassability( false ),
			bShowTerrainShades( false ),
			nMinAlpha( 1 )
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
	string EnumToString( NDb::EMinimapLayerType eValue );
	EMinimapLayerType StringToEnum_NDb_EMinimapLayerType( const string &szValue );
}

template <>
struct SKnownEnum<NDb::EMinimapLayerType>
{
	enum { isKnown = 1 };
	static string ToString( NDb::EMinimapLayerType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EMinimapLayerType ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_EMinimapLayerType( szValue ); }
};

namespace NDb
{
	string EnumToString( NDb::EImageScaleMethod eValue );
	EImageScaleMethod StringToEnum_NDb_EImageScaleMethod( const string &szValue );
}

template <>
struct SKnownEnum<NDb::EImageScaleMethod>
{
	enum { isKnown = 1 };
	static string ToString( NDb::EImageScaleMethod eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EImageScaleMethod ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_EImageScaleMethod( szValue ); }
};

