#pragma once

// automatically generated file, don't change manually!

#include "3Dmotor_export.h"

#include "Misc/Geom.h"
#include "System/FilePath.h"

#include <cstdint>

#include <boost/uuid/uuid.hpp>

struct IXmlSaver;

namespace NDb
{
	struct SAnimBase;
	struct SMaterial;
	struct SGeometry;
	struct SSkeleton;
	struct SAIGeometry;
	struct SAmbientLight;
	enum EConvertionType;
	struct SDistanceFog;
	struct SModel;
	struct SParticleInstance;
	struct STexture;
	struct SSunFlares;
	struct SHeightFog;
	struct SDepthOfField;

	struct SModel : public CResource
	{
		OBJECT_BASIC_METHODS( SModel )
	public:
		enum { typeID = 0x12069B88 };
		std::vector< CDBPtr< SMaterial > > materials;
		CDBPtr< SGeometry > pGeometry;
		CDBPtr< SSkeleton > pSkeleton;
		std::vector< CDBPtr< SAnimBase > > animations;
		float fWindPower;

		SModel() :
			fWindPower( 1 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};

	enum EConvertionType
	{
		CONVERT_ORDINARY = 0,
		CONVERT_BUMP = 1,
		CONVERT_TRANSPARENT = 2,
		CONVERT_TRANSPARENT_ADD = 3,
		CONVERT_LINEAR_PICTURE = 4,
		CONVERT_ORDINARY_FASTMIP = 5,
	};

	struct STexture : public CResource
	{
		OBJECT_BASIC_METHODS( STexture )
	public:
		enum { typeID = 0x12069B8E };

		enum EType
		{
			REGULAR = 0,
			TEXTURE_2D = 1,
		};

		enum EAddrType
		{
			CLAMP = 0,
			WRAP = 1,
			WRAP_X = 2,
			WRAP_Y = 3,
		};

		enum EFormat
		{
			TF_DXT1 = 0,
			TF_DXT3 = 1,
			TF_8888 = 2,
			TF_565 = 3,
		};
		NFile::CFilePath szDestName;
		EType eType;
		EConvertionType eConversionType;
		EAddrType eAddrType;
		EFormat eFormat;
		int nWidth;
		int nHeight;
		float fMappingSize;
		int nNMips;
		float fGain;
		int nAverageColor;
		bool bInstantLoad;
		bool bIsDXT;
		bool bFlipY;

		STexture() :
			eType( REGULAR ),
			eConversionType( CONVERT_ORDINARY ),
			eAddrType( CLAMP ),
			eFormat( TF_DXT1 ),
			nWidth( 0 ),
			nHeight( 0 ),
			fMappingSize( 0.0f ),
			nNMips( 0 ),
			fGain( 0.0f ),
			nAverageColor( 0 ),
			bInstantLoad( false ),
			bIsDXT( false ),
			bFlipY( false )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};

	struct SCubeTexture : public CResource
	{
		OBJECT_BASIC_METHODS( SCubeTexture )
	public:
		enum { typeID = 0x12069B82 };
		CDBPtr< STexture > pPositiveX;
		CDBPtr< STexture > pPositiveY;
		CDBPtr< STexture > pPositiveZ;
		CDBPtr< STexture > pNegativeX;
		CDBPtr< STexture > pNegativeY;
		CDBPtr< STexture > pNegativeZ;

		SCubeTexture() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};

	struct SSunFlare
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		float fDistance;
		CDBPtr< STexture > pTexture;
		bool bFade;
		float fScale;

		SSunFlare() :
			__dwCheckSum( 0 ),
			fDistance( 0.0f ),
			bFade( false ),
			fScale( 0.0f )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SSunFlares : public CResource
	{
		OBJECT_BASIC_METHODS( SSunFlares )
	public:
		enum { typeID = 0xB4406170 };
		std::vector< SSunFlare > flares;
		CDBPtr< STexture > pOverBright;

		SSunFlares() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};

