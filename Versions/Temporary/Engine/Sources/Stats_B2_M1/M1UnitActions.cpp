// automatically generated file, don't change manually!

#include "stdafx.h"
#include "../libdb/ReportMetaInfo.h"
#include "../libdb/Checksum.h"
#include "../System/XmlSaver.h"
#include "m1unitactions.h"

namespace NDb
{



void SM1UnitSpecAction::ReportMetaInfo() const
{
	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "Type", (BYTE*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
}

int SM1UnitSpecAction::operator&( IXmlSaver &saver )
{
	saver.Add( "Type", &eType );

	return 0;
}

int SM1UnitSpecAction::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eType );

	return 0;
}

DWORD SM1UnitSpecAction::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eType;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SM1UnitActions::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "M1UnitActions", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "DefaultPerformableActions", (BYTE*)&defaultPerformableActions - pThis, sizeof(defaultPerformableActions), NTypeDef::TYPE_TYPE_BINARY );
	NMetaInfo::ReportMetaInfo( "DefaultActionsToEndure", (BYTE*)&defaultActionsToEndure - pThis, sizeof(defaultActionsToEndure), NTypeDef::TYPE_TYPE_BINARY );
	NMetaInfo::ReportSimpleArrayMetaInfo( "ActionParams", &actionParams, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SM1UnitActions::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "DefaultPerformableActions", &defaultPerformableActions );
	saver.Add( "DefaultActionsToEndure", &defaultActionsToEndure );
	saver.Add( "ActionParams", &actionParams );

	return 0;
}

int SM1UnitActions::operator&( IBinSaver &saver )
{
	saver.Add( 2, &defaultPerformableActions );
	saver.Add( 3, &defaultActionsToEndure );
	saver.Add( 4, &actionParams );

	return 0;
}

DWORD SM1UnitActions::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << defaultPerformableActions << defaultActionsToEndure << actionParams;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SM1UnitActionBuild::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "M1UnitActionBuild", typeID, sizeof(*this) );
	SM1UnitSpecAction::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SM1UnitActionBuild::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SM1UnitSpecAction*)(this) );

	return 0;
}

int SM1UnitActionBuild::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SM1UnitSpecAction*)this );

	return 0;
}

DWORD SM1UnitActionBuild::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SM1UnitSpecAction::CalcCheckSum();
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SM1UnitActionTransform::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "M1UnitActionTransform", typeID, sizeof(*this) );
	SM1UnitSpecAction::ReportMetaInfo();

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportMetaInfo( "TransformationTime", (BYTE*)&nTransformationTime - pThis, sizeof(nTransformationTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "StatsModifier", (BYTE*)&pStatsModifier - pThis, sizeof(pStatsModifier), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SM1UnitActionTransform::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SM1UnitSpecAction*)(this) );
	saver.Add( "TransformationTime", &nTransformationTime );
	saver.Add( "StatsModifier", &pStatsModifier );

	return 0;
}

int SM1UnitActionTransform::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SM1UnitSpecAction*)this );
	saver.Add( 2, &nTransformationTime );
	saver.Add( 3, &pStatsModifier );

	return 0;
}

DWORD SM1UnitActionTransform::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << SM1UnitSpecAction::CalcCheckSum() << nTransformationTime << pStatsModifier;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SM1ParameterModifier::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "MultParam", (BYTE*)&fMultParam - pThis, sizeof(fMultParam), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "AddParam", (BYTE*)&fAddParam - pThis, sizeof(fAddParam), NTypeDef::TYPE_TYPE_FLOAT );
}

int SM1ParameterModifier::operator&( IXmlSaver &saver )
{
	saver.Add( "MultParam", &fMultParam );
	saver.Add( "AddParam", &fAddParam );

	return 0;
}

int SM1ParameterModifier::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fMultParam );
	saver.Add( 3, &fAddParam );

	return 0;
}

DWORD SM1ParameterModifier::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fMultParam << fAddParam;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SShellStatsModifier::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "damage", &damage, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "piercing", &piercing, pThis ); 
}

int SShellStatsModifier::operator&( IXmlSaver &saver )
{
	saver.Add( "damage", &damage );
	saver.Add( "piercing", &piercing );

	return 0;
}

int SShellStatsModifier::operator&( IBinSaver &saver )
{
	saver.Add( 2, &damage );
	saver.Add( 3, &piercing );

	return 0;
}

DWORD SShellStatsModifier::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << damage << piercing;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SWeaponStatsModifier::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "range", &range, pThis ); 
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "shells", &shells, pThis );
}

int SWeaponStatsModifier::operator&( IXmlSaver &saver )
{
	saver.Add( "range", &range );
	saver.Add( "shells", &shells );

	return 0;
}

int SWeaponStatsModifier::operator&( IBinSaver &saver )
{
	saver.Add( 2, &range );
	saver.Add( 3, &shells );

	return 0;
}

DWORD SWeaponStatsModifier::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << range << shells;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SPlatformWeaponsStatsModifier::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "Guns", &guns, pThis );
}

int SPlatformWeaponsStatsModifier::operator&( IXmlSaver &saver )
{
	saver.Add( "Guns", &guns );

	return 0;
}

int SPlatformWeaponsStatsModifier::operator&( IBinSaver &saver )
{
	saver.Add( 2, &guns );

	return 0;
}

DWORD SPlatformWeaponsStatsModifier::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << guns;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SWeaponsStatsModifier::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "Platforms", &platforms, pThis );
}

int SWeaponsStatsModifier::operator&( IXmlSaver &saver )
{
	saver.Add( "Platforms", &platforms );

	return 0;
}

int SWeaponsStatsModifier::operator&( IBinSaver &saver )
{
	saver.Add( 2, &platforms );

	return 0;
}

DWORD SWeaponsStatsModifier::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << platforms;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SM1UnitStatsModifier::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "M1UnitStatsModifier", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "weapons", &weapons, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "sightRange", &sightRange, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "speed", &speed, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "rotateSpeed", &rotateSpeed, pThis ); 
	NMetaInfo::FinishMetaInfoReport();
}

int SM1UnitStatsModifier::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "weapons", &weapons );
	saver.Add( "sightRange", &sightRange );
	saver.Add( "speed", &speed );
	saver.Add( "rotateSpeed", &rotateSpeed );

	return 0;
}

int SM1UnitStatsModifier::operator&( IBinSaver &saver )
{
	saver.Add( 2, &weapons );
	saver.Add( 3, &sightRange );
	saver.Add( 4, &speed );
	saver.Add( 5, &rotateSpeed );

	return 0;
}

DWORD SM1UnitStatsModifier::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << weapons << sightRange << speed << rotateSpeed;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

}
using namespace NDb;
BASIC_REGISTER_DATABASE_CLASS( SM1UnitSpecAction )
REGISTER_DATABASE_CLASS( 0x33196B41, SM1UnitActions ) 
REGISTER_DATABASE_CLASS( 0x331ADBC0, SM1UnitActionBuild ) 
REGISTER_DATABASE_CLASS( 0x331BEB41, SM1UnitActionTransform ) 
REGISTER_DATABASE_CLASS( 0x3016A480, SM1UnitStatsModifier ) 

