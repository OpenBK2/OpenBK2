#pragma once

// automatically generated file, don't change manually!

#include "Stats_B2_M1_export.h"

#include "3Dmotor/DBScene.h"
#include "AckTypes.h"
#include "dbpassprofile.h"
#include "dbplanemanuvers.h"
#include "Season.h"
#include "useractions.h"
#include "3Dmotor/DBScene.h"
#include "commands_actions.h"
#include "constructorinfo.h"
#include "IconsSet.h"
#include "prefix_rpgstats.h"
#include "unittypes.h"
#include "System/FilePath.h"

struct IXmlSaver;

namespace NDb
{
	enum EUnitSpecialAbility;
	enum EUnitSpecialAbilityGroup;
	struct SUnitStatsModifier;
	struct SWeaponRPGStats;
	enum EObjectVisType;
	enum EObjGameType;
	enum ESeason;
	struct SComplexSoundDesc;
	struct SEffect;
	struct SIconsSet;
	struct SComplexEffect;
	enum ESelectionType;
	struct SBurningFuel;
	struct SModel;
	struct SVisObj;
	struct SComplexEffect;
	struct STexture;
	struct SProjectile;
	struct SCraterSet;
	struct SAttachedModelVisObj;
	struct SDynamicDebrisSet;
	enum EDesignBuildingType;
	enum EBuildingType;
	struct SArmorPattern;
	enum EUnitPoliticalSide;
	struct SUnitSpecialAblityDesc;
	enum EDBUnitRPGType;
	struct SArmorPatternPlacement;
	struct SM1UnitType;
	struct SM1UnitActions;
	struct SAnimBase;
	struct SM1UnitSpecific;
	enum EEncyclopediaFilterUnitType;
	struct SAckSetRPGStats;
	struct SUnitActions;
	enum EDesignUnitType;
	struct SComplexSeasonedEffect;
	enum EReinforcementType;
	enum EUserAction;
	struct SPartyDependentInfo;
	struct SMechUnitRPGStats;
	struct SInfantryRPGStats;

	enum EUnitSpecialAbility
	{
		ABILITY_NOT_ABILITY = 0,
		ABILITY_PLACE_CHARGE = 1,
		ABILITY_PLACE_CONTROLLED_CHARGE = 2,
		ABILITY_DETONATE = 3,
		ABILITY_EXACT_SHOT = 4,
		ABILITY_CRITICAL_TARGETING = 5,
		ABILITY_RAPID_SHOT_MODE = 6,
		ABILITY_REPAIR_LESSER_DAMAGE = 7,
		ABILITY_HOLD_SECTOR = 8,
		ABILITY_TRACK_TARGETING = 9,
		ABILITY_HIGH_PASSABLENESS_MODE = 10,
		ABILITY_MANUVERABLE_FIGHT_MODE = 11,
		ABILITY_USE_BINOCULAR = 12,
		ABILITY_REMOVE_MINE_FIELD = 13,
		ABILITY_RAPID_FIRE_MODE = 14,
		ABILITY_SET_MINE_FIELD = 15,
		ABILITY_DIG_TRENCHES = 16,
		ABILITY_ADRENALINE_RUSH = 17,
		ABILITY_DIE_HARD = 18,
		ABILITY_BUILD_OBSTACLES = 19,
		ABILITY_CAMOFLAGE_MODE = 20,
		ABILITY_ADAVNCED_CAMOFLAGE_MODE = 21,
		ABILITY_THROW_GRENADE = 22,
		ABILITY_THROW_ANTITANK_GRENADE = 23,
		ABILITY_SPY_MODE = 24,
		ABILITY_LOW_HEIGHT_MODE = 25,
		ABILITY_OVERLOAD_MODE = 26,
		ABILITY_DROP_BOMBS = 27,
		ABILITY_FIRE_AA_MISSILES = 28,
		ABILITY_FIRE_AA_GUIDED_MISSILES = 29,
		ABILITY_FIRE_AS_MISSILES = 30,
		ABILITY_REPEAT_LAST_SHOT = 31,
		ABILITY_AREA_FIRE = 32,
		ABILITY_SMOKE_SHOTS = 33,
		ABILITY_ANTIATRILLERY_FIRE = 34,
		ABILITY_TAKE_HIDED_REINFORCEMENT = 35,
		ABILITY_COMMAND_VOICE = 36,
		ABILITY_AUTHORITY = 37,
		ABILITY_ARMOR_TUTOR = 38,
		ABILITY_TAKE_PLACE = 39,
		ABILITY_LAND_MINE = 40,
		ABILITY_ENTRENCH_SELF = 41,
		ABILITY_SUPRESS = 42,
		ABILITY_AMBUSH = 43,
		ABILITY_MOBILE_FORTRESS = 44,
		ABILITY_FLAMETHROWER = 45,
		ABILITY_CAUTION = 46,
		ABILITY_FIRST_AID = 47,
		ABILITY_LINKED_GRENADES = 48,
		ABILITY_MASTER_OF_STREETS = 49,
		ABILITY_ZEROING_IN = 50,
		ABILITY_SUPPORT_FIRE = 51,
		ABILITY_COVER_FIRE = 52,
		ABILITY_PATROL = 53,
		ABILITY_EXACT_BOMBING = 54,
		ABILITY_DROP_PARATROOPERS = 55,
		ABILITY_MASTER_PILOT = 56,
		ABILITY_SKY_GUARD = 57,
		ABILITY_SURVIVAL = 58,
		ABILITY_TANK_HUNTER = 59,
		ABILITY_RADIO_CONTROLLED_MODE = 60,
		_ABILITY_COUNT = 61,
	};

	enum EUnitSpecialAbilityGroup
	{
		ABILITY_GROUP_NOGROUP = 0,
		ABILITY_GROUP_CAMOFLAGE = 1,
	};

	struct SUnitSpecialAblityDesc : public CResource
	{
		OBJECT_BASIC_METHODS( SUnitSpecialAblityDesc )
	public:
		enum { typeID = 0x110832C0 };
	private:
		mutable DWORD __dwCheckSum;
	public:
		EUnitSpecialAbility eName;
		int nRefreshTime;
		int nWorkTime;
		int nSwitchOnTime;
		int nSwitchOffTime;
		EUnitSpecialAbilityGroup eGroupID;
		int nDisableGroupTime;
		CDBPtr< SUnitStatsModifier > pStatsBonus;
		float fParameter;
		bool bStopCurrentAction;
		NFile::CFilePath szLocalizedNameFileRef;
		NFile::CFilePath szLocalizedDescFileRef;

		#include "include_unitspecialablitydesc.h"

		SUnitSpecialAblityDesc() :
			__dwCheckSum( 0 ),
			eName( ABILITY_NOT_ABILITY ),
			nRefreshTime( 0 ),
			nWorkTime( 0 ),
			nSwitchOnTime( 0 ),
			nSwitchOffTime( 0 ),
			eGroupID( ABILITY_GROUP_NOGROUP ),
			nDisableGroupTime( 0 ),
			fParameter( 0 ),
			bStopCurrentAction( false )
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

	enum EArmorDirection
	{
		RPG_FRONT = 0,
		RPG_LEFT = 1,
		RPG_BACK = 2,
		RPG_RIGHT = 3,
		RPG_TOP = 4,
		RPG_BOTTOM = 5,
	};

	enum EObjGameType
	{
		SGVOGT_UNKNOWN = 0,
		SGVOGT_UNIT = 1,
		SGVOGT_BUILDING = 2,
		SGVOGT_FORTIFICATION = 3,
		SGVOGT_ENTRENCHMENT = 4,
		SGVOGT_TANK_PIT = 5,
		SGVOGT_BRIDGE = 6,
		SGVOGT_MINE = 7,
		SGVOGT_OBJECT = 8,
		SGVOGT_FENCE = 9,
		SGVOGT_TERRAOBJ = 10,
		SGVOGT_EFFECT = 11,
		SGVOGT_PROJECTILE = 12,
		SGVOGT_SHADOW = 13,
		SGVOGT_ICON = 14,
		SGVOGT_SQUAD = 15,
		SGVOGT_FLASH = 16,
		SGVOGT_FLAG = 17,
		SGVOGT_SOUND = 18,
		SGVOGT_FLORA = 19,
	};

	enum EObjectVisType
	{
		SGVOT_MESH = 0,
		SGVOT_SPRITE = 1,
	};

	struct SCommonRPGStats : public CResource
	{
	public:
	private:
		mutable DWORD __dwCheckSum;
	public:
		std::string szKeyName;
		std::string szParentName;
		std::string szStatsType;
		EObjectVisType eVisType;
		EObjGameType eGameType;

		#include "include_commonrpgstats.h"

