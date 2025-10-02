#pragma once

// automatically generated file, don't change manually!

#include "rpgstats.h"

struct IXmlSaver;

namespace NDb
{
	struct SVisObj;

	struct SAttachedModelVisObj : public CResource
	{
		OBJECT_BASIC_METHODS( SAttachedModelVisObj )
	public:
		enum { typeID = 0x3013FC00 };

		struct SSDamageLevel
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CDBPtr< SVisObj > pVisObj;

			SSDamageLevel() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		CDBPtr< SVisObj > pvisualObject;
		vector< SSDamageLevel > damageLevels;
		CDBPtr< SVisObj > pAnimableModel;
		CDBPtr< SVisObj > pTransportableModel;
		vector< Svector_AnimDescs > animdescs;

		SAttachedModelVisObj() { }
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

