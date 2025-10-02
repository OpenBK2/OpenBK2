#pragma once

// automatically generated file, don't change manually!

#include "system/filepath.h"

struct IXmlSaver;

namespace NDb
{
	struct SFade;
	struct SPlayTime;
	struct SMusicTrack;
	struct SPlayList;
	struct SComposition;

	struct SPlayTime : public CResource
	{
		OBJECT_BASIC_METHODS( SPlayTime )
	public:
		enum { typeID = 0x111814C0 };
		int nNumer;
		int nNumberRandom;
		int nPlayTime;
		int nPlayTimeRandom;

		SPlayTime() :
			nNumer( 0 ),
			nNumberRandom( 0 ),
			nPlayTime( 0 ),
			nPlayTimeRandom( 0 )
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

	struct SPlayPause
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		int nPauseTime;
		int nPauseRandom;

		SPlayPause() :
			__dwCheckSum( 0 ),
			nPauseTime( 0 ),
			nPauseRandom( 0 )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SFade : public CResource
	{
		OBJECT_BASIC_METHODS( SFade )
	public:
		enum { typeID = 0x11181300 };
		float fFinalVolume;
		int nFadeTime;
		bool bPause;

		SFade() :
			fFinalVolume( 0 ),
			nFadeTime( 0 ),
			bPause( false )
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

	struct SMusicTrack : public CResource
	{
		OBJECT_BASIC_METHODS( SMusicTrack )
	public:
		enum { typeID = 0x11181301 };
		NFile::CFilePath szMusicFileName;

		SMusicTrack() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SComposition : public CResource
	{
		OBJECT_BASIC_METHODS( SComposition )
	public:
		enum { typeID = 0x11181302 };
		CDBPtr< SMusicTrack > pTrack;
		CDBPtr< SFade > pFadeIn;
		CDBPtr< SFade > pFadeOut;
		CDBPtr< SPlayTime > pPlayTime;
		SPlayPause playPauseAfter;

		SComposition() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SVoice : public CResource
	{
		OBJECT_BASIC_METHODS( SVoice )
	public:
		enum { typeID = 0x11181380 };
		CDBPtr< SMusicTrack > pTrack;
		CDBPtr< SPlayTime > pPlayTime;
		CDBPtr< SFade > pFadeIn;
		CDBPtr< SFade > pFadeOut;
		CDBPtr< SFade > pMusicStreamFadeIn;
		CDBPtr< SFade > pMusicStreamFadeOut;

		SVoice() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SCompositionDesc
	{
	private:
		mutable DWORD __dwCheckSum;
	public:
		CDBPtr< SComposition > pComposition;
		float fWeight;

		SCompositionDesc() :
			__dwCheckSum( 0 ),
			fWeight( 0.0f )
		{ }
		//
		void ReportMetaInfo( const string &szAddName, BYTE *pThis ) const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const;
	};

	struct SPlayList : public CResource
	{
		OBJECT_BASIC_METHODS( SPlayList )
	public:
		enum { typeID = 0x11181303 };
		vector< CDBPtr< SComposition > > stillOrder;
		vector< SCompositionDesc > randomOrder;
		CDBPtr< SFade > pFadeIn;
		CDBPtr< SFade > pFadeOut;

		SPlayList() { }
		//
		int GetTypeID() const { return typeID; }
		//
		void ReportMetaInfo() const;
		//
		int operator&( IBinSaver &saver );
		int operator&( IXmlSaver &saver );
		DWORD CalcCheckSum() const { return 0; }
	};

	struct SMapMusic : public CResource
	{
		OBJECT_BASIC_METHODS( SMapMusic )
	public:
		enum { typeID = 0x11181305 };
		vector< CDBPtr< SPlayList > > playLists;

		SMapMusic() { }
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

