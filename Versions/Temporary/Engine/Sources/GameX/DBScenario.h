#pragma once

// automatically generated file, don't change manually!

#include "Stats_B2_M1/RPGStats.h"
#include "Stats_B2_M1/UIEntries.h"
#include "System/FilePath.h"

struct IXmlSaver;

namespace NDb
{
	struct SPlayerRank;
	struct SDifficultyLevel;
	struct STexture;
	struct SMapMusic;
	struct SChapter;
	struct SUnitStatsModifier;
	struct SReinforcementTypes;
	struct SMedal;

	struct SRankExperience
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		float fExperience;
		CDBPtr< SPlayerRank > pRank;
		int nAddPromotion;

		SRankExperience() :
			__dwCheckSum( 0 ),
			fExperience( 0.0f ),
			nAddPromotion( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SLeaderExpLevel
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		NFile::CFilePath szRankNameFileRef;
		int nExpNeeded;
		CDBPtr< SUnitStatsModifier > pStatsBonus;

		#include "include_LeaderExpLevel.h"

		SLeaderExpLevel() :
			__dwCheckSum( 0 ),
			nExpNeeded( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SMedalConditions
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		CDBPtr< SMedal > pMedal;
		float fParameter;
		int nStartingChapter;

		SMedalConditions() :
			__dwCheckSum( 0 ),
			fParameter( 0.0f ),
			nStartingChapter( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SCampaign : public CResource
	{
		OBJECT_BASIC_METHODS( SCampaign )
	public:
		enum { typeID = 0x10083400 };
	private:
		mutable uint32_t __dwCheckSum;
	public:

		struct SLeader
		{
		private:
			mutable uint32_t __dwCheckSum;
		public:
			NFile::CFilePath szNameFileRef;
			CDBPtr< STexture > pPicture;

			SLeader() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			uint32_t CalcCheckSum() const;
		};
		std::vector< CDBPtr< SChapter > > chapters;
		NFile::CFilePath szLocalizedNameFileRef;
		NFile::CFilePath szLocalizedDescFileRef;
		NFile::CFilePath szScriptFileRef;
		std::vector< SUIScreenEntry > screens;
		CDBPtr< STexture > pTextureNotStarted;
		CDBPtr< STexture > pTextureCompleted;
		CDBPtr< STexture > pTextureNotStartedSelected;
		CDBPtr< STexture > pTextureCompletedSelected;
		CDBPtr< STexture > pTextureMissionCompleted;
		CDBPtr< STexture > pTextureMenuBackground;
		CDBPtr< STexture > pTextureMenuIcon;
		CDBPtr< STexture > pTextureChapterFinishBonus;
		std::vector< SRankExperience > rankExperiences;
		std::vector< SLeaderExpLevel > leaderRanks;
		std::vector< SLeader > leaders;
		std::vector< CDBPtr< SDifficultyLevel > > difficultyLevels;
		NFile::CFilePath szIntroMovie;
		NFile::CFilePath szOutroMovie;
		CDBPtr< SReinforcementTypes > pReinforcementTypes;
		CDBPtr< SMapMusic > pIntermissionMusic;
		CDBPtr< STexture > pSaveLoadFlag;
		std::vector< SMedalConditions > medalsForChapter;
		std::vector< SMedalConditions > medalsForKills;
		std::vector< SMedalConditions > medalsForTactics;
		std::vector< SMedalConditions > medalsForEconomy;
		CDBPtr< SMedal > pMedalForMunchkinism;

		#include "include_Campaign.h"

		SCampaign() :
			__dwCheckSum( 0 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};
	enum EReinforcementType : int;
	struct SMapInfo;
	enum EMissionEnableType : int;
	enum EDBUnitRPGType : int;
	struct SChapterBonus;
	struct STexture;
	struct SReinforcement;
	enum EChapterBonusType : int;

	struct SUnitClassEntry
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		CDBPtr< SReinforcement > pReinforcement;

		SUnitClassEntry() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SEnemyEntry
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		CDBPtr< SMechUnitRPGStats > pMechUnit;
		CDBPtr< SSquadRPGStats > pSquad;

		SEnemyEntry() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	enum EMissionEnableType : int
	{
		MET_REGULAR = 0,
		MET_CHAPTER_START = 1,
		MET_CHAPTER_END = 2,
		MET_CHAPTER_START_END = 3,
	};

	enum EMissionType
	{
		EMT_FINAL = 0,
		EMT_AIR_COVER = 1,
		EMT_ART_COVER = 2,
		EMT_ATTACK = 3,
		EMT_DEFENCE = 4,
		EMT_CONVOY_PROTECT = 5,
		EMT_CONVOY_DESTROY = 6,
	};

	enum EMissionWeather
	{
		EMW_SUN = 0,
		EMW_RAIN = 1,
		EMW_SNOW = 2,
		EMW_SANDSTORM = 3,
	};

	enum EMissionDayTime
	{
		EMDT_DAY = 0,
		EMDT_DUSK = 1,
		EMDT_NIGHT = 2,
		EMDT_DAWN = 3,
	};

	enum EMissionDifficulty
	{
		EMD_EASY = 0,
		EMD_MEDIUM = 1,
		EMD_HARD = 2,
		EMD_VERY_HARD = 3,
	};

	struct SMissionEnableInfo
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		CDBPtr< SMapInfo > pMap;
		CVec2 vPlaceOnChapterMap;
		int nMissionsToEnable;
		EMissionEnableType eMissionEnableType;
		std::vector< CDBPtr< SChapterBonus > > reward;
		std::vector< SEnemyEntry > expectedEnemy;
		int nRecommendedCalls;
		float fPotentialIncomplete;
		float fPotentialComplete;
		EMissionType eType;
		EMissionDifficulty eDifficulty;
		EMissionDayTime eTime;
		EMissionWeather eWeather;
		bool bShowPotentialComplete;
		int nRecommendedOrder;
		CVec2 vEndOffset;

		SMissionEnableInfo() :
			__dwCheckSum( 0 ),
			vPlaceOnChapterMap( VNULL2 ),
			nMissionsToEnable( 0 ),
			eMissionEnableType( MET_REGULAR ),
			nRecommendedCalls( 0 ),
			fPotentialIncomplete( -10 ),
			fPotentialComplete( 10 ),
			eType( EMT_FINAL ),
			eDifficulty( EMD_EASY ),
			eTime( EMDT_DAY ),
			eWeather( EMW_SUN ),
			bShowPotentialComplete( false ),
			nRecommendedOrder( 0 ),
			vEndOffset( VNULL2 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SScenarioUnitModifier
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		EReinforcementType eType;
		int nQuantity;
		CDBPtr< SReinforcement > pUnits;

		SScenarioUnitModifier() :
			__dwCheckSum( 0 ),
			eType( RT_MAIN_INFANTRY ),
			nQuantity( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SBaseReinforcements
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		std::vector< CDBPtr< SReinforcement > > reinforcements;

		SBaseReinforcements() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	enum EChapterBonusType : int
	{
		CBT_REINF_DISABLE = 0,
		CBT_REINF_CHANGE = 1,
		CBT_ADD_CALLS = 2,
	};

	struct SChapterGeneralInfo
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		NFile::CFilePath szDescFileRef;
		CDBPtr< STexture > pPortrait;
		EReinforcementType eReinforcementType;
		CDBPtr< SUnitStatsModifier > pStatBonus;

		SChapterGeneralInfo() :
			__dwCheckSum( 0 ),
			eReinforcementType( RT_MAIN_INFANTRY )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SChapterBonus : public CResource
	{
		OBJECT_BASIC_METHODS( SChapterBonus )
	public:
		enum { typeID = 0x1917A440 };
	private:
		mutable uint32_t __dwCheckSum;
	public:
		EChapterBonusType eBonusType;
		bool bApplyToEnemy;
		EReinforcementType eReinforcementType;
		CDBPtr< SReinforcement > pReinforcementSet;
		int nNumberOfCalls;

		SChapterBonus() :
			__dwCheckSum( 0 ),
			eBonusType( CBT_REINF_DISABLE ),
			bApplyToEnemy( false ),
			eReinforcementType( RT_MAIN_INFANTRY ),
			nNumberOfCalls( 0 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SChapter : public CResource
	{
		OBJECT_BASIC_METHODS( SChapter )
	public:
		enum { typeID = 0x10083401 };
	private:
		mutable uint32_t __dwCheckSum;
	public:
		NFile::CFilePath szLocalizedNameFileRef;
		NFile::CFilePath szLocalizedNameSaveLoadFileRef;
		NFile::CFilePath szLocalizedDateFileRef;
		NFile::CFilePath szLocalizedDescriptionFileRef;
		std::vector< SMissionEnableInfo > missionPath;
		NFile::CFilePath szScriptFileRef;
		bool bUseMapReinforcements;
		std::vector< SBaseReinforcements > basePlayerReinforcements;
		std::vector< SUnitClassEntry > reinforcementModifiers;
		CDBPtr< STexture > pMapPicture;
		NFile::CFilePath szSeaNoiseMask;
		NFile::CFilePath szDifferentColourMap;
		int nPositiveColour;
		int nNegativeColour;
		float fMainStrikeAngle;
		float fMainStrikePower;
		int nReinforcementCalls;
		CDBPtr< SMapInfo > pDetailsMap;
		std::vector< CDBPtr< STexture > > arrowTextures;
		NFile::CFilePath szIntroMovie;
		SChapterGeneralInfo general;

		#include "include_Chapter.h"

		SChapter() :
			__dwCheckSum( 0 ),
			bUseMapReinforcements( false ),
			nPositiveColour( 0 ),
			nNegativeColour( 0 ),
			fMainStrikeAngle( 0.0f ),
			fMainStrikePower( 0.0f ),
			nReinforcementCalls( 0 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};
	struct STexture;

	struct SMedal : public CResource
	{
		OBJECT_BASIC_METHODS( SMedal )
	public:
		enum { typeID = 0x170C9480 };
	private:
		mutable uint32_t __dwCheckSum;
	public:
		NFile::CFilePath szLocalizedNameFileRef;
		NFile::CFilePath szLocalizedDescFileRef;
		CDBPtr< STexture > pIconTexture;
		CDBPtr< STexture > pPictureTexture;

		#include "include_Medal.h"

		SMedal() :
			__dwCheckSum( 0 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};
}

namespace NDb
{
	std::string EnumToString( NDb::EMissionEnableType eValue );
	EMissionEnableType StringToEnum_NDb_EMissionEnableType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EMissionEnableType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EMissionEnableType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EMissionEnableType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EMissionEnableType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EMissionType eValue );
	EMissionType StringToEnum_NDb_EMissionType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EMissionType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EMissionType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EMissionType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EMissionType( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EMissionWeather eValue );
	EMissionWeather StringToEnum_NDb_EMissionWeather( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EMissionWeather>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EMissionWeather eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EMissionWeather ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EMissionWeather( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EMissionDayTime eValue );
	EMissionDayTime StringToEnum_NDb_EMissionDayTime( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EMissionDayTime>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EMissionDayTime eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EMissionDayTime ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EMissionDayTime( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EMissionDifficulty eValue );
	EMissionDifficulty StringToEnum_NDb_EMissionDifficulty( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EMissionDifficulty>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EMissionDifficulty eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EMissionDifficulty ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EMissionDifficulty( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EChapterBonusType eValue );
	EChapterBonusType StringToEnum_NDb_EChapterBonusType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EChapterBonusType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EChapterBonusType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EChapterBonusType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EChapterBonusType( szValue ); }
};

