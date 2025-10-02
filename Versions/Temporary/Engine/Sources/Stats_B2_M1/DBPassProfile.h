#pragma once

#include "Stats_B2_M1_export.h"


// automatically generated file, don't change manually!


interface IXmlSaver;

namespace NDb
{

	struct SPolygon2D
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< CVec2 > verts;
		int nFake;

		SPolygon2D() :
			__dwCheckSum( 0 ),
			nFake( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct STATS_B2_M1_EXPORT SPassProfile
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< SPolygon2D > polygons;

		SPassProfile() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};
}
