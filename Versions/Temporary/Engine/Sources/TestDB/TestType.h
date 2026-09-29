#pragma once

// automatically generated file, don't change manually!

#include "TestDB/BinaryFlags.h"

#include <cstdint>

#include <boost/uuid/uuid.hpp>

struct IXmlSaver;

namespace NDb
{

	struct SWeapon : public CResource
	{
		OBJECT_BASIC_METHODS( SWeapon )
	public:
		enum { typeID = 0x1019230D };
	private:
		mutable uint32_t __dwCheckSum;
	public:
		int nAmmoPerBurst;

		SWeapon() :
			__dwCheckSum( 0 ),
			nAmmoPerBurst( 0 )
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

	struct SHPObject : public CResource
	{
	public:
	private:
		mutable uint32_t __dwCheckSum;
	public:
		std::wstring wszName;
		float fHP;
		bool bHasPassability;
		CBinaryFlags flags;
		std::string szDesignerName;

		SHPObject() :
			__dwCheckSum( 0 ),
			fHP( 0.0f ),
			bHasPassability( false )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SUnitBase : public SHPObject
	{
	public:
	private:
		mutable uint32_t __dwCheckSum;
	public:

		enum EUnitType : int
		{
			UNIT_TYPE_UNKNOWN = 0,
			UNIT_TYPE_INFANTRY_SNIPER = 1,
			UNIT_TYPE_ARMOR_MEDIUM = 2,
			UNIT_TYPE_ARMOR_HEAVY = 3,
			UNIT_TYPE_AVIA_FIGHTER = 4,
			UNIT_TYPE_AUTO_ENGINEER = 5,
			UNIT_TYPE_SPG_ASSAULT = 6,
		};
		EUnitType eUnitType;
		float fSight;
		float fSpeed;
		int nBoundTileRadius;

		SUnitBase() :
			__dwCheckSum( 0 ),
			eUnitType( UNIT_TYPE_UNKNOWN ),
			fSight( 0.0f ),
			fSpeed( 0.0f ),
			nBoundTileRadius( 0 )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct SMechUnit : public SUnitBase
	{
		OBJECT_BASIC_METHODS( SMechUnit )
	public:
		enum { typeID = 0x1019230E };
	private:
		mutable uint32_t __dwCheckSum;
	public:

		struct SJogging
		{
		private:
			mutable uint32_t __dwCheckSum;
		public:
			float fAmplitude;
			float fPhase;
			float fShift;
			CVec3 vTremble;

			SJogging() :
				__dwCheckSum( 0 ),
				fAmplitude( 0.0f ),
				fPhase( 0.0f ),
				fShift( 0.0f ),
				vTremble( VNULL3 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			uint32_t CalcCheckSum() const;
		};

		struct SStruct1
		{
		private:
			mutable uint32_t __dwCheckSum;
		public:
			int nTypeInt;
			float fTypeFloat;
			bool bTypeBool;
			boost::uuids::uuid typeGUID;
			std::string szTypeString;
			std::wstring wszTypeWString;
			EUnitType eTypeEnumUnitType;
			CBinaryFlags typeBinaryFlags;

			SStruct1() :
				__dwCheckSum( 0 ),
				nTypeInt( 0 ),
				fTypeFloat( 0.0f ),
				bTypeBool( false ),
				wszTypeWString( L"Очень клёвое default value" ),
				eTypeEnumUnitType( UNIT_TYPE_UNKNOWN )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			uint32_t CalcCheckSum() const;
		};

		struct SStruct2
		{
		private:
			mutable uint32_t __dwCheckSum;
		public:
			std::vector< SStruct1 > structs;
			std::vector< boost::uuids::uuid > guids;

			SStruct2() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			uint32_t CalcCheckSum() const;
		};
		SJogging jx;
		SJogging jy;
		boost::uuids::uuid guid;
		std::vector< int > simpleArrayInt;
		std::vector< float > simpleArrayFloat;
		std::vector< boost::uuids::uuid > simpleArrayGUID;
		std::vector< CBinaryFlags > simpleArrayBinaryFlags;
		std::vector< EUnitType > simpleArrayEnumUnitType;
		std::vector< std::string > simpleArrayString;
		std::vector< std::wstring > simpleArrayWString;
		std::vector< SStruct1 > complexArrayStruct1;
		std::vector< SStruct2 > complexArrayStruct2;
		CDBPtr< SWeapon > pWeapon;
		std::vector< CDBPtr< SWeapon > > weapons;

		SMechUnit() :
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

	struct SMapInfo2 : public CResource
	{
		OBJECT_BASIC_METHODS( SMapInfo2 )
	public:
		enum { typeID = 0x101A6C80 };
	private:
		mutable uint32_t __dwCheckSum;
	public:

		struct SMapObject
		{
		private:
			mutable uint32_t __dwCheckSum;
		public:
			float fHP;
			CVec3 vPos;
			CQuat qRot;
			boost::uuids::uuid linkID;
			boost::uuids::uuid linkWith;
			CDBPtr< SHPObject > pObject;

			SMapObject() :
				__dwCheckSum( 0 ),
				fHP( 1 ),
				vPos( VNULL3 ),
				qRot( QNULL )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			uint32_t CalcCheckSum() const;
		};
		std::vector< SMapObject > objects;

		SMapInfo2() :
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
	std::string EnumToString( NDb::SUnitBase::EUnitType eValue );
	SUnitBase::EUnitType StringToEnum_NDb_SUnitBase_EUnitType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::SUnitBase::EUnitType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::SUnitBase::EUnitType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SUnitBase::EUnitType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_SUnitBase_EUnitType( szValue ); }
};
