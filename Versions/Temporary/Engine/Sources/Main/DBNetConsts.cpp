// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "DBNetConsts.h"

#include "Main_export.h"

#include <cstdint>

namespace NDb
{



void SNetGameConsts::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "NetGameConsts", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "MaxLatency", (uint8_t*)&nMaxLatency - pThis, sizeof(nMaxLatency), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "TimeToStartLagByNoSegmentData", (uint8_t*)&nTimeToStartLagByNoSegmentData - pThis, sizeof(nTimeToStartLagByNoSegmentData), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "TimeToAllowDropByLag", (uint8_t*)&nTimeToAllowDropByLag - pThis, sizeof(nTimeToAllowDropByLag), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "TimeOutTime", (uint8_t*)&nTimeOutTime - pThis, sizeof(nTimeOutTime), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "TimeBWTimeOuts", (uint8_t*)&nTimeBWTimeOuts - pThis, sizeof(nTimeBWTimeOuts), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Port", (uint8_t*)&nPort - pThis, sizeof(nPort), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::FinishMetaInfoReport();
}

int SNetGameConsts::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "MaxLatency", &nMaxLatency );
	saver.Add( "TimeToStartLagByNoSegmentData", &nTimeToStartLagByNoSegmentData );
	saver.Add( "TimeToAllowDropByLag", &nTimeToAllowDropByLag );
	saver.Add( "TimeOutTime", &nTimeOutTime );
	saver.Add( "TimeBWTimeOuts", &nTimeBWTimeOuts );
	saver.Add( "Port", &nPort );

	return 0;
}

int SNetGameConsts::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nMaxLatency );
	saver.Add( 3, &nTimeToStartLagByNoSegmentData );
	saver.Add( 4, &nTimeToAllowDropByLag );
	saver.Add( 5, &nTimeOutTime );
	saver.Add( 6, &nTimeBWTimeOuts );
	saver.Add( 7, &nPort );

	return 0;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( MAIN, 0x300A7B40, SNetGameConsts )

