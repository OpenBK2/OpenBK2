// automatically generated file, don't change manually!

#include "stdafx.h"
#include "../libdb/ReportMetaInfo.h"
#include "../libdb/Checksum.h"
#include "../System/XmlSaver.h"
#include "dbgameoptions.h"

namespace NDb
{



void SOptionSystem::SOptionsCategory::SOptionEntry::SOptionEntryState::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "NameFileRef", (BYTE*)&szNameFileRef - pThis, sizeof(szNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "TooltipFileRef", (BYTE*)&szTooltipFileRef - pThis, sizeof(szTooltipFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "Value", (BYTE*)&szValue - pThis, sizeof(szValue), NTypeDef::TYPE_TYPE_STRING );
}

int SOptionSystem::SOptionsCategory::SOptionEntry::SOptionEntryState::operator&( IXmlSaver &saver )
{
	saver.Add( "NameFileRef", &szNameFileRef );
	saver.Add( "TooltipFileRef", &szTooltipFileRef );
	saver.Add( "Value", &szValue );

	return 0;
}

int SOptionSystem::SOptionsCategory::SOptionEntry::SOptionEntryState::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szNameFileRef );
	saver.Add( 3, &szTooltipFileRef );
	saver.Add( 4, &szValue );

	return 0;
}

DWORD SOptionSystem::SOptionsCategory::SOptionEntry::SOptionEntryState::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szValue;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}


string EnumToString( NDb::SOptionSystem::SOptionsCategory::SOptionEntry::EOptionEditorType eValue )
{
	switch ( eValue )
	{
	case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_EDITLINE:
		return "OPTION_EDITOR_EDITLINE";
	case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_CHECKBOX:
		return "OPTION_EDITOR_CHECKBOX";
	case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_SLIDER:
		return "OPTION_EDITOR_SLIDER";
	case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_DROPLIST:
		return "OPTION_EDITOR_DROPLIST";
	case NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_EDITNUMBER:
		return "OPTION_EDITOR_EDITNUMBER";
	default:
		return "OPTION_EDITOR_EDITLINE";
	}
}

NDb::SOptionSystem::SOptionsCategory::SOptionEntry::EOptionEditorType NDb::StringToEnum_NDb_SOptionSystem_SOptionsCategory_SOptionEntry_EOptionEditorType( const string &szValue )
{
	if ( szValue == "OPTION_EDITOR_EDITLINE" )
		return NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_EDITLINE;
	if ( szValue == "OPTION_EDITOR_CHECKBOX" )
		return NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_CHECKBOX;
	if ( szValue == "OPTION_EDITOR_SLIDER" )
		return NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_SLIDER;
	if ( szValue == "OPTION_EDITOR_DROPLIST" )
		return NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_DROPLIST;
	if ( szValue == "OPTION_EDITOR_EDITNUMBER" )
		return NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_EDITNUMBER;
	return NDb::SOptionSystem::SOptionsCategory::SOptionEntry::OPTION_EDITOR_EDITLINE;
}


void SOptionSystem::SOptionsCategory::SOptionEntry::SSliderSingleValue::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "ProgName", (BYTE*)&szProgName - pThis, sizeof(szProgName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportSimpleArrayMetaInfo( szAddName + "Values", &values, pThis );
}

int SOptionSystem::SOptionsCategory::SOptionEntry::SSliderSingleValue::operator&( IXmlSaver &saver )
{
	saver.Add( "ProgName", &szProgName );
	saver.Add( "Values", &values );

	return 0;
}

int SOptionSystem::SOptionsCategory::SOptionEntry::SSliderSingleValue::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szProgName );
	saver.Add( 3, &values );

	return 0;
}

DWORD SOptionSystem::SOptionsCategory::SOptionEntry::SSliderSingleValue::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szProgName << values;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SOptionSystem::SOptionsCategory::SOptionEntry::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "ProgName", (BYTE*)&szProgName - pThis, sizeof(szProgName), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "NameFileRef", (BYTE*)&szNameFileRef - pThis, sizeof(szNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "TooltipFileRef", (BYTE*)&szTooltipFileRef - pThis, sizeof(szTooltipFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "States", &states, pThis );
	NMetaInfo::ReportMetaInfo( szAddName + "EditorType", (BYTE*)&eEditorType - pThis, sizeof(eEditorType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( szAddName + "DefaultValue", (BYTE*)&szDefaultValue - pThis, sizeof(szDefaultValue), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "ModeFlags", (BYTE*)&nModeFlags - pThis, sizeof(nModeFlags), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "SliderValues", &sliderValues, pThis );
}

int SOptionSystem::SOptionsCategory::SOptionEntry::operator&( IXmlSaver &saver )
{
	saver.Add( "ProgName", &szProgName );
	saver.Add( "NameFileRef", &szNameFileRef );
	saver.Add( "TooltipFileRef", &szTooltipFileRef );
	saver.Add( "States", &states );
	saver.Add( "EditorType", &eEditorType );
	saver.Add( "DefaultValue", &szDefaultValue );
	saver.Add( "ModeFlags", &nModeFlags );
	saver.Add( "SliderValues", &sliderValues );

	return 0;
}

int SOptionSystem::SOptionsCategory::SOptionEntry::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szProgName );
	saver.Add( 3, &szNameFileRef );
	saver.Add( 4, &szTooltipFileRef );
	saver.Add( 5, &states );
	saver.Add( 6, &eEditorType );
	saver.Add( 7, &szDefaultValue );
	saver.Add( 8, &nModeFlags );
	saver.Add( 9, &sliderValues );

	return 0;
}

DWORD SOptionSystem::SOptionsCategory::SOptionEntry::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << szProgName << states << eEditorType << szDefaultValue << nModeFlags << sliderValues;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SOptionSystem::SOptionsCategory::ReportMetaInfo( const string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "NameFileRef", (BYTE*)&szNameFileRef - pThis, sizeof(szNameFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportMetaInfo( szAddName + "TooltipFileRef", (BYTE*)&szTooltipFileRef - pThis, sizeof(szTooltipFileRef), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::ReportStructArrayMetaInfo( szAddName + "Options", &options, pThis );
}

int SOptionSystem::SOptionsCategory::operator&( IXmlSaver &saver )
{
	saver.Add( "NameFileRef", &szNameFileRef );
	saver.Add( "TooltipFileRef", &szTooltipFileRef );
	saver.Add( "Options", &options );

	return 0;
}

int SOptionSystem::SOptionsCategory::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szNameFileRef );
	saver.Add( 3, &szTooltipFileRef );
	saver.Add( 4, &options );

	return 0;
}

DWORD SOptionSystem::SOptionsCategory::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << options;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SOptionSystem::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "OptionSystem", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructArrayMetaInfo( "Categories", &categories, pThis );
	NMetaInfo::FinishMetaInfoReport();
}

int SOptionSystem::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Categories", &categories );

	return 0;
}

int SOptionSystem::operator&( IBinSaver &saver )
{
	saver.Add( 2, &categories );

	return 0;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( 0x100CCC01, SOptionSystem ) 
