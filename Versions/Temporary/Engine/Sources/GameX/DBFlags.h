#pragma once

// automatically generated file, don't change manually!

#include "System/FilePath.h"

#include <cstdint>

struct IXmlSaver;

namespace NDb
{

	struct SFlagImage : public CResource
	{
		OBJECT_BASIC_METHODS( SFlagImage )
	public:
		enum { typeID = 0x1F1A6C10 };
	private:
		mutable uint32_t __dwCheckSum;
	public:
		NFile::CFilePath szImage;

		SFlagImage() :
			__dwCheckSum( 0 )
		{ }
		//
		int GetTypeID() const override { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver ) override;
		int operator&( IXmlSaver &saver ) override;
		uint32_t CalcCheckSum() const override;
	};

	struct SFlagAuthority : public CResource
	{
		OBJECT_BASIC_METHODS( SFlagAuthority )
	public:
		enum { typeID = 0x1F1A6C11 };
	private:
		mutable uint32_t __dwCheckSum;
	public:

		struct SPeriod
		{
		private:
			mutable uint32_t __dwCheckSum;
		public:
			int nFromYear;
			int nToYear;
			CDBPtr< SFlagImage > pImage;

			SPeriod() :
				__dwCheckSum( 0 ),
				nFromYear( 0 ),
				nToYear( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			uint32_t CalcCheckSum() const;
		};
		NFile::CFilePath szNameFileRef;
		std::vector< SPeriod > periods;

		SFlagAuthority() :
			__dwCheckSum( 0 )
		{ }
		//
		int GetTypeID() const override { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver ) override;
		int operator&( IXmlSaver &saver ) override;
		uint32_t CalcCheckSum() const override;
	};

	struct SFlagTerritory : public CResource
	{
		OBJECT_BASIC_METHODS( SFlagTerritory )
	public:
		enum { typeID = 0x1F1A6C12 };
	private:
		mutable uint32_t __dwCheckSum;
	public:
		NFile::CFilePath szNameFileRef;
		std::vector< CDBPtr< SFlagAuthority > > authorities;

		SFlagTerritory() :
			__dwCheckSum( 0 )
		{ }
		//
		int GetTypeID() const override { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver ) override;
		int operator&( IXmlSaver &saver ) override;
		uint32_t CalcCheckSum() const override;
	};
}
