// automatically generated file, don't change manually!

#include "stdafx.h"
#include "../libdb/ReportMetaInfo.h"
#include "../libdb/Checksum.h"
#include "../System/XmlSaver.h"
#include "dbclientconsts.h"

namespace NDb
{



void SWCActionsPriority::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "SelfActions", &selfActions, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "FriendActions", &friendActions, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "EnemyActions", &enemyActions, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "NeutralActions", &neutralActions, pThis );
}

int SWCActionsPriority::operator&( IXmlSaver &saver )
{
	saver.Add( "SelfActions", &selfActions );
	saver.Add( "FriendActions", &friendActions );
	saver.Add( "EnemyActions", &enemyActions );
	saver.Add( "NeutralActions", &neutralActions );

	return 0;
}

int SWCActionsPriority::operator&( IBinSaver &saver )
{
	saver.Add( 2, &selfActions );
	saver.Add( 3, &friendActions );
	saver.Add( 4, &enemyActions );
	saver.Add( 5, &neutralActions );

	return 0;
}

DWORD SWCActionsPriority::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << selfActions << friendActions << enemyActions << neutralActions;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SM1WCActionsPriority::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "SelfActions", &selfActions, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "FriendActions", &friendActions, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "EnemyActions", &enemyActions, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "NeutralActions", &neutralActions, pThis );
}

int SM1WCActionsPriority::operator&( IXmlSaver &saver )
{
	saver.Add( "SelfActions", &selfActions );
	saver.Add( "FriendActions", &friendActions );
	saver.Add( "EnemyActions", &enemyActions );
	saver.Add( "NeutralActions", &neutralActions );

	return 0;
}

int SM1WCActionsPriority::operator&( IBinSaver &saver )
{
	saver.Add( 2, &selfActions );
	saver.Add( 3, &friendActions );
	saver.Add( 4, &enemyActions );
	saver.Add( 5, &neutralActions );

	return 0;
}

DWORD SM1WCActionsPriority::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << selfActions << friendActions << enemyActions << neutralActions;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SCursor::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "FileName", (BYTE*)&szFileName - pThis, sizeof(szFileName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "Action", (BYTE*)&eAction - pThis, sizeof(eAction), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "M1Action", (BYTE*)&eM1Action - pThis, sizeof(eM1Action), NTypeDef::TYPE_TYPE_ENUM );
}

int SCursor::operator&( IXmlSaver &saver )
{
	saver.Add( "FileName", &szFileName );
	saver.Add( "Action", &eAction );
	saver.Add( "M1Action", &eM1Action );

	return 0;
}

int SCursor::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szFileName );
	saver.Add( 3, &eAction );
	saver.Add( 4, &eM1Action );

	return 0;
}

DWORD SCursor::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eAction << eM1Action;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SAckParameter::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "AckType", (BYTE*)&eAckType - pThis, sizeof(eAckType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "AckClass", (BYTE*)&eAckClass - pThis, sizeof(eAckClass), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "TextStringFileRef", (BYTE*)&szTextStringFileRef - pThis, sizeof(szTextStringFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "TimeAfterPrevious", (BYTE*)&nTimeAfterPrevious - pThis, sizeof(nTimeAfterPrevious), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "Position", (BYTE*)&ePosition - pThis, sizeof(ePosition), NTypeDef::TYPE_TYPE_ENUM );
}

int SAckParameter::operator&( IXmlSaver &saver )
{
	saver.Add( "AckType", &eAckType );
	saver.Add( "AckClass", &eAckClass );
	saver.Add( "TextStringFileRef", &szTextStringFileRef );
	saver.Add( "TimeAfterPrevious", &nTimeAfterPrevious );
	saver.Add( "Position", &ePosition );

	return 0;
}

int SAckParameter::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eAckType );
	saver.Add( 3, &eAckClass );
	saver.Add( 4, &szTextStringFileRef );
	saver.Add( 5, &nTimeAfterPrevious );
	saver.Add( 6, &ePosition );

	return 0;
}

DWORD SAckParameter::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eAckType << eAckClass << nTimeAfterPrevious << ePosition;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SAckManagerConsts::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "MinAckRadius", (BYTE*)&nMinAckRadius - pThis, sizeof(nMinAckRadius), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "MaxAckRadius", (BYTE*)&nMaxAckRadius - pThis, sizeof(nMaxAckRadius), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "TimeAckWait", (BYTE*)&nTimeAckWait - pThis, sizeof(nTimeAckWait), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "NumSelectionsBeforeAnnoyed", (BYTE*)&nNumSelectionsBeforeAnnoyed - pThis, sizeof(nNumSelectionsBeforeAnnoyed), NTypeDef::TYPE_TYPE_INT );
}

