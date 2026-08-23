#pragma once

// automatically generated file, don't change manually!

#include "System/FilePath.h"

#include <cstdint>

struct IXmlSaver;

namespace NDb
{
	struct SReinforcement;
	enum EHistoricalSide : int;
	struct SPartyDependentInfo;
	struct STexture;
	struct SAIExpLevel;
	struct SBackground;
	struct SMedal;

	enum EHistoricalSide : int
	{
		HS_ALLIES = 0,
		HS_AXIS = 1,
	};

	struct SMultiplayerTechLevel
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		NFile::CFilePath szNameFileRef;
		NFile::CFilePath szDescriptionFileRef;

		#include "include_MultiplayerTechLevel.h"

		SMultiplayerTechLevel() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct STechLevelReinfSet
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		std::vector< CDBPtr< SReinforcement > > reinforcements;
		bool bDisabled;
		CDBPtr< SReinforcement > pStartingUnits;

		STechLevelReinfSet() :
			__dwCheckSum( 0 ),
			bDisabled(false)	// Make levels are enabled by default
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SLadderRank
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		int nLevel;
		NFile::CFilePath szNameFileRef;
		CDBPtr< STexture > pTexture;

		SLadderRank() :
			__dwCheckSum( 0 ),
			nLevel( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SMultiplayerSide
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		NFile::CFilePath szNameFileRef;
		CDBPtr< SPartyDependentInfo > pPartyInfo;
		CDBPtr< STexture > pListItemIcon;
		EHistoricalSide eHistoricalSide;
		std::vector< STechLevelReinfSet > techLevels;
		std::vector< SLadderRank > ladderRanks;
		std::vector< CDBPtr< SMedal > > medals;

		#include "include_MultiplayerSide.h"

		SMultiplayerSide() :
			__dwCheckSum( 0 ),
			eHistoricalSide( HS_ALLIES )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SMultiplayerConsts : public CResource
	{
		OBJECT_BASIC_METHODS( SMultiplayerConsts )
	public:
		enum { typeID = 0x191B2300 };
	private:
		mutable uint32_t __dwCheckSum;
	public:

		struct SPlayerColor
		{
		private:
			mutable uint32_t __dwCheckSum;
		public:
			int nColor;
			CDBPtr< SBackground > pUnitFullInfo;

			SPlayerColor() :
				__dwCheckSum( 0 ),
				nColor( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			uint32_t CalcCheckSum() const;
		};
		std::vector< SMultiplayerTechLevel > techLevels;
		std::vector< SMultiplayerSide > sides;
		CDBPtr< STexture > pRandomCountryIcon;
		std::vector< CDBPtr< SPartyDependentInfo > > diplomacyInfo;
		std::vector< CDBPtr< SAIExpLevel > > expLevels;
		std::vector< SPlayerColor > playerColorInfos;
		CVec2 vReinfCounterRecycle;
		int nTimeUserMPPause;
		int nTimeUserMPLag;

		SMultiplayerConsts() :
			__dwCheckSum( 0 ),
			vReinfCounterRecycle( VNULL2 ),
			nTimeUserMPPause( 120 ),
			nTimeUserMPLag( 60 )
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
	std::string EnumToString( NDb::EHistoricalSide eValue );
	EHistoricalSide StringToEnum_NDb_EHistoricalSide( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EHistoricalSide>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EHistoricalSide eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EHistoricalSide ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EHistoricalSide( szValue ); }
};

