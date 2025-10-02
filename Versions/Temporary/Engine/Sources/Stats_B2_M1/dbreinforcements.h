#pragma once

#include "Stats_B2_M1_export.h"


// automatically generated file, don't change manually!

#include "rpgstats.h"

struct IXmlSaver;

namespace NDb
{

	struct SIntArray
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< int > data;

		SIntArray() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SReinforcementGroupInfoEntry
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nGroupID;
		SIntArray groupsVector;

		SReinforcementGroupInfoEntry() :
			__dwCheckSum( 0 ),
			nGroupID( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SReinforcementGroupInfo
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< SReinforcementGroupInfoEntry > infos;

		#include "include_reinforcementgroupinfo.h"

		SReinforcementGroupInfo() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SReinforcementMaskEntry
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nDirection;
		CVec2 vPosition;

		SReinforcementMaskEntry() :
			__dwCheckSum( 0 ),
			nDirection( 0 ),
			vPosition( VNULL2 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SReinforcementMask
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< SReinforcementMaskEntry > positions;

		SReinforcementMask() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SReinforcementDefinition
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SMechUnitRPGStats > pMechUnit;
		CDBPtr< SSquadRPGStats > pSquad;
		EDBUnitRPGType eUnitType;
		bool bIsAmmunition;
		EReinforcementType eType;
		CDBPtr< SReinforcement > pReinforcement;

		SReinforcementDefinition() :
			__dwCheckSum( 0 ),
			eUnitType( DB_RPG_TYPE_SOLDIER ),
			bIsAmmunition( false ),
			eType( RT_MAIN_INFANTRY )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct STATS_B2_M1_EXPORT STypedDeployTemplate
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		EReinforcementType eType;
		CDBPtr< SDeployTemplate > pTemplate;

		STypedDeployTemplate() :
			__dwCheckSum( 0 ),
			eType( RT_MAIN_INFANTRY )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct STATS_B2_M1_EXPORT SReinforcementPosition
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CVec2 vPosition;
		CVec2 vAviationPosition;
		int nDirection;
		CDBPtr< SDeployTemplate > pTemplate;
		vector< STypedDeployTemplate > typedTemplates;
		bool bIsDefault;
		bool b___delete_from_here_to_the_end;
		EReinforcementType eType;
		int nFactoryID;
		int nPositionID;

		SReinforcementPosition() :
			__dwCheckSum( 0 ),
			vPosition( VNULL2 ),
			vAviationPosition( VNULL2 ),
			nDirection( 0 ),
			bIsDefault( false ),
			b___delete_from_here_to_the_end( false ),
			eType( RT_MAIN_INFANTRY ),
			nFactoryID( -1 ),
			nPositionID( 1 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SScriptReinforcementEntry
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SReinforcement > pReinforcement;
		string szName;

		SScriptReinforcementEntry() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SPlayerReinforcementEnable
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		SReinforcementPosition newPointOnEnable;
		EReinforcementType eReinforcementToEnable;
		bool bSetPoint;
		int nGivenReinforcementPointID;

		SPlayerReinforcementEnable() :
			__dwCheckSum( 0 ),
			eReinforcementToEnable( RT_MAIN_INFANTRY ),
			bSetPoint( false ),
			nGivenReinforcementPointID( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};
}