	struct SAmbientLight : public CResource
	{
		OBJECT_BASIC_METHODS( SAmbientLight )
	public:
		enum { typeID = 0x12069B80 };
		CVec3 vLightColor;
		CVec3 vAmbientColor;
		CVec3 vShadeColor;
		CVec3 vIncidentShadowColor;
		CVec3 vParticlesColor;
		bool bWhitening;
		float fPitch;
		float fYaw;
		float fShadowPitch;
		float fShadowYaw;
		CDBPtr< SCubeTexture > pSky;
		CVec3 vGlossColor;
		CVec3 vFogColor;
		float fFogStartDistance;
		float fFogDistance;
		float fVapourHeight;
		float fVapourDensity;
		float fVapourNoiseParam;
		float fVapourSpeed;
		float fVapourSwitchTime;
		CVec3 vVapourColor;
		CVec3 vShadowColor;
		bool bInGameUse;
		CVec3 vBackColor;
		CDBPtr< SAmbientLight > pGForce2Light;
		float fVapourStartHeight;
		float fBlurStrength;
		CVec3 vGroundAmbientColor;
		float fMaxShadowHeight;
		CDBPtr< SSunFlares > pSunFlares;
		CDBPtr< STexture > pHaze;
		CDBPtr< STexture > pCloudTex;
		CVec2 vCloudSize;
		float fCloudDir;
		float fCloudSpeed;
		CDBPtr< SParticleInstance > pRain;
		float fSunFlarePitch;
		float fSunFlareYaw;
		float fShadowsMaxDetailLength;
		CDBPtr< SHeightFog > pHeightFog;
		CDBPtr< SDepthOfField > pDepthOfField;
		CDBPtr< SDistanceFog > pDistanceFog;
		CDBPtr< SModel > pSkyDome;
		CVec3 vDymanicLightsModifications;

		#include "include_AmbientLight.h"

		SAmbientLight() :
			vLightColor( VNULL3 ),
			vAmbientColor( VNULL3 ),
			vShadeColor( VNULL3 ),
			vIncidentShadowColor( VNULL3 ),
			vParticlesColor( VNULL3 ),
			bWhitening( true ),
			fPitch( 0.0f ),
			fYaw( 0.0f ),
			fShadowPitch( 100 ),
			fShadowYaw( 100 ),
			vGlossColor( VNULL3 ),
			vFogColor( VNULL3 ),
			fFogStartDistance( 1000 ),
			fFogDistance( 2000 ),
			fVapourHeight( 0.0f ),
			fVapourDensity( 0 ),
			fVapourNoiseParam( 0.0f ),
			fVapourSpeed( 0.0f ),
			fVapourSwitchTime( 0.0f ),
			vVapourColor( VNULL3 ),
			vShadowColor( VNULL3 ),
			bInGameUse( true ),
			vBackColor( VNULL3 ),
			fVapourStartHeight( 0.0f ),
			fBlurStrength( 0.0f ),
			vGroundAmbientColor( VNULL3 ),
			fMaxShadowHeight( 20 ),
			vCloudSize( VNULL2 ),
			fCloudDir( 0 ),
			fCloudSpeed( 1 ),
			fSunFlarePitch( 0.0f ),
			fSunFlareYaw( 0.0f ),
			fShadowsMaxDetailLength( 10 ),
			vDymanicLightsModifications( VNULL3 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};

	struct SHeightFog : public CResource
	{
		OBJECT_BASIC_METHODS( SHeightFog )
	public:
		enum { typeID = 0x1318BB40 };
		CVec3 vFogColor;
		float fMinHeight;
		float fMaxHeight;

		SHeightFog() :
			vFogColor( VNULL3 ),
			fMinHeight( 0 ),
			fMaxHeight( 20 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};

	struct SDepthOfField : public CResource
	{
		OBJECT_BASIC_METHODS( SDepthOfField )
	public:
		enum { typeID = 0x13192480 };
		float fFocalDist;
		float fFocusRange;

		SDepthOfField() :
			fFocalDist( 4 ),
			fFocusRange( 8 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};

	struct SDistanceFog : public CResource
	{
		OBJECT_BASIC_METHODS( SDistanceFog )
	public:
		enum { typeID = 0x1319E340 };
		CVec3 vColor;
		float fMinDist;
		float fMaxDist;
		float fMinZDis;
		float fMaxZDis;
		CDBPtr< STexture > pColorTexture;

		SDistanceFog() :
			vColor( VNULL3 ),
			fMinDist( 0.0f ),
			fMaxDist( 0.0f ),
			fMinZDis( -10000 ),
			fMaxZDis( -10001 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};

	struct SSkeleton : public CResource
	{
		OBJECT_BASIC_METHODS( SSkeleton )
	public:
		enum { typeID = 0x12069B8A };
		std::vector< CDBPtr< SAnimBase > > animations;
		boost::uuids::uuid uid;

		SSkeleton() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};

	struct _3DMOTOR_EXPORT SAnimBase : public CResource
	{
	public:
	private:
		mutable uint32_t __dwCheckSum;
	public:
		boost::uuids::uuid uid;

