#pragma once

// automatically generated file, don't change manually!


struct IXmlSaver;

namespace NDb
{
	struct SSoundDesc;
	enum ESoundType;

	enum ESoundType
	{
		NORMAL = 0,
		PEACEFULL = 1,
		COMBAT = 2,
	};

	struct SComplexSoundDesc : public CResource
	{
		OBJECT_BASIC_METHODS( SComplexSoundDesc )
	public:
		enum { typeID = 0x11069BC3 };

		struct SSoundStats
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CDBPtr< SSoundDesc > pPathName;
			float fMinDist;
			float fMaxDist;
			float fProbability;
			ESoundType esoundType;

			SSoundStats() :
				__dwCheckSum( 0 ),
				fMinDist( 10 ),
				fMaxDist( 15 ),
				fProbability( 10 ),
				esoundType( NORMAL )
			{ }
			//
			void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		vector< SSoundStats > sounds;
		bool bLooped;

		#include "include_complexsounddesc.h"

		SComplexSoundDesc() :
			bLooped( false )
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

namespace NDb
{
	string EnumToString( NDb::ESoundType eValue );
	ESoundType StringToEnum_NDb_ESoundType( const string &szValue );
}

template <>
struct SKnownEnum<NDb::ESoundType>
{
	enum { isKnown = 1 };
	static string ToString( NDb::ESoundType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::ESoundType ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_ESoundType( szValue ); }
};

