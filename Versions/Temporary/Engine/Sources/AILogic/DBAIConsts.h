#pragma once

// automatically generated file, don't change manually!

#include "stats_b2_m1/dbreinforcements.h"
#include "stats_b2_m1/RPGStats.h"

struct IXmlSaver;

namespace NDb
{
	struct SFenceRPGStats;
	struct SEntrenchmentRPGStats;
	struct SSquadRPGStats;
	struct SUnitStatsModifier;
	enum EReinforcementType;
	enum EDesignUnitType;
	struct SObjectBaseRPGStats;
	enum EDBUnitRPGType;
	struct SMineRPGStats;
	struct SAIExpLevel;
	struct SInfantryRPGStats;
	struct SStaticObjectRPGStats;
	struct SMechUnitRPGStats;
	struct SManuverDescriptor;
	struct SVisObj;

	struct STankPitInfo
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< CDBPtr< SMechUnitRPGStats > > sandBagTankPits;
		vector< CDBPtr< SMechUnitRPGStats > > digTankPits;

		STankPitInfo() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SCommonInfo
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< CDBPtr< SObjectBaseRPGStats > > antitankObjects;
		CDBPtr< SFenceRPGStats > pAPFence;
		CDBPtr< SMineRPGStats > pMineUniversal;
		CDBPtr< SMineRPGStats > pMineAT;
		CDBPtr< SMineRPGStats > pMineAP;
		CDBPtr< SMineRPGStats > pMineCharge;
		CDBPtr< SMineRPGStats > pLandMine;
		CDBPtr< SEntrenchmentRPGStats > pEntrenchment;
		CDBPtr< SMechUnitRPGStats > pTorpedoStats;
		CDBPtr< SInfantryRPGStats > pMosinStats;
		CDBPtr< SMechUnitRPGStats > pG152mmML20Stats;
		CDBPtr< SSquadRPGStats > pSingleUnitFormation;
		CDBPtr< SStaticObjectRPGStats > pShellBox;
		vector< CDBPtr< SAIExpLevel > > expLevels;
		float fExpReinfDistributionCoeff;
		float fExpCommanderDistributionCoeff;
		float fExpCommanderUnitPenaltyCoeff;
		float fExpCommanderPenaltyCoeff;
		CDBPtr< SUnitStatsModifier > pNightStatModifier;
		CDBPtr< SUnitStatsModifier > pBadWeatherStatModifier;

		SCommonInfo() :
			__dwCheckSum( 0 ),
			fExpReinfDistributionCoeff( 0.0f ),
			fExpCommanderDistributionCoeff( 0.0f ),
			fExpCommanderUnitPenaltyCoeff( 0.0f ),
			fExpCommanderPenaltyCoeff( 0.0f )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SWarFogConsts
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nMaxRadius;
		bool bUseHeights;
		float fTanEpsilon;
		float fUnitHeight;

		SWarFogConsts() :
			__dwCheckSum( 0 ),
			nMaxRadius( 0 ),
			bUseHeights( false ),
			fTanEpsilon( 0.0f ),
			fUnitHeight( 0.0f )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SReinforcementRemap
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		EReinforcementType eReinfType;
		EDBUnitRPGType eUnitRPGType;

		SReinforcementRemap() :
			__dwCheckSum( 0 ),
			eReinfType( RT_MAIN_INFANTRY ),
			eUnitRPGType( DB_RPG_TYPE_SOLDIER )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SReinforcementExpediency
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< float > expediency;

		SReinforcementExpediency() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SUnitTypePriority
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		EDesignUnitType eUnitType;
		int nPriority;

		SUnitTypePriority() :
			__dwCheckSum( 0 ),
			eUnitType( UNIT_TYPE_UNKNOWN ),
			nPriority( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SReinfRecycleTime
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		EReinforcementType eType;
		int nTime;

		SReinfRecycleTime() :
			__dwCheckSum( 0 ),
			eType( RT_MAIN_INFANTRY ),
			nTime( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SAIGameConsts : public CResource
	{
		OBJECT_BASIC_METHODS( SAIGameConsts )
	public:
		enum { typeID = 0x11074CC0 };
	private:
		mutable DWORD __dwCheckSum;
	public:

		struct SCommonScriptEntry
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			NFile::CFilePath szScriptFileRef;

			SCommonScriptEntry() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		STankPitInfo tankPits;
		vector< CDBPtr< SMechUnitRPGStats > > foxHoles;
		SCommonInfo common;
		vector< CDBPtr< SManuverDescriptor > > planeManuvers;
		SWarFogConsts warFog;
		vector< SCommonScriptEntry > commonScriptFileRefs;
		vector< SReinforcementRemap > reinforcementTypes;
		vector< SReinforcementExpediency > reinfExpediency;
		vector< SUnitTypePriority > unitTypesPriorities;
		CDBPtr< SVisObj > pParachute;
		int nRemoveParachuteTime;
		vector< SReinfRecycleTime > reinforcementRecycleTime;
		CDBPtr< SWeaponRPGStats > pAviationGroundCrashExplosion;
		CDBPtr< SWeaponRPGStats > pFlamethrowerDeathExplotion;
		int nGroundCrashPlaneSize;
		vector< STypedDeployTemplate > typedTemplates;

		SAIGameConsts() :
			__dwCheckSum( 0 ),
			nRemoveParachuteTime( 0 ),
			nGroundCrashPlaneSize( 100 )
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

