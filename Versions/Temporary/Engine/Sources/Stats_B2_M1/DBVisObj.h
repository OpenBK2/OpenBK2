#pragma once

// automatically generated file, don't change manually!

#include "3dmotor/dbscene.h"
#include "season.h"

struct IXmlSaver;

namespace NDb
{
	struct SModel;
	enum ESeason;

	struct SVisObj : public CResource
	{
		OBJECT_BASIC_METHODS( SVisObj )
	public:
		enum { typeID = 0x11073C40 };

		struct SSingleObj
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CDBPtr< SModel > pModel;
			CDBPtr< SModel > pLowLevelModel;
			ESeason eSeason;

			SSingleObj() :
				__dwCheckSum( 0 ),
				eSeason( SEASON_SUMMER )
			{ }
			//
			void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		bool bForceAnimated;
		vector< SSingleObj > models;

		SVisObj() :
			bForceAnimated( false )
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

