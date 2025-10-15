// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "IconsSet.h"

#include <cstdint>

#include "Stats_B2_M1_export.h"

namespace NDb
{


std::string EnumToString( NDb::SIconsSet::SIconType::EIconTypeEnum eValue )
{
	switch ( eValue )
	{
	case NDb::SIconsSet::SIconType::ICONTYPE_NONE:
		return "ICONTYPE_NONE";
	case NDb::SIconsSet::SIconType::ICONTYPE_HPBAR:
		return "ICONTYPE_HPBAR";
	case NDb::SIconsSet::SIconType::ICONTYPE_GROUP00:
		return "ICONTYPE_GROUP00";
	case NDb::SIconsSet::SIconType::ICONTYPE_GROUP01:
		return "ICONTYPE_GROUP01";
	case NDb::SIconsSet::SIconType::ICONTYPE_GROUP02:
		return "ICONTYPE_GROUP02";
	case NDb::SIconsSet::SIconType::ICONTYPE_GROUP03:
		return "ICONTYPE_GROUP03";
	case NDb::SIconsSet::SIconType::ICONTYPE_GROUP04:
		return "ICONTYPE_GROUP04";
	case NDb::SIconsSet::SIconType::ICONTYPE_GROUP05:
		return "ICONTYPE_GROUP05";
	case NDb::SIconsSet::SIconType::ICONTYPE_GROUP06:
		return "ICONTYPE_GROUP06";
	case NDb::SIconsSet::SIconType::ICONTYPE_GROUP07:
		return "ICONTYPE_GROUP07";
	case NDb::SIconsSet::SIconType::ICONTYPE_GROUP08:
		return "ICONTYPE_GROUP08";
	case NDb::SIconsSet::SIconType::ICONTYPE_GROUP09:
		return "ICONTYPE_GROUP09";
	case NDb::SIconsSet::SIconType::ICONTYPE_BROKENTRUCK:
		return "ICONTYPE_BROKENTRUCK";
	case NDb::SIconsSet::SIconType::ICONTYPE_NEW_ABILITY:
		return "ICONTYPE_NEW_ABILITY";
	default:
		return "ICONTYPE_NONE";
	}
}

NDb::SIconsSet::SIconType::EIconTypeEnum NDb::StringToEnum_NDb_SIconsSet_SIconType_EIconTypeEnum( const std::string &szValue )
{
	if ( szValue == "ICONTYPE_NONE" )
		return NDb::SIconsSet::SIconType::ICONTYPE_NONE;
	if ( szValue == "ICONTYPE_HPBAR" )
		return NDb::SIconsSet::SIconType::ICONTYPE_HPBAR;
	if ( szValue == "ICONTYPE_GROUP00" )
		return NDb::SIconsSet::SIconType::ICONTYPE_GROUP00;
	if ( szValue == "ICONTYPE_GROUP01" )
		return NDb::SIconsSet::SIconType::ICONTYPE_GROUP01;
	if ( szValue == "ICONTYPE_GROUP02" )
		return NDb::SIconsSet::SIconType::ICONTYPE_GROUP02;
	if ( szValue == "ICONTYPE_GROUP03" )
		return NDb::SIconsSet::SIconType::ICONTYPE_GROUP03;
	if ( szValue == "ICONTYPE_GROUP04" )
		return NDb::SIconsSet::SIconType::ICONTYPE_GROUP04;
	if ( szValue == "ICONTYPE_GROUP05" )
		return NDb::SIconsSet::SIconType::ICONTYPE_GROUP05;
	if ( szValue == "ICONTYPE_GROUP06" )
		return NDb::SIconsSet::SIconType::ICONTYPE_GROUP06;
	if ( szValue == "ICONTYPE_GROUP07" )
		return NDb::SIconsSet::SIconType::ICONTYPE_GROUP07;
	if ( szValue == "ICONTYPE_GROUP08" )
		return NDb::SIconsSet::SIconType::ICONTYPE_GROUP08;
	if ( szValue == "ICONTYPE_GROUP09" )
		return NDb::SIconsSet::SIconType::ICONTYPE_GROUP09;
	if ( szValue == "ICONTYPE_BROKENTRUCK" )
		return NDb::SIconsSet::SIconType::ICONTYPE_BROKENTRUCK;
	if ( szValue == "ICONTYPE_NEW_ABILITY" )
		return NDb::SIconsSet::SIconType::ICONTYPE_NEW_ABILITY;
	return NDb::SIconsSet::SIconType::ICONTYPE_NONE;
}


void SIconsSet::SIconType::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Type", (uint8_t*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Rect", &rcRect, pThis ); 
}

