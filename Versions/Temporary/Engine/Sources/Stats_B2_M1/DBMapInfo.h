#pragma once

// automatically generated file, don't change manually!

#include "Stats_B2_M1_export.h"

#include "b2_m1_terrain/dbterrain.h"
#include "dbconstructorprofile.h"
#include "dbreinforcements.h"
#include "rpgstats.h"
#include "season.h"
#include "prefix_dbmapinfo.h"
#include "system/filepath.h"

struct IXmlSaver;

namespace NDb
{

	enum EMPGameType
	{
		MP_GT_STANDARD = 0,
		MP_GT_COUNT = 1,
	};

	struct SMPMapInfo
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< EMPGameType > gameTypes;

		SMPMapInfo() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};
	struct SDBConstructorProfile;
	enum ESeason;
	struct SMissionBonus;
	struct SMaterial;
	struct SComplexEffect;
	enum EParcelType;
	enum EReinforcementType;
	struct SReinforcement;
	struct SObjectBaseRPGStats;
	enum EDayNight;
	struct SMapMusic;
	struct SDeployTemplate;
	enum EDBUnitRPGType;
	struct SHPObjectRPGStats;
	enum EScriptAreaTypes;
	struct SMissionObjective;
	struct SPartyDependentInfo;
	struct STexture;
	struct SPlayerRank;
	struct SVisObj;
	enum EReinforcementType;
	struct SMapInfo;
	struct SReinforcement;
	struct SMissionBonus;
	struct SDifficultyLevel;

	struct STATS_B2_M1_EXPORT SCameraPlacement
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CVec3 vAnchor;
		float fYaw;
		float fPitch;
		float fDist;
		bool bUseAnchorOnly;

		SCameraPlacement() :
			__dwCheckSum( 0 ),
			vAnchor( VNULL3 ),
			fYaw( 0.0f ),
			fPitch( 0.0f ),
			fDist( 0.0f ),
			bUseAnchorOnly( true )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct STATS_B2_M1_EXPORT SScriptCameraPlacement
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		string szName;
		CVec3 vPosition;
		float fYaw;
		float fPitch;
		float fFOV;

		SScriptCameraPlacement() :
			__dwCheckSum( 0 ),
			vPosition( VNULL3 ),
			fYaw( 0.0f ),
			fPitch( 0.0f ),
			fFOV( 0.0f )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SScriptMovieKey
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		bool bIsTangentIn;
		bool bIsTangentOut;
		string szKeyParam;
		float fStartTime;

		SScriptMovieKey() :
			__dwCheckSum( 0 ),
			bIsTangentIn( false ),
			bIsTangentOut( false ),
			fStartTime( 0.0f )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SScriptMovieKeyPos : public SScriptMovieKey
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nPositionIndex;

		SScriptMovieKeyPos() :
			__dwCheckSum( 0 ),
			nPositionIndex( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SScriptMovieKeyFollow : public SScriptMovieKey
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nObjectScriptID;

		SScriptMovieKeyFollow() :
			__dwCheckSum( 0 ),
			nObjectScriptID( -1 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SScriptMovieSequence
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< SScriptMovieKeyPos > posKeys;
		vector< SScriptMovieKeyFollow > followKeys;

		#include "include_scriptmoviesequence.h"

		SScriptMovieSequence() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct STATS_B2_M1_EXPORT SScriptMovies
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< SScriptCameraPlacement > scriptCameraPlacements;
		vector< SScriptMovieSequence > scriptMovieSequences;

		#include "include_scriptmovies.h"

		SScriptMovies() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SPartyDependentInfo : public CResource
	{
		OBJECT_BASIC_METHODS( SPartyDependentInfo )
	public:
		enum { typeID = 0x11074C80 };
	private:
		mutable DWORD __dwCheckSum;
	public:
		string szGeneralPartyName;
		CDBPtr< SSquadRPGStats > pGunCrewSquad;
		CDBPtr< SSquadRPGStats > pHowitzerGunCrewSquad;
		CDBPtr< SSquadRPGStats > pHeavyMachinegunSquad;
		CDBPtr< SSquadRPGStats > pAAGunSquad;
		CDBPtr< SSquadRPGStats > pResupplyEngineerSquad;
		NFile::CFilePath szLocalizedNameFileRef;
		CDBPtr< STexture > pMinimapKeyObjectIcon;
		CDBPtr< STexture > pMinimapKeyObjectIconSelected;
		CDBPtr< STexture > pStatisticsIcon;
		CDBPtr< SVisObj > pParatrooperVisObj;
		CDBPtr< STexture > pListItemIcon;
		CDBPtr< SObjectBaseRPGStats > pKeyBuildingFlag;

		#include "include_PartyDependentInfo.h"

		SPartyDependentInfo() :
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

	struct SMissionObjective : public CResource
	{
		OBJECT_BASIC_METHODS( SMissionObjective )
	public:
		enum { typeID = 0x1711F2C0 };
		NFile::CFilePath szHeaderFileRef;
		NFile::CFilePath szBriefingFileRef;
		NFile::CFilePath szDescriptionFileRef;
		bool bIsPrimary;
		vector< CVec2 > mapPositions;
		int nExperience;

		#include "include_MissionObjective.h"

		SMissionObjective() :
			bIsPrimary( true ),
			nExperience( 0 )
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

	struct STATS_B2_M1_EXPORT SMapObjectInfo
	{
	private:
		mutable DWORD __dwCheckSum;
	public:

		struct SLinkInfo
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			int nLinkID;
			int nLinkWith;
			bool bIntention;

			SLinkInfo() :
				__dwCheckSum( 0 ),
				nLinkID( -1 ),
				nLinkWith( -1 ),
				bIntention( false )
			{ }
			//
			void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		CVec3 vPos;
		int nDir;
		int nPlayer;
		int nScriptID;
		float fHP;
		int nFrameIndex;
		SLinkInfo link;
		CDBPtr< SHPObjectRPGStats > pObject;
		CDBPtr< SDBConstructorProfile > pConstructorProfile;

		SMapObjectInfo() :
			__dwCheckSum( 0 ),
			vPos( VNULL3 ),
			nDir( 0 ),
			nPlayer( 0 ),
			nScriptID( -1 ),
			fHP( 1 ),
			nFrameIndex( -1 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SEntrenchmentInfo
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< SIntArray > sections;

		SEntrenchmentInfo() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	enum EScriptAreaTypes
	{
		EAT_RECTANGLE = 0,
		EAT_CIRCLE = 1,
	};

	struct STATS_B2_M1_EXPORT SScriptArea
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		EScriptAreaTypes eType;
		string szName;
		CVec2 vCenter;
		CVec2 vAABBHalfSize;
		float fR;

		SScriptArea() :
			__dwCheckSum( 0 ),
			eType( EAT_RECTANGLE ),
			vCenter( VNULL2 ),
			vAABBHalfSize( VNULL2 ),
			fR( 0.0f )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct STATS_B2_M1_EXPORT SAIStartCommand
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nCmdType;
		vector< int > unitLinkIDs;
		int nLinkID;
		CVec2 vPos;
		bool bFromExplosion;
		float fNumber;

		#include "include_aistartcommand.h"

		SAIStartCommand() :
			__dwCheckSum( 0 ),
			nCmdType( 0 ),
			nLinkID( 0 ),
			vPos( VNULL2 ),
			bFromExplosion( false ),
			fNumber( 0.0f )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct STATS_B2_M1_EXPORT SBattlePosition
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nArtilleryLinkID;
		int nTruckLinkID;
		CVec2 vPos;

		SBattlePosition() :
			__dwCheckSum( 0 ),
			nArtilleryLinkID( 0 ),
			nTruckLinkID( 0 ),
			vPos( VNULL2 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SMapSoundInfo
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SComplexSoundDesc > psound;
		CVec2 vPos;

		SMapSoundInfo() :
			__dwCheckSum( 0 ),
			vPos( VNULL2 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SEditAreaInfo
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		string szName;
		vector< CVec2 > points;

		SEditAreaInfo() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	enum EParcelType
	{
		EPATCH_UNKNOWN = 0,
		EPATCH_DEFENCE = 1,
		EPATCH_REINFORCE = 2,
	};

	struct SReinforcePoint
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CVec2 vCenter;
		float fDirection;

		#include "include_reinforcepoint.h"

		SReinforcePoint() :
			__dwCheckSum( 0 ),
			vCenter( VNULL2 ),
			fDirection( 0.0f )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct STATS_B2_M1_EXPORT SAIGeneralParcel
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< SReinforcePoint > reinforcePoints;
		EParcelType eType;
		CVec2 vCenter;
		float fRadius;
		float fImportance;
		float fDefenceDirection;
		int nMinUnitsToReinforce;

		#include "include_aigeneralparcelinfo.h"

		SAIGeneralParcel() :
			__dwCheckSum( 0 ),
			eType( EPATCH_UNKNOWN ),
			vCenter( VNULL2 ),
			fRadius( 0.0f ),
			fImportance( 0 ),
			fDefenceDirection( 0.0f ),
			nMinUnitsToReinforce( 3 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SAIGeneralSide
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< int > mobileScriptIDs;
		vector< SAIGeneralParcel > parcels;
		int nMaxMobileTanks;

		SAIGeneralSide() :
			__dwCheckSum( 0 ),
			nMaxMobileTanks( 20 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SBonusInstance
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nLinkID;

		SBonusInstance() :
			__dwCheckSum( 0 ),
			nLinkID( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SBuildingBonuses
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nPointID;

		SBuildingBonuses() :
			__dwCheckSum( 0 ),
			nPointID( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct STATS_B2_M1_EXPORT SPlayerBonusData
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nLinkID;
		vector< SBuildingBonuses > playerBonuses;
		bool bStorage;

		SPlayerBonusData() :
			__dwCheckSum( 0 ),
			nLinkID( 0 ),
			bStorage( false )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	enum ESuperWeaponType
	{
		SUPER_WEAPON_BOMBER = 0,
		SUPER_WEAPON_ROCKET = 1,
		SUPER_WEAPON_ARTILLERY = 2,
	};

	struct SMapPlayerInfo
	{
	private:
		mutable DWORD __dwCheckSum;
	public:

		struct SDeployPosition
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CVec2 vPosition;
			int nDirection;

			SDeployPosition() :
				__dwCheckSum( 0 ),
				vPosition( VNULL2 ),
				nDirection( 0 )
			{ }
			//
			void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};

		struct SSuperWeaponInfo
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			ESuperWeaponType eSuperWeaponType;
			int nCount;
			float fRecycleTime;
			float fFlyTime;

			SSuperWeaponInfo() :
				__dwCheckSum( 0 ),
				eSuperWeaponType( SUPER_WEAPON_BOMBER ),
				nCount( 0 ),
				fRecycleTime( 5.0000f ),
				fFlyTime( 0.0000f )
			{ }
			//
			void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		SCameraPlacement camera;
		SAIGeneralSide general;
		CDBPtr< SPartyDependentInfo > pPartyInfo;
		vector< SReinforcementPosition > reinforcementPoints;
		vector< CDBPtr< SReinforcement > > reinforcementTypes;
		CDBPtr< SPlayerRank > pDefaultRank;
		int nDiplomacySide;
		float fRecycleTimeCoefficient;
		int nReinforcementCalls;
		NFile::CFilePath szLocalizedPlayerNameFileRef;
		CVec2 vMPStartPos;
		vector< CDBPtr< SReinforcement > > scriptReinforcements;
		vector< SScriptReinforcementEntry > scriptReinforcementsTextID;
		SSuperWeaponInfo superWeapon;

		#include "include_MapPlayerInfo.h"

		SMapPlayerInfo() :
			__dwCheckSum( 0 ),
			nDiplomacySide( 0 ),
			fRecycleTimeCoefficient( 1.0000f ),
			nReinforcementCalls( 0 ),
			vMPStartPos( VNULL2 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SMapInfo : public STerrain
	{
		OBJECT_BASIC_METHODS( SMapInfo )
	public:
		enum { typeID = 0x10071C00 };
	private:
		mutable DWORD __dwCheckSum;
	public:
		NFile::CFilePath szMapDesignerFileRef;
		CVec2 vNorthPoint;
		int nNortType;
		vector< SMapPlayerInfo > players;
		vector< SMapObjectInfo > objects;
		ESeason eSeason;
		EDayNight eDayTime;
		vector< int > diplomacies;
		vector< SEntrenchmentInfo > entrenchments;
		vector< SIntArray > bridges;
		vector< SMapObjectInfo > scenarioObjects;
		SReinforcementGroupInfo reinforcements;
		NFile::CFilePath szScriptFileRef;
		vector< SScriptArea > scriptAreas;
		vector< SAIStartCommand > startCommandsList;
		vector< SBattlePosition > reservePositionsList;
		vector< SMapSoundInfo > soundsList;
		CDBPtr< SComplexSoundDesc > pForestCircleSound;
		CDBPtr< SComplexSoundDesc > pForestAmbientSounds;
		int nMapType;
		int nAttackingSide;
		vector< SPlayerBonusData > playerBonusObjects;
		vector< CDBPtr< SMissionBonus > > bonuses;
		CDBPtr< SMaterial > pMiniMap;
		NFile::CFilePath szLocalizedNameFileRef;
		NFile::CFilePath szLocalizedDescriptionFileRef;
		NFile::CFilePath szLoadingDescriptionFileRef;
		CDBPtr< STexture > pLoadingPicture;
		vector< SCameraPlacement > cameraPositions;
		SScriptMovies scriptMovies;
		vector< CVec2 > finalPositions;
		vector< CDBPtr< SMissionObjective > > objectives;
		CDBPtr< SMapMusic > pMusic;
		CDBPtr< SMapMusic > pMusicWin;
		CDBPtr< SMapMusic > pMusicLost;
		SMPMapInfo mPInfo;
		int nBorderLockSize;
		int nBorderCameraSize;
		vector< CDBPtr< SComplexEffect > > scriptEffects;
		vector< CDBPtr< SDifficultyLevel > > customDifficultyLevels;

		#include "include_mapinfo.h"

		SMapInfo() :
			__dwCheckSum( 0 ),
			vNorthPoint( VNULL2 ),
			nNortType( 0 ),
			eSeason( SEASON_SUMMER ),
			eDayTime( DAY_DAY ),
			nMapType( 0 ),
			nAttackingSide( 0 ),
			nBorderLockSize( 0 ),
			nBorderCameraSize( 0 )
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

	struct SMultiplayerMap : public CResource
	{
		OBJECT_BASIC_METHODS( SMultiplayerMap )
	public:
		enum { typeID = 0x19221C80 };
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SMapInfo > pMap;
		NFile::CFilePath szMapNameFileRef;
		int nSizeX;
		int nSizeY;
		int nPlayers;

		SMultiplayerMap() :
			__dwCheckSum( 0 ),
			nSizeX( 0 ),
			nSizeY( 0 ),
			nPlayers( 0 )
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

	enum EBonusType
	{
		BT_REPLACE_REINFORCEMENT = 0,
		BT_ENABLE_REINFORCEMENT = 1,
	};

	struct SMissionBonus : public CResource
	{
	public:
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SMapInfo > pMapToApply;
		NFile::CFilePath szTextDescFileRef;
		int nPlayer;
		EReinforcementType eReinforcementToChange;
		bool bHumanPlayer;

		#include "include_MissionBonus.h"

		SMissionBonus() :
			__dwCheckSum( 0 ),
			nPlayer( 0 ),
			eReinforcementToChange( RT_MAIN_INFANTRY ),
			bHumanPlayer( false )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SReinforcementChange : public SMissionBonus
	{
		OBJECT_BASIC_METHODS( SReinforcementChange )
	public:
		enum { typeID = 0x110BC4C1 };
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SReinforcement > pNewReinforcement;

		SReinforcementChange() :
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

	struct SReinforcementEnable : public SMissionBonus
	{
		OBJECT_BASIC_METHODS( SReinforcementEnable )
	public:
		enum { typeID = 0x110BC481 };
	private:
		mutable DWORD __dwCheckSum;
	public:

		SReinforcementEnable() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SReinforcementDisable : public SMissionBonus
	{
		OBJECT_BASIC_METHODS( SReinforcementDisable )
	public:
		enum { typeID = 0x110BC4C0 };
	private:
		mutable DWORD __dwCheckSum;
	public:

		SReinforcementDisable() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SAddReinforcementCalls : public SMissionBonus
	{
		OBJECT_BASIC_METHODS( SAddReinforcementCalls )
	public:
		enum { typeID = 0x11163C00 };
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nCalls;

		SAddReinforcementCalls() :
			__dwCheckSum( 0 ),
			nCalls( 0 )
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

	struct SStartUnisAvalabiltyEntry
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		EReinforcementType eStartReinforcmentType;
		int nNumber;

		SStartUnisAvalabiltyEntry() :
			__dwCheckSum( 0 ),
			eStartReinforcmentType( RT_MAIN_INFANTRY ),
			nNumber( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};
	struct SUnitStatsModifier;

	struct SDifficultyLevel : public CResource
	{
		OBJECT_BASIC_METHODS( SDifficultyLevel )
	public:
		enum { typeID = 0x1712D2C0 };
	private:
		mutable DWORD __dwCheckSum;
	public:
		NFile::CFilePath szLocalizedNameFileRef;
		CDBPtr< SUnitStatsModifier > pPlayerStatModifier;
		CDBPtr< SUnitStatsModifier > pEnemyStatModifier;
		float fEnemyReinfCallsCoeff;
		float fEnemyReinfRecycleCoeff;

		#include "include_DifficultyLevel.h"

		SDifficultyLevel() :
			__dwCheckSum( 0 ),
			fEnemyReinfCallsCoeff( 1 ),
			fEnemyReinfRecycleCoeff( 1 )
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
	STATS_B2_M1_EXPORT string EnumToString( NDb::EMPGameType eValue );
	STATS_B2_M1_EXPORT EMPGameType StringToEnum_NDb_EMPGameType( const string &szValue );
}

template <>
struct SKnownEnum<NDb::EMPGameType>
{
	enum { isKnown = 1 };
	static string ToString( NDb::EMPGameType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EMPGameType ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_EMPGameType( szValue ); }
};

namespace NDb
{
	string EnumToString( NDb::EScriptAreaTypes eValue );
	EScriptAreaTypes StringToEnum_NDb_EScriptAreaTypes( const string &szValue );
}

template <>
struct SKnownEnum<NDb::EScriptAreaTypes>
{
	enum { isKnown = 1 };
	static string ToString( NDb::EScriptAreaTypes eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EScriptAreaTypes ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_EScriptAreaTypes( szValue ); }
};

namespace NDb
{
	string EnumToString( NDb::EParcelType eValue );
	EParcelType StringToEnum_NDb_EParcelType( const string &szValue );
}

template <>
struct SKnownEnum<NDb::EParcelType>
{
	enum { isKnown = 1 };
	static string ToString( NDb::EParcelType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EParcelType ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_EParcelType( szValue ); }
};

namespace NDb
{
	string EnumToString( NDb::ESuperWeaponType eValue );
	ESuperWeaponType StringToEnum_NDb_ESuperWeaponType( const string &szValue );
}

template <>
struct SKnownEnum<NDb::ESuperWeaponType>
{
	enum { isKnown = 1 };
	static string ToString( NDb::ESuperWeaponType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::ESuperWeaponType ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_ESuperWeaponType( szValue ); }
};

namespace NDb
{
	string EnumToString( NDb::EBonusType eValue );
	EBonusType StringToEnum_NDb_EBonusType( const string &szValue );
}

template <>
struct SKnownEnum<NDb::EBonusType>
{
	enum { isKnown = 1 };
	static string ToString( NDb::EBonusType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EBonusType ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_EBonusType( szValue ); }
};

