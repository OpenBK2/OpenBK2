#pragma once

#include "B2_M1_Terrain_export.h"


// automatically generated file, don't change manually!


interface IXmlSaver;

namespace NDb
{
	struct SVSODesc;
	struct SComplexSoundDesc;
	struct SMaterial;
	struct SWater;

	struct STerrainAIProperties
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		float fPassability;
		int nAIClass;
		int nAIPassabilityClass;
		bool bCanEntrench;
		int nSoilType;

		STerrainAIProperties() :
			__dwCheckSum( 0 ),
			fPassability( 1.0000f ),
			nAIClass( 0 ),
			nAIPassabilityClass( 0 ),
			bCanEntrench( true ),
			nSoilType( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SVSOLayerBaseDesc
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		float fCenterOpacity;
		float fTilingStep;

		SVSOLayerBaseDesc() :
			__dwCheckSum( 0 ),
			fCenterOpacity( 1 ),
			fTilingStep( 0.1000f )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SVSOLayerBorderDesc : public SVSOLayerBaseDesc
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SMaterial > pMaterial;
		int nUseFromPixel;
		int nUseToPixel;

		SVSOLayerBorderDesc() :
			__dwCheckSum( 0 ),
			nUseFromPixel( 0 ),
			nUseToPixel( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SVSOLayerCenterDesc : public SVSOLayerBaseDesc
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		float fDisturbance;
		int nNumCells;
		vector< CDBPtr< SMaterial > > materials;
		int nUseFromPixel;
		int nUseToPixel;
		float fStreamSpeed;

		SVSOLayerCenterDesc() :
			__dwCheckSum( 0 ),
			fDisturbance( 0.3000f ),
			nNumCells( 8 ),
			nUseFromPixel( 0 ),
			nUseToPixel( 0 ),
			fStreamSpeed( 0.1000f )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SVSODesc : public CResource
	{
	public:
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nType;
		int nPriority;
		STerrainAIProperties aIProperty;
		int nMiniMapCenterColor;
		int nMiniMapBorderColor;
		int nMiniMapCenterWidth;
		CDBPtr< SComplexSoundDesc > pAmbientSound;
		CDBPtr< SComplexSoundDesc > pCycledSound;

		SVSODesc() :
			__dwCheckSum( 0 ),
			nType( 0 ),
			nPriority( 0 ),
			nMiniMapCenterColor( 0 ),
			nMiniMapBorderColor( 0 ),
			nMiniMapCenterWidth( 100 )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct B2_M1_TERRAIN_EXPORT SVSOPoint
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CVec3 vPos;
		CVec3 vNorm;
		float fWidth;
		float fOpacity;
		bool bKeyPoint;
		float fRadius;
		float fReserved;

		SVSOPoint() :
			__dwCheckSum( 0 ),
			vPos( VNULL3 ),
			vNorm( VNULL3 ),
			fWidth( 0.0f ),
			fOpacity( 0.0f ),
			bKeyPoint( false ),
			fRadius( 0.0f ),
			fReserved( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SVSOInstance
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SVSODesc > pDescriptor;
		vector< SVSOPoint > points;
		vector< CVec3 > controlPoints;
		int nVSOID;
		int nCMArrowType;
		int nCMArrowMission;
		int nCMArrowMission2;

		SVSOInstance() :
			__dwCheckSum( 0 ),
			nVSOID( 0 ),
			nCMArrowType( 0 ),
			nCMArrowMission( 0 ),
			nCMArrowMission2( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SRoadDesc : public SVSODesc
	{
		OBJECT_BASIC_METHODS( SRoadDesc )
	public:
		enum { typeID = 0x1007C380 };
	private:
		mutable DWORD __dwCheckSum;
	public:
		SVSOLayerBorderDesc leftBorder;
		SVSOLayerBorderDesc rightBorder;
		SVSOLayerCenterDesc center;
		float fDefaultOpacity;

		SRoadDesc() :
			__dwCheckSum( 0 ),
			fDefaultOpacity( 1 )
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

	struct SCragDesc : public SVSODesc
	{
		OBJECT_BASIC_METHODS( SCragDesc )
	public:
		enum { typeID = 0x1308AC00 };
	private:
		mutable DWORD __dwCheckSum;
	public:
		float fBorderRand;
		float fDepth;
		float fDepthRand;
		float fRandX;
		float fRandY;
		bool bHasPeak;
		CDBPtr< SMaterial > pRidgeMaterial;
		CDBPtr< SMaterial > pFootMaterial;
		bool bLeftSided;
		float fRidgeTexGeomScale;

		SCragDesc() :
			__dwCheckSum( 0 ),
			fBorderRand( 1.0000f ),
			fDepth( 0.1500f ),
			fDepthRand( 0.6000f ),
			fRandX( 0.2500f ),
			fRandY( 0.6500f ),
			bHasPeak( true ),
			bLeftSided( true ),
			fRidgeTexGeomScale( 50 )
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

	struct SRiverDesc : public SVSODesc
	{
		OBJECT_BASIC_METHODS( SRiverDesc )
	public:
		enum { typeID = 0x10094B80 };
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SMaterial > pBottomMaterial;
		CDBPtr< SMaterial > pPrecipiceMaterial;
		CDBPtr< SMaterial > pWaterMaterial;
		float fStreamSpeed;
		float fBorderRand;
		float fDepth;
		float fDepthRand;
		float fRandX;
		float fRandY;
		float fRidgeTexGeomScale;
		vector< SVSOLayerCenterDesc > waterLayers;
		bool bHasPeak;
		float fDefaultWidth;
		float fDefaultOpacity;

		SRiverDesc() :
			__dwCheckSum( 0 ),
			fStreamSpeed( 0.1000f ),
			fBorderRand( 0.5000f ),
			fDepth( 0.1500f ),
			fDepthRand( 0.6000f ),
			fRandX( 0.2500f ),
			fRandY( 0.6500f ),
			fRidgeTexGeomScale( 50 ),
			bHasPeak( true ),
			fDefaultWidth( 256 ),
			fDefaultOpacity( 1 )
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

	struct SCoastDesc : public SVSODesc
	{
		OBJECT_BASIC_METHODS( SCoastDesc )
	public:
		enum { typeID = 0x140C9400 };
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SWater > pWater;
		int nMiniMapGradientWidth;

		SCoastDesc() :
			__dwCheckSum( 0 ),
			nMiniMapGradientWidth( 10 )
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

	struct SLakeDesc : public SVSODesc
	{
		OBJECT_BASIC_METHODS( SLakeDesc )
	public:
		enum { typeID = 0x100C8300 };
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SWater > pWaterParams;
		bool bIsLake;
		int nMiniMapGradientWidth;

		SLakeDesc() :
			__dwCheckSum( 0 ),
			bIsLake( true ),
			nMiniMapGradientWidth( 10 )
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
}
