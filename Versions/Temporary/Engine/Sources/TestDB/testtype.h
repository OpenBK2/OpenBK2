#pragma once

// automatically generated file, don't change manually!

#include "binaryflags.h"

#include <cstdint>

namespace NDb
{

	struct SWeapon : public CResource
	{
		OBJECT_BASIC_METHODS( SWeapon )
	public:
		enum { typeID = 0x1019230D };
		int nAmmoPerBurst;

		SWeapon() :
			nAmmoPerBurst( 0 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
	};

	struct SHPObject : public CResource
	{
	public:
		wstring wszName;
		float fHP;
		bool bHasPassability;
		CBinaryFlags flags;
		string szDesignerName;

		SHPObject() :
			fHP( 0.0f ),
			bHasPassability( false )
		{ }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
	};

	struct SUnitBase : public SHPObject
	{
	public:

		enum EUnitType
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
	};

	struct SMechUnit : public SUnitBase
	{
		OBJECT_BASIC_METHODS( SMechUnit )
	public:
		enum { typeID = 0x1019230E };

		struct SJogging
		{
			float fAmplitude;
			float fPhase;
			float fShift;
			CVec3 vTremble;

			SJogging() :
				fAmplitude( 0.0f ),
				fPhase( 0.0f ),
				fShift( 0.0f ),
				vTremble( VNULL3 )
			{ }
			//
			void ReportMetaInfo( const string &szAddName, uint8_t *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
		};

		struct SStruct1
		{
			int nTypeInt;
			float fTypeFloat;
			bool bTypeBool;
			GUID typeGUID;
			string szTypeString;
			wstring wszTypeWString;
			EUnitType eTypeEnumUnitType;
			CBinaryFlags typeBinaryFlags;

			SStruct1() :
				nTypeInt( 0 ),
				fTypeFloat( 0.0f ),
				bTypeBool( false ),
				wszTypeWString( L"Очень клёвое default value" ),
				eTypeEnumUnitType( UNIT_TYPE_UNKNOWN )
			{ }
			//
			void ReportMetaInfo( const string &szAddName, uint8_t *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
		};

		struct SStruct2
		{
			vector< SStruct1 > structs;
			vector< GUID > guids;

			SStruct2() { }
			//
			void ReportMetaInfo( const string &szAddName, uint8_t *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
		};
		SJogging jx;
		SJogging jy;
		GUID guid;
		vector< int > simpleArrayInt;
		vector< float > simpleArrayFloat;
		vector< GUID > simpleArrayGUID;
		vector< CBinaryFlags > simpleArrayBinaryFlags;
		vector< EUnitType > simpleArrayEnumUnitType;
		vector< string > simpleArrayString;
		vector< wstring > simpleArrayWString;
		vector< SStruct1 > complexArrayStruct1;
		vector< SStruct2 > complexArrayStruct2;
		CDBPtr< SWeapon > pWeapon;
		vector< CDBPtr< SWeapon > > weapons;

		SMechUnit() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
	};

	struct SMapInfo2 : public CResource
	{
		OBJECT_BASIC_METHODS( SMapInfo2 )
	public:
		enum { typeID = 0x101A6C80 };

		struct SMapObject
		{
			float fHP;
			CVec3 vPos;
			CQuat qRot;
			GUID linkID;
			GUID linkWith;
			CDBPtr< SHPObject > pObject;

			SMapObject() :
				fHP( 1 ),
				vPos( VNULL3 ),
				qRot( QNULL )
			{ }
			//
			void ReportMetaInfo( const string &szAddName, uint8_t *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
		};
		vector< SMapObject > objects;

		SMapInfo2() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
	};
}

namespace NDb
{
	string EnumToString( NDb::SUnitBase::EUnitType eValue );
	SUnitBase::EUnitType StringToEnum_NDb_SUnitBase_EUnitType( const string &szValue );
}

template <>
struct SKnownEnum<NDb::SUnitBase::EUnitType>
{
	enum { isKnown = 1 };
	static string ToString( NDb::SUnitBase::EUnitType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SUnitBase::EUnitType ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_SUnitBase_EUnitType( szValue ); }
};

