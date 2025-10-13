#pragma once

// automatically generated file, don't change manually!

#include "System/FilePath.h"

struct IXmlSaver;

namespace NDb
{

	struct SOptionSystem : public CResource
	{
		OBJECT_BASIC_METHODS( SOptionSystem )
	public:
		enum { typeID = 0x100CCC01 };

		struct SOptionsCategory
		{
		private:
			mutable uint32_t __dwCheckSum;
		public:

			struct SOptionEntry
			{
			private:
				mutable uint32_t __dwCheckSum;
			public:

				struct SOptionEntryState
				{
				private:
					mutable uint32_t __dwCheckSum;
				public:
					NFile::CFilePath szNameFileRef;
					NFile::CFilePath szTooltipFileRef;
					std::string szValue;

					#include "include_OptionEntryState.h"

					SOptionEntryState() :
						__dwCheckSum( 0 )
					{ }
					//
					void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
					//
					int operator&( IBinSaver &saver );
					int operator&( IXmlSaver &saver );
					uint32_t CalcCheckSum() const;
				};

				enum EOptionEditorType
				{
					OPTION_EDITOR_EDITLINE = 0,
					OPTION_EDITOR_CHECKBOX = 1,
					OPTION_EDITOR_SLIDER = 2,
					OPTION_EDITOR_DROPLIST = 3,
					OPTION_EDITOR_EDITNUMBER = 4,
				};

				struct SSliderSingleValue
				{
				private:
					mutable uint32_t __dwCheckSum;
				public:
					std::string szProgName;
					std::vector< std::string > values;

					SSliderSingleValue() :
						__dwCheckSum( 0 )
					{ }
					//
					void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
					//
					int operator&( IBinSaver &saver );
					int operator&( IXmlSaver &saver );
					uint32_t CalcCheckSum() const;
				};
				std::string szProgName;
				NFile::CFilePath szNameFileRef;
				NFile::CFilePath szTooltipFileRef;
				std::vector< SOptionEntryState > states;
				EOptionEditorType eEditorType;
				std::string szDefaultValue;
				int nModeFlags;
				std::vector< SSliderSingleValue > sliderValues;

				#include "include_OptionEntry.h"

				SOptionEntry() :
					__dwCheckSum( 0 ),
					eEditorType( OPTION_EDITOR_EDITLINE ),
					nModeFlags( 0 )
				{ }
				//
				void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
				//
				int operator&( IBinSaver &saver );
				int operator&( IXmlSaver &saver );
				uint32_t CalcCheckSum() const;
			};
			NFile::CFilePath szNameFileRef;
			NFile::CFilePath szTooltipFileRef;
			std::vector< SOptionEntry > options;

			#include "include_OptionCategory.h"

			SOptionsCategory() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			uint32_t CalcCheckSum() const;
		};
		std::vector< SOptionsCategory > categories;

		#include "include_OptionSystem.h"

		SOptionSystem() { }
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

namespace NDb
{
	std::string EnumToString( NDb::SOptionSystem::SOptionsCategory::SOptionEntry::EOptionEditorType eValue );
	SOptionSystem::SOptionsCategory::SOptionEntry::EOptionEditorType StringToEnum_NDb_SOptionSystem_SOptionsCategory_SOptionEntry_EOptionEditorType( const std::string &szValue );
}

template <>
struct SKnownEnum<NDb::SOptionSystem::SOptionsCategory::SOptionEntry::EOptionEditorType>
{
	enum { isKnown = 1 };
	static std::string ToString( NDb::SOptionSystem::SOptionsCategory::SOptionEntry::EOptionEditorType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SOptionSystem::SOptionsCategory::SOptionEntry::EOptionEditorType ToEnum( const std::string &szValue ) { return NDb::StringToEnum_NDb_SOptionSystem_SOptionsCategory_SOptionEntry_EOptionEditorType( szValue ); }
};

