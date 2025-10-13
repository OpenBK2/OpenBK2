#pragma once

// automatically generated file, don't change manually!

#include "RPGStats.h"

#include <cstdint>

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
			mutable uint32_t __dwCheckSum;
		public:
			CDBPtr< SVisObj > pVisObj;

			SSDamageLevel() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			uint32_t CalcCheckSum() const;
		};
		CDBPtr< SVisObj > pvisualObject;
		std::vector< SSDamageLevel > damageLevels;
		CDBPtr< SVisObj > pAnimableModel;
		CDBPtr< SVisObj > pTransportableModel;
		std::vector< Svector_AnimDescs > animdescs;

		SAttachedModelVisObj() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		uint32_t CalcCheckSum() const { return 0; }
	};
}

