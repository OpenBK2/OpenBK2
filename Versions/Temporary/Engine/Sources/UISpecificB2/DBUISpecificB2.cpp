// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "dbuispecificb2.h"
#include "dbuispecificcpp.h"

#include "UISpecificB2_export.h"

#include <cstdint>

namespace NDb
{



void SARSetSpecialAbility::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ARSetSpecialAbility", typeID, sizeof(*this) );
	SUIDesc::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Ability", (uint8_t*)&eAbility - pThis, sizeof(eAbility), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "AbilityParam", (uint8_t*)&eAbilityParam - pThis, sizeof(eAbilityParam), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::FinishMetaInfoReport();
}

int SARSetSpecialAbility::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIDesc*)(this) );
	saver.Add( "Ability", &eAbility );
	saver.Add( "AbilityParam", &eAbilityParam );

	return 0;
}

int SARSetSpecialAbility::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIDesc*)this );
	saver.Add( 2, &eAbility );
	saver.Add( 3, &eAbilityParam );

	return 0;
}



void SARSetForcedAction::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ARSetForcedAction", typeID, sizeof(*this) );
	SUIDesc::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "UserAction", (uint8_t*)&eUserAction - pThis, sizeof(eUserAction), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "M1UserAction", (uint8_t*)&eM1UserAction - pThis, sizeof(eM1UserAction), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::FinishMetaInfoReport();
}

int SARSetForcedAction::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIDesc*)(this) );
	saver.Add( "UserAction", &eUserAction );
	saver.Add( "M1UserAction", &eM1UserAction );

	return 0;
}

int SARSetForcedAction::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIDesc*)this );
	saver.Add( 2, &eUserAction );
	saver.Add( 3, &eM1UserAction );

	return 0;
}


std::string EnumToString( NDb::EActionButtonPanel eValue )
{
	switch ( eValue )
	{
	case NDb::ACTION_BTN_PANEL_DEFAULT:
		return "ACTION_BTN_PANEL_DEFAULT";
	case NDb::ACTION_BTN_PANEL_ESC:
		return "ACTION_BTN_PANEL_ESC";
	case NDb::ACTION_BTN_PANEL_FORMATIONS:
		return "ACTION_BTN_PANEL_FORMATIONS";
	case NDb::ACTION_BTN_PANEL_RADIO_CONTROLLED:
		return "ACTION_BTN_PANEL_RADIO_CONTROLLED";
	default:
		return "ACTION_BTN_PANEL_DEFAULT";
	}
}

NDb::EActionButtonPanel NDb::StringToEnum_NDb_EActionButtonPanel( const std::string &szValue )
{
	if ( szValue == "ACTION_BTN_PANEL_DEFAULT" )
		return NDb::ACTION_BTN_PANEL_DEFAULT;
	if ( szValue == "ACTION_BTN_PANEL_ESC" )
		return NDb::ACTION_BTN_PANEL_ESC;
	if ( szValue == "ACTION_BTN_PANEL_FORMATIONS" )
		return NDb::ACTION_BTN_PANEL_FORMATIONS;
	if ( szValue == "ACTION_BTN_PANEL_RADIO_CONTROLLED" )
		return NDb::ACTION_BTN_PANEL_RADIO_CONTROLLED;
	return NDb::ACTION_BTN_PANEL_DEFAULT;
}


void SActionButton::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Action", (uint8_t*)&eAction - pThis, sizeof(eAction), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "Button", (uint8_t*)&pButton - pThis, sizeof(pButton), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "NewButton", (uint8_t*)&pNewButton - pThis, sizeof(pNewButton), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "IsAbility", (uint8_t*)&bIsAbility - pThis, sizeof(bIsAbility), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "Autocast", (uint8_t*)&bAutocast - pThis, sizeof(bAutocast), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "Passive", (uint8_t*)&bPassive - pThis, sizeof(bPassive), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "TooltipFileRef", (uint8_t*)&szTooltipFileRef - pThis, sizeof(szTooltipFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "Icon", (uint8_t*)&pIcon - pThis, sizeof(pIcon), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "ForegroundIcon", (uint8_t*)&pForegroundIcon - pThis, sizeof(pForegroundIcon), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "IconDisabled", (uint8_t*)&pIconDisabled - pThis, sizeof(pIconDisabled), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "ForegroundIconDisabled", (uint8_t*)&pForegroundIconDisabled - pThis, sizeof(pForegroundIconDisabled), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "PressEffect", (uint8_t*)&bPressEffect - pThis, sizeof(bPressEffect), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "Panel", (uint8_t*)&ePanel - pThis, sizeof(ePanel), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "TargetPanel", (uint8_t*)&eTargetPanel - pThis, sizeof(eTargetPanel), NTypeDef::TYPE_TYPE_ENUM );
}