int SAckManagerConsts::operator&( IXmlSaver &saver )
{
	saver.Add( "MinAckRadius", &nMinAckRadius );
	saver.Add( "MaxAckRadius", &nMaxAckRadius );
	saver.Add( "TimeAckWait", &nTimeAckWait );
	saver.Add( "NumSelectionsBeforeAnnoyed", &nNumSelectionsBeforeAnnoyed );

	return 0;
}

int SAckManagerConsts::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nMinAckRadius );
	saver.Add( 3, &nMaxAckRadius );
	saver.Add( 4, &nTimeAckWait );
	saver.Add( 5, &nNumSelectionsBeforeAnnoyed );

	return 0;
}

DWORD SAckManagerConsts::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nMinAckRadius << nMaxAckRadius << nTimeAckWait << nNumSelectionsBeforeAnnoyed;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMapCommandAck::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "VisObj", (BYTE*)&pVisObj - pThis, sizeof(pVisObj), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "ShowTime", (BYTE*)&fShowTime - pThis, sizeof(fShowTime), NTypeDef::TYPE_TYPE_FLOAT );
}

int SMapCommandAck::operator&( IXmlSaver &saver )
{
	saver.Add( "VisObj", &pVisObj );
	saver.Add( "ShowTime", &fShowTime );

	return 0;
}

int SMapCommandAck::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pVisObj );
	saver.Add( 3, &fShowTime );

	return 0;
}

DWORD SMapCommandAck::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fShowTime;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SClientGameConsts::SMechUnitIconsSet::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Type", (BYTE*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "IconsSet", (BYTE*)&pIconsSet - pThis, sizeof(pIconsSet), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Raising", (BYTE*)&fRaising - pThis, sizeof(fRaising), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "HPBarLen", (BYTE*)&fHPBarLen - pThis, sizeof(fHPBarLen), NTypeDef::TYPE_TYPE_FLOAT );
}

int SClientGameConsts::SMechUnitIconsSet::operator&( IXmlSaver &saver )
{
	saver.Add( "Type", &eType );
	saver.Add( "IconsSet", &pIconsSet );
	saver.Add( "Raising", &fRaising );
	saver.Add( "HPBarLen", &fHPBarLen );

	return 0;
}

int SClientGameConsts::SMechUnitIconsSet::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eType );
	saver.Add( 3, &pIconsSet );
	saver.Add( 4, &fRaising );
	saver.Add( 5, &fHPBarLen );

	return 0;
}

DWORD SClientGameConsts::SMechUnitIconsSet::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eType << fRaising << fHPBarLen;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SClientGameConsts::SSquadIconsSet::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Type", (BYTE*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "IconsSet", (BYTE*)&pIconsSet - pThis, sizeof(pIconsSet), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Raising", (BYTE*)&fRaising - pThis, sizeof(fRaising), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "HPBarLen", (BYTE*)&fHPBarLen - pThis, sizeof(fHPBarLen), NTypeDef::TYPE_TYPE_FLOAT );
}

int SClientGameConsts::SSquadIconsSet::operator&( IXmlSaver &saver )
{
	saver.Add( "Type", &eType );
	saver.Add( "IconsSet", &pIconsSet );
	saver.Add( "Raising", &fRaising );
	saver.Add( "HPBarLen", &fHPBarLen );

	return 0;
}

int SClientGameConsts::SSquadIconsSet::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eType );
	saver.Add( 3, &pIconsSet );
	saver.Add( 4, &fRaising );
	saver.Add( 5, &fHPBarLen );

	return 0;
}

DWORD SClientGameConsts::SSquadIconsSet::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eType << fRaising << fHPBarLen;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SClientGameConsts::SBuildingIconsSet::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Type", (BYTE*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "IconsSet", (BYTE*)&pIconsSet - pThis, sizeof(pIconsSet), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Raising", (BYTE*)&fRaising - pThis, sizeof(fRaising), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "HPBarLen", (BYTE*)&fHPBarLen - pThis, sizeof(fHPBarLen), NTypeDef::TYPE_TYPE_FLOAT );
}

int SClientGameConsts::SBuildingIconsSet::operator&( IXmlSaver &saver )
{
	saver.Add( "Type", &eType );
	saver.Add( "IconsSet", &pIconsSet );
	saver.Add( "Raising", &fRaising );
	saver.Add( "HPBarLen", &fHPBarLen );

	return 0;
}

