#pragma once

#include "Stats_B2_M1_export.h"

#include <cstdint>

// automatically generated file, don't change manually!

struct IXmlSaver;

namespace NDb
{

	struct SPolygon2D
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		std::vector< CVec2 > verts;
		int nFake;

		SPolygon2D() :
			__dwCheckSum( 0 ),
			nFake( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};

	struct STATS_B2_M1_EXPORT SPassProfile
	{
	private:
		mutable uint32_t __dwCheckSum;
	public:
		std::vector< SPolygon2D > polygons;

		SPassProfile() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const;
	};
}