int SActionButton::operator&( IXmlSaver &saver )
{
	saver.Add( "Action", &eAction );
	saver.Add( "Button", &pButton );
	saver.Add( "NewButton", &pNewButton );
	saver.Add( "IsAbility", &bIsAbility );
	saver.Add( "Autocast", &bAutocast );
	saver.Add( "Passive", &bPassive );
	saver.Add( "Pos", &vPos );
	saver.Add( "TooltipFileRef", &szTooltipFileRef );
	saver.Add( "Icon", &pIcon );
	saver.Add( "ForegroundIcon", &pForegroundIcon );
	saver.Add( "IconDisabled", &pIconDisabled );
	saver.Add( "ForegroundIconDisabled", &pForegroundIconDisabled );
	saver.Add( "PressEffect", &bPressEffect );
	saver.Add( "Panel", &ePanel );
	saver.Add( "TargetPanel", &eTargetPanel );

	return 0;
}

int SActionButton::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eAction );
	saver.Add( 3, &pButton );
	saver.Add( 4, &pNewButton );
	saver.Add( 5, &bIsAbility );
	saver.Add( 6, &bAutocast );
	saver.Add( 7, &bPassive );
	saver.Add( 8, &vPos );
	saver.Add( 9, &szTooltipFileRef );
	saver.Add( 10, &pIcon );
	saver.Add( 11, &pForegroundIcon );
	saver.Add( 12, &pIconDisabled );
	saver.Add( 13, &pForegroundIconDisabled );
	saver.Add( 14, &bPressEffect );
	saver.Add( 15, &ePanel );
	saver.Add( 16, &eTargetPanel );

	return 0;
}

uint32_t SActionButton::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eAction << bIsAbility << bAutocast << bPassive << vPos << bPressEffect << ePanel << eTargetPanel;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SM1ActionButton::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Action", (uint8_t*)&eAction - pThis, sizeof(eAction), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "CrapName", (uint8_t*)&szCrapName - pThis, sizeof(szCrapName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "Button", (uint8_t*)&pButton - pThis, sizeof(pButton), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "NewButton", (uint8_t*)&pNewButton - pThis, sizeof(pNewButton), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "IsAbility", (uint8_t*)&bIsAbility - pThis, sizeof(bIsAbility), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "Autocast", (uint8_t*)&bAutocast - pThis, sizeof(bAutocast), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "Passive", (uint8_t*)&bPassive - pThis, sizeof(bPassive), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis ); 
	NMetaInfo::ReportMetaInfo( szAddName + "TooltipFileRef", (uint8_t*)&szTooltipFileRef - pThis, sizeof(szTooltipFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "Icon", (uint8_t*)&pIcon - pThis, sizeof(pIcon), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "ForegroundIcon", (uint8_t*)&pForegroundIcon - pThis, sizeof(pForegroundIcon), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "IconDisabled", (uint8_t*)&pIconDisabled - pThis, sizeof(pIconDisabled), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "ForegroundIconDisabled", (uint8_t*)&pForegroundIconDisabled - pThis, sizeof(pForegroundIconDisabled), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "PressEffect", (uint8_t*)&bPressEffect - pThis, sizeof(bPressEffect), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( szAddName + "Panel", (uint8_t*)&ePanel - pThis, sizeof(ePanel), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "TargetPanel", (uint8_t*)&eTargetPanel - pThis, sizeof(eTargetPanel), NTypeDef::TYPE_TYPE_ENUM );
}

int SM1ActionButton::operator&( IXmlSaver &saver )
{
	saver.Add( "Action", &eAction );
	saver.Add( "CrapName", &szCrapName );
	saver.Add( "Button", &pButton );
	saver.Add( "NewButton", &pNewButton );
	saver.Add( "IsAbility", &bIsAbility );
	saver.Add( "Autocast", &bAutocast );
	saver.Add( "Passive", &bPassive );
	saver.Add( "Pos", &vPos );
	saver.Add( "TooltipFileRef", &szTooltipFileRef );
	saver.Add( "Icon", &pIcon );
	saver.Add( "ForegroundIcon", &pForegroundIcon );
	saver.Add( "IconDisabled", &pIconDisabled );
	saver.Add( "ForegroundIconDisabled", &pForegroundIconDisabled );
	saver.Add( "PressEffect", &bPressEffect );
	saver.Add( "Panel", &ePanel );
	saver.Add( "TargetPanel", &eTargetPanel );

	return 0;
}

int SM1ActionButton::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eAction );
	saver.Add( 3, &szCrapName );
	saver.Add( 4, &pButton );
	saver.Add( 5, &pNewButton );
	saver.Add( 6, &bIsAbility );
	saver.Add( 7, &bAutocast );
	saver.Add( 8, &bPassive );
	saver.Add( 9, &vPos );
	saver.Add( 10, &szTooltipFileRef );
	saver.Add( 11, &pIcon );
	saver.Add( 12, &pForegroundIcon );
	saver.Add( 13, &pIconDisabled );
	saver.Add( 14, &pForegroundIconDisabled );
	saver.Add( 15, &bPressEffect );
	saver.Add( 16, &ePanel );
	saver.Add( 17, &eTargetPanel );

	return 0;
}