int SClientGameConsts::SBuildingIconsSet::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eType );
	saver.Add( 3, &pIconsSet );
	saver.Add( 4, &fRaising );
	saver.Add( 5, &fHPBarLen );

	return 0;
}

DWORD SClientGameConsts::SBuildingIconsSet::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eType << fRaising << fHPBarLen;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SClientGameConsts::SPassengerIconsSet::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "IconsSet", (BYTE*)&pIconsSet - pThis, sizeof(pIconsSet), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "HPBarLen", (BYTE*)&fHPBarLen - pThis, sizeof(fHPBarLen), NTypeDef::TYPE_TYPE_FLOAT );
}

int SClientGameConsts::SPassengerIconsSet::operator&( IXmlSaver &saver )
{
	saver.Add( "IconsSet", &pIconsSet );
	saver.Add( "HPBarLen", &fHPBarLen );

	return 0;
}

int SClientGameConsts::SPassengerIconsSet::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pIconsSet );
	saver.Add( 3, &fHPBarLen );

	return 0;
}

DWORD SClientGameConsts::SPassengerIconsSet::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fHPBarLen;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SClientGameConsts::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ClientGameConsts", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Camera", (BYTE*)&pCamera - pThis, sizeof(pCamera), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( "Cursors", &cursors, pThis );
	NMetaInfo::ReportStructMetaInfo( "ActionsPriority", &actionsPriority, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "M1ActionsPriority", &m1ActionsPriority, pThis ); 
	NMetaInfo::ReportStructArrayMetaInfo( "AcksParameters", &acksParameters, pThis );
	NMetaInfo::ReportStructMetaInfo( "AckConsts", &ackConsts, pThis ); 
	NMetaInfo::ReportStructArrayMetaInfo( "MechUnitIconsSets", &mechUnitIconsSets, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "SquadIconsSets", &squadIconsSets, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "BuildingIconsSets", &buildingIconsSets, pThis );
	NMetaInfo::ReportStructMetaInfo( "PassengerIconsSet", &passengerIconsSet, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "MapCommandAck", &mapCommandAck, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "MapCommandAckDir", &mapCommandAckDir, pThis ); 
	NMetaInfo::ReportMetaInfo( "MapPointer", (BYTE*)&pMapPointer - pThis, sizeof(pMapPointer), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Noises", (BYTE*)&szNoises - pThis, sizeof(szNoises), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int SClientGameConsts::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Camera", &pCamera );
	saver.Add( "Cursors", &cursors );
	saver.Add( "ActionsPriority", &actionsPriority );
	saver.Add( "M1ActionsPriority", &m1ActionsPriority );
	saver.Add( "AcksParameters", &acksParameters );
	saver.Add( "AckConsts", &ackConsts );
	saver.Add( "MechUnitIconsSets", &mechUnitIconsSets );
	saver.Add( "SquadIconsSets", &squadIconsSets );
	saver.Add( "BuildingIconsSets", &buildingIconsSets );
	saver.Add( "PassengerIconsSet", &passengerIconsSet );
	saver.Add( "MapCommandAck", &mapCommandAck );
	saver.Add( "MapCommandAckDir", &mapCommandAckDir );
	saver.Add( "MapPointer", &pMapPointer );
	saver.Add( "Noises", &szNoises );

	return 0;
}

int SClientGameConsts::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pCamera );
	saver.Add( 3, &cursors );
	saver.Add( 4, &actionsPriority );
	saver.Add( 5, &m1ActionsPriority );
	saver.Add( 6, &acksParameters );
	saver.Add( 7, &ackConsts );
	saver.Add( 8, &mechUnitIconsSets );
	saver.Add( 9, &squadIconsSets );
	saver.Add( 10, &buildingIconsSets );
	saver.Add( 11, &passengerIconsSet );
	saver.Add( 12, &mapCommandAck );
	saver.Add( 13, &mapCommandAckDir );
	saver.Add( 14, &pMapPointer );
	saver.Add( 15, &szNoises );

	return 0;
}

DWORD SClientGameConsts::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << pCamera << cursors << actionsPriority << m1ActionsPriority << acksParameters << ackConsts << mechUnitIconsSets << squadIconsSets << buildingIconsSets << passengerIconsSet << mapCommandAck << mapCommandAckDir;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( 0x1007BA80, SClientGameConsts ) 
