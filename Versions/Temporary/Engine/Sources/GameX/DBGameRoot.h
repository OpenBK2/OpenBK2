#pragma once

// automatically generated file, don't change manually!

#include "stats_b2_m1/RPGStats.h"
#include "stats_b2_m1/uientries.h"
#include "dbscenario.h"
#include "UI/dbuserinterface.h"
#include "System/FilePath.h"

struct IXmlSaver;

namespace NDb
{
	struct SWindowScreen;
	struct SMapInfo;
	struct STextEntry;
	struct SComplexSoundDesc;
	struct SCampaign;
	struct SNotificationEvent;
	struct SGameConsts;
	struct SOptionSystem;
	struct SNotification;
	struct SMechUnitRPGStats;
	struct SMultiplayerMap;

	struct SUISoundEntry
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		std::string szType;
		CDBPtr< SComplexSoundDesc > pSound;

		SUISoundEntry() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SMainMenuBackground
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SMapInfo > pMap;
		CDBPtr< STexture > pPicture;

		SMainMenuBackground() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SHallOfFameRecord
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		NFile::CFilePath szNameFileRef;
		int nScore;

		SHallOfFameRecord() :
			__dwCheckSum( 0 ),
			nScore( 0 )
		{ }
		//
		void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SGameRoot : public CResource
	{
		OBJECT_BASIC_METHODS( SGameRoot )
	public:
		enum { typeID = 0x1007B4C1 };
	private:
		mutable DWORD __dwCheckSum;
	public:

		struct STutorialMap
		{
		private:
			mutable DWORD __dwCheckSum;
		public:
			CDBPtr< SMapInfo > pMapInfo;
			NFile::CFilePath szDifficultyFileRef;

			STutorialMap() :
				__dwCheckSum( 0 )
			{ }
			//
			void ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		CDBPtr< SGameConsts > pConsts;
		std::vector< CDBPtr< SCampaign > > campaigns;
		std::vector< STutorialMap > tutorialMaps;
		CDBPtr< SWindowScreen > pScreenVideoPlayer;
		std::vector< SUIScreenEntry > screens;
		std::vector< CDBPtr< STextEntry > > textEntries;
		std::vector< CDBPtr< SFont > > fonts;
		std::vector< SUISoundEntry > sounds;
		std::vector< SUITextureEntry > textures;
		CDBPtr< SOptionSystem > pGameOptions;
		SMainMenuBackground mainMenuBackground;
		CDBPtr< STexture > pInterfacesBackground;
		std::vector< CDBPtr< SNotification > > notifications;
		std::vector< CDBPtr< SNotificationEvent > > notificationEvents;
		NFile::CFilePath szIntroMovie;
		std::vector< CDBPtr< SMechUnitRPGStats > > encyclopediaMechUnits;
		std::vector< NFile::CFilePath > citationFileRefs;
		std::vector< CDBPtr< SMultiplayerMap > > multiplayerMaps;
		CDBPtr< SMultiplayerMap > pTestMap;
		CDBPtr< SMapMusic > pMainMenuMusic;
		std::vector< SHallOfFameRecord > hallOfFameDefaultRecords;

		#include "include_GameRoot.h"

		SGameRoot() :
			__dwCheckSum( 0 )
		{ }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};
}