uint32_t SM1ActionButton::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eAction << szCrapName << bIsAbility << bAutocast << bPassive << vPos << bPressEffect << ePanel << eTargetPanel;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SActionButtonInfo::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "ActionButtonInfo", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Action", (uint8_t*)&eAction - pThis, sizeof(eAction), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "NoButton", (uint8_t*)&bNoButton - pThis, sizeof(bNoButton), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "IsAbility", (uint8_t*)&bIsAbility - pThis, sizeof(bIsAbility), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Autocast", (uint8_t*)&bAutocast - pThis, sizeof(bAutocast), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Passive", (uint8_t*)&bPassive - pThis, sizeof(bPassive), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Slot", (uint8_t*)&nSlot - pThis, sizeof(nSlot), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Panel", (uint8_t*)&ePanel - pThis, sizeof(ePanel), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "TargetPanel", (uint8_t*)&eTargetPanel - pThis, sizeof(eTargetPanel), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "TooltipFileRef", (uint8_t*)&szTooltipFileRef - pThis, sizeof(szTooltipFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "Icon", (uint8_t*)&pIcon - pThis, sizeof(pIcon), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "IconDisabled", (uint8_t*)&pIconDisabled - pThis, sizeof(pIconDisabled), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "ForegroundIcon", (uint8_t*)&pForegroundIcon - pThis, sizeof(pForegroundIcon), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "ForegroundIconDisabled", (uint8_t*)&pForegroundIconDisabled - pThis, sizeof(pForegroundIconDisabled), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "PressEffect", (uint8_t*)&bPressEffect - pThis, sizeof(bPressEffect), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "HotkeyCmd", (uint8_t*)&szHotkeyCmd - pThis, sizeof(szHotkeyCmd), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int SActionButtonInfo::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Action", &eAction );
	saver.Add( "NoButton", &bNoButton );
	saver.Add( "IsAbility", &bIsAbility );
	saver.Add( "Autocast", &bAutocast );
	saver.Add( "Passive", &bPassive );
	saver.Add( "Slot", &nSlot );
	saver.Add( "Panel", &ePanel );
	saver.Add( "TargetPanel", &eTargetPanel );
	saver.Add( "TooltipFileRef", &szTooltipFileRef );
	saver.Add( "Icon", &pIcon );
	saver.Add( "IconDisabled", &pIconDisabled );
	saver.Add( "ForegroundIcon", &pForegroundIcon );
	saver.Add( "ForegroundIconDisabled", &pForegroundIconDisabled );
	saver.Add( "PressEffect", &bPressEffect );
	saver.Add( "HotkeyCmd", &szHotkeyCmd );

	return 0;
}

int SActionButtonInfo::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eAction );
	saver.Add( 3, &bNoButton );
	saver.Add( 4, &bIsAbility );
	saver.Add( 5, &bAutocast );
	saver.Add( 6, &bPassive );
	saver.Add( 7, &nSlot );
	saver.Add( 8, &ePanel );
	saver.Add( 9, &eTargetPanel );
	saver.Add( 10, &szTooltipFileRef );
	saver.Add( 11, &pIcon );
	saver.Add( 12, &pIconDisabled );
	saver.Add( 13, &pForegroundIcon );
	saver.Add( 14, &pForegroundIconDisabled );
	saver.Add( 15, &bPressEffect );
	saver.Add( 16, &szHotkeyCmd );

	return 0;
}



void SPlayersColors::SPlayer::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Color", (uint8_t*)&nColor - pThis, sizeof(nColor), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( szAddName + "UnitFullInfo", (uint8_t*)&pUnitFullInfo - pThis, sizeof(pUnitFullInfo), NTypeDef::TYPE_TYPE_REF );
}

int SPlayersColors::SPlayer::operator&( IXmlSaver &saver )
{
	saver.Add( "Color", &nColor );
	saver.Add( "UnitFullInfo", &pUnitFullInfo );

	return 0;
}

int SPlayersColors::SPlayer::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nColor );
	saver.Add( 3, &pUnitFullInfo );

	return 0;
}

uint32_t SPlayersColors::SPlayer::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << nColor << pUnitFullInfo;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SPlayersColors::SUnitFullInfo::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "UserForward", (uint8_t*)&pUserForward - pThis, sizeof(pUserForward), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "NeutralForward", (uint8_t*)&pNeutralForward - pThis, sizeof(pNeutralForward), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "FriendForwards", &friendForwards, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "EnemyForwards", &enemyForwards, pThis );
}