		#include "include_AnimBase.h"

		SAnimBase() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SAnimLight : public CResource
	{
		OBJECT_BASIC_METHODS( SAnimLight )
	public:
		enum { typeID = 0x1206A301 };
		std::string szSelectNode;
		boost::uuids::uuid uid;

		SAnimLight() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};

	struct SParticle : public CResource
	{
		OBJECT_BASIC_METHODS( SParticle )
	public:
		enum { typeID = 0x12069B89 };
		CVec2 vWrapSize;
		SBound bound;
		bool bPerParticleFog;
		boost::uuids::uuid uid;

		SParticle() :
			vWrapSize( VNULL2 ),
			bPerParticleFog( false )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};

	struct SLightInstance : public CResource
	{
		OBJECT_BASIC_METHODS( SLightInstance )
	public:
		enum { typeID = 0x1206A2C1 };
		CDBPtr< SAnimLight > pLight;
		CVec3 vPosition;
		CQuat qRotation;
		float fScale;
		float fSpeed;
		float fOffset;
		float fEndCycle;
		int nCycleCount;
		int nGlueToBone;

		SLightInstance() :
			vPosition( VNULL3 ),
			qRotation( QNULL ),
			fScale( 1 ),
			fSpeed( 1 ),
			fOffset( 0.0f ),
			fEndCycle( 0.0f ),
			nCycleCount( 1 ),
			nGlueToBone( 0 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};

	struct SParticleInstance : public CResource
	{
		OBJECT_BASIC_METHODS( SParticleInstance )
	public:
		enum { typeID = 0x1206A2C0 };

		enum ELight
		{
			L_NORMAL = 0,
			L_LIT = 1,
		};

		enum EStatic
		{
			P_STATIC = 0,
			P_DYNAMIC = 1,
		};
		ELight eLight;
		CDBPtr< SParticle > pParticle;
		CVec3 vPosition;
		CQuat qRotation;
		float fScale;
		float fSpeed;
		float fOffset;
		float fEndCycle;
		int nCycleCount;
		CVec2 vPivot;
		std::vector< CDBPtr< STexture > > textures;
		bool bIsCrown;
		EStatic eStatic;
		bool bDoesCastShadow;
		int nGlueToBone;
		bool bLeaveParticlesWhereStarted;
		int nPriority;

		SParticleInstance() :
			eLight( L_NORMAL ),
			vPosition( VNULL3 ),
			qRotation( QNULL ),
			fScale( 1 ),
			fSpeed( 1 ),
			fOffset( 0.0f ),
			fEndCycle( 0.0f ),
			nCycleCount( 1 ),
			vPivot( VNULL2 ),
			bIsCrown( false ),
			eStatic( P_STATIC ),
			bDoesCastShadow( false ),
			nGlueToBone( 0 ),
			bLeaveParticlesWhereStarted( false ),
			nPriority( 0 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};

	struct SModelInstance : public CResource
	{
		OBJECT_BASIC_METHODS( SModelInstance )
	public:
		enum { typeID = 0x5014B340 };
		CDBPtr< SModel > pModel;
		CDBPtr< SAnimBase > pSkelAnim;
		CVec3 vPosition;
		CQuat qRotation;
		float fScale;
		float fOffset;
		float fCycleLength;
		int nCycleCount;
		int nGlueToBone;

		SModelInstance() :
			vPosition( VNULL3 ),
			qRotation( QNULL ),
			fScale( 1 ),
			fOffset( 0 ),
			fCycleLength( 0 ),
			nCycleCount( 0 ),
			nGlueToBone( 0 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};

	struct SEffect : public CResource
	{
		OBJECT_BASIC_METHODS( SEffect )
	public:
		enum { typeID = 0x12069B83 };
		std::vector< CDBPtr< SParticleInstance > > instances;
		std::vector< CDBPtr< SLightInstance > > lights;
		std::vector< CDBPtr< SModelInstance > > models;
		bool bWindAffected;
		float fWindPower;
		float fDuration;

		SEffect() :
			bWindAffected( false ),
			fWindPower( 1 ),
			fDuration( 2.5000f )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};

	struct SDecal : public CResource
	{
		OBJECT_BASIC_METHODS( SDecal )
	public:
		enum { typeID = 0x131A73C0 };
		CDBPtr< SMaterial > pMaterial;
		float fRadius;
		int nFadeInTime;
		int nNoFadingTime;
		int nFadeOutTime;
		float fExplosionHeight;

