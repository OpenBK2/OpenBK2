#pragma once

// automatically generated file, don't change manually!

#include "M1Actions.h"
#include "RPGStats.h"
#include "Commands_Actions.h"

#include <cstdint>

struct IXmlSaver;

namespace NDb
{
	struct SM1UnitStatsModifier;
	enum EM1Action : int;
	struct SM1UnitSpecAction;

	struct SM1UnitSpecAction : public CResource
	{
	public:
	private:
		mutable uint32_t __dwCheckSum;
	public:
		EM1Action eType;

		SM1UnitSpecAction() :
			__dwCheckSum( 0 ),
			eType( M1_ACTION_UNKNOWN )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SM1UnitActions : public CResource
	{
		OBJECT_BASIC_METHODS( SM1UnitActions )
	public:
		enum { typeID = 0x33196B41 };
	private:
		mutable uint32_t __dwCheckSum;
	public:
		CUserCommands defaultPerformableActions;
		CUserCommands defaultActionsToEndure;
		std::vector< CDBPtr< SM1UnitSpecAction > > actionParams;

		SM1UnitActions() :
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

	struct SM1UnitActionBuild : public SM1UnitSpecAction
	{
		OBJECT_BASIC_METHODS( SM1UnitActionBuild )
	public:
		enum { typeID = 0x331ADBC0 };
	private:
		mutable uint32_t __dwCheckSum;
	public:

		SM1UnitActionBuild() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SM1UnitActionTransform : public SM1UnitSpecAction
	{
		OBJECT_BASIC_METHODS( SM1UnitActionTransform )
	public:
		enum { typeID = 0x331BEB41 };
	private:
		mutable uint32_t __dwCheckSum;
	public:
		int nTransformationTime;
		CDBPtr< SM1UnitStatsModifier > pStatsModifier;

		SM1UnitActionTransform() :
			__dwCheckSum( 0 ),
			nTransformationTime( 0 )
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

	struct SM1ParameterModifier
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		float fMultParam;
		float fAddParam;

		SM1ParameterModifier() :
			__dwCheckSum( 0 ),
			fMultParam( 1 ),
			fAddParam( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SShellStatsModifier
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		SM1ParameterModifier damage;
		SM1ParameterModifier piercing;

		SShellStatsModifier() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SWeaponStatsModifier
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		SM1ParameterModifier range;
		std::vector< SShellStatsModifier > shells;

		SWeaponStatsModifier() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SPlatformWeaponsStatsModifier
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		std::vector< SWeaponStatsModifier > guns;

		SPlatformWeaponsStatsModifier() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SWeaponsStatsModifier
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		std::vector< SPlatformWeaponsStatsModifier > platforms;

		SWeaponsStatsModifier() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SM1UnitStatsModifier : public CResource
	{
		OBJECT_BASIC_METHODS( SM1UnitStatsModifier )
	public:
		enum { typeID = 0x3016A480 };
	private:
		mutable uint32_t __dwCheckSum;
	public:
		SWeaponsStatsModifier weapons;
		SM1ParameterModifier sightRange;
		SM1ParameterModifier speed;
		SM1ParameterModifier rotateSpeed;

		SM1UnitStatsModifier() :
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