int SPlayersColors::SUnitFullInfo::operator&( IXmlSaver &saver )
{
	saver.Add( "UserForward", &pUserForward );
	saver.Add( "NeutralForward", &pNeutralForward );
	saver.Add( "FriendForwards", &friendForwards );
	saver.Add( "EnemyForwards", &enemyForwards );

	return 0;
}

int SPlayersColors::SUnitFullInfo::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pUserForward );
	saver.Add( 3, &pNeutralForward );
	saver.Add( 4, &friendForwards );
	saver.Add( 5, &enemyForwards );

	return 0;
}

uint32_t SPlayersColors::SUnitFullInfo::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << pUserForward << pNeutralForward << friendForwards << enemyForwards;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SPlayersColors::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "UserColor", &vUserColor, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "NeutralColor", &vNeutralColor, pThis ); 
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "FriendColors", &friendColors, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "EnemyColors", &enemyColors, pThis );
	NMetaInfo::ReportStructMetaInfo( szAddName + "UnitFullInfo", &unitFullInfo, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "UserInfo", &userInfo, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "NeutralInfo", &neutralInfo, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "FriendInfo", &friendInfo, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "EnemyInfo", &enemyInfo, pThis ); 
}

int SPlayersColors::operator&( IXmlSaver &saver )
{
	saver.Add( "UserColor", &vUserColor );
	saver.Add( "NeutralColor", &vNeutralColor );
	saver.Add( "FriendColors", &friendColors );
	saver.Add( "EnemyColors", &enemyColors );
	saver.Add( "UnitFullInfo", &unitFullInfo );
	saver.Add( "UserInfo", &userInfo );
	saver.Add( "NeutralInfo", &neutralInfo );
	saver.Add( "FriendInfo", &friendInfo );
	saver.Add( "EnemyInfo", &enemyInfo );

	return 0;
}

int SPlayersColors::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vUserColor );
	saver.Add( 3, &vNeutralColor );
	saver.Add( 4, &friendColors );
	saver.Add( 5, &enemyColors );
	saver.Add( 6, &unitFullInfo );
	saver.Add( 7, &userInfo );
	saver.Add( 8, &neutralInfo );
	saver.Add( 9, &friendInfo );
	saver.Add( 10, &enemyInfo );

	return 0;
}

uint32_t SPlayersColors::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vUserColor << vNeutralColor << friendColors << enemyColors << unitFullInfo << userInfo << neutralInfo << friendInfo << enemyInfo;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SReinfButton::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Type", (uint8_t*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "Button", (uint8_t*)&pButton - pThis, sizeof(pButton), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "Texture", (uint8_t*)&pTexture - pThis, sizeof(pTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "TextureDisabled", (uint8_t*)&pTextureDisabled - pThis, sizeof(pTextureDisabled), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( szAddName + "DescFileRef", (uint8_t*)&szDescFileRef - pThis, sizeof(szDescFileRef), NTypeDef::TYPE_TYPE_STRING );
}

int SReinfButton::operator&( IXmlSaver &saver )
{
	saver.Add( "Type", &eType );
	saver.Add( "Button", &pButton );
	saver.Add( "Texture", &pTexture );
	saver.Add( "TextureDisabled", &pTextureDisabled );
	saver.Add( "DescFileRef", &szDescFileRef );

	return 0;
}

int SReinfButton::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eType );
	saver.Add( 3, &pButton );
	saver.Add( 4, &pTexture );
	saver.Add( 5, &pTextureDisabled );
	saver.Add( 6, &szDescFileRef );

	return 0;
}

uint32_t SReinfButton::CalcCheckSum() const
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



void SSeasonColor::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Season", (uint8_t*)&eSeason - pThis, sizeof(eSeason), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportStructMetaInfo( szAddName + "Color", &vColor, pThis ); 
}

int SSeasonColor::operator&( IXmlSaver &saver )
{
	saver.Add( "Season", &eSeason );
	saver.Add( "Color", &vColor );

	return 0;
}

int SSeasonColor::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eSeason );
	saver.Add( 3, &vColor );

	return 0;
}

uint32_t SSeasonColor::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eSeason << vColor;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMLTag::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Name", (uint8_t*)&szName - pThis, sizeof(szName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "TextFileRef", (uint8_t*)&szTextFileRef - pThis, sizeof(szTextFileRef), NTypeDef::TYPE_TYPE_STRING );
}

int SMLTag::operator&( IXmlSaver &saver )
{
	saver.Add( "Name", &szName );
	saver.Add( "TextFileRef", &szTextFileRef );

	return 0;
}

int SMLTag::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szName );
	saver.Add( 3, &szTextFileRef );

	return 0;
}

uint32_t SMLTag::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szName;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SMPLocalizedGameType::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "GameType", (uint8_t*)&eGameType - pThis, sizeof(eGameType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "LocalizedTextFileRef", (uint8_t*)&szLocalizedTextFileRef - pThis, sizeof(szLocalizedTextFileRef), NTypeDef::TYPE_TYPE_STRING );
}

