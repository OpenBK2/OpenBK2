#pragma once

// automatically generated file, don't change manually!

#include "system/filepath.h"

struct IXmlSaver;

namespace NDb
{
	struct SReinforcement;
	enum EHistoricalSide;
	struct SPartyDependentInfo;
	struct STexture;
	struct SAIExpLevel;
	struct SBackground;
	struct SMedal;

	enum EHistoricalSide
	{
		HS_ALLIES = 0,
		HS_AXIS = 1,
	};

	struct SMultiplayerTechLevel
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		NFile::CFilePath szNameFileRef;
		NFile::CFilePath szDescriptionFileRef;

		#include "include_MultiplayerTechLevel.h"

		SMultiplayerTechLevel() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct STechLevelReinfSet
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< CDBPtr< SReinforcement > > reinforcements;
		CDBPtr< SReinforcement > pStartingUnits;

		STechLevelReinfSet() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SLadderRank
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nLevel;
		NFile::CFilePath szNameFileRef;
		CDBPtr< STexture > pTexture;

		SLadderRank() :
			__dwCheckSum( 0 ),
			nLevel( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SMultiplayerSide
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		NFile::CFilePath szNameFileRef;
		CDBPtr< SPartyDependentInfo > pPartyInfo;
		CDBPtr< STexture > pListItemIcon;
		EHistoricalSide eHistoricalSide;
		vector< STechLevelReinfSet > techLevels;
		vector< SLadderRank > ladderRanks;
		vector< CDBPtr< SMedal > > medals;

		#include "include_MultiplayerSide.h"

		SMultiplayerSide() :
			__dwCheckSum( 0 ),
			eHistoricalSide( HS_ALLIES )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SMultiplayerConsts : public CResource
	{
		OBJECT_BASIC_METHODS( SMultiplayerConsts )
	public:
		enum { typeID = 0x191B2300 };
	private:
		mutable DWORD __dwCheckSum;
	public:

		struct SPlayerColor
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			int nColor;
			CDBPtr< SBackground > pUnitFullInfo;

			SPlayerColor() :
				__dwCheckSum( 0 ),
				nColor( 0 )
			{ }
			//
			void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		vector< SMultiplayerTechLevel > techLevels;
		vector< SMultiplayerSide > sides;
		CDBPtr< STexture > pRandomCountryIcon;
		vector< CDBPtr< SPartyDependentInfo > > diplomacyInfo;
		vector< CDBPtr< SAIExpLevel > > expLevels;
		vector< SPlayerColor > playerColorInfos;
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
		DWORD CalcCheckSum() const;
	};
}

namespace NDb
{
	string EnumToString( NDb::EHistoricalSide eValue );
	EHistoricalSide StringToEnum_NDb_EHistoricalSide( const string &szValue );
}

template <>
struct SKnownEnum<NDb::EHistoricalSide>
{
	enum { isKnown = 1 };
	static string ToString( NDb::EHistoricalSide eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EHistoricalSide ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_EHistoricalSide( szValue ); }
};