int SIconsSet::SIconType::operator&( IXmlSaver &saver )
{
	saver.Add( "Type", &eType );
	saver.Add( "Rect", &rcRect );

	return 0;
}

int SIconsSet::SIconType::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eType );
	saver.Add( 3, &rcRect );

	return 0;
}

uint32_t SIconsSet::SIconType::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eType << rcRect;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SIconsSet::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "IconsSet", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "MaterialZCheck", (uint8_t*)&pMaterialZCheck - pThis, sizeof(pMaterialZCheck), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "MaterialNonZCheck", (uint8_t*)&pMaterialNonZCheck - pThis, sizeof(pMaterialNonZCheck), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( "icons", &icons, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SIconsSet::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "MaterialZCheck", &pMaterialZCheck );
	saver.Add( "MaterialNonZCheck", &pMaterialNonZCheck );
	saver.Add( "icons", &icons );

	return 0;
}

int SIconsSet::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pMaterialZCheck );
	saver.Add( 3, &pMaterialNonZCheck );
	saver.Add( 4, &icons );

	return 0;
}


std::string EnumToString( NDb::SVisObjIconsSet::SVisObjIcon::EVisObjIconType eValue )
{
	switch ( eValue )
	{
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_NONE:
		return "VOIT_NONE";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GROUP00:
		return "VOIT_GROUP00";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GROUP01:
		return "VOIT_GROUP01";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GROUP02:
		return "VOIT_GROUP02";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GROUP03:
		return "VOIT_GROUP03";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GROUP04:
		return "VOIT_GROUP04";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GROUP05:
		return "VOIT_GROUP05";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GROUP06:
		return "VOIT_GROUP06";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GROUP07:
		return "VOIT_GROUP07";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GROUP08:
		return "VOIT_GROUP08";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GROUP09:
		return "VOIT_GROUP09";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_BROKENTRUCK:
		return "VOIT_BROKENTRUCK";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_NEW_ABILITY:
		return "VOIT_NEW_ABILITY";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_INFANTRY:
		return "VOIT_INFANTRY";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_PROCESS:
		return "VOIT_PROCESS";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GUARD:
		return "VOIT_GUARD";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_NEED_REPAIR_KEY_OBJECT:
		return "VOIT_NEED_REPAIR_KEY_OBJECT";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_NEED_RESUPPLY:
		return "VOIT_NEED_RESUPPLY";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_AGRESSIVE:
		return "VOIT_AGRESSIVE";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_BEST_SHOT:
		return "VOIT_BEST_SHOT";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_PACIFIST:
		return "VOIT_PACIFIST";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_INVISIBLE:
		return "VOIT_INVISIBLE";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_ABILITY_RANK_2:
		return "VOIT_ABILITY_RANK_2";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_ABILITY_RANK_3:
		return "VOIT_ABILITY_RANK_3";
	case NDb::SVisObjIconsSet::SVisObjIcon::VOIT_ABILITY_RANK_4:
		return "VOIT_ABILITY_RANK_4";
	default:
		return "VOIT_NONE";
	}
}