int SMPLocalizedGameType::operator&( IXmlSaver &saver )
{
	saver.Add( "GameType", &eGameType );
	saver.Add( "LocalizedTextFileRef", &szLocalizedTextFileRef );

	return 0;
}

int SMPLocalizedGameType::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eGameType );
	saver.Add( 3, &szLocalizedTextFileRef );

	return 0;
}

uint32_t SMPLocalizedGameType::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eGameType;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SUIConstsB2::SSeasonName::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Season", (uint8_t*)&eSeason - pThis, sizeof(eSeason), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "NameFileRef", (uint8_t*)&szNameFileRef - pThis, sizeof(szNameFileRef), NTypeDef::TYPE_TYPE_STRING );
}

int SUIConstsB2::SSeasonName::operator&( IXmlSaver &saver )
{
	saver.Add( "Season", &eSeason );
	saver.Add( "NameFileRef", &szNameFileRef );

	return 0;
}

int SUIConstsB2::SSeasonName::operator&( IBinSaver &saver )
{
	saver.Add( 2, &eSeason );
	saver.Add( 3, &szNameFileRef );

	return 0;
}

uint32_t SUIConstsB2::SSeasonName::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << eSeason;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SUIConstsB2::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "UIConstsB2", typeID, sizeof(*this) );
	SUIGameConsts::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "ActionButtons", &actionButtons, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "M1ActionButtons", &m1ActionButtons, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "ActionButtonInfos", &actionButtonInfos, pThis );
	NMetaInfo::ReportStructMetaInfo( "PlayersColors", &playersColors, pThis ); 
	NMetaInfo::ReportStructArrayMetaInfo( "ReinfButtons", &reinfButtons, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "ChatSeasonColors", &chatSeasonColors, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "SeasonNames", &seasonNames, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "Tags", &tags, pThis );
	NMetaInfo::ReportStructArrayMetaInfo( "MPLocalizedGameTypes", &mPLocalizedGameTypes, pThis );
	NMetaInfo::ReportSimpleArrayMetaInfo( "ChapterMapArrows", &chapterMapArrows, pThis );
	NMetaInfo::ReportMetaInfo( "ForbiddenWordsFileRef", (uint8_t*)&szForbiddenWordsFileRef - pThis, sizeof(szForbiddenWordsFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int SUIConstsB2::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIGameConsts*)(this) );
	saver.Add( "ActionButtons", &actionButtons );
	saver.Add( "M1ActionButtons", &m1ActionButtons );
	saver.Add( "ActionButtonInfos", &actionButtonInfos );
	saver.Add( "PlayersColors", &playersColors );
	saver.Add( "ReinfButtons", &reinfButtons );
	saver.Add( "ChatSeasonColors", &chatSeasonColors );
	saver.Add( "SeasonNames", &seasonNames );
	saver.Add( "Tags", &tags );
	saver.Add( "MPLocalizedGameTypes", &mPLocalizedGameTypes );
	saver.Add( "ChapterMapArrows", &chapterMapArrows );
	saver.Add( "ForbiddenWordsFileRef", &szForbiddenWordsFileRef );

	return 0;
}

int SUIConstsB2::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIGameConsts*)this );
	saver.Add( 2, &actionButtons );
	saver.Add( 3, &m1ActionButtons );
	saver.Add( 4, &actionButtonInfos );
	saver.Add( 5, &playersColors );
	saver.Add( 6, &reinfButtons );
	saver.Add( 7, &chatSeasonColors );
	saver.Add( 8, &seasonNames );
	saver.Add( 9, &tags );
	saver.Add( 10, &mPLocalizedGameTypes );
	saver.Add( 11, &chapterMapArrows );
	saver.Add( 12, &szForbiddenWordsFileRef );

	return 0;
}



void SWindowMiniMapShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowMiniMapShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructMetaInfo( "Point00", &vPoint00, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "Point01", &vPoint01, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "Point10", &vPoint10, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "Point11", &vPoint11, pThis ); 
	NMetaInfo::ReportStructArrayMetaInfo( "PlayerColors", &playerColors, pThis );
	NMetaInfo::ReportStructMetaInfo( "ViewportFrameColor", &vViewportFrameColor, pThis ); 
	NMetaInfo::ReportMetaInfo( "Rotable", (uint8_t*)&bRotable - pThis, sizeof(bRotable), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "RotableBackgroundTexture", (uint8_t*)&pRotableBackgroundTexture - pThis, sizeof(pRotableBackgroundTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "RotableForegroundTexture", (uint8_t*)&pRotableForegroundTexture - pThis, sizeof(pRotableForegroundTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "RotableBackgroundSize", &vRotableBackgroundSize, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "RotableSize", &vRotableSize, pThis ); 
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowMiniMapShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );
	saver.Add( "Point00", &vPoint00 );
	saver.Add( "Point01", &vPoint01 );
	saver.Add( "Point10", &vPoint10 );
	saver.Add( "Point11", &vPoint11 );
	saver.Add( "PlayerColors", &playerColors );
	saver.Add( "ViewportFrameColor", &vViewportFrameColor );
	saver.Add( "Rotable", &bRotable );
	saver.Add( "RotableBackgroundTexture", &pRotableBackgroundTexture );
	saver.Add( "RotableForegroundTexture", &pRotableForegroundTexture );
	saver.Add( "RotableBackgroundSize", &vRotableBackgroundSize );
	saver.Add( "RotableSize", &vRotableSize );

	return 0;
}

int SWindowMiniMapShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );
	saver.Add( 2, &vPoint00 );
	saver.Add( 3, &vPoint01 );
	saver.Add( 4, &vPoint10 );
	saver.Add( 5, &vPoint11 );
	saver.Add( 6, &playerColors );
	saver.Add( 7, &vViewportFrameColor );
	saver.Add( 8, &bRotable );
	saver.Add( 9, &pRotableBackgroundTexture );
	saver.Add( 10, &pRotableForegroundTexture );
	saver.Add( 11, &vRotableBackgroundSize );
	saver.Add( 12, &vRotableSize );

	return 0;
}



void SWindowMiniMap::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowMiniMap", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowMiniMap::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );

	return 0;
}

int SWindowMiniMap::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );

	return 0;
}



void SWindowSelectionShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowSelectionShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowSelectionShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );

	return 0;
}

int SWindowSelectionShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );

	return 0;
}



void SWindowSelection::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowSelection", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "SelectorTexture", (uint8_t*)&pSelectorTexture - pThis, sizeof(pSelectorTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowSelection::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );
	saver.Add( "SelectorTexture", &pSelectorTexture );

	return 0;
}

int SWindowSelection::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );
	saver.Add( 2, &pSelectorTexture );

	return 0;
}



void SWindowRoundProgressBarShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowRoundProgressBarShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Texture", (uint8_t*)&pTexture - pThis, sizeof(pTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "Color", (uint8_t*)&nColor - pThis, sizeof(nColor), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowRoundProgressBarShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );
	saver.Add( "Texture", &pTexture );
	saver.Add( "Color", &nColor );

	return 0;
}

int SWindowRoundProgressBarShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );
	saver.Add( 2, &pTexture );
	saver.Add( 3, &nColor );

	return 0;
}



void SWindowRoundProgressBar::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowRoundProgressBar", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowRoundProgressBar::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );

	return 0;
}

int SWindowRoundProgressBar::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );

	return 0;
}



void SWindow3DControlShared::SObjectParams::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Pos", &vPos, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "Size", &vSize, pThis ); 
}

int SWindow3DControlShared::SObjectParams::operator&( IXmlSaver &saver )
{
	saver.Add( "Pos", &vPos );
	saver.Add( "Size", &vSize );

	return 0;
}

int SWindow3DControlShared::SObjectParams::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vPos );
	saver.Add( 3, &vSize );

	return 0;
}

uint32_t SWindow3DControlShared::SObjectParams::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vPos << vSize;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SWindow3DControlShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Window3DControlShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "Places", &places, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindow3DControlShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );
	saver.Add( "Places", &places );

	return 0;
}

int SWindow3DControlShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );
	saver.Add( 2, &places );

	return 0;
}



void SWindow3DControl::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "Window3DControl", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SWindow3DControl::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );

	return 0;
}

int SWindow3DControl::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );

	return 0;
}



void SWindowFrameSequenceShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowFrameSequenceShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Texture", (uint8_t*)&pTexture - pThis, sizeof(pTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "FrameSize", &vFrameSize, pThis ); 
	NMetaInfo::ReportMetaInfo( "FrameCountX", (uint8_t*)&nFrameCountX - pThis, sizeof(nFrameCountX), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "FrameCountY", (uint8_t*)&nFrameCountY - pThis, sizeof(nFrameCountY), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Time", (uint8_t*)&nTime - pThis, sizeof(nTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "FrameCount", (uint8_t*)&nFrameCount - pThis, sizeof(nFrameCount), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "RandomStartFrame", (uint8_t*)&bRandomStartFrame - pThis, sizeof(bRandomStartFrame), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "RandomAddTime", (uint8_t*)&nRandomAddTime - pThis, sizeof(nRandomAddTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowFrameSequenceShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );
	saver.Add( "Texture", &pTexture );
	saver.Add( "FrameSize", &vFrameSize );
	saver.Add( "FrameCountX", &nFrameCountX );
	saver.Add( "FrameCountY", &nFrameCountY );
	saver.Add( "Time", &nTime );
	saver.Add( "FrameCount", &nFrameCount );
	saver.Add( "RandomStartFrame", &bRandomStartFrame );
	saver.Add( "RandomAddTime", &nRandomAddTime );

	return 0;
}

int SWindowFrameSequenceShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );
	saver.Add( 2, &pTexture );
	saver.Add( 3, &vFrameSize );
	saver.Add( 4, &nFrameCountX );
	saver.Add( 5, &nFrameCountY );
	saver.Add( 6, &nTime );
	saver.Add( 7, &nFrameCount );
	saver.Add( 8, &bRandomStartFrame );
	saver.Add( 9, &nRandomAddTime );

	return 0;
}



void SWindowFrameSequence::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowFrameSequence", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowFrameSequence::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );

	return 0;
}

int SWindowFrameSequence::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );

	return 0;
}



void SUISPlaySound::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "UISPlaySound", typeID, sizeof(*this) );
	SUIStateBase::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructMetaInfo( "pSoundToPlay", &pSoundToPlay, pThis ); 
	NMetaInfo::FinishMetaInfoReport();
}

int SUISPlaySound::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIStateBase*)(this) );
	saver.Add( "pSoundToPlay", &pSoundToPlay );

	return 0;
}

int SUISPlaySound::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIStateBase*)this );
	saver.Add( 2, &pSoundToPlay );

	return 0;
}



void SUISB2MoveShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "UISB2MoveShared", typeID, sizeof(*this) );
	SUIStateBaseShared::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructMetaInfo( "vOffset", &vOffset, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "vAccel", &vAccel, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "fMoveTime", &fMoveTime, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "vOffset2", &vOffset2, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "vAccel2", &vAccel2, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "fMoveTime2", &fMoveTime2, pThis ); 
	NMetaInfo::FinishMetaInfoReport();
}

int SUISB2MoveShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIStateBaseShared*)(this) );
	saver.Add( "vOffset", &vOffset );
	saver.Add( "vAccel", &vAccel );
	saver.Add( "fMoveTime", &fMoveTime );
	saver.Add( "vOffset2", &vOffset2 );
	saver.Add( "vAccel2", &vAccel2 );
	saver.Add( "fMoveTime2", &fMoveTime2 );

	return 0;
}

int SUISB2MoveShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIStateBaseShared*)this );
	saver.Add( 2, &vOffset );
	saver.Add( 3, &vAccel );
	saver.Add( 4, &fMoveTime );
	saver.Add( 5, &vOffset2 );
	saver.Add( 6, &vAccel2 );
	saver.Add( 7, &fMoveTime2 );

	return 0;
}



void SUISB2Move::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "UISB2Move", typeID, sizeof(*this) );
	SUIStateBase::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructMetaInfo( "Offset", &vOffset, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "AccelCoeff", &vAccelCoeff, pThis ); 
	NMetaInfo::ReportMetaInfo( "MoveTime", (uint8_t*)&fMoveTime - pThis, sizeof(fMoveTime), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportStructMetaInfo( "OffsetBounce", &vOffsetBounce, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "AccelCoeffBounce", &vAccelCoeffBounce, pThis ); 
	NMetaInfo::ReportMetaInfo( "MoveTimeBounce", (uint8_t*)&fMoveTimeBounce - pThis, sizeof(fMoveTimeBounce), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "ElementToMove", (uint8_t*)&szElementToMove - pThis, sizeof(szElementToMove), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( "Border", (uint8_t*)&bBorder - pThis, sizeof(bBorder), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "MaxMoveTime", (uint8_t*)&fMaxMoveTime - pThis, sizeof(fMaxMoveTime), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SUISB2Move::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SUIStateBase*)(this) );
	saver.Add( "Offset", &vOffset );
	saver.Add( "AccelCoeff", &vAccelCoeff );
	saver.Add( "MoveTime", &fMoveTime );
	saver.Add( "OffsetBounce", &vOffsetBounce );
	saver.Add( "AccelCoeffBounce", &vAccelCoeffBounce );
	saver.Add( "MoveTimeBounce", &fMoveTimeBounce );
	saver.Add( "ElementToMove", &szElementToMove );
	saver.Add( "Border", &bBorder );
	saver.Add( "MaxMoveTime", &fMaxMoveTime );

	return 0;
}

int SUISB2Move::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SUIStateBase*)this );
	saver.Add( 2, &vOffset );
	saver.Add( 3, &vAccelCoeff );
	saver.Add( 4, &fMoveTime );
	saver.Add( 5, &vOffsetBounce );
	saver.Add( 6, &vAccelCoeffBounce );
	saver.Add( 7, &fMoveTimeBounce );
	saver.Add( 8, &szElementToMove );
	saver.Add( 9, &bBorder );
	saver.Add( 10, &fMaxMoveTime );

	return 0;
}



void SWindowPotentialLinesShared::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowPotentialLinesShared", typeID, sizeof(*this) );
	SWindowShared::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Colour", (uint8_t*)&nColour - pThis, sizeof(nColour), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowPotentialLinesShared::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindowShared*)(this) );
	saver.Add( "Colour", &nColour );

	return 0;
}

