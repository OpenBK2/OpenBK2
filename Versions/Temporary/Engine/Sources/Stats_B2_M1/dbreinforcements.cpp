// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "dbreinforcements.h"

namespace NDb
{



void SIntArray::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "data", &data, pThis );
}

int SIntArray::operator&( IXmlSaver &saver )
{
	saver.Add( "data", &data );

	return 0;
}

int SIntArray::operator&( IBinSaver &saver )
{
	saver.Add( 2, &data );

	return 0;
}

DWORD SIntArray::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << data;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SReinforcementGroupInfoEntry::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "GroupID", (BYTE*)&nGroupID - pThis, sizeof(nGroupID), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructMetaInfo( szAddName + "GroupsVector", &groupsVector, pThis ); 
}

int SReinforcementGroupInfoEntry::operator&( IXmlSaver &saver )
{
	saver.Add( "GroupID", &nGroupID );
	saver.Add( "GroupsVector", &groupsVector );

	return 0;
}

int SReinforcementGroupInfoEntry::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nGroupID );
	saver.Add( 3, &groupsVector );

	return 0;
}

DWORD SReinforcementGroupInfoEntry::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nGroupID << groupsVector;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SReinforcementGroupInfo::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "infos", &infos, pThis );
}

int SReinforcementGroupInfo::operator&( IXmlSaver &saver )
{
	saver.Add( "infos", &infos );

	return 0;
}

int SReinforcementGroupInfo::operator&( IBinSaver &saver )
{
	saver.Add( 2, &infos );

	return 0;
}

DWORD SReinforcementGroupInfo::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << infos;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SReinforcementMaskEntry::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Direction", (BYTE*)&nDirection - pThis, sizeof(nDirection), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Position", &vPosition, pThis ); 
}

int SReinforcementMaskEntry::operator&( IXmlSaver &saver )
{
	saver.Add( "Direction", &nDirection );
	saver.Add( "Position", &vPosition );

	return 0;
}

int SReinforcementMaskEntry::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nDirection );
	saver.Add( 3, &vPosition );

	return 0;
}

DWORD SReinforcementMaskEntry::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nDirection << vPosition;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SReinforcementMask::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "Positions", &positions, pThis );
}

int SReinforcementMask::operator&( IXmlSaver &saver )
{
	saver.Add( "Positions", &positions );

	return 0;
}

int SReinforcementMask::operator&( IBinSaver &saver )
{
	saver.Add( 2, &positions );

	return 0;
}

DWORD SReinforcementMask::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << positions;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SReinforcementDefinition::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "MechUnit", (BYTE*)&pMechUnit - pThis, sizeof(pMechUnit), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Squad", (BYTE*)&pSquad - pThis, sizeof(pSquad), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "UnitType", (BYTE*)&eUnitType - pThis, sizeof(eUnitType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "IsAmmunition", (BYTE*)&bIsAmmunition - pThis, sizeof(bIsAmmunition), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "Type", (BYTE*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "Reinforcement", (BYTE*)&pReinforcement - pThis, sizeof(pReinforcement), NTypeDef::TYPE_TYPE_REF );
}

int SReinforcementDefinition::operator&( IXmlSaver &saver )
{
	saver.Add( "MechUnit", &pMechUnit );
	saver.Add( "Squad", &pSquad );
	saver.Add( "UnitType", &eUnitType );
	saver.Add( "IsAmmunition", &bIsAmmunition );
	saver.Add( "Type", &eType );
	saver.Add( "Reinforcement", &pReinforcement );

	return 0;
}

int SReinforcementDefinition::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pMechUnit );
	saver.Add( 3, &pSquad );
	saver.Add( 4, &eUnitType );
	saver.Add( 5, &bIsAmmunition );
	saver.Add( 6, &eType );
	saver.Add( 7, &pReinforcement );

	return 0;
}

DWORD SReinforcementDefinition::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << pMechUnit << pSquad << eUnitType << bIsAmmunition << eType << pReinforcement;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void STypedDeployTemplate::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Type", (BYTE*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "Template", (BYTE*)&pTemplate - pThis, sizeof(pTemplate), NTypeDef::TYPE_TYPE_REF );
}

int STypedDeployTemplate::operator&( IXmlSaver &saver )
{
	saver.Add( "Type", &eType );
	saver.Add( "Template", &pTemplate );

	return 0;
}

int STypedDeployTemplate::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eType );
	saver.Add( 3, &pTemplate );

	return 0;
}

