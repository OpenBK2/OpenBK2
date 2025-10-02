// automatically generated file, don't change manually!

#include "stdafx.h"
#include "../libdb/ReportMetaInfo.h"
#include "../libdb/Checksum.h"
#include "../System/XmlSaver.h"
#include "dbnotifications.h"

namespace NDb
{


string EnumToString( NDb::ENotificationType eValue )
{
	switch ( eValue )
	{
	case NDb::NTF_OBJECTIVE_RECEIVED:
		return "NTF_OBJECTIVE_RECEIVED";
	case NDb::NTF_OBJECTIVE_COMPLETED:
		return "NTF_OBJECTIVE_COMPLETED";
	case NDb::NTF_OBJECTIVE_FAILED:
		return "NTF_OBJECTIVE_FAILED";
	case NDb::NTF_KEY_OBJECT_CAPTURED:
		return "NTF_KEY_OBJECT_CAPTURED";
	case NDb::NTF_KEY_OBJECT_LOSED:
		return "NTF_KEY_OBJECT_LOSED";
	case NDb::NTF_ENEMY_ARTILLERY:
		return "NTF_ENEMY_ARTILLERY";
	case NDb::NTF_ENEMY_AA_FIRE:
		return "NTF_ENEMY_AA_FIRE";
	case NDb::NTF_REINFORCEMENT_ARRIVED:
		return "NTF_REINFORCEMENT_ARRIVED";
	case NDb::NTF_OBJECTIVES_NOTIFY_REPEAT:
		return "NTF_OBJECTIVES_NOTIFY_REPEAT";
	case NDb::NTF_AVIA_BAD_WEATHER_RETREAT:
		return "NTF_AVIA_BAD_WEATHER_RETREAT";
	case NDb::NTF_ENEMY_AVIA:
		return "NTF_ENEMY_AVIA";
	case NDb::NTF_ENEMY_UNIT:
		return "NTF_ENEMY_UNIT";
	case NDb::NTF_KEY_OBJECT_ATTACKED:
		return "NTF_KEY_OBJECT_ATTACKED";
	case NDb::NTF_MINE_DETECTED:
		return "NTF_MINE_DETECTED";
	case NDb::NTF_UNIT_ATTACKED:
		return "NTF_UNIT_ATTACKED";
	case NDb::NTF_UNITS_GIVEN:
		return "NTF_UNITS_GIVEN";
	case NDb::NTF_MINIMAP_FLARE:
		return "NTF_MINIMAP_FLARE";
	case NDb::NTF_COUNT:
		return "NTF_COUNT";
	default:
		return "NTF_OBJECTIVE_RECEIVED";
	}
}

NDb::ENotificationType NDb::StringToEnum_NDb_ENotificationType( const string &szValue )
{
	if ( szValue == "NTF_OBJECTIVE_RECEIVED" )
		return NDb::NTF_OBJECTIVE_RECEIVED;
	if ( szValue == "NTF_OBJECTIVE_COMPLETED" )
		return NDb::NTF_OBJECTIVE_COMPLETED;
	if ( szValue == "NTF_OBJECTIVE_FAILED" )
		return NDb::NTF_OBJECTIVE_FAILED;
	if ( szValue == "NTF_KEY_OBJECT_CAPTURED" )
		return NDb::NTF_KEY_OBJECT_CAPTURED;
	if ( szValue == "NTF_KEY_OBJECT_LOSED" )
		return NDb::NTF_KEY_OBJECT_LOSED;
	if ( szValue == "NTF_ENEMY_ARTILLERY" )
		return NDb::NTF_ENEMY_ARTILLERY;
	if ( szValue == "NTF_ENEMY_AA_FIRE" )
		return NDb::NTF_ENEMY_AA_FIRE;
	if ( szValue == "NTF_REINFORCEMENT_ARRIVED" )
		return NDb::NTF_REINFORCEMENT_ARRIVED;
	if ( szValue == "NTF_OBJECTIVES_NOTIFY_REPEAT" )
		return NDb::NTF_OBJECTIVES_NOTIFY_REPEAT;
	if ( szValue == "NTF_AVIA_BAD_WEATHER_RETREAT" )
		return NDb::NTF_AVIA_BAD_WEATHER_RETREAT;
	if ( szValue == "NTF_ENEMY_AVIA" )
		return NDb::NTF_ENEMY_AVIA;
	if ( szValue == "NTF_ENEMY_UNIT" )
		return NDb::NTF_ENEMY_UNIT;
	if ( szValue == "NTF_KEY_OBJECT_ATTACKED" )
		return NDb::NTF_KEY_OBJECT_ATTACKED;
	if ( szValue == "NTF_MINE_DETECTED" )
		return NDb::NTF_MINE_DETECTED;
	if ( szValue == "NTF_UNIT_ATTACKED" )
		return NDb::NTF_UNIT_ATTACKED;
	if ( szValue == "NTF_UNITS_GIVEN" )
		return NDb::NTF_UNITS_GIVEN;
	if ( szValue == "NTF_MINIMAP_FLARE" )
		return NDb::NTF_MINIMAP_FLARE;
	if ( szValue == "NTF_COUNT" )
		return NDb::NTF_COUNT;
	return NDb::NTF_OBJECTIVE_RECEIVED;
}

string EnumToString( NDb::EMinimapFigureType eValue )
{
	switch ( eValue )
	{
	case NDb::MFT_TRIANGLE:
		return "MFT_TRIANGLE";
	case NDb::MFT_SQUARE:
		return "MFT_SQUARE";
	case NDb::MFT_CIRCLE:
		return "MFT_CIRCLE";
	default:
		return "MFT_TRIANGLE";
	}
}

NDb::EMinimapFigureType NDb::StringToEnum_NDb_EMinimapFigureType( const string &szValue )
{
	if ( szValue == "MFT_TRIANGLE" )
		return NDb::MFT_TRIANGLE;
	if ( szValue == "MFT_SQUARE" )
		return NDb::MFT_SQUARE;
	if ( szValue == "MFT_CIRCLE" )
		return NDb::MFT_CIRCLE;
	return NDb::MFT_TRIANGLE;
}


void SNotification::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Notification", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Type", (BYTE*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "TextFileRef", (BYTE*)&szTextFileRef - pThis, sizeof(szTextFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "Sound", (BYTE*)&pSound - pThis, sizeof(pSound), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "FigureType", (BYTE*)&eFigureType - pThis, sizeof(eFigureType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "Color", (BYTE*)&nColor - pThis, sizeof(nColor), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Time", (BYTE*)&fTime - pThis, sizeof(fTime), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Size", (BYTE*)&fSize - pThis, sizeof(fSize), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "RotationSpeed", (BYTE*)&fRotationSpeed - pThis, sizeof(fRotationSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "DuplicateDelay", (BYTE*)&fDuplicateDelay - pThis, sizeof(fDuplicateDelay), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SNotification::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Type", &eType );
	saver.Add( "TextFileRef", &szTextFileRef );
	saver.Add( "Sound", &pSound );
	saver.Add( "FigureType", &eFigureType );
	saver.Add( "Color", &nColor );
	saver.Add( "Time", &fTime );
	saver.Add( "Size", &fSize );
	saver.Add( "RotationSpeed", &fRotationSpeed );
	saver.Add( "DuplicateDelay", &fDuplicateDelay );

	return 0;
}

int SNotification::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eType );
	saver.Add( 3, &szTextFileRef );
	saver.Add( 4, &pSound );
	saver.Add( 5, &eFigureType );
	saver.Add( 6, &nColor );
	saver.Add( 7, &fTime );
	saver.Add( 8, &fSize );
	saver.Add( 9, &fRotationSpeed );
	saver.Add( 10, &fDuplicateDelay );

	return 0;
}


string EnumToString( NDb::ENotificationEventType eValue )
{
	switch ( eValue )
	{
	case NDb::NEVT_OBJECTIVE_COMPLETED:
		return "NEVT_OBJECTIVE_COMPLETED";
	case NDb::NEVT_OBJECTIVE_RECEIVED:
		return "NEVT_OBJECTIVE_RECEIVED";
	case NDb::NEVT_OBJECTIVE_FAILED:
		return "NEVT_OBJECTIVE_FAILED";
	case NDb::NEVT_KEY_POINT_CAPTURED:
		return "NEVT_KEY_POINT_CAPTURED";
	case NDb::NEVT_KEY_POINT_ATTACKED:
		return "NEVT_KEY_POINT_ATTACKED";
	case NDb::NEVT_KEY_POINT_LOST:
		return "NEVT_KEY_POINT_LOST";
	case NDb::NEVT_PLAYER_ELIMINATED:
		return "NEVT_PLAYER_ELIMINATED";
	case NDb::NEVT_REINF_NEW_TYPE:
		return "NEVT_REINF_NEW_TYPE";
	case NDb::NEVT_REINF_LEVELUP:
		return "NEVT_REINF_LEVELUP";
	case NDb::NEVT_REINF_CANT_CALL:
		return "NEVT_REINF_CANT_CALL";
	case NDb::NEVT_AVIA_AVAILABLE:
		return "NEVT_AVIA_AVAILABLE";
	case NDb::NEVT_AVIA_BAD_WEATHER_RETREAT:
		return "NEVT_AVIA_BAD_WEATHER_RETREAT";
	case NDb::NEVT_ENEMY_AVIA_DETECTED:
		return "NEVT_ENEMY_AVIA_DETECTED";
	case NDb::NEVT_ENEMY_ART_DETECTED:
		return "NEVT_ENEMY_ART_DETECTED";
	case NDb::NEVT_ENEMY_AA_DETECTED:
		return "NEVT_ENEMY_AA_DETECTED";
	case NDb::NEVT_ENEMY_UNIT_DETECTED:
		return "NEVT_ENEMY_UNIT_DETECTED";
	case NDb::NEVT_UNIT_ATTACKED:
		return "NEVT_UNIT_ATTACKED";
	case NDb::NEVT_UNIT_BLOWUP_AT_MINE:
		return "NEVT_UNIT_BLOWUP_AT_MINE";
	case NDb::NEVT_UNIT_OUT_OF_AMMUNITION:
		return "NEVT_UNIT_OUT_OF_AMMUNITION";
	case NDb::NEVT_ENGINEERING_MINE_DETECTED:
		return "NEVT_ENGINEERING_MINE_DETECTED";
	case NDb::NEVT_ENGINEERING_COMPLETED:
		return "NEVT_ENGINEERING_COMPLETED";
	case NDb::NEVT_ENGINEERING_INTERRUPTED:
		return "NEVT_ENGINEERING_INTERRUPTED";
	case NDb::NEVT_OBJECTIVE_MOVED:
		return "NEVT_OBJECTIVE_MOVED";
	case NDb::NEVT_UNITS_GIVEN:
		return "NEVT_UNITS_GIVEN";
	case NDb::NEVT_COUNT:
		return "NEVT_COUNT";
	default:
		return "NEVT_OBJECTIVE_COMPLETED";
	}
}

NDb::ENotificationEventType NDb::StringToEnum_NDb_ENotificationEventType( const string &szValue )
{
	if ( szValue == "NEVT_OBJECTIVE_COMPLETED" )
		return NDb::NEVT_OBJECTIVE_COMPLETED;
	if ( szValue == "NEVT_OBJECTIVE_RECEIVED" )
		return NDb::NEVT_OBJECTIVE_RECEIVED;
	if ( szValue == "NEVT_OBJECTIVE_FAILED" )
		return NDb::NEVT_OBJECTIVE_FAILED;
	if ( szValue == "NEVT_KEY_POINT_CAPTURED" )
		return NDb::NEVT_KEY_POINT_CAPTURED;
	if ( szValue == "NEVT_KEY_POINT_ATTACKED" )
		return NDb::NEVT_KEY_POINT_ATTACKED;
	if ( szValue == "NEVT_KEY_POINT_LOST" )
		return NDb::NEVT_KEY_POINT_LOST;
	if ( szValue == "NEVT_PLAYER_ELIMINATED" )
		return NDb::NEVT_PLAYER_ELIMINATED;
	if ( szValue == "NEVT_REINF_NEW_TYPE" )
		return NDb::NEVT_REINF_NEW_TYPE;
	if ( szValue == "NEVT_REINF_LEVELUP" )
		return NDb::NEVT_REINF_LEVELUP;
	if ( szValue == "NEVT_REINF_CANT_CALL" )
		return NDb::NEVT_REINF_CANT_CALL;
	if ( szValue == "NEVT_AVIA_AVAILABLE" )
		return NDb::NEVT_AVIA_AVAILABLE;
	if ( szValue == "NEVT_AVIA_BAD_WEATHER_RETREAT" )
		return NDb::NEVT_AVIA_BAD_WEATHER_RETREAT;
	if ( szValue == "NEVT_ENEMY_AVIA_DETECTED" )
		return NDb::NEVT_ENEMY_AVIA_DETECTED;
	if ( szValue == "NEVT_ENEMY_ART_DETECTED" )
		return NDb::NEVT_ENEMY_ART_DETECTED;
	if ( szValue == "NEVT_ENEMY_AA_DETECTED" )
		return NDb::NEVT_ENEMY_AA_DETECTED;
	if ( szValue == "NEVT_ENEMY_UNIT_DETECTED" )
		return NDb::NEVT_ENEMY_UNIT_DETECTED;
	if ( szValue == "NEVT_UNIT_ATTACKED" )
		return NDb::NEVT_UNIT_ATTACKED;
	if ( szValue == "NEVT_UNIT_BLOWUP_AT_MINE" )
		return NDb::NEVT_UNIT_BLOWUP_AT_MINE;
	if ( szValue == "NEVT_UNIT_OUT_OF_AMMUNITION" )
		return NDb::NEVT_UNIT_OUT_OF_AMMUNITION;
	if ( szValue == "NEVT_ENGINEERING_MINE_DETECTED" )
		return NDb::NEVT_ENGINEERING_MINE_DETECTED;
	if ( szValue == "NEVT_ENGINEERING_COMPLETED" )
		return NDb::NEVT_ENGINEERING_COMPLETED;
	if ( szValue == "NEVT_ENGINEERING_INTERRUPTED" )
		return NDb::NEVT_ENGINEERING_INTERRUPTED;
	if ( szValue == "NEVT_OBJECTIVE_MOVED" )
		return NDb::NEVT_OBJECTIVE_MOVED;
	if ( szValue == "NEVT_UNITS_GIVEN" )
		return NDb::NEVT_UNITS_GIVEN;
	if ( szValue == "NEVT_COUNT" )
		return NDb::NEVT_COUNT;
	return NDb::NEVT_OBJECTIVE_COMPLETED;
}


void SNotificationEvent::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "NotificationEvent", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Type", (BYTE*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "Texture", (BYTE*)&pTexture - pThis, sizeof(pTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "TextFileRef", (BYTE*)&szTextFileRef - pThis, sizeof(szTextFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "TooltipFileRef", (BYTE*)&szTooltipFileRef - pThis, sizeof(szTooltipFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "Sound", (BYTE*)&pSound - pThis, sizeof(pSound), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "ShowByCamera", (BYTE*)&bShowByCamera - pThis, sizeof(bShowByCamera), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "AutoRemoveTime", (BYTE*)&fAutoRemoveTime - pThis, sizeof(fAutoRemoveTime), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Notification", (BYTE*)&pNotification - pThis, sizeof(pNotification), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "NoDupArea", (BYTE*)&fNoDupArea - pThis, sizeof(fNoDupArea), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "NoDupTime", (BYTE*)&fNoDupTime - pThis, sizeof(fNoDupTime), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SNotificationEvent::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Type", &eType );
	saver.Add( "Texture", &pTexture );
	saver.Add( "TextFileRef", &szTextFileRef );
	saver.Add( "TooltipFileRef", &szTooltipFileRef );
	saver.Add( "Sound", &pSound );
	saver.Add( "ShowByCamera", &bShowByCamera );
	saver.Add( "AutoRemoveTime", &fAutoRemoveTime );
	saver.Add( "Notification", &pNotification );
	saver.Add( "NoDupArea", &fNoDupArea );
	saver.Add( "NoDupTime", &fNoDupTime );

	return 0;
}

int SNotificationEvent::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eType );
	saver.Add( 3, &pTexture );
	saver.Add( 4, &szTextFileRef );
	saver.Add( 5, &szTooltipFileRef );
	saver.Add( 6, &pSound );
	saver.Add( 7, &bShowByCamera );
	saver.Add( 8, &fAutoRemoveTime );
	saver.Add( 9, &pNotification );
	saver.Add( 10, &fNoDupArea );
	saver.Add( 11, &fNoDupTime );

	return 0;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( 0x17135D40, SNotification ) 
REGISTER_DATABASE_CLASS( 0x171BCB00, SNotificationEvent ) 