int SWindowPotentialLinesShared::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindowShared*)this );
	saver.Add( 2, &nColour );

	return 0;
}



void SWindowPotentialLines::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "WindowPotentialLines", typeID, sizeof(*this) );
	SWindow::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::FinishMetaInfoReport();
}

int SWindowPotentialLines::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SWindow*)(this) );

	return 0;
}

int SWindowPotentialLines::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SWindow*)this );

	return 0;
}



void SBackgroundFrameSequence::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "BackgroundFrameSequence", typeID, sizeof(*this) );
	SBackground::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "SequenceTexture", (uint8_t*)&pSequenceTexture - pThis, sizeof(pSequenceTexture), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "FrameSize", &vFrameSize, pThis ); 
	NMetaInfo::ReportMetaInfo( "FrameCountX", (uint8_t*)&nFrameCountX - pThis, sizeof(nFrameCountX), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "FrameCountY", (uint8_t*)&nFrameCountY - pThis, sizeof(nFrameCountY), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Time", (uint8_t*)&nTime - pThis, sizeof(nTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "FrameCount", (uint8_t*)&nFrameCount - pThis, sizeof(nFrameCount), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "RandomStartFrame", (uint8_t*)&bRandomStartFrame - pThis, sizeof(bRandomStartFrame), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "RandomAddTime", (uint8_t*)&nRandomAddTime - pThis, sizeof(nRandomAddTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SBackgroundFrameSequence::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SBackground*)(this) );
	saver.Add( "SequenceTexture", &pSequenceTexture );
	saver.Add( "FrameSize", &vFrameSize );
	saver.Add( "FrameCountX", &nFrameCountX );
	saver.Add( "FrameCountY", &nFrameCountY );
	saver.Add( "Time", &nTime );
	saver.Add( "FrameCount", &nFrameCount );
	saver.Add( "RandomStartFrame", &bRandomStartFrame );
	saver.Add( "RandomAddTime", &nRandomAddTime );

	return 0;
}

int SBackgroundFrameSequence::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SBackground*)this );
	saver.Add( 2, &pSequenceTexture );
	saver.Add( 3, &vFrameSize );
	saver.Add( 4, &nFrameCountX );
	saver.Add( 5, &nFrameCountY );
	saver.Add( 6, &nTime );
	saver.Add( 7, &nFrameCount );
	saver.Add( 8, &bRandomStartFrame );
	saver.Add( 9, &nRandomAddTime );

	return 0;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( UISPECIFICB2, 0x1508D300, SARSetSpecialAbility )
REGISTER_DATABASE_CLASS( UISPECIFICB2, 0x1508A340, SARSetForcedAction )
REGISTER_DATABASE_CLASS( UISPECIFICB2, 0x1717BAC0, SActionButtonInfo )
REGISTER_DATABASE_CLASS( UISPECIFICB2, 0x1109C340, SUIConstsB2 )
REGISTER_DATABASE_CLASS( UISPECIFICB2, 0x1508E480, SWindowMiniMapShared )
REGISTER_DATABASE_CLASS( UISPECIFICB2, 0x1508E481, SWindowMiniMap )
REGISTER_DATABASE_CLASS( UISPECIFICB2, 0x110BD482, SWindowSelectionShared )
REGISTER_DATABASE_CLASS( UISPECIFICB2, 0x110BD480, SWindowSelection )
REGISTER_DATABASE_CLASS( UISPECIFICB2, 0x171713C0, SWindowRoundProgressBarShared )
REGISTER_DATABASE_CLASS( UISPECIFICB2, 0x171713C1, SWindowRoundProgressBar )
REGISTER_DATABASE_CLASS( UISPECIFICB2, 0x17176480, SWindow3DControlShared )
REGISTER_DATABASE_CLASS( UISPECIFICB2, 0x17176400, SWindow3DControl )
REGISTER_DATABASE_CLASS( UISPECIFICB2, 0x1717A440, SWindowFrameSequenceShared )
REGISTER_DATABASE_CLASS( UISPECIFICB2, 0x1717A441, SWindowFrameSequence )
REGISTER_DATABASE_CLASS( UISPECIFICB2, 0x11075C03, SUISPlaySound )
REGISTER_DATABASE_CLASS( UISPECIFICB2, 0x171B1C40, SUISB2MoveShared )
REGISTER_DATABASE_CLASS( UISPECIFICB2, 0x171B1C41, SUISB2Move )
REGISTER_DATABASE_CLASS( UISPECIFICB2, 0x191B53C0, SWindowPotentialLinesShared )
REGISTER_DATABASE_CLASS( UISPECIFICB2, 0x191B53C1, SWindowPotentialLines )
REGISTER_DATABASE_CLASS( UISPECIFICB2, 0x171C1B81, SBackgroundFrameSequence )