		SDecal() :
			fRadius( 4 ),
			nFadeInTime( 2000 ),
			nNoFadingTime( 10000 ),
			nFadeOutTime( 2000 ),
			fExplosionHeight( 2 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};

	struct SFont : public CResource
	{
		OBJECT_BASIC_METHODS( SFont )
	public:
		enum { typeID = 0x12069B84 };

		enum EPitch
		{
			DEFAULT = 0,
		};

		enum ECharset
		{
			ANSI = 0,
			BALTIC = 1,
			CHINESEBIG5 = 2,
			DEF_CHARSET = 3,
			EASTEUROPE = 4,
			GB2312 = 5,
			GREEK = 6,
			HANGUL = 7,
			RUSSIAN = 8,
			SHIFTJIS = 9,
			SYMBOL = 10,
			TURKISH = 11,
			HEBREW = 12,
			ARABIC = 13,
			THAI = 14,
		};
		CDBPtr< STexture > pTexture;
		boost::uuids::uuid uid;
		int nHeight;
		int nThickness;
		bool bItalic;
		bool bAntialiased;
		EPitch ePitch;
		ECharset eCharset;
		std::string szFaceName;
		std::string szName;
		NFile::CFilePath szCharactersFile;

		SFont() :
			nHeight( 20 ),
			nThickness( 400 ),
			bItalic( false ),
			bAntialiased( true ),
			ePitch( DEFAULT ),
			eCharset( ANSI ),
			szFaceName( "Times New Roman" )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};

	struct SAIGeometry : public CResource
	{
		OBJECT_BASIC_METHODS( SAIGeometry )
	public:
		enum { typeID = 0x1007EC80 };
		float fVolume;
		float fSolidPart;
		CVec3 vAABBCenter;
		CVec3 vAABBHalfSize;
		boost::uuids::uuid uid;

		SAIGeometry() :
			fVolume( 0.0f ),
			fSolidPart( 0.0f ),
			vAABBCenter( VNULL3 ),
			vAABBHalfSize( VNULL3 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};

	struct SGeometry : public CResource
	{
		OBJECT_BASIC_METHODS( SGeometry )
	public:
		enum { typeID = 0x12069B85 };
		boost::uuids::uuid uid;
		CVec3 vSize;
		CVec3 vCenter;
		CDBPtr< SAIGeometry > pAIGeometry;
		int nNumMeshes;
		std::vector< int > materialQuantities;
		std::vector< std::string > meshNames;
		std::vector< int > meshAnimated;
		std::vector< int > meshWindAffected;

		SGeometry() :
			vSize( VNULL3 ),
			vCenter( VNULL3 ),
			nNumMeshes( 0 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};

	enum EAddressMode
	{
		AM_WRAP = 0,
		AM_CLAMP = 1,
	};

	struct SMaterial : public CResource
	{
		OBJECT_BASIC_METHODS( SMaterial )
	public:
		enum { typeID = 0x12069B87 };

		enum ELightingMode
		{
			L_NORMAL = 0,
			L_SELFILLUM = 1,
		};

		enum EEffect
		{
			M_GENERIC = 0,
			M_WATER = 1,
			M_TRACKS = 2,
			M_TERRAIN = 3,
			M_CLOUDS_H5 = 4,
			M_ANIM_WATER = 5,
			M_SURF = 6,
			M_SIMPLE_SKY = 7,
			M_REFLECT_WATER = 8,
		};

		enum EAlphaMode
		{
			AM_OPAQUE = 0,
			AM_OVERLAY = 1,
			AM_OVERLAY_ZWRITE = 2,
			AM_TRANSPARENT = 3,
			AM_ALPHA_TEST = 4,
			AM_DECAL = 5,
		};

		enum EDynamicMode
		{
			DM_DONT_CARE = 0,
			DM_FORCE_STATIC = 1,
			DM_FORCE_DYNAMIC = 2,
		};
		CDBPtr< STexture > pTexture;
		CDBPtr< STexture > pBump;
		float fSpecFactor;
		CVec3 vSpecColor;
		CDBPtr< STexture > pGloss;
		float fMetalMirror;
		float fDielMirror;
		CDBPtr< STexture > pMirror;
		bool bCastShadow;
		bool bReceiveShadow;
		int nPriority;
		CVec3 vTranslucentColor;
		float fFloatParam;
		CDBPtr< STexture > pDetailTexture;
		float fDetailScale;
		bool bProjectOnTerrain;
		ELightingMode eLightingMode;
		EDynamicMode eDynamicMode;
		bool bIs2Sided;
		EEffect eEffect;
		EAlphaMode eAlphaMode;
		bool bAffectedByFog;
		bool bAddPlaced;
		bool bIgnoreZBuffer;
		bool bBackFaceCastShadow;

