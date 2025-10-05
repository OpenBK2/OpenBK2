// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "dbmusicsystem.h"

namespace NDb
{



void SPlayTime::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "PlayTime", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Numer", (BYTE*)&nNumer - pThis, sizeof(nNumer), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "NumberRandom", (BYTE*)&nNumberRandom - pThis, sizeof(nNumberRandom), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "PlayTime", (BYTE*)&nPlayTime - pThis, sizeof(nPlayTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "PlayTimeRandom", (BYTE*)&nPlayTimeRandom - pThis, sizeof(nPlayTimeRandom), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SPlayTime::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Numer", &nNumer );
	saver.Add( "NumberRandom", &nNumberRandom );
	saver.Add( "PlayTime", &nPlayTime );
	saver.Add( "PlayTimeRandom", &nPlayTimeRandom );

	return 0;
}

int SPlayTime::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nNumer );
	saver.Add( 3, &nNumberRandom );
	saver.Add( 4, &nPlayTime );
	saver.Add( 5, &nPlayTimeRandom );

	return 0;
}



void SPlayPause::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "PauseTime", (BYTE*)&nPauseTime - pThis, sizeof(nPauseTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "PauseRandom", (BYTE*)&nPauseRandom - pThis, sizeof(nPauseRandom), NTypeDef::TYPE_TYPE_INT );
}

int SPlayPause::operator&( IXmlSaver &saver )
{
	saver.Add( "PauseTime", &nPauseTime );
	saver.Add( "PauseRandom", &nPauseRandom );

	return 0;
}

int SPlayPause::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nPauseTime );
	saver.Add( 3, &nPauseRandom );

	return 0;
}

DWORD SPlayPause::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nPauseTime << nPauseRandom;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SFade::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Fade", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "FinalVolume", (BYTE*)&fFinalVolume - pThis, sizeof(fFinalVolume), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "FadeTime", (BYTE*)&nFadeTime - pThis, sizeof(nFadeTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Pause", (BYTE*)&bPause - pThis, sizeof(bPause), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::FinishMetaInfoReport();
}

int SFade::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "FinalVolume", &fFinalVolume );
	saver.Add( "FadeTime", &nFadeTime );
	saver.Add( "Pause", &bPause );

	return 0;
}

int SFade::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fFinalVolume );
	saver.Add( 3, &nFadeTime );
	saver.Add( 4, &bPause );

	return 0;
}



void SMusicTrack::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "MusicTrack", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "MusicFileName", (BYTE*)&szMusicFileName - pThis, sizeof(szMusicFileName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int SMusicTrack::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "MusicFileName", &szMusicFileName );

	return 0;
}

int SMusicTrack::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szMusicFileName );

	return 0;
}



void SComposition::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Composition", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Track", (BYTE*)&pTrack - pThis, sizeof(pTrack), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "FadeIn", (BYTE*)&pFadeIn - pThis, sizeof(pFadeIn), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "FadeOut", (BYTE*)&pFadeOut - pThis, sizeof(pFadeOut), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "PlayTime", (BYTE*)&pPlayTime - pThis, sizeof(pPlayTime), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "PlayPauseAfter", &playPauseAfter, pThis ); 
	NMetaInfo::FinishMetaInfoReport();
}

int SComposition::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Track", &pTrack );
	saver.Add( "FadeIn", &pFadeIn );
	saver.Add( "FadeOut", &pFadeOut );
	saver.Add( "PlayTime", &pPlayTime );
	saver.Add( "PlayPauseAfter", &playPauseAfter );

	return 0;
}

int SComposition::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pTrack );
	saver.Add( 3, &pFadeIn );
	saver.Add( 4, &pFadeOut );
	saver.Add( 5, &pPlayTime );
	saver.Add( 6, &playPauseAfter );

	return 0;
}



void SVoice::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Voice", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Track", (BYTE*)&pTrack - pThis, sizeof(pTrack), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "PlayTime", (BYTE*)&pPlayTime - pThis, sizeof(pPlayTime), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "FadeIn", (BYTE*)&pFadeIn - pThis, sizeof(pFadeIn), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "FadeOut", (BYTE*)&pFadeOut - pThis, sizeof(pFadeOut), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "MusicStreamFadeIn", (BYTE*)&pMusicStreamFadeIn - pThis, sizeof(pMusicStreamFadeIn), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "MusicStreamFadeOut", (BYTE*)&pMusicStreamFadeOut - pThis, sizeof(pMusicStreamFadeOut), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SVoice::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Track", &pTrack );
	saver.Add( "PlayTime", &pPlayTime );
	saver.Add( "FadeIn", &pFadeIn );
	saver.Add( "FadeOut", &pFadeOut );
	saver.Add( "MusicStreamFadeIn", &pMusicStreamFadeIn );
	saver.Add( "MusicStreamFadeOut", &pMusicStreamFadeOut );

	return 0;
}

int SVoice::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pTrack );
	saver.Add( 3, &pPlayTime );
	saver.Add( 4, &pFadeIn );
	saver.Add( 5, &pFadeOut );
	saver.Add( 6, &pMusicStreamFadeIn );
	saver.Add( 7, &pMusicStreamFadeOut );

	return 0;
}



void SCompositionDesc::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Composition", (BYTE*)&pComposition - pThis, sizeof(pComposition), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Weight", (BYTE*)&fWeight - pThis, sizeof(fWeight), NTypeDef::TYPE_TYPE_FLOAT );
}

int SCompositionDesc::operator&( IXmlSaver &saver )
{
	saver.Add( "Composition", &pComposition );
	saver.Add( "Weight", &fWeight );

	return 0;
}

int SCompositionDesc::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pComposition );
	saver.Add( 3, &fWeight );

	return 0;
}

DWORD SCompositionDesc::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fWeight;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SPlayList::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "PlayList", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportSimpleArrayMetaInfo( "StillOrder", &stillOrder, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "RandomOrder", &randomOrder, pThis );
	NMetaInfo::ReportMetaInfo( "FadeIn", (BYTE*)&pFadeIn - pThis, sizeof(pFadeIn), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "FadeOut", (BYTE*)&pFadeOut - pThis, sizeof(pFadeOut), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SPlayList::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "StillOrder", &stillOrder );
	saver.Add( "RandomOrder", &randomOrder );
	saver.Add( "FadeIn", &pFadeIn );
	saver.Add( "FadeOut", &pFadeOut );

	return 0;
}

int SPlayList::operator&( IBinSaver &saver )
{
	saver.Add( 2, &stillOrder );
	saver.Add( 3, &randomOrder );
	saver.Add( 4, &pFadeIn );
	saver.Add( 5, &pFadeOut );

	return 0;
}



void SMapMusic::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "MapMusic", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportSimpleArrayMetaInfo( "PlayLists", &playLists, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SMapMusic::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "PlayLists", &playLists );

	return 0;
}

int SMapMusic::operator&( IBinSaver &saver )
{
	saver.Add( 2, &playLists );

	return 0;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( 0x111814C0, SPlayTime ) 
REGISTER_DATABASE_CLASS( 0x11181300, SFade ) 
REGISTER_DATABASE_CLASS( 0x11181301, SMusicTrack ) 
REGISTER_DATABASE_CLASS( 0x11181302, SComposition ) 
REGISTER_DATABASE_CLASS( 0x11181380, SVoice ) 
REGISTER_DATABASE_CLASS( 0x11181303, SPlayList ) 
REGISTER_DATABASE_CLASS( 0x11181305, SMapMusic ) 