		SCommonRPGStats() :
			__dwCheckSum( 0 ),
			eVisType( SGVOT_MESH ),
			eGameType( SGVOGT_UNKNOWN )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SComplexEffect : public CResource
	{
		OBJECT_BASIC_METHODS( SComplexEffect )
	public:
		enum { typeID = 0x15096402 };
		CDBPtr< SEffect > pSceneEffect;
		std::vector< CDBPtr< SEffect > > sceneEffects;
		CDBPtr< SComplexSoundDesc > pSoundEffect;

		#include "include_complexeffect.h"

		SComplexEffect() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SSeasonEffect
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		ESeason eSeasonToUse;
		CDBPtr< SEffect > pSceneEffect;

		SSeasonEffect() :
			__dwCheckSum( 0 ),
			eSeasonToUse( SEASON_WINTER )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SComplexSeasonedEffect : public CResource
	{
		OBJECT_BASIC_METHODS( SComplexSeasonedEffect )
	public:
		enum { typeID = 0x110CEC40 };
		CDBPtr< SComplexSoundDesc > pSoundEffect;
		std::vector< SSeasonEffect > seasons;

		SComplexSeasonedEffect() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SAckSetRPGStats : public SCommonRPGStats
	{
		OBJECT_BASIC_METHODS( SAckSetRPGStats )
	public:
		enum { typeID = 0x1106AC40 };
		std::vector< SAckType > types;
		int nVoiceNumber;

		#include "include_acksetrpgstats.h"

		SAckSetRPGStats() :
			nVoiceNumber( 0 )
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

	enum ESelectionType
	{
		SELECTION_TYPE_GROUND = 0,
		SELECTION_TYPE_WATER = 1,
		SELECTION_TYPE_AIR = 2,
		SELECTION_TYPE_NONE = 3,
		SELECTION_TYPE_CANNOT_SELECT = 4,
	};

	struct STATS_B2_M1_EXPORT SAttachedLightEffect
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		std::string szLocatorName;
		CVec3 vPos;
		CQuat qRot;
		CVec3 vFlarePos;
		float fFlareSize;
		CVec3 vPointLightPos;
		float fPointLightSize;
		float fConeLength;
		float fConeSize;
		CVec3 vColour;
		CDBPtr< SComplexEffect > pAdditionalEffect;
		bool bOnAtDay;
		bool bOnAtNight;
		CDBPtr< STexture > pConeTexture;
		CDBPtr< STexture > pFlareTexture;

		SAttachedLightEffect() :
			__dwCheckSum( 0 ),
			vPos( VNULL3 ),
			qRot( QNULL ),
			vFlarePos( VNULL3 ),
			fFlareSize( 0 ),
			vPointLightPos( VNULL3 ),
			fPointLightSize( 0.0f ),
			fConeLength( 0.0f ),
			fConeSize( 0.0f ),
			vColour( VNULL3 ),
			bOnAtDay( false ),
			bOnAtNight( true )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SIconsSetParams
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		bool bCustom;
		float fRaising;
		float fHPBarLen;

		SIconsSetParams() :
			__dwCheckSum( 0 ),
			bCustom( false ),
			fRaising( 0.0f ),
			fHPBarLen( 0.0f )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SHPObjectRPGStats : public SCommonRPGStats
	{
	public:
	private:
		mutable DWORD __dwCheckSum;
	public:

		struct SByteArray2
		{
		private:
			mutable DWORD __dwCheckSum;
		public:

			struct SByteArray1
			{
			private:
				mutable DWORD __dwCheckSum;
			public:
				std::vector< int > data;

				SByteArray1() :
					__dwCheckSum( 0 )
				{ }
				//
				void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
				//
				int operator&( IBinSaver &saver );
				int operator&( IXmlSaver &saver );
				DWORD CalcCheckSum() const;
			};
			std::vector< SByteArray1 > data;

			#include "include_ByteArray2.h"

			SByteArray2() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SDefenseRPGStats
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			int nArmorMin;
			int nArmorMax;
			float fSilhouette;

			SDefenseRPGStats() :
				__dwCheckSum( 0 ),
				nArmorMin( 0 ),
				nArmorMax( 0 ),
				fSilhouette( 0.0f )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SDamageLevel
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			float fDamageHP;
			CDBPtr< SVisObj > pVisObj;
			CDBPtr< SComplexEffect > pDamageEffectWindow;
			CDBPtr< SComplexEffect > pDamageEffectSmoke;

			SDamageLevel() :
				__dwCheckSum( 0 ),
				fDamageHP( 0.0f )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SModelSurfacePoint
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CVec3 vPos;
			CVec4 vOrient;

			SModelSurfacePoint() :
				__dwCheckSum( 0 ),
				vPos( VNULL3 ),
				vOrient( VNULL4 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		float fMaxHP;
		std::vector< SDamageLevel > damageLevels;
		float fRepairCost;
		std::vector< SDefenseRPGStats > defences;
		CDBPtr< SVisObj > pvisualObject;
		CDBPtr< SVisObj > pinfoVisualObject;
		CDBPtr< STexture > pIconTexture;
		CDBPtr< STexture > pIconFlagBackground;
		NFile::CFilePath szLocalizedNameFileRef;
		float fSelectionScale;
		ESelectionType eSelectionType;
		std::vector< SAttachedLightEffect > lightEffects;
		std::vector< SModelSurfacePoint > surfacePoints;
		CDBPtr< SIconsSet > pIconsSet;
		SIconsSetParams iconsSetParams;

		#include "include_hpobjectrpgstats.h"

		SHPObjectRPGStats() :
			__dwCheckSum( 0 ),
			fMaxHP( 100 ),
			fRepairCost( 0.0f ),
			fSelectionScale( 1.4000f ),
			eSelectionType( SELECTION_TYPE_GROUND )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SBurningFuel : public CResource
	{
		OBJECT_BASIC_METHODS( SBurningFuel )
	public:
		enum { typeID = 0x111C33C0 };
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nBurnTime;
		CDBPtr< SWeaponRPGStats > pWeaponFireEachSecond;
		CDBPtr< SWeaponRPGStats > pWeaponFireOnDesctruction;

		SBurningFuel() :
			__dwCheckSum( 0 ),
			nBurnTime( 0 )
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

	struct SStaticObjectRPGStats : public SHPObjectRPGStats
	{
	public:
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nAIPassabilityClass;
		bool bBurn;
		CDBPtr< SComplexEffect > pEffectExplosion;
		CDBPtr< SComplexEffect > pEffectDeath;
		CDBPtr< SComplexEffect > pEffectDisappear;
		bool bLeaveCorpse;
		bool bDestructableCorpse;
		CDBPtr< SBurningFuel > pShootOnDestruction;

		#include "include_staticobjectrpgstats.h"

		SStaticObjectRPGStats() :
			__dwCheckSum( 0 ),
			nAIPassabilityClass( 0 ),
			bBurn( false ),
			bLeaveCorpse( false ),
			bDestructableCorpse( true )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SCraterSet : public CResource
	{
		OBJECT_BASIC_METHODS( SCraterSet )
	public:
		enum { typeID = 0x120AEBC0 };

		struct SSingleSeasonCraters
		{
		private:
			mutable DWORD __dwCheckSum;
		public:

			struct SCraterDesc
			{
			private:
				mutable DWORD __dwCheckSum;
			public:
				CDBPtr< SMaterial > pMaterial;
				float fScale;

				SCraterDesc() :
					__dwCheckSum( 0 ),
					fScale( 0.0500f )
				{ }
				//
				void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
				//
				int operator&( IBinSaver &saver );
				int operator&( IXmlSaver &saver );
				DWORD CalcCheckSum() const;
			};
			ESeason eSeason;
			std::vector< SCraterDesc > craters;

			SSingleSeasonCraters() :
				__dwCheckSum( 0 ),
				eSeason( SEASON_WINTER )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		std::vector< SSingleSeasonCraters > craters;

		SCraterSet() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SProjectile : public CResource
	{
		OBJECT_BASIC_METHODS( SProjectile )
	public:
		enum { typeID = 0x300C3B80 };
		CDBPtr< SModel > pModel;
		CDBPtr< SComplexEffect > pSmokyExhaustEffect;
		std::string szSmokyEffectLocator;
		float fSmokyExhaustEffectInterval;
		CDBPtr< SComplexEffect > pAttachedEffect;
		CDBPtr< SComplexEffect > pEffectBeforeHit;
		std::string szAttachedEffectLocator;

		SProjectile() :
			fSmokyExhaustEffectInterval( 2 )
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

	struct SWeaponRPGStats : public SCommonRPGStats
	{
		OBJECT_BASIC_METHODS( SWeaponRPGStats )
	public:
		enum { typeID = 0x11069B82 };
	private:
		mutable DWORD __dwCheckSum;
	public:

		struct SShell
		{
		private:
			mutable DWORD __dwCheckSum;
		public:

			enum ETrajectoryType
			{
				TRAJECTORY_LINE = 0,
				TRAJECTORY_HOWITZER = 1,
				TRAJECTORY_BOMB = 2,
				TRAJECTORY_CANNON = 3,
				TRAJECTORY_ROCKET = 4,
				TRAJECTORY_GRENADE = 5,
				TRAJECTORY_TORPEDO = 6,
				TRAJECTORY_AA_ROCKET = 7,
				TRAJECTORY_FLAME_THROWER = 8,
			};

			enum EShellDamageType
			{
				DAMAGE_HEALTH = 0,
				DAMAGE_MORALE = 1,
				DAMAGE_FOG = 2,
			};
			EShellDamageType eDamageType;
			int nPiercing;
			int nDamageRandom;
			float fDamagePower;
			int nPiercingRandom;
			float fArea;
			float fArea2;
			float fSpeed;
			float fTraceSpeedCoeff;
			float fTraceProbability;
			float fTraceLength;
			float fTraceWidth;
			CDBPtr< SMaterial > pTraceMaterial;
			float fDetonationPower;
			ETrajectoryType etrajectory;
			float fBrokeTrackProbability;
			std::string szFireSound;
			CDBPtr< SComplexEffect > pEffectGunFire;
			CDBPtr< SComplexEffect > pEffectTrajectory;
			CDBPtr< SComplexEffect > pEffectHitDirect;
			CDBPtr< SComplexEffect > pEffectHitMiss;
			CDBPtr< SComplexEffect > pEffectHitReflect;
			CDBPtr< SComplexEffect > pEffectHitGround;
			CDBPtr< SComplexEffect > pEffectHitWater;
			CDBPtr< SComplexEffect > pEffectHitAir;
			CDBPtr< SCraterSet > pCraters;
			float fFireRate;
			float fRelaxTime;
			CDBPtr< SProjectile > pvisProjectile;

			#include "include_weaponrpgstats_shell.h"

			SShell() :
				__dwCheckSum( 0 ),
				eDamageType( DAMAGE_HEALTH ),
				nPiercing( 0 ),
				nDamageRandom( 100 ),
				fDamagePower( 50 ),
				nPiercingRandom( 100 ),
				fArea( 0.0f ),
				fArea2( 0.0f ),
				fSpeed( 800 ),
				fTraceSpeedCoeff( 0.0f ),
				fTraceProbability( 0.0f ),
				fTraceLength( 0.0f ),
				fTraceWidth( 0.0f ),
				fDetonationPower( 0.0f ),
				etrajectory( TRAJECTORY_LINE ),
				fBrokeTrackProbability( 0.0f ),
				fFireRate( 0.0f ),
				fRelaxTime( 0.0f )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		enum EWeaponType
		{
			WEAPON_PISTOL = 0,
			WEAPON_MACHINEGUN = 1,
			WEAPON_SUBMACHINEGUN = 2,
			WEAPON_RIFLE = 3,
			WEAPON_SNIPER_RIFLE = 4,
			WEAPON_ANTITANK_RIFLE = 5,
			WEAPON_BAZOOKA = 6,
			WEAPON_PIAT = 7,
			WEAPON_RIFLE_AMERICAN = 8,
			WEAPON_FLAME_THROWER = 9,
			WEAPON_STEN = 10,
			WEAPON_PANZERFAUST = 11,
			WEAPON_LUFTFAUST = 12,
			WEAPON_HEAVY_CANNON = 13,
			WEAPON_HIDED = 14,
			_WEAPON_COUNTER = 15,
		};
		EWeaponType eWeaponType;
		CDBPtr< STexture > pWeaponTypeTexture;
		float fDispersion;
		float fAimingTime;
		int nAmmoPerBurst;
		float fRangeMax;
		float fRangeMin;
		int nCeiling;
		float fRevealRadius;
		float fDeltaAngle;
		std::vector< SShell > shells;
		CDBPtr< SVisObj > pVisObj;
		NFile::CFilePath szLocalizedNameFileRef;

		#include "include_weaponrpgstats.h"

		SWeaponRPGStats() :
			__dwCheckSum( 0 ),
			eWeaponType( WEAPON_HEAVY_CANNON ),
			fDispersion( 0.0f ),
			fAimingTime( 0.0f ),
			nAmmoPerBurst( 0 ),
			fRangeMax( 0.0f ),
			fRangeMin( 0.0f ),
			nCeiling( 0 ),
			fRevealRadius( 0.0f ),
			fDeltaAngle( 0.0f )
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

	struct SBaseGunRPGStats
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SWeaponRPGStats > pWeapon;
		CDBPtr< SAttachedModelVisObj > pAttachedGunVisObj;
		std::string szAttachedGunLocator;
		CVec3 vShootPointOffset;
		bool bShootEffectInvert;
		int nPriority;
		bool bIsPrimary;
		int nAmmo;
		float fDirection;
		float fReloadCost;
		bool bTargetAAOnly;

		#include "include_basegunrpgstats.h"

		SBaseGunRPGStats() :
			__dwCheckSum( 0 ),
			vShootPointOffset( VNULL3 ),
			bShootEffectInvert( false ),
			nPriority( 0 ),
			bIsPrimary( false ),
			nAmmo( 0 ),
			fDirection( 0.0f ),
			fReloadCost( 1 ),
			bTargetAAOnly( false )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SDynamicDebrisSet : public CResource
	{
		OBJECT_BASIC_METHODS( SDynamicDebrisSet )
	public:
		enum { typeID = 0x140BAB41 };

		struct SDynamicDebrisDesc
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CDBPtr< STexture > pTexture;
			ESeason eSeason;
			float fWidth;

			SDynamicDebrisDesc() :
				__dwCheckSum( 0 ),
				eSeason( SEASON_SUMMER ),
				fWidth( 0.0f )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		std::vector< SDynamicDebrisDesc > debris;

		SDynamicDebrisSet() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SObjectBaseRPGStats : public SStaticObjectRPGStats
	{
	public:
	private:
		mutable DWORD __dwCheckSum;
	public:

		struct SDynamicDebris
		{
		private:
			mutable DWORD __dwCheckSum;
		public:

			struct SDynamicMaskDesc
			{
			private:
				mutable DWORD __dwCheckSum;
			public:
				CDBPtr< SMaterial > pMaterial;
				CVec2 vOrigin;
				CVec2 vSize;
				ESeason eSeason;
				float fWidth;

				SDynamicMaskDesc() :
					__dwCheckSum( 0 ),
					vOrigin( VNULL2 ),
					vSize( VNULL2 ),
					eSeason( SEASON_SUMMER ),
					fWidth( 0.0f )
				{ }
				//
				void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
				//
				int operator&( IBinSaver &saver );
				int operator&( IXmlSaver &saver );
				DWORD CalcCheckSum() const;
			};
			CDBPtr< SDynamicDebrisSet > pDebris;
			std::vector< SDynamicMaskDesc > masks;

			SDynamicDebris() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SAmbientSound
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CDBPtr< SComplexSoundDesc > pSoundDesc;
			ESeason eSeason;
			int nDayTime;

			SAmbientSound() :
				__dwCheckSum( 0 ),
				eSeason( SEASON_SUMMER ),
				nDayTime( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		SDynamicDebris dynamicDebris;
		SByteArray2 passability;
		SPassProfile passProfile;
		CVec2 vOrigin;
		bool bUsePassabilityForVisibility;
		SByteArray2 visibility;
		CVec2 vVisOrigin;
		CDBPtr< SComplexSoundDesc > pAmbientSound;
		CDBPtr< SComplexSoundDesc > pCycledSound;
		std::vector< SAmbientSound > ambientSounds;
		std::vector< CDBPtr< SComplexSoundDesc > > cycledSoundTimed;
		bool bCanFall;
		int nObjectHeight;
		CDBPtr< SComplexEffect > pFallEffect;
		int nFallDuration;
		float fFallCycles;
		CDBPtr< SComplexSeasonedEffect > pSeasonedEffectExplosion;
		CDBPtr< SComplexSeasonedEffect > pSeasonedEffectDeath;
		CDBPtr< SComplexSeasonedEffect > pSeasonedFallEffect;

		#include "include_objectbaserpgstats.h"

		SObjectBaseRPGStats() :
			__dwCheckSum( 0 ),
			vOrigin( VNULL2 ),
			bUsePassabilityForVisibility( true ),
			vVisOrigin( VNULL2 ),
			bCanFall( false ),
			nObjectHeight( 1 ),
			nFallDuration( 3000 ),
			fFallCycles( 2 )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct STerraObjSetRPGStats : public SStaticObjectRPGStats
	{
		OBJECT_BASIC_METHODS( STerraObjSetRPGStats )
	public:
		enum { typeID = 0x11069BC1 };
	private:
		mutable DWORD __dwCheckSum;
	public:

		struct SSegment
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			SByteArray2 passability;
			CVec2 vOrigin;
			SByteArray2 visibility;
			CVec2 vVisOrigin;

			#include "include_terraobjsetrpgstats_segment.h"

			SSegment() :
				__dwCheckSum( 0 ),
				vOrigin( VNULL2 ),
				vVisOrigin( VNULL2 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		std::vector< SSegment > segments;
		std::vector< int > fronts;
		std::vector< int > backs;

		#include "include_terraobjsetrpgstats.h"

		STerraObjSetRPGStats() :
			__dwCheckSum( 0 )
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

	struct SObjectRPGStats : public SObjectBaseRPGStats
	{
		OBJECT_BASIC_METHODS( SObjectRPGStats )
	public:
		enum { typeID = 0x11069BC4 };
	private:
		mutable DWORD __dwCheckSum;
	public:
		std::vector< std::string > specificJoints;
		bool bHideForPerformance;

		SObjectRPGStats() :
			__dwCheckSum( 0 ),
			bHideForPerformance( false )
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

	enum EBuildingType
	{
		TYPE_BULDING = 0,
		TYPE_MAIN_RU_STORAGE = 1,
		TYPE_TEMP_RU_STORAGE = 2,
		TYPE_DOT = 3,
	};

	enum EDesignBuildingType
	{
		BUILDING_TYPE_UNKNOWN = 0,
		Small = 1,
		Medium = 2,
		Large = 3,
	};

	struct SSlotDamageLevel
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		float fDamageHP;
		CDBPtr< SVisObj > pVisObj;
		CDBPtr< SComplexEffect > pDamageEffect;

		SSlotDamageLevel() :
			__dwCheckSum( 0 ),
			fDamageHP( 0.0f )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SWindowInfo
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SVisObj > pDayObj;
		CDBPtr< SVisObj > pNightObj;
		CDBPtr< SVisObj > pDestroyedObj;
		CDBPtr< SComplexEffect > pDestroyEffect;
		std::vector< SSlotDamageLevel > dayDamageLevels;
		std::vector< SSlotDamageLevel > nightDamageLevels;
		float fMaxHP;

		SWindowInfo() :
			__dwCheckSum( 0 ),
			fMaxHP( 100 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SBuildingRPGStats : public SObjectBaseRPGStats
	{
		OBJECT_BASIC_METHODS( SBuildingRPGStats )
	public:
		enum { typeID = 0x11069BC9 };
	private:
		mutable DWORD __dwCheckSum;
	public:

		struct SEntrance
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CVec3 vPos;
			bool bStormable;
			int nDir;

			#include "include_buildingrpgstats_entrance.h"

			SEntrance() :
				__dwCheckSum( 0 ),
				vPos( VNULL3 ),
				bStormable( false ),
				nDir( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SSlot
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			std::string szLocatorName;
			int nNumFirePlaces;
			CVec3 vPos;
			CVec3 vDamageCenter;
			CQuat qRot;
			float fDirection;
			float fAngle;
			float fCoverage;
			SBaseGunRPGStats gun;
			float fRotationSpeed;
			SWindowInfo window;
			CVec2 vWindowScale;
			CDBPtr< SUnitStatsModifier > pSoldierStatsModifier;
			float fSightMultiplier;

			#include "include_buildingrpgstats_slot.h"

			SSlot() :
				__dwCheckSum( 0 ),
				nNumFirePlaces( 1 ),
				vPos( VNULL3 ),
				vDamageCenter( VNULL3 ),
				qRot( QNULL ),
				fDirection( 0.0f ),
				fAngle( 180 ),
				fCoverage( 1 ),
				fRotationSpeed( 1 ),
				vWindowScale( VNULL2 ),
				fSightMultiplier( 1 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SFirePoint
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CVec3 vPos;
			float fDirection;
			float fVerticalAngle;
			std::string szFireEffect;
			float fCoverage;
			CVec2 vPicturePosition;

			#include "include_buildingrpgstats_firepoint.h"

			SFirePoint() :
				__dwCheckSum( 0 ),
				vPos( VNULL3 ),
				fDirection( 0.0f ),
				fVerticalAngle( 0.0f ),
				fCoverage( 0.0f ),
				vPicturePosition( VNULL2 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SDirectionExplosion
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CVec3 vPos;
			float fDirection;
			float fVerticalAngle;
			CVec2 vPicturePosition;

			#include "include_buildingrpgstats_directionexplosion.h"

			SDirectionExplosion() :
				__dwCheckSum( 0 ),
				vPos( VNULL3 ),
				fDirection( 0.0f ),
				fVerticalAngle( 0.0f ),
				vPicturePosition( VNULL2 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		EBuildingType etype;
		int nRestSlots;
		int nMedicalSlots;
		CDBPtr< SWeaponRPGStats > pPrimaryGun;
		std::vector< SSlot > slots;
		std::vector< SEntrance > entrances;
		std::vector< SFirePoint > firePoints;
		std::vector< SFirePoint > smokePoints;
		std::vector< SDirectionExplosion > dirExplosions;
		float fSightMultiplier;
		CDBPtr< SUnitStatsModifier > pSoldierStatsModifier;
		EDesignBuildingType eBuildingType;
		bool bVisibilitySrink;
		CDBPtr< SArmorPattern > pArmorPattern;
		float fDamageCoeff;

		#include "include_buildingrpgstats.h"

		SBuildingRPGStats() :
			__dwCheckSum( 0 ),
			etype( TYPE_BULDING ),
			nRestSlots( 0 ),
			nMedicalSlots( 0 ),
			fSightMultiplier( 1 ),
			eBuildingType( BUILDING_TYPE_UNKNOWN ),
			bVisibilitySrink( true ),
			fDamageCoeff( 0.0f )
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

	struct SBridgeRPGStats : public SStaticObjectRPGStats
	{
		OBJECT_BASIC_METHODS( SBridgeRPGStats )
	public:
		enum { typeID = 0x11069BCA };
	private:
		mutable DWORD __dwCheckSum;
	public:

		enum EDirection
		{
			VERTICAL = 0,
			HORIZONTAL = 1,
		};

		struct SSegmentRPGStats
		{
		private:
			mutable DWORD __dwCheckSum;
		public:

			enum ESegmentType
			{
				SLAB = 0,
				GIRDER = 1,
			};
			ESegmentType eType;
			SByteArray2 passability;
			CVec2 vOrigin;
			bool bUsePassabilityForVisibility;
			SByteArray2 visibility;
			CVec2 vVisOrigin;
			CVec2 vRelPos;
			CDBPtr< SVisObj > pVisObj;
			int nFrameIndex;

			#include "include_bridgerpgstats_segmentrpgstats.h"

			SSegmentRPGStats() :
				__dwCheckSum( 0 ),
				eType( SLAB ),
				vOrigin( VNULL2 ),
				bUsePassabilityForVisibility( true ),
				vVisOrigin( VNULL2 ),
				vRelPos( VNULL2 ),
				nFrameIndex( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SSpan
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			int nSlab;
			int nBackGirder;
			int nFrontGirder;
			float fWidth;
			float fLength;

			SSpan() :
				__dwCheckSum( 0 ),
				nSlab( 0 ),
				nBackGirder( 0 ),
				nFrontGirder( 0 ),
				fWidth( 0.0f ),
				fLength( 0.0f )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SDamageState
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			std::vector< SSpan > spans;
			std::vector< int > begins;
			std::vector< int > lines;
			std::vector< int > ends;

			SDamageState() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SBridgeFirePoint
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CVec2 vPos;
			float fDirection;
			float fVerticalAngle;
			std::string szFireEffect;
			CVec2 vPicturePosition;

			#include "include_bridgerpgstats_firepoint.h"

			SBridgeFirePoint() :
				__dwCheckSum( 0 ),
				vPos( VNULL2 ),
				fDirection( 0.0f ),
				fVerticalAngle( 0.0f ),
				vPicturePosition( VNULL2 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SBridgeDirectionExplosion
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CVec2 vPos;
			float fDirection;
			float fVerticalAngle;
			std::string szFireEffect;
			CVec2 vPicturePosition;

			#include "include_bridgerpgstats_directionexplosion.h"

			SBridgeDirectionExplosion() :
				__dwCheckSum( 0 ),
				vPos( VNULL2 ),
				fDirection( 0.0f ),
				fVerticalAngle( 0.0f ),
				vPicturePosition( VNULL2 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SElementRPGStats
		{
		private:
			mutable DWORD __dwCheckSum;
		public:

			struct SBridgeDamageState
			{
			private:
				mutable DWORD __dwCheckSum;
			public:
				float fDamageHP;
				std::vector< CDBPtr< SVisObj > > visObjects;
				CDBPtr< SComplexEffect > pSmokeEffect;

				SBridgeDamageState() :
					__dwCheckSum( 0 ),
					fDamageHP( 0.0f )
				{ }
				//
				void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
				//
				int operator&( IBinSaver &saver );
				int operator&( IXmlSaver &saver );
				DWORD CalcCheckSum() const;
			};
			SByteArray2 passability;
			CVec2 vOrigin;
			CVec2 vSize;
			std::vector< SBridgeDamageState > damageStates;
			std::vector< CDBPtr< SVisObj > > visualObjects;

			SElementRPGStats() :
				__dwCheckSum( 0 ),
				vOrigin( VNULL2 ),
				vSize( VNULL2 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		SElementRPGStats center;
		SElementRPGStats end;
		float fHeight;
		EDirection edirection;
		std::vector< SSegmentRPGStats > segments;
		std::vector< SDamageState > states;
		std::vector< SBridgeFirePoint > firePoints;
		std::vector< SBridgeFirePoint > smokePoints;
		std::string szSmokeEffect;
		std::vector< SBridgeDirectionExplosion > dirExplosions;
		std::string szDirExplosionEffect;
		float fSightMultiplier;

		#include "include_bridgerpgstats.h"

		SBridgeRPGStats() :
			__dwCheckSum( 0 ),
			fHeight( 0.0f ),
			edirection( VERTICAL ),
			fSightMultiplier( 1 )
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

	enum EEntrenchSegmType
	{
		EST_LINE = 0,
		EST_FIREPLACE = 1,
		EST_TERMINATOR = 2,
		EST_ARC = 3,
	};

	struct SEntrenchmentRPGStats : public SHPObjectRPGStats
	{
		OBJECT_BASIC_METHODS( SEntrenchmentRPGStats )
	public:
		enum { typeID = 0x11069BC8 };
	private:
		mutable DWORD __dwCheckSum;
	public:

		struct SEntrenchSegmentRPGStats
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CDBPtr< SVisObj > pVisObj;
			CVec2 vAABBCenter;
			CVec3 vAABBHalfSize;
			float fCoverage;
			std::vector< CVec2 > fireplaces;
			EEntrenchSegmType eType;

			#include "include_entrenchmentrpgstats_segmentrpgstats.h"

			SEntrenchSegmentRPGStats() :
				__dwCheckSum( 0 ),
				vAABBCenter( VNULL2 ),
				vAABBHalfSize( VNULL3 ),
				fCoverage( 0.0f ),
				eType( EST_LINE )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		std::vector< SEntrenchSegmentRPGStats > segments;
		std::vector< int > lines;
		std::vector< int > fireplaces;
		std::vector< int > terminators;
		std::vector< int > arcs;
		CDBPtr< SUnitStatsModifier > pInnerUnitBonus;

		#include "include_entrenchmentrpgstats.h"

		SEntrenchmentRPGStats() :
			__dwCheckSum( 0 )
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

	enum EFenceDirection
	{
		FENCE_DIRECTION_0 = 0,
		FENCE_DIRECTION_1 = 1,
		FENCE_DIRECTION_2 = 2,
		FENCE_DIRECTION_3 = 3,
	};

	enum EFenceDamageType
	{
		FENCE_TYPE_NORMAL = 0,
		FENCE_TYPE_LDAMAGE = 1,
		FENCE_TYPE_RDAMAGE = 2,
		FENCE_TYPE_CDAMAGE = 3,
	};

	struct SFenceRPGStats : public SStaticObjectRPGStats
	{
		OBJECT_BASIC_METHODS( SFenceRPGStats )
	public:
		enum { typeID = 0x11069BC7 };
	private:
		mutable DWORD __dwCheckSum;
	public:

		enum EFencePlacementMode
		{
			FENCE_PLACE_ON_TERRAIN = 0,
			FENCE_PLACE_STAGGERED = 1,
		};

		struct SFenceSegmentRPGStats
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			SByteArray2 passability;
			CVec2 vOrigin;
			SByteArray2 visibility;
			CVec2 vVisOrigin;
			int nSpriteIndex;

			#include "include_fencerpgstats_segmentrpgstats.h"

			SFenceSegmentRPGStats() :
				__dwCheckSum( 0 ),
				vOrigin( VNULL2 ),
				vVisOrigin( VNULL2 ),
				nSpriteIndex( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SDir
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			std::vector< int > centers;
			std::vector< int > ldamages;
			std::vector< int > rdamages;
			std::vector< int > cdamages;

			SDir() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SSegments
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			std::vector< CDBPtr< SVisObj > > visObjes;
			SByteArray2 passability;
			SPassProfile passProfile;
			bool bUsePassabilityForVisibility;
			CVec2 vOrigin;

			SSegments() :
				__dwCheckSum( 0 ),
				bUsePassabilityForVisibility( false ),
				vOrigin( VNULL2 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		std::vector< SFenceSegmentRPGStats > stats;
		std::vector< SDir > dirs;
		EFencePlacementMode ePlacementType;
		SSegments centerSegments;
		SSegments damagedSegments;
		SSegments damagedSegmentsOtherSide;
		SSegments destroyedSegments;
		float fFenceHeight;

		#include "include_fencerpgstats.h"

		SFenceRPGStats() :
			__dwCheckSum( 0 ),
			ePlacementType( FENCE_PLACE_STAGGERED ),
			fFenceHeight( 100 )
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

	enum EMineType
	{
		MT_INFANTRY = 0,
		MT_TECHNICS = 1,
		MT_CHARGE = 2,
		MT_LANDMINE = 3,
		MT_INFANTRY_AND_TECHNICS = 4,
	};

	struct SMineRPGStats : public SObjectBaseRPGStats
	{
		OBJECT_BASIC_METHODS( SMineRPGStats )
	public:
		enum { typeID = 0x11069BC5 };
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SWeaponRPGStats > pWeapon;
		EMineType etype;
		float fWeight;
		std::string szFlagModel;
		int nTriggerRange;
		bool bRangeDetonator;

		SMineRPGStats() :
			__dwCheckSum( 0 ),
			etype( MT_INFANTRY ),
			fWeight( 0.0f ),
			nTriggerRange( 3 ),
			bRangeDetonator( false )
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

	struct SAnimDesc
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nLength;
		int nAction;
		int nFrameIndex;
		int nAABB_A;
		int nAABB_D;
		CDBPtr< SAnimBase > pAnimation;

		SAnimDesc() :
			__dwCheckSum( 0 ),
			nLength( 0 ),
			nAction( 0 ),
			nFrameIndex( 0 ),
			nAABB_A( 0 ),
			nAABB_D( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct Svector_AnimDescs
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		std::vector< SAnimDesc > anims;

		Svector_AnimDescs() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SUnitActions : public CResource
	{
		OBJECT_BASIC_METHODS( SUnitActions )
	public:
		enum { typeID = 0x11141380 };
	private:
		mutable DWORD __dwCheckSum;
	public:
		CUserCommands availCommands;
		CUserCommands availExposures;
		CUserActions availUserActions;
		CUserActions availUserExposures;
		std::vector< CDBPtr< SUnitSpecialAblityDesc > > specialAbilities;

		#include "include_unitbaserpgstats_actions.h"

		SUnitActions() :
			__dwCheckSum( 0 )
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

	enum EDBUnitRPGType
	{
		DB_RPG_TYPE_SOLDIER = 0,
		DB_RPG_TYPE_ENGINEER = 1,
		DB_RPG_TYPE_SNIPER = 2,
		DB_RPG_TYPE_OFFICER = 3,
		DB_RPG_TYPE_TRN_CARRIER = 4,
		DB_RPG_TYPE_TRN_SUPPORT = 5,
		DB_RPG_TYPE_TRN_MEDICINE = 6,
		DB_RPG_TYPE_TRN_TRACTOR = 7,
		DB_RPG_TYPE_TRN_MILITARY_AUTO = 8,
		DB_RPG_TYPE_TRN_CIVILIAN_AUTO = 9,
		DB_RPG_TYPE_ART_GUN = 10,
		DB_RPG_TYPE_ART_HOWITZER = 11,
		DB_RPG_TYPE_ART_HEAVY_GUN = 12,
		DB_RPG_TYPE_ART_AAGUN = 13,
		DB_RPG_TYPE_ART_ROCKET = 14,
		DB_RPG_TYPE_ART_SUPER = 15,
		DB_RPG_TYPE_ART_MORTAR = 16,
		DB_RPG_TYPE_ART_HEAVY_MG = 17,
		DB_RPG_TYPE_SPG_ASSAULT = 18,
		DB_RPG_TYPE_SPG_ANTITANK = 19,
		DB_RPG_TYPE_SPG_SUPER = 20,
		DB_RPG_TYPE_SPG_AAGUN = 21,
		DB_RPG_TYPE_ARM_LIGHT = 22,
		DB_RPG_TYPE_ARM_MEDIUM = 23,
		DB_RPG_TYPE_ARM_HEAVY = 24,
		DB_RPG_TYPE_ARM_SUPER = 25,
		DB_RPG_TYPE_AVIA_SCOUT = 26,
		DB_RPG_TYPE_AVIA_BOMBER = 27,
		DB_RPG_TYPE_AVIA_ATTACK = 28,
		DB_RPG_TYPE_AVIA_FIGHTER = 29,
		DB_RPG_TYPE_AVIA_SUPER = 30,
		DB_RPG_TYPE_AVIA_LANDER = 31,
		DB_RPG_TYPE_TRAIN_LOCOMOTIVE = 32,
		DB_RPG_TYPE_TRAIN_CARGO = 33,
		DB_RPG_TYPE_TRAIN_CARRIER = 34,
		DB_RPG_TYPE_TRAIN_SUPER = 35,
		DB_RPG_TYPE_TRAIN_ARMOR = 36,
		DB_RPG_TYPE_COUNT = 37,
	};

	enum EUnitPoliticalSide
	{
		POLITICAL_SIDE_UNKNOWN = 0,
		POLITICAL_SIDE_ALLIES = 1,
		POLITICAL_SIDE_GERMAN = 2,
		POLITICAL_SIDE_JAPAN = 3,
		POLITICAL_SIDE_USSR = 4,
		POLITICAL_SIDE_USA = 5,
	};

	enum EEncyclopediaFilterUnitType
	{
		EFUT_UNKNOWN = 0,
		EFUT_ARTILLERY = 1,
		EFUT_ARMOR = 2,
		EFUT_AIR = 3,
		EFUT_SEA = 4,
		EFUT_TRANSPORT = 5,
		EFUT_MISC = 6,
		EFUT_COUNT = 7,
	};

	struct SBoundCircle
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		float fRadius;
		bool bIsRound;

		SBoundCircle() :
			__dwCheckSum( 0 ),
			fRadius( 0.0f ),
			bIsRound( false )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct STATS_B2_M1_EXPORT SUnitBaseRPGStats : public SHPObjectRPGStats
	{
	public:
	private:
		mutable DWORD __dwCheckSum;
	public:

		struct SAABBDesc
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CVec2 vCenter;
			CVec2 vHalfSize;

			#include "include_unitbaserpgstats_aabbdesc.h"

			SAABBDesc() :
				__dwCheckSum( 0 ),
				vCenter( VNULL2 ),
				vHalfSize( VNULL2 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		EDBUnitRPGType eDBtype;
		CDBPtr< SM1UnitType > pM1UnitTargetType;
		CDBPtr< SM1UnitSpecific > pM1UnitSpecific;
		CDBPtr< SM1UnitActions > pM1UnitActions;
		EUnitPoliticalSide ePoliticalSide;
		NFile::CFilePath szFullDescriptionFileRef;
		EEncyclopediaFilterUnitType eEncyclopediaFilterUnitType;
		int nAIPassabilityClass;
		std::vector< CDBPtr< SAckSetRPGStats > > acksNames;
		float fSight;
		float fSightPower;
		float fSpeed;
		float fRotateSpeed;
		float fPassability;
		int nPriority;
		float fCamouflage;
		int nMaxArmor;
		int nMinArmor;
		int nBoundTileRadius;
		float fWeight;
		float fPrice;
		CVec2 vAABBCenter;
		CVec2 vAABBHalfSize;
		SBoundCircle boundCircle;
		std::vector< SAABBDesc > aabb_as;
		std::vector< SAABBDesc > aabb_ds;
		std::vector< Svector_AnimDescs > animdescs;
		float fSmallAABBCoeff;
		CDBPtr< SUnitActions > pActions;
		float fUninstallTransport;
		float fUninstallRotate;
		CDBPtr< SArmorPattern > pArmorPattern;
		float fExpPrice;

		#include "include_unitbaserpgstats.h"

		SUnitBaseRPGStats() :
			__dwCheckSum( 0 ),
			eDBtype( DB_RPG_TYPE_SOLDIER ),
			ePoliticalSide( POLITICAL_SIDE_UNKNOWN ),
			eEncyclopediaFilterUnitType( EFUT_UNKNOWN ),
			nAIPassabilityClass( 0 ),
			fSight( 20 ),
			fSightPower( 1 ),
			fSpeed( 0.0f ),
			fRotateSpeed( 0.0f ),
			fPassability( 0.0f ),
			nPriority( 0 ),
			fCamouflage( 0.0f ),
			nMaxArmor( 0 ),
			nMinArmor( 0 ),
			nBoundTileRadius( 0 ),
			fWeight( 0.0f ),
			fPrice( 1 ),
			vAABBCenter( VNULL2 ),
			vAABBHalfSize( VNULL2 ),
			fSmallAABBCoeff( 0.0f ),
			fUninstallTransport( 0.0f ),
			fUninstallRotate( 0.0f ),
			fExpPrice( 1 )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SArmorPatternPlacement : public CResource
	{
		OBJECT_BASIC_METHODS( SArmorPatternPlacement )
	public:
		enum { typeID = 0x1711A341 };
		CTPoint<float> ptFrontPos;
		CTPoint<float> ptSidePos;
		CTPoint<float> ptBackPos;
		CTPoint<float> ptTopPos;

		SArmorPatternPlacement() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SArmorPattern : public CResource
	{
		OBJECT_BASIC_METHODS( SArmorPattern )
	public:
		enum { typeID = 0x1711A340 };
		CDBPtr< STexture > pPicture;
		CDBPtr< SArmorPatternPlacement > pPlacement;

		SArmorPattern() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SInfantryRPGStats : public SUnitBaseRPGStats
	{
		OBJECT_BASIC_METHODS( SInfantryRPGStats )
	public:
		enum { typeID = 0x11069B81 };
	private:
		mutable DWORD __dwCheckSum;
	public:

		struct SInfantryGun : public SBaseGunRPGStats
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			int nFake;

			SInfantryGun() :
				__dwCheckSum( 0 ),
				nFake( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		std::vector< SInfantryGun > guns;
		bool bCanAttackUp;
		bool bCanAttackDown;
		float fRunSpeed;
		float fCrawlSpeed;
		std::vector< int > animtimes;
		float fArmor;
		std::string szGunBoneName;

		#include "include_infantryrpgstats.h"

		SInfantryRPGStats() :
			__dwCheckSum( 0 ),
			bCanAttackUp( false ),
			bCanAttackDown( false ),
			fRunSpeed( 0.0f ),
			fCrawlSpeed( 0.0f ),
			fArmor( 0.0f )
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

	struct SBoardedMechUnitPosition
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CVec3 vPos;
		int nDirection;

		SBoardedMechUnitPosition() :
			__dwCheckSum( 0 ),
			vPos( VNULL3 ),
			nDirection( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SGunnersVector
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		std::vector< CVec2 > gunners;

		SGunnersVector() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	enum EDesignUnitType
	{
		UNIT_TYPE_UNKNOWN = 0,
		Heavy_Anti_Aircraft_Gun = 1,
		Light_Anti_Aircraft_Gun = 2,
		Anti_Aircraft_SPG = 3,
		Anti_Tank_Gun = 4,
		Assault_Gun = 5,
		Field_Artillery = 6,
		Heavy_Machine_Gun = 7,
		MLRS = 8,
		Engineering_Track = 9,
		Resupply_Track = 10,
		Tractor = 11,
		APC = 12,
		Armored_Fighting_Vehicle = 13,
		Fighter = 14,
		Bomber = 15,
		Ground_Attack_Plane = 16,
		Cargo_Plane = 17,
		Recon_Plane = 18,
		Coast_Gun = 19,
		Landing_Boat = 20,
		Mortar = 21,
		Light_Tank = 22,
		Medium_Tank = 23,
		Heavy_Tank = 24,
		Tank_Destroyer = 25,
		Torpedo_Boat = 26,
		Train = 27,
		Super = 28,
	};

	struct SMechUnitRPGStats : public SUnitBaseRPGStats
	{
		OBJECT_BASIC_METHODS( SMechUnitRPGStats )
	public:
		enum { typeID = 0x11069B80 };
	private:
		mutable DWORD __dwCheckSum;
	public:

		struct SMechUnitGun : public SBaseGunRPGStats
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			std::string szShootPoint;
			CVec3 vAIShootPointPos;
			bool bRecoil;
			float fRecoilLength;
			int nrecoilTime;
			int nRecoilShakeTime;
			float fRecoilShakeAngle;
			std::string szRecoilPoint;
			std::string szRotatePoint;
			int nModelPart;

			SMechUnitGun() :
				__dwCheckSum( 0 ),
				vAIShootPointPos( VNULL3 ),
				bRecoil( false ),
				fRecoilLength( 0.0f ),
				nrecoilTime( 0 ),
				nRecoilShakeTime( 0 ),
				fRecoilShakeAngle( 0.0f ),
				nModelPart( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SConstraint
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			float fMin;
			float fMax;

			#include "include_mechunitrpgstats_constraint.h"

			SConstraint() :
				__dwCheckSum( 0 ),
				fMin( 0.0f ),
				fMax( 0.0f )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SPlatform
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			float fRecoilShakeAngle;
			float fHorizontalRotationSpeed;
			float fVerticalRotationSpeed;
			int nModelPart;
			SConstraint constraint;
			SConstraint constraintVertical;
			int nGunCarriageParts;
			std::string szRotatePoint;
			CVec3 vAIRotatePointPos;
			CDBPtr< SAttachedModelVisObj > pAttachedPlatformVisObj;
			std::string szAttachedPlatformLocator;
			int nParentPlatform;
			std::vector< SMechUnitGun > guns;

			#include "include_mechunitrpgstats_platform.h"

			SPlatform() :
				__dwCheckSum( 0 ),
				fRecoilShakeAngle( 0.0f ),
				fHorizontalRotationSpeed( 0.0f ),
				fVerticalRotationSpeed( 0.0f ),
				nModelPart( 0 ),
				nGunCarriageParts( 0 ),
				vAIRotatePointPos( VNULL3 ),
				nParentPlatform( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SArmor
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			float fMin;
			float fMax;

			#include "include_mechunitrpgstats_armor.h"

			SArmor() :
				__dwCheckSum( 0 ),
				fMin( 100 ),
				fMax( 120 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SJoggingParams
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			float fPeriod1;
			float fPeriod2;
			float fAmp1;
			float fAmp2;
			float fPhaze1;
			float fPhaze2;

			SJoggingParams() :
				__dwCheckSum( 0 ),
				fPeriod1( 0.0f ),
				fPeriod2( 0.0f ),
				fAmp1( 0.0f ),
				fAmp2( 0.0f ),
				fPhaze1( 0.0f ),
				fPhaze2( 0.0f )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SSlotInfo
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			std::string szAttachedLocator;
			std::vector< CDBPtr< SAttachedModelVisObj > > attachedVisObjects;

			SSlotInfo() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SShipEffects
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CDBPtr< SComplexEffect > pBoardSideEffect;
			std::vector< std::string > boardSideLocators;
			CDBPtr< SComplexEffect > pRastrumEffect;
			std::vector< std::string > rastrumLocators;

			SShipEffects() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SSmokeTrailEffect
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			std::string szLocatorName;
			float fInterval;
			CDBPtr< SComplexEffect > pEffect;

			SSmokeTrailEffect() :
				__dwCheckSum( 0 ),
				fInterval( 2 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SCameraPlacement
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CVec3 vAnchor;
			float fDistance;
			float fPitch;
			float fYaw;
			float fFov;

			SCameraPlacement() :
				__dwCheckSum( 0 ),
				vAnchor( VNULL3 ),
				fDistance( 0.0f ),
				fPitch( 0.0f ),
				fYaw( 0.0f ),
				fFov( 0.0f )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		EDesignUnitType eUnitType;
		std::vector< SPlatform > platforms;
		std::vector< SSlotInfo > slots;
		std::vector< SArmor > armors;
		float fTowingForce;
		int nCrew;
		int nPassangers;
		float fTurnRadius;
		std::vector< std::string > exhaustPoints;
		std::vector< std::string > damagePoints;
		std::vector< int > peoplePointIndices;
		std::string szFatalitySmokePoint;
		std::string szShootDustPoint;
		CVec2 vTowPoint;
		CVec2 vEntrancePoint;
		std::vector< CVec2 > peoplePoints;
		CVec2 vAmmoPoint;
		std::vector< SGunnersVector > gunners;
		CVec2 vHookPoint;
		CVec2 vFrontWheel;
		CVec2 vBackWheel;
		CDBPtr< SComplexEffect > pEffectDiesel;
		CDBPtr< SComplexSeasonedEffect > pEffectWheelDust;
		CDBPtr< SComplexSeasonedEffect > pEffectWheelSplash;
		CDBPtr< SComplexEffect > pEffectSmoke;
		CDBPtr< SComplexEffect > pEffectFatality;
		std::vector< SSmokeTrailEffect > smokeTrails;
		CDBPtr< SComplexSeasonedEffect > pEffectShootDust;
		CDBPtr< SComplexSeasonedEffect > pEffectEntrenching;
		CDBPtr< SComplexEffect > pEffectDisappear;
		SJoggingParams jx;
		SJoggingParams jy;
		SJoggingParams jz;
		bool bLeavesTracks;
		float fTrackWidth;
		float fTrackOffset;
		float fTrackStart;
		float fTrackEnd;
		float fTrackIntensity;
		int nTrackLifetime;
		float fTrackFrequency;
		CDBPtr< SComplexSoundDesc > pSoundMoveStart;
		CDBPtr< SComplexSoundDesc > pSoundMoveCycle;
		CDBPtr< SComplexSoundDesc > pSoundMoveStop;
		CDBPtr< SComplexSoundDesc > pSoundIdle;
		CDBPtr< SComplexSoundDesc > pSoundDive;
		float fMaxHeight;
		float fDivingAngle;
		float fClimbAngle;
		float fTiltAngle;
		float fTiltRatio;
		float fTiltAcceleration;
		float fTiltSpeed;
		CDBPtr< SUnitStatsModifier > pGAPAirAttackModifier;
		CDBPtr< SCraterSet > pdeathCraters;
		float fReinforcementPrice;
		float fFuel;
		std::vector< EManuverID > allowedPlaneManuvers;
		CDBPtr< SVisObj > pAnimableModel;
		CDBPtr< SVisObj > pTransportableModel;
		SShipEffects shipEffects;
		std::vector< SBoardedMechUnitPosition > boardedMechUnitPosition;
		bool bDestructableCorpse;
		CDBPtr< SUnitStatsModifier > pInnerUnitBonus;

		#include "include_mechunitrpgstats.h"

		SMechUnitRPGStats() :
			__dwCheckSum( 0 ),
			eUnitType( UNIT_TYPE_UNKNOWN ),
			fTowingForce( 0.0f ),
			nCrew( 0 ),
			nPassangers( 0 ),
			fTurnRadius( 0.0f ),
			vTowPoint( VNULL2 ),
			vEntrancePoint( VNULL2 ),
			vAmmoPoint( VNULL2 ),
			vHookPoint( VNULL2 ),
			vFrontWheel( VNULL2 ),
			vBackWheel( VNULL2 ),
			bLeavesTracks( false ),
			fTrackWidth( 0.2000f ),
			fTrackOffset( 0 ),
			fTrackStart( 0.2000f ),
			fTrackEnd( 0.2000f ),
			fTrackIntensity( 0.7000f ),
			nTrackLifetime( 10000 ),
			fTrackFrequency( 0.5000f ),
			fMaxHeight( 0.0f ),
			fDivingAngle( 0.0f ),
			fClimbAngle( 0.0f ),
			fTiltAngle( 0.0f ),
			fTiltRatio( 0.0f ),
			fTiltAcceleration( 180 ),
			fTiltSpeed( 360 ),
			fReinforcementPrice( 1 ),
			fFuel( 1000 ),
			bDestructableCorpse( true )
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

	enum EEvent
	{
		HIT_NEAR = 0,
	};

	enum EDesignSquadType
	{
		SQUAD_TYPE_UNKNOWN = 0,
		Main_Squad = 1,
		Machine_Gun_Squad = 2,
		Anti_Tank_Squad = 3,
		Assault_Squad = 4,
		Special_Squad = 5,
		Single_Unit = 6,
	};

	struct SSquadRPGStats : public SHPObjectRPGStats
	{
		OBJECT_BASIC_METHODS( SSquadRPGStats )
	public:
		enum { typeID = 0x11069BC2 };
	private:
		mutable DWORD __dwCheckSum;
	public:

		enum ESquadType
		{
			RIFLEMANS = 0,
			INFANTRY = 1,
			SUBMACHINEGUNNERS = 2,
			MACHINEGUNNERS = 3,
			AT_TEAM = 4,
			MORTAR_TEAM = 5,
			SNIPERS = 6,
			GUNNERS = 7,
			ENGINEERS = 8,
		};

		struct SFormation
		{
		private:
			mutable DWORD __dwCheckSum;
		public:

			struct SEntry
			{
			private:
				mutable DWORD __dwCheckSum;
			public:
				CVec2 vPos;
				float fDir;

				#include "include_squadrpgstats_formation_entry.h"

				SEntry() :
					__dwCheckSum( 0 ),
					vPos( VNULL2 ),
					fDir( 0.0f )
				{ }
				//
				void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
				//
				int operator&( IBinSaver &saver );
				int operator&( IXmlSaver &saver );
				DWORD CalcCheckSum() const;
			};

			enum EFormationMoveType
			{
				DEFAULT = 0,
				MOVEMENT = 1,
				DEFENSIVE = 2,
				OFFENSIVE = 3,
				SNEAK = 4,
			};
			EFormationMoveType etype;
			std::vector< SEntry > order;
			int nLieFlag;
			CDBPtr< SUnitStatsModifier > pStatsModifiers;
			std::vector< int > changesByEvent;
			float fSpeedBonus;
			float fDispersionBonus;
			float fFireRateBonus;
			float fRelaxTimeBonus;
			float fCoverBonus;
			float fVisibleBonus;

			#include "include_squadrpgstats_formation.h"

			SFormation() :
				__dwCheckSum( 0 ),
				etype( DEFAULT ),
				nLieFlag( 0 ),
				fSpeedBonus( 1 ),
				fDispersionBonus( 1 ),
				fFireRateBonus( 1 ),
				fRelaxTimeBonus( 1 ),
				fCoverBonus( 1 ),
				fVisibleBonus( 1 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		EDesignSquadType eSquadType;
		std::string szIcon;
		ESquadType etype;
		std::vector< CDBPtr< SInfantryRPGStats > > members;
		std::vector< SFormation > formations;
		CUserCommands availCommands;
		CUserCommands availExposures;
		CUserActions availUserActions;
		CUserActions availUserExposures;
		float fReinforcementPrice;
		float fEntrenchCover;

		#include "include_squadrpgstats.h"

		SSquadRPGStats() :
			__dwCheckSum( 0 ),
			eSquadType( SQUAD_TYPE_UNKNOWN ),
			etype( RIFLEMANS ),
			fReinforcementPrice( 1 ),
			fEntrenchCover( 0.5000f )
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

	enum EReinforcementType
	{
		RT_MAIN_INFANTRY = 0,
		RT_ASSAULT_INFANTRY = 1,
		RT_ELITE_INFANTRY = 2,
		RT_ARTILLERY_ANTITANK = 3,
		RT_ARTILLERY = 4,
		RT_ASSAULT_GUNS = 5,
		RT_TANK_DESTROYERS = 6,
		RT_ARTILLERY_ROCKET = 7,
		RT_LIGHT_TANKS = 8,
		RT_TANKS = 9,
		RT_HEAVY_TANKS = 10,
		RT_LIGHT_AAA = 11,
		RT_HEAVY_AAA = 12,
		RT_FIGHTERS = 13,
		RT_BOMBERS = 14,
		RT_GROUND_ATTACK_PLANES = 15,
		RT_RECON = 16,
		RT_PARATROOPS = 17,
		RT_ENGINEERING = 18,
		RT_HEAVY_ARTILLERY = 19,
		RT_SUPER_WEAPON = 20,
		RT_EXTRA_GROUND_1 = 21,
		RT_EXTRA_GROUND_2 = 22,
		RT_EXTRA_GROUND_3 = 23,
		RT_EXTRA_GROUND_4 = 24,
		RT_EXTRA_GROUND_5 = 25,
		RT_EXTRA_GROUND_6 = 26,
		RT_EXTRA_GROUND_7 = 27,
		RT_EXTRA_GROUND_8 = 28,
		RT_EXTRA_GROUND_9 = 29,
		RT_EXTRA_GROUND_10 = 30,
		RT_EXTRA_AIR_1 = 31,
		RT_EXTRA_AIR_2 = 32,
		RT_EXTRA_AIR_3 = 33,
		RT_EXTRA_AIR_4 = 34,
		RT_EXTRA_AIR_5 = 35,
		RT_EXTRA_MIXED_1 = 36,
		RT_EXTRA_MIXED_2 = 37,
		RT_EXTRA_MIXED_3 = 38,
		RT_EXTRA_MIXED_4 = 39,
		RT_EXTRA_MIXED_5 = 40,
		RT_EXTRA_MAXLVL_GROUND_1 = 41,
		RT_EXTRA_MAXLVL_GROUND_2 = 42,
		RT_EXTRA_MAXLVL_GROUND_3 = 43,
		RT_EXTRA_MAXLVL_GROUND_4 = 44,
		RT_EXTRA_MAXLVL_GROUND_5 = 45,
		RT_EXTRA_MAXLVL_AIR_1 = 46,
		RT_EXTRA_MAXLVL_AIR_2 = 47,
		RT_EXTRA_MAXLVL_AIR_3 = 48,
		RT_EXTRA_MAXLVL_AIR_4 = 49,
		RT_EXTRA_MAXLVL_AIR_5 = 50,
		RT_EXTRA_MAXLVL_MIXED_1 = 51,
		RT_EXTRA_MAXLVL_MIXED_2 = 52,
		RT_EXTRA_MAXLVL_MIXED_3 = 53,
		RT_EXTRA_MAXLVL_MIXED_4 = 54,
		RT_EXTRA_MAXLVL_MIXED_5 = 55,
		_RT_NONE = 56,
	};

	struct SReinforcementEntry
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SMechUnitRPGStats > pMechUnit;
		CDBPtr< SSquadRPGStats > pSquad;
		int nLinkIndex;
		int nLinkWith;

		SReinforcementEntry() :
			__dwCheckSum( 0 ),
			nLinkIndex( -1 ),
			nLinkWith( -1 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SDeployTemplate : public CResource
	{
		OBJECT_BASIC_METHODS( SDeployTemplate )
	public:
		enum { typeID = 0x120C4CC0 };
	private:
		mutable DWORD __dwCheckSum;
	public:

		struct SDeployTemplateEntry
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CVec2 vPosition;
			int nDirection;

			SDeployTemplateEntry() :
				__dwCheckSum( 0 ),
				vPosition( VNULL2 ),
				nDirection( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		std::vector< SDeployTemplateEntry > entries;
		bool bIsDefault;
		EReinforcementType eReinforcementType;

		SDeployTemplate() :
			__dwCheckSum( 0 ),
			bIsDefault( true ),
			eReinforcementType( RT_MAIN_INFANTRY )
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

	struct SReinforcement : public CResource
	{
		OBJECT_BASIC_METHODS( SReinforcement )
	public:
		enum { typeID = 0x120A6B80 };
	private:
		mutable DWORD __dwCheckSum;
	public:
		EReinforcementType eType;
		CDBPtr< STexture > pIconTexture;
		NFile::CFilePath szLocalizedNameFileRef;
		std::vector< SReinforcementEntry > entries;
		NFile::CFilePath szTooltipFileRef;
		std::vector< CDBPtr< SMechUnitRPGStats > > transports;
		NFile::CFilePath szLocalizedDescFileRef;
		CDBPtr< SDeployTemplate > pTemplateOverride;

		#include "include_Reinforcement.h"

		SReinforcement() :
			__dwCheckSum( 0 ),
			eType( RT_TANKS )
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

	struct SPlayerRank : public CResource
	{
		OBJECT_BASIC_METHODS( SPlayerRank )
	public:
		enum { typeID = 0x120C5B00 };
	private:
		mutable DWORD __dwCheckSum;
	public:
		NFile::CFilePath szRankNameFileRef;
		CDBPtr< STexture > pStrap;
		CDBPtr< SPartyDependentInfo > pParty;

		#include "include_PlayerRank.h"

		SPlayerRank() :
			__dwCheckSum( 0 )
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

	struct SReinforcementTypeInfo
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		EReinforcementType eType;
		NFile::CFilePath szTooltipFileRef;
		EUserAction eUserAction;

		#include "include_ReinforcementTypeInfo.h"

		SReinforcementTypeInfo() :
			__dwCheckSum( 0 ),
			eType( RT_MAIN_INFANTRY ),
			eUserAction( USER_ACTION_REINF_COMMON )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SReinforcementTypes : public CResource
	{
		OBJECT_BASIC_METHODS( SReinforcementTypes )
	public:
		enum { typeID = 0x17146480 };
	private:
		mutable DWORD __dwCheckSum;
	public:
		std::vector< SReinforcementTypeInfo > typeInfo;

		SReinforcementTypes() :
			__dwCheckSum( 0 )
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

	struct SAIExpLevel : public CResource
	{
		OBJECT_BASIC_METHODS( SAIExpLevel )
	public:
		enum { typeID = 0x11069BCC };
	private:
		mutable DWORD __dwCheckSum;
	public:

		struct SLevel
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			float fExperience;
			CDBPtr< SUnitStatsModifier > pStatsBonus;

			SLevel() :
				__dwCheckSum( 0 ),
				fExperience( 1 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		std::string szTypeName;
		EReinforcementType eDBType;
		std::vector< SLevel > levels;

		#include "include_aiexplevel.h"

		SAIExpLevel() :
			__dwCheckSum( 0 ),
			eDBType( RT_MAIN_INFANTRY )
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

	struct STATS_B2_M1_EXPORT SUnitStatsModifier : public CResource
	{
		OBJECT_BASIC_METHODS( SUnitStatsModifier )
	public:
		enum { typeID = 0x19126C00 };
	private:
		mutable DWORD __dwCheckSum;
	public:

		struct SParameterModifier
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			float fAddBonus;
			float fMultBonus;
			int nZeroCount;

			#include "include_unitstatsmodifier_parametermodifier.h"

			SParameterModifier() :
				__dwCheckSum( 0 ),
				fAddBonus( 0 ),
				fMultBonus( 1 ),
				nZeroCount( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		SParameterModifier durability;
		SParameterModifier smallAABBCoeff;
		SParameterModifier camouflage;
		SParameterModifier sightPower;
		SParameterModifier sightRange;
		SParameterModifier speed;
		SParameterModifier rotateSpeed;
		SParameterModifier weaponDispersion;
		SParameterModifier weaponDamage;
		SParameterModifier weaponPiercing;
		SParameterModifier weaponTrackDamageProb;
		SParameterModifier weaponRelaxTime;
		SParameterModifier weaponAimTime;
		SParameterModifier weaponShellSpeed;
		SParameterModifier weaponArea;
		SParameterModifier weaponArea2;
		SParameterModifier cover;

		#include "include_unitstatsmodifier.h"

		SUnitStatsModifier() :
			__dwCheckSum( 0 )
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

namespace NDb
{
	std::string EnumToString( NDb::EUnitSpecialAbility eValue );
	EUnitSpecialAbility StringToEnum_NDb_EUnitSpecialAbility( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EUnitSpecialAbility>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EUnitSpecialAbility eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EUnitSpecialAbility ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EUnitSpecialAbility( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EUnitSpecialAbilityGroup eValue );
	EUnitSpecialAbilityGroup StringToEnum_NDb_EUnitSpecialAbilityGroup( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EUnitSpecialAbilityGroup>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EUnitSpecialAbilityGroup eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EUnitSpecialAbilityGroup ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EUnitSpecialAbilityGroup( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EArmorDirection eValue );
	EArmorDirection StringToEnum_NDb_EArmorDirection( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EArmorDirection>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EArmorDirection eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EArmorDirection ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EArmorDirection( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EObjGameType eValue );
	EObjGameType StringToEnum_NDb_EObjGameType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EObjGameType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EObjGameType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EObjGameType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EObjGameType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EObjectVisType eValue );
	EObjectVisType StringToEnum_NDb_EObjectVisType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EObjectVisType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EObjectVisType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EObjectVisType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EObjectVisType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::ESelectionType eValue );
	ESelectionType StringToEnum_NDb_ESelectionType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::ESelectionType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::ESelectionType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::ESelectionType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_ESelectionType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::SWeaponRPGStats::SShell::ETrajectoryType eValue );
	SWeaponRPGStats::SShell::ETrajectoryType StringToEnum_NDb_SWeaponRPGStats_SShell_ETrajectoryType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::SWeaponRPGStats::SShell::ETrajectoryType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::SWeaponRPGStats::SShell::ETrajectoryType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SWeaponRPGStats::SShell::ETrajectoryType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_SWeaponRPGStats_SShell_ETrajectoryType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::SWeaponRPGStats::SShell::EShellDamageType eValue );
	SWeaponRPGStats::SShell::EShellDamageType StringToEnum_NDb_SWeaponRPGStats_SShell_EShellDamageType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::SWeaponRPGStats::SShell::EShellDamageType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::SWeaponRPGStats::SShell::EShellDamageType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SWeaponRPGStats::SShell::EShellDamageType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_SWeaponRPGStats_SShell_EShellDamageType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::SWeaponRPGStats::EWeaponType eValue );
	SWeaponRPGStats::EWeaponType StringToEnum_NDb_SWeaponRPGStats_EWeaponType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::SWeaponRPGStats::EWeaponType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::SWeaponRPGStats::EWeaponType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SWeaponRPGStats::EWeaponType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_SWeaponRPGStats_EWeaponType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EBuildingType eValue );
	EBuildingType StringToEnum_NDb_EBuildingType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EBuildingType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EBuildingType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EBuildingType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EBuildingType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EDesignBuildingType eValue );
	EDesignBuildingType StringToEnum_NDb_EDesignBuildingType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EDesignBuildingType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EDesignBuildingType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EDesignBuildingType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EDesignBuildingType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::SBridgeRPGStats::EDirection eValue );
	SBridgeRPGStats::EDirection StringToEnum_NDb_SBridgeRPGStats_EDirection( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::SBridgeRPGStats::EDirection>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::SBridgeRPGStats::EDirection eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SBridgeRPGStats::EDirection ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_SBridgeRPGStats_EDirection( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::SBridgeRPGStats::SSegmentRPGStats::ESegmentType eValue );
	SBridgeRPGStats::SSegmentRPGStats::ESegmentType StringToEnum_NDb_SBridgeRPGStats_SSegmentRPGStats_ESegmentType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::SBridgeRPGStats::SSegmentRPGStats::ESegmentType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::SBridgeRPGStats::SSegmentRPGStats::ESegmentType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SBridgeRPGStats::SSegmentRPGStats::ESegmentType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_SBridgeRPGStats_SSegmentRPGStats_ESegmentType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EEntrenchSegmType eValue );
	EEntrenchSegmType StringToEnum_NDb_EEntrenchSegmType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EEntrenchSegmType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EEntrenchSegmType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EEntrenchSegmType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EEntrenchSegmType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EFenceDirection eValue );
	EFenceDirection StringToEnum_NDb_EFenceDirection( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EFenceDirection>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EFenceDirection eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EFenceDirection ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EFenceDirection( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EFenceDamageType eValue );
	EFenceDamageType StringToEnum_NDb_EFenceDamageType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EFenceDamageType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EFenceDamageType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EFenceDamageType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EFenceDamageType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::SFenceRPGStats::EFencePlacementMode eValue );
	SFenceRPGStats::EFencePlacementMode StringToEnum_NDb_SFenceRPGStats_EFencePlacementMode( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::SFenceRPGStats::EFencePlacementMode>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::SFenceRPGStats::EFencePlacementMode eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SFenceRPGStats::EFencePlacementMode ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_SFenceRPGStats_EFencePlacementMode( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EMineType eValue );
	EMineType StringToEnum_NDb_EMineType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EMineType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EMineType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EMineType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EMineType( szValue ); }
};

namespace NDb
{
	STATS_B2_M1_EXPORT std::string EnumToString( NDb::EDBUnitRPGType eValue );
	STATS_B2_M1_EXPORT EDBUnitRPGType StringToEnum_NDb_EDBUnitRPGType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EDBUnitRPGType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EDBUnitRPGType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EDBUnitRPGType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EDBUnitRPGType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EUnitPoliticalSide eValue );
	EUnitPoliticalSide StringToEnum_NDb_EUnitPoliticalSide( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EUnitPoliticalSide>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EUnitPoliticalSide eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EUnitPoliticalSide ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EUnitPoliticalSide( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EEncyclopediaFilterUnitType eValue );
	EEncyclopediaFilterUnitType StringToEnum_NDb_EEncyclopediaFilterUnitType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EEncyclopediaFilterUnitType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EEncyclopediaFilterUnitType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EEncyclopediaFilterUnitType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EEncyclopediaFilterUnitType( szValue ); }
};

namespace NDb
{
	STATS_B2_M1_EXPORT std::string EnumToString( NDb::EDesignUnitType eValue );
	STATS_B2_M1_EXPORT EDesignUnitType StringToEnum_NDb_EDesignUnitType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EDesignUnitType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EDesignUnitType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EDesignUnitType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EDesignUnitType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EEvent eValue );
	EEvent StringToEnum_NDb_EEvent( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EEvent>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EEvent eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EEvent ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EEvent( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EDesignSquadType eValue );
	EDesignSquadType StringToEnum_NDb_EDesignSquadType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EDesignSquadType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EDesignSquadType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EDesignSquadType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EDesignSquadType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::SSquadRPGStats::ESquadType eValue );
	SSquadRPGStats::ESquadType StringToEnum_NDb_SSquadRPGStats_ESquadType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::SSquadRPGStats::ESquadType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::SSquadRPGStats::ESquadType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SSquadRPGStats::ESquadType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_SSquadRPGStats_ESquadType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::SSquadRPGStats::SFormation::EFormationMoveType eValue );
	SSquadRPGStats::SFormation::EFormationMoveType StringToEnum_NDb_SSquadRPGStats_SFormation_EFormationMoveType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::SSquadRPGStats::SFormation::EFormationMoveType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::SSquadRPGStats::SFormation::EFormationMoveType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SSquadRPGStats::SFormation::EFormationMoveType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_SSquadRPGStats_SFormation_EFormationMoveType( szValue ); }
};

namespace NDb
{
	STATS_B2_M1_EXPORT std::string EnumToString( NDb::EReinforcementType eValue );
	STATS_B2_M1_EXPORT EReinforcementType StringToEnum_NDb_EReinforcementType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EReinforcementType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EReinforcementType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EReinforcementType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EReinforcementType( szValue ); }
};

