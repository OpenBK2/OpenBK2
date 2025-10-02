#pragma once

// automatically generated file, don't change manually!

#include "../stats_b2_m1/rpgstats.h"
#include "../stats_b2_m1/uientries.h"
#include "dbscenario.h"
#include "../ui/dbuserinterface.h"
#include "../system/filepath.h"

interface IXmlSaver;

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
		string szType;
		CDBPtr< SComplexSoundDesc > pSound;

		SUISoundEntry() :
			__dwCheckSum( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
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
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
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
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
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
			void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
			//
			int operator&( IBinSaver &saver );
			int operator&( IXmlSaver &saver );
			DWORD CalcCheckSum() const;
		};
		CDBPtr< SGameConsts > pConsts;
		vector< CDBPtr< SCampaign > > campaigns;
		vector< STutorialMap > tutorialMaps;
		CDBPtr< SWindowScreen > pScreenVideoPlayer;
		vector< SUIScreenEntry > screens;
		vector< CDBPtr< STextEntry > > textEntries;
		vector< CDBPtr< SFont > > fonts;
		vector< SUISoundEntry > sounds;
		vector< SUITextureEntry > textures;
		CDBPtr< SOptionSystem > pGameOptions;
		SMainMenuBackground mainMenuBackground;
		CDBPtr< STexture > pInterfacesBackground;
		vector< CDBPtr< SNotification > > notifications;
		vector< CDBPtr< SNotificationEvent > > notificationEvents;
		NFile::CFilePath szIntroMovie;
		vector< CDBPtr< SMechUnitRPGStats > > encyclopediaMechUnits;
		vector< NFile::CFilePath > citationFileRefs;
		vector< CDBPtr< SMultiplayerMap > > multiplayerMaps;
		CDBPtr< SMultiplayerMap > pTestMap;
		CDBPtr< SMapMusic > pMainMenuMusic;
		vector< SHallOfFameRecord > hallOfFameDefaultRecords;

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
