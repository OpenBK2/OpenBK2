#pragma once

#include <cstdint>

// automatically generated file, don't change manually!

struct IXmlSaver;

namespace NDb
{
	struct SHeightRange;
	enum EPlanesAttitude;
	struct SDirectionRange;
	struct SDistanceRange;
	enum EManuverDestination;
	struct SSpeedRange;
	enum EManuverID;

	enum ESpeedRelation
	{
		ESR_NEAR_STALL = 0,
		ESR_SMALL = 1,
		ESR_NORMAL = 2,
		ESR_MAXIMUM = 3,
		_ESR_COUNT = 4,
	};

	enum EPlanesAttitude
	{
		EPA_ATTACK = 0,
		EPA_RETREAT = 1,
	};

	enum EManuverDestination
	{
		EMD_PREDICTED_POINT = 0,
		EMD_MANUVER_DEPENDENT = 1,
	};

	enum EManuverID
	{
		DB_EMID_GENERIC = 0,
		DB_EMID_STEEP_CLIMB = 1,
	};

	struct SDirectionRange : public CResource
	{
		OBJECT_BASIC_METHODS( SDirectionRange )
	public:
		enum { typeID = 0x1108EB80 };
	private:
		mutable uint32_t __dwCheckSum;
	public:
		float fMin;
		float fMax;

		SDirectionRange() :
			__dwCheckSum( 0 ),
			fMin( 0.0f ),
			fMax( 0.0f )
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

	struct SSpeedRange : public CResource
	{
		OBJECT_BASIC_METHODS( SSpeedRange )
	public:
		enum { typeID = 0x1108EB81 };
	private:
		mutable uint32_t __dwCheckSum;
	public:
		float fMin;
		float fMax;

		SSpeedRange() :
			__dwCheckSum( 0 ),
			fMin( 0.0f ),
			fMax( 0.0f )
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

	struct SDistanceRange : public CResource
	{
		OBJECT_BASIC_METHODS( SDistanceRange )
	public:
		enum { typeID = 0x1108EB82 };
	private:
		mutable uint32_t __dwCheckSum;
	public:
		float fMin;
		float fMax;

		SDistanceRange() :
			__dwCheckSum( 0 ),
			fMin( 0.0f ),
			fMax( 0.0f )
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

	struct SHeightRange : public CResource
	{
		OBJECT_BASIC_METHODS( SHeightRange )
	public:
		enum { typeID = 0x1108EB83 };
	private:
		mutable uint32_t __dwCheckSum;
	public:
		float fMin;
		float fMax;

		SHeightRange() :
			__dwCheckSum( 0 ),
			fMin( 0.0f ),
			fMax( 0.0f )
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

	struct SManuverConditions
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		CDBPtr< SDirectionRange > pEnemyDirection;
		CDBPtr< SDirectionRange > pSelfDirection;
		CDBPtr< SDirectionRange > pSpeedAngle;
		CDBPtr< SDistanceRange > pDistance;
		CDBPtr< SHeightRange > pSelfHeight;
		CDBPtr< SHeightRange > pHeightDifference;
		CDBPtr< SSpeedRange > pSelfSpeed;
		CDBPtr< SSpeedRange > pEnemySpeed;

		SManuverConditions() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SManuverDescriptor : public CResource
	{
		OBJECT_BASIC_METHODS( SManuverDescriptor )
	public:
		enum { typeID = 0x1108EB84 };
	private:
		mutable uint32_t __dwCheckSum;
	public:
		SManuverConditions conditions;
		EManuverID eManuverID;
		EManuverDestination eDestination;
		EPlanesAttitude eAttitude;

		#include "include_ManuverDescriptor.h"

		SManuverDescriptor() :
			__dwCheckSum( 0 ),
			eManuverID( DB_EMID_GENERIC ),
			eDestination( EMD_PREDICTED_POINT ),
			eAttitude( EPA_ATTACK )
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
	std::string EnumToString( NDb::ESpeedRelation eValue );
	ESpeedRelation StringToEnum_NDb_ESpeedRelation( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::ESpeedRelation>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::ESpeedRelation eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::ESpeedRelation ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_ESpeedRelation( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EPlanesAttitude eValue );
	EPlanesAttitude StringToEnum_NDb_EPlanesAttitude( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EPlanesAttitude>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EPlanesAttitude eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EPlanesAttitude ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EPlanesAttitude( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EManuverDestination eValue );
	EManuverDestination StringToEnum_NDb_EManuverDestination( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EManuverDestination>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EManuverDestination eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EManuverDestination ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EManuverDestination( szValue ); }
};

namespace NDb
{
	std::string EnumToString( NDb::EManuverID eValue );
	EManuverID StringToEnum_NDb_EManuverID( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EManuverID>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EManuverID eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EManuverID ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EManuverID( szValue ); }
};

