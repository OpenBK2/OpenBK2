#pragma once

// automatically generated file, don't change manually!

#include "../system/filepath.h"

interface IXmlSaver;

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
			mutable DWORD __dwCheckSum;
		public:

			struct SOptionEntry
			{
			private:
				mutable DWORD __dwCheckSum;
			public:

				struct SOptionEntryState
				{
				private:
					mutable DWORD __dwCheckSum;
				public:
					NFile::CFilePath szNameFileRef;
					NFile::CFilePath szTooltipFileRef;
					string szValue;

					#include "include_OptionEntryState.h"

					SOptionEntryState() :
						__dwCheckSum( 0 )
					{ }
					//
					void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
					//
					int operator&( IBinSaver &saver );
					int operator&( IXmlSaver &saver );
					DWORD CalcCheckSum() const;
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
					mutable DWORD __dwCheckSum;
				public:
					string szProgName;
					vector< string > values;

					SSliderSingleValue() :
						__dwCheckSum( 0 )
					{ }
					//
					void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
					//
					int operator&( IBinSaver &saver );
					int operator&( IXmlSaver &saver );
					DWORD CalcCheckSum() const;
				};
				string szProgName;
				NFile::CFilePath szNameFileRef;
				NFile::CFilePath szTooltipFileRef;
				vector< SOptionEntryState > states;
				EOptionEditorType eEditorType;
				string szDefaultValue;
				int nModeFlags;
				vector< SSliderSingleValue > sliderValues;

				#include "include_OptionEntry.h"

				SOptionEntry() :
					__dwCheckSum( 0 ),
					eEditorType( OPTION_EDITOR_EDITLINE ),
					nModeFlags( 0 )
				{ }
				//
				void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
				//
				int operator&( IBinSaver &saver );
				int operator&( IXmlSaver &saver );
				DWORD CalcCheckSum() const;
			};
			NFile::CFilePath szNameFileRef;
			NFile::CFilePath szTooltipFileRef;
			vector< SOptionEntry > options;

			#include "include_OptionCategory.h"

			SOptionsCategory() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		vector< SOptionsCategory > categories;

		#include "include_OptionSystem.h"

		SOptionSystem() { }
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
	string EnumToString( NDb::SOptionSystem::SOptionsCategory::SOptionEntry::EOptionEditorType eValue );
	SOptionSystem::SOptionsCategory::SOptionEntry::EOptionEditorType StringToEnum_NDb_SOptionSystem_SOptionsCategory_SOptionEntry_EOptionEditorType( const string &szValue );
}

template <>
struct SKnownEnum<NDb::SOptionSystem::SOptionsCategory::SOptionEntry::EOptionEditorType>
{
	enum { isKnown = 1 };
	static string ToString( NDb::SOptionSystem::SOptionsCategory::SOptionEntry::EOptionEditorType eValue ) { return NDb::EnumToString( eValue ); }
	static NDb::SOptionSystem::SOptionsCategory::SOptionEntry::EOptionEditorType ToEnum( const string &szValue ) { return NDb::StringToEnum_NDb_SOptionSystem_SOptionsCategory_SOptionEntry_EOptionEditorType( szValue ); }
};
