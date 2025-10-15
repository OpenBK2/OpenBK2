// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "dbuiconsts.h"

#include "UI_export.h"

#include <cstdint>

namespace NDb
{



void STooltipContext::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "TooltipContext", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Window", (uint8_t*)&pWindow - pThis, sizeof(pWindow), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportSimpleArrayMetaInfo( "AppearCommands", &appearCommands, pThis );
	NMetaInfo::ReportMetaInfo( "MouseMaxOffsetToAppear", (uint8_t*)&nMouseMaxOffsetToAppear - pThis, sizeof(nMouseMaxOffsetToAppear), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "AppearDelay", (uint8_t*)&nAppearDelay - pThis, sizeof(nAppearDelay), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "SingleLineWidth", (uint8_t*)&nSingleLineWidth - pThis, sizeof(nSingleLineWidth), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "HorisontalToVerticalRatio", (uint8_t*)&fHorisontalToVerticalRatio - pThis, sizeof(fHorisontalToVerticalRatio), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int STooltipContext::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "Window", &pWindow );
	saver.Add( "AppearCommands", &appearCommands );
	saver.Add( "MouseMaxOffsetToAppear", &nMouseMaxOffsetToAppear );
	saver.Add( "AppearDelay", &nAppearDelay );
	saver.Add( "SingleLineWidth", &nSingleLineWidth );
	saver.Add( "HorisontalToVerticalRatio", &fHorisontalToVerticalRatio );

	return 0;
}

int STooltipContext::operator&( IBinSaver &saver )
{
	saver.Add( 2, &pWindow );
	saver.Add( 3, &appearCommands );
	saver.Add( 4, &nMouseMaxOffsetToAppear );
	saver.Add( 5, &nAppearDelay );
	saver.Add( 6, &nSingleLineWidth );
	saver.Add( 7, &fHorisontalToVerticalRatio );

	return 0;
}



void SUIGameConsts::ReportMetaInfo() const
{
	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportSimpleArrayMetaInfo( "Contexts", &contexts, pThis );
	NMetaInfo::ReportMetaInfo( "Console", (uint8_t*)&pConsole - pThis, sizeof(pConsole), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "DebugInfo", (uint8_t*)&pDebugInfo - pThis, sizeof(pDebugInfo), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportMetaInfo( "StatsWindow", (uint8_t*)&pStatsWindow - pThis, sizeof(pStatsWindow), NTypeDef::TYPE_TYPE_REF );
	NMetaInfo::ReportStructMetaInfo( "ButtonClickSound", &buttonClickSound, pThis ); 
}

int SUIGameConsts::operator&( IXmlSaver &saver )
{
	saver.Add( "Contexts", &contexts );
	saver.Add( "Console", &pConsole );
	saver.Add( "DebugInfo", &pDebugInfo );
	saver.Add( "StatsWindow", &pStatsWindow );
	saver.Add( "ButtonClickSound", &buttonClickSound );

	return 0;
}

int SUIGameConsts::operator&( IBinSaver &saver )
{
	saver.Add( 2, &contexts );
	saver.Add( 3, &pConsole );
	saver.Add( 4, &pDebugInfo );
	saver.Add( 5, &pStatsWindow );
	saver.Add( 6, &buttonClickSound );

	return 0;
}

uint32_t SUIGameConsts::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << contexts << buttonClickSound;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( UI, 0x15087B80, STooltipContext )
BASIC_REGISTER_DATABASE_CLASS( UI, SUIGameConsts )

