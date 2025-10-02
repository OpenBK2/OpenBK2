#pragma once

#include "Stats_B2_M1_export.h"


// automatically generated file, don't change manually!

#include "../system/filepath.h"

struct IXmlSaver;

namespace NDb
{
	struct SWindowScreen;
	struct STexture;

	struct STATS_B2_M1_EXPORT SUIScreenEntry
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SWindowScreen > pScreen;
		string szType;
		NFile::CFilePath szHelpHeaderFileRef;
		NFile::CFilePath szHelpDescFileRef;
		bool bHelpNoMultiplayer;

		#include "include_UIScreenEntry.h"

		SUIScreenEntry() :
			__dwCheckSum( 0 ),
			bHelpNoMultiplayer( false )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SUITextEntry
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		NFile::CFilePath szTextFileRef;
		string szTextID;

		#include "include_UITextEntry.h"

		SUITextEntry() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct STATS_B2_M1_EXPORT SUITextureEntry
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		string szTextID;
		CDBPtr< STexture > pTexture;

		SUITextureEntry() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct STextEntry : public CResource
	{
		OBJECT_BASIC_METHODS( STextEntry )
	public:
		enum { typeID = 0x171AE380 };
		string szName;
		NFile::CFilePath szTextFileRef;

		#include "include_TextEntry.h"

		STextEntry() { }
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

