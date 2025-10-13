// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "dbsounddesc.h"

#include <cstdint>

namespace NDb
{



void SSoundDesc::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "SoundDesc", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "SoundPath", (uint8_t*)&szSoundPath - pThis, sizeof(szSoundPath), NTypeDef::TYPE_TYPE_STRING );
	NMetaInfo::FinishMetaInfoReport();
}

int SSoundDesc::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "SoundPath", &szSoundPath );

	return 0;
}

int SSoundDesc::operator&( IBinSaver &saver )
{
	saver.Add( 2, &szSoundPath );

	return 0;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( 0x1107BAC0, SSoundDesc ) 

