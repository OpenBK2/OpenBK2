#pragma once

// automatically generated file, don't change manually!

#include "../stats_b2_m1/season.h"
#include "../system/filepath.h"

struct IXmlSaver;

namespace NDb
{
	struct SHPObjectRPGStats;
	struct STGTerraSet;

	struct SFieldTileDesc
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nValue;
		int nWeight;

		SFieldTileDesc() :
			__dwCheckSum( 0 ),
			nValue( 0 ),
			nWeight( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SFieldObjectDesc
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SHPObjectRPGStats > pValue;
		int nWeight;

		SFieldObjectDesc() :
			__dwCheckSum( 0 ),
			nWeight( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SFieldPatternSize
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nMin;
		int nMax;

		SFieldPatternSize() :
			__dwCheckSum( 0 ),
			nMin( 0 ),
			nMax( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SFieldTileShell
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< SFieldTileDesc > tiles;
		float fWidth;

		SFieldTileShell() :
			__dwCheckSum( 0 ),
			fWidth( 0.0f )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SFieldObjectShell
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		vector< SFieldObjectDesc > objects;
		float fWidth;
		int nBetweenDistance;
		float fRatio;

		SFieldObjectShell() :
			__dwCheckSum( 0 ),
			fWidth( 0.0f ),
			nBetweenDistance( 0 ),
			fRatio( 0.0f )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SField : public CResource
	{
		OBJECT_BASIC_METHODS( SField )
	public:
		enum { typeID = 0x14130C40 };
		vector< SFieldTileShell > tileShells;
		CDBPtr< STGTerraSet > pTerraSet;
		vector< SFieldObjectShell > objectShells;
		NFile::CFilePath szProfileFileName;
		float fHeight;
		SFieldPatternSize patternSize;
		float fPositiveRatio;

		SField() :
			fHeight( 0.0f ),
			fPositiveRatio( 0.0f )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};
}

