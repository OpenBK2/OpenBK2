// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "dbprelight.h"

#include "B2_M1_Terrain_export.h"

#include <cstdint>

namespace NDb
{



void SPreLight::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "PreLight", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructMetaInfo( "LightColor", &vLightColor, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "AmbientColor", &vAmbientColor, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "ShadeColor", &vShadeColor, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "ShadeAmbientColor", &vShadeAmbientColor, pThis ); 
	NMetaInfo::ReportMetaInfo( "Whitening", (uint8_t*)&bWhitening - pThis, sizeof(bWhitening), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Pitch", (uint8_t*)&fPitch - pThis, sizeof(fPitch), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SPreLight::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "LightColor", &vLightColor );
	saver.Add( "AmbientColor", &vAmbientColor );
	saver.Add( "ShadeColor", &vShadeColor );
	saver.Add( "ShadeAmbientColor", &vShadeAmbientColor );
	saver.Add( "Whitening", &bWhitening );
	saver.Add( "Pitch", &fPitch );

	return 0;
}

int SPreLight::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vLightColor );
	saver.Add( 3, &vAmbientColor );
	saver.Add( 4, &vShadeColor );
	saver.Add( 5, &vShadeAmbientColor );
	saver.Add( 6, &bWhitening );
	saver.Add( 7, &fPitch );

	return 0;
}



void STwoSidedLight::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "TwoSidedLight", typeID, sizeof(*this) );

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportStructMetaInfo( "LightColor", &vLightColor, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "AmbientColor", &vAmbientColor, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "ShadeColor", &vShadeColor, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "ShadeAmbientColor", &vShadeAmbientColor, pThis ); 
	NMetaInfo::ReportMetaInfo( "Whitening", (uint8_t*)&bWhitening - pThis, sizeof(bWhitening), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "Pitch", (uint8_t*)&fPitch - pThis, sizeof(fPitch), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( "Yaw", (uint8_t*)&fYaw - pThis, sizeof(fYaw), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int STwoSidedLight::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "LightColor", &vLightColor );
	saver.Add( "AmbientColor", &vAmbientColor );
	saver.Add( "ShadeColor", &vShadeColor );
	saver.Add( "ShadeAmbientColor", &vShadeAmbientColor );
	saver.Add( "Whitening", &bWhitening );
	saver.Add( "Pitch", &fPitch );
	saver.Add( "Yaw", &fYaw );

	return 0;
}

int STwoSidedLight::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vLightColor );
	saver.Add( 3, &vAmbientColor );
	saver.Add( 4, &vShadeColor );
	saver.Add( 5, &vShadeAmbientColor );
	saver.Add( 6, &bWhitening );
	saver.Add( 7, &fPitch );
	saver.Add( 8, &fYaw );

	return 0;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( B2_M1_TERRAIN, 0x10082C80, SPreLight )
REGISTER_DATABASE_CLASS( B2_M1_TERRAIN, 0x10087440, STwoSidedLight )