DWORD STypedDeployTemplate::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eType << pTemplate;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SReinforcementPosition::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Position", &vPosition, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "AviationPosition", &vAviationPosition, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "Direction", (BYTE*)&nDirection - pThis, sizeof(nDirection), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "Template", (BYTE*)&pTemplate - pThis, sizeof(pTemplate), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "TypedTemplates", &typedTemplates, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "IsDefault", (BYTE*)&bIsDefault - pThis, sizeof(bIsDefault), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "___delete_from_here_to_the_end", (BYTE*)&b___delete_from_here_to_the_end - pThis, sizeof(b___delete_from_here_to_the_end), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "Type", (BYTE*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "FactoryID", (BYTE*)&nFactoryID - pThis, sizeof(nFactoryID), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "PositionID", (BYTE*)&nPositionID - pThis, sizeof(nPositionID), NTypeDef::TYPE_TYPE_INT );
}

int SReinforcementPosition::operator&( IXmlSaver &saver )
{
	saver.Add( "Position", &vPosition );
	saver.Add( "AviationPosition", &vAviationPosition );
	saver.Add( "Direction", &nDirection );
	saver.Add( "Template", &pTemplate );
	saver.Add( "TypedTemplates", &typedTemplates );
	saver.Add( "IsDefault", &bIsDefault );
	saver.Add( "___delete_from_here_to_the_end", &b___delete_from_here_to_the_end );
	saver.Add( "Type", &eType );
	saver.Add( "FactoryID", &nFactoryID );
	saver.Add( "PositionID", &nPositionID );

	return 0;
}

int SReinforcementPosition::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vPosition );
	saver.Add( 3, &vAviationPosition );
	saver.Add( 4, &nDirection );
	saver.Add( 5, &pTemplate );
	saver.Add( 6, &typedTemplates );
	saver.Add( 7, &bIsDefault );
	saver.Add( 8, &b___delete_from_here_to_the_end );
	saver.Add( 9, &eType );
	saver.Add( 10, &nFactoryID );
	saver.Add( 11, &nPositionID );

	return 0;
}

DWORD SReinforcementPosition::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vPosition << vAviationPosition << nDirection << pTemplate << typedTemplates << bIsDefault << b___delete_from_here_to_the_end << eType << nFactoryID << nPositionID;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SScriptReinforcementEntry::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Reinforcement", (BYTE*)&pReinforcement - pThis, sizeof(pReinforcement), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Name", (BYTE*)&szName - pThis, sizeof(szName), NTypeDef::TYPE_TYPE_STRING );
}

int SScriptReinforcementEntry::operator&( IXmlSaver &saver )
{
	saver.Add( "Reinforcement", &pReinforcement );
	saver.Add( "Name", &szName );

	return 0;
}

int SScriptReinforcementEntry::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pReinforcement );
	saver.Add( 3, &szName );

	return 0;
}

DWORD SScriptReinforcementEntry::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << pReinforcement << szName;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SPlayerReinforcementEnable::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "NewPointOnEnable", &newPointOnEnable, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "ReinforcementToEnable", (BYTE*)&eReinforcementToEnable - pThis, sizeof(eReinforcementToEnable), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "SetPoint", (BYTE*)&bSetPoint - pThis, sizeof(bSetPoint), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "GivenReinforcementPointID", (BYTE*)&nGivenReinforcementPointID - pThis, sizeof(nGivenReinforcementPointID), NTypeDef::TYPE_TYPE_INT );
}

int SPlayerReinforcementEnable::operator&( IXmlSaver &saver )
{
	saver.Add( "NewPointOnEnable", &newPointOnEnable );
	saver.Add( "ReinforcementToEnable", &eReinforcementToEnable );
	saver.Add( "SetPoint", &bSetPoint );
	saver.Add( "GivenReinforcementPointID", &nGivenReinforcementPointID );

	return 0;
}

int SPlayerReinforcementEnable::operator&( IBinSaver &saver )
{
	saver.Add( 2, &newPointOnEnable );
	saver.Add( 3, &eReinforcementToEnable );
	saver.Add( 4, &bSetPoint );
	saver.Add( 5, &nGivenReinforcementPointID );

	return 0;
}

DWORD SPlayerReinforcementEnable::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << newPointOnEnable << eReinforcementToEnable << bSetPoint << nGivenReinforcementPointID;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

}
using namespace NDb;