NDb::SVisObjIconsSet::SVisObjIcon::EVisObjIconType NDb::StringToEnum_NDb_SVisObjIconsSet_SVisObjIcon_EVisObjIconType( const std::string &szValue )
{
	if ( szValue == "VOIT_NONE" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_NONE;
	if ( szValue == "VOIT_GROUP00" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GROUP00;
	if ( szValue == "VOIT_GROUP01" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GROUP01;
	if ( szValue == "VOIT_GROUP02" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GROUP02;
	if ( szValue == "VOIT_GROUP03" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GROUP03;
	if ( szValue == "VOIT_GROUP04" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GROUP04;
	if ( szValue == "VOIT_GROUP05" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GROUP05;
	if ( szValue == "VOIT_GROUP06" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GROUP06;
	if ( szValue == "VOIT_GROUP07" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GROUP07;
	if ( szValue == "VOIT_GROUP08" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GROUP08;
	if ( szValue == "VOIT_GROUP09" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GROUP09;
	if ( szValue == "VOIT_BROKENTRUCK" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_BROKENTRUCK;
	if ( szValue == "VOIT_NEW_ABILITY" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_NEW_ABILITY;
	if ( szValue == "VOIT_INFANTRY" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_INFANTRY;
	if ( szValue == "VOIT_PROCESS" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_PROCESS;
	if ( szValue == "VOIT_GUARD" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_GUARD;
	if ( szValue == "VOIT_NEED_REPAIR_KEY_OBJECT" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_NEED_REPAIR_KEY_OBJECT;
	if ( szValue == "VOIT_NEED_RESUPPLY" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_NEED_RESUPPLY;
	if ( szValue == "VOIT_AGRESSIVE" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_AGRESSIVE;
	if ( szValue == "VOIT_BEST_SHOT" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_BEST_SHOT;
	if ( szValue == "VOIT_PACIFIST" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_PACIFIST;
	if ( szValue == "VOIT_INVISIBLE" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_INVISIBLE;
	if ( szValue == "VOIT_ABILITY_RANK_2" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_ABILITY_RANK_2;
	if ( szValue == "VOIT_ABILITY_RANK_3" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_ABILITY_RANK_3;
	if ( szValue == "VOIT_ABILITY_RANK_4" )
		return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_ABILITY_RANK_4;
	return NDb::SVisObjIconsSet::SVisObjIcon::VOIT_NONE;
}


void SVisObjIconsSet::SVisObjIcon::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Type", (uint8_t*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "Priority", (uint8_t*)&nPriority - pThis, sizeof(nPriority), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructMetaInfo( szAddName + "texCoords", &rctexCoords, pThis ); 
}

int SVisObjIconsSet::SVisObjIcon::operator&( IXmlSaver &saver )
{
	saver.Add( "Type", &eType );
	saver.Add( "Priority", &nPriority );
	saver.Add( "texCoords", &rctexCoords );

	return 0;
}

int SVisObjIconsSet::SVisObjIcon::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eType );
	saver.Add( 3, &nPriority );
	saver.Add( 4, &rctexCoords );

	return 0;
}

uint32_t SVisObjIconsSet::SVisObjIcon::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eType << nPriority << rctexCoords;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SVisObjIconsSet::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "VisObjIconsSet", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Texture", (uint8_t*)&pTexture - pThis, sizeof(pTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructArrayMetaInfo( "hpBarBorders", &hpBarBorders, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "hpBarColors", &hpBarColors, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "icons", &icons, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "hpBarRanges", &hpBarRanges, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SVisObjIconsSet::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Texture", &pTexture );
	saver.Add( "hpBarBorders", &hpBarBorders );
	saver.Add( "hpBarColors", &hpBarColors );
	saver.Add( "icons", &icons );
	saver.Add( "hpBarRanges", &hpBarRanges );

	return 0;
}

int SVisObjIconsSet::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pTexture );
	saver.Add( 3, &hpBarBorders );
	saver.Add( 4, &hpBarColors );
	saver.Add( 5, &icons );
	saver.Add( 6, &hpBarRanges );

	return 0;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( STATS_B2_M1, 0x1311B302, SIconsSet )
REGISTER_DATABASE_CLASS( STATS_B2_M1, 0x1313C400, SVisObjIconsSet )