		SMaterial() :
			fSpecFactor( 0.0f ),
			vSpecColor( VNULL3 ),
			fMetalMirror( 0.0f ),
			fDielMirror( 0.0f ),
			bCastShadow( true ),
			bReceiveShadow( true ),
			nPriority( 0 ),
			vTranslucentColor( VNULL3 ),
			fFloatParam( 0.0f ),
			fDetailScale( 5 ),
			bProjectOnTerrain( false ),
			eLightingMode( L_NORMAL ),
			eDynamicMode( DM_DONT_CARE ),
			bIs2Sided( false ),
			eEffect( M_GENERIC ),
			eAlphaMode( AM_OPAQUE ),
			bAffectedByFog( true ),
			bAddPlaced( false ),
			bIgnoreZBuffer( false ),
			bBackFaceCastShadow( false )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};

	struct SSpot : public CResource
	{
		OBJECT_BASIC_METHODS( SSpot )
	public:
		enum { typeID = 0x12069B8B };
		CDBPtr< SMaterial > pMaterial;

		SSpot() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};
}

namespace NDb
{
	std::string EnumToString( NDb::EConvertionType eValue );
	EConvertionType StringToEnum_NDb_EConvertionType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EConvertionType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EConvertionType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EConvertionType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EConvertionType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::STexture::EType eValue );
	STexture::EType StringToEnum_NDb_STexture_EType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::STexture::EType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::STexture::EType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::STexture::EType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_STexture_EType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::STexture::EAddrType eValue );
	STexture::EAddrType StringToEnum_NDb_STexture_EAddrType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::STexture::EAddrType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::STexture::EAddrType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::STexture::EAddrType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_STexture_EAddrType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::STexture::EFormat eValue );
	STexture::EFormat StringToEnum_NDb_STexture_EFormat( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::STexture::EFormat>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::STexture::EFormat eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::STexture::EFormat ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_STexture_EFormat( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::SParticleInstance::ELight eValue );
	SParticleInstance::ELight StringToEnum_NDb_SParticleInstance_ELight( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::SParticleInstance::ELight>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::SParticleInstance::ELight eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SParticleInstance::ELight ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_SParticleInstance_ELight( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::SParticleInstance::EStatic eValue );
	SParticleInstance::EStatic StringToEnum_NDb_SParticleInstance_EStatic( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::SParticleInstance::EStatic>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::SParticleInstance::EStatic eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SParticleInstance::EStatic ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_SParticleInstance_EStatic( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::SFont::EPitch eValue );
	SFont::EPitch StringToEnum_NDb_SFont_EPitch( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::SFont::EPitch>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::SFont::EPitch eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SFont::EPitch ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_SFont_EPitch( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::SFont::ECharset eValue );
	SFont::ECharset StringToEnum_NDb_SFont_ECharset( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::SFont::ECharset>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::SFont::ECharset eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SFont::ECharset ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_SFont_ECharset( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EAddressMode eValue );
	EAddressMode StringToEnum_NDb_EAddressMode( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EAddressMode>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EAddressMode eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EAddressMode ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EAddressMode( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::SMaterial::ELightingMode eValue );
	SMaterial::ELightingMode StringToEnum_NDb_SMaterial_ELightingMode( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::SMaterial::ELightingMode>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::SMaterial::ELightingMode eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SMaterial::ELightingMode ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_SMaterial_ELightingMode( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::SMaterial::EEffect eValue );
	SMaterial::EEffect StringToEnum_NDb_SMaterial_EEffect( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::SMaterial::EEffect>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::SMaterial::EEffect eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SMaterial::EEffect ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_SMaterial_EEffect( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::SMaterial::EAlphaMode eValue );
	SMaterial::EAlphaMode StringToEnum_NDb_SMaterial_EAlphaMode( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::SMaterial::EAlphaMode>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::SMaterial::EAlphaMode eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SMaterial::EAlphaMode ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_SMaterial_EAlphaMode( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::SMaterial::EDynamicMode eValue );
	SMaterial::EDynamicMode StringToEnum_NDb_SMaterial_EDynamicMode( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::SMaterial::EDynamicMode>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::SMaterial::EDynamicMode eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SMaterial::EDynamicMode ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_SMaterial_EDynamicMode( szValue ); }
};

