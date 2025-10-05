// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "dbcameraconsts.h"

namespace NDb
{



void SCameraLimits::SCLLimit::ReportMetaInfo( const std::string &szAddName, BYTE *pThis ) const
{
	NMetaInfo::ReportMetaInfo( szAddName + "Min", (BYTE*)&fMin - pThis, sizeof(fMin), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Max", (BYTE*)&fMax - pThis, sizeof(fMax), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Ave", (BYTE*)&fAve - pThis, sizeof(fAve), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "AutoSpeed", (BYTE*)&fAutoSpeed - pThis, sizeof(fAutoSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "ManualSpeed", (BYTE*)&fManualSpeed - pThis, sizeof(fManualSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::ReportMetaInfo( szAddName + "Cyclic", (BYTE*)&bCyclic - pThis, sizeof(bCyclic), NTypeDef::TYPE_TYPE_BOOL );
}

int SCameraLimits::SCLLimit::operator&( IXmlSaver &saver )
{
	saver.Add( "Min", &fMin );
	saver.Add( "Max", &fMax );
	saver.Add( "Ave", &fAve );
	saver.Add( "AutoSpeed", &fAutoSpeed );
	saver.Add( "ManualSpeed", &fManualSpeed );
	saver.Add( "Cyclic", &bCyclic );

	return 0;
}

int SCameraLimits::SCLLimit::operator&( IBinSaver &saver )
{
	saver.Add( 2, &fMin );
	saver.Add( 3, &fMax );
	saver.Add( 4, &fAve );
	saver.Add( 5, &fAutoSpeed );
	saver.Add( 6, &fManualSpeed );
	saver.Add( 7, &bCyclic );

	return 0;
}

DWORD SCameraLimits::SCLLimit::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << fMin << fMax << fAve << fAutoSpeed << fManualSpeed << bCyclic;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SCameraLimits::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "CameraLimits", typeID, sizeof(*this) );

	BYTE *pThis = (BYTE*)this;
	NMetaInfo::ReportStructMetaInfo( "DistanceLimit", &distanceLimit, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "PitchLimit", &pitchLimit, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "YawLimit", &yawLimit, pThis ); 
	NMetaInfo::ReportMetaInfo( "FOV", (BYTE*)&fFOV - pThis, sizeof(fFOV), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SCameraLimits::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.Add( "DistanceLimit", &distanceLimit );
	saver.Add( "PitchLimit", &pitchLimit );
	saver.Add( "YawLimit", &yawLimit );
	saver.Add( "FOV", &fFOV );

	return 0;
}

int SCameraLimits::operator&( IBinSaver &saver )
{
	saver.Add( 2, &distanceLimit );
	saver.Add( 3, &pitchLimit );
	saver.Add( 4, &yawLimit );
	saver.Add( 5, &fFOV );

	return 0;
}

DWORD SCameraLimits::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << distanceLimit << pitchLimit << yawLimit << fFOV;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( 0x1007B4C0, SCameraLimits ) 

