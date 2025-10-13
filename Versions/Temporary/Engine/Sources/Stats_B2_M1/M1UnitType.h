#pragma once

#include <cstdint>

// automatically generated file, don't change manually!

struct IXmlSaver;

namespace NDb
{
	enum EM1UnitBaseType;

	enum EM1UnitBaseType
	{
		M1_MECH = 0,
		M1_SOLDIER = 1,
		M1_PLANE = 2,
		M1_HELICOPTER = 3,
	};

	struct SM1UnitType : public CResource
	{
		OBJECT_BASIC_METHODS( SM1UnitType )
	public:
		enum { typeID = 0x33193B01 };
	private:
		mutable uint32_t __dwCheckSum;
	public:
		EM1UnitBaseType eBaseType;

		SM1UnitType() :
			__dwCheckSum( 0 ),
			eBaseType( M1_MECH )
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
	std::string EnumToString( NDb::EM1UnitBaseType eValue );
	EM1UnitBaseType StringToEnum_NDb_EM1UnitBaseType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::EM1UnitBaseType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::EM1UnitBaseType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::EM1UnitBaseType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_EM1UnitBaseType( szValue ); }
};

