// automatically generated file, don't change manually!

#include "stdafx.h"
#include "libdb/ReportMetaInfo.h"
#include "libdb/Checksum.h"
#include "System/XmlSaver.h"
#include "DBAnimB2.h"

#include "Stats_B2_M1_export.h"

#include <cstdint>

namespace NDb
{



void SAnimAABB::ReportMetaInfo( const std::string &szAddName, uint8_t *pThis ) const
{
	NMetaInfo::ReportStructMetaInfo( szAddName + "Center", &vCenter, pThis ); 
	NMetaInfo::ReportStructMetaInfo( szAddName + "HalfSize", &vHalfSize, pThis ); 
}

int SAnimAABB::operator&( IXmlSaver &saver )
{
	saver.Add( "Center", &vCenter );
	saver.Add( "HalfSize", &vHalfSize );

	return 0;
}

int SAnimAABB::operator&( IBinSaver &saver )
{
	saver.Add( 2, &vCenter );
	saver.Add( 3, &vHalfSize );

	return 0;
}

uint32_t SAnimAABB::CalcCheckSum() const
{
	if ( __dwCheckSum != 0 )
		return __dwCheckSum;
	__dwCheckSum = 1;

	CCheckSum checkSum;
	checkSum << vCenter << vHalfSize;
	__dwCheckSum = checkSum.GetCheckSum();
	if ( __dwCheckSum == 0 )
		__dwCheckSum = 1;

	return __dwCheckSum;
}



void SAnimB2::ReportMetaInfo() const
{
	NMetaInfo::StartMetaInfoReport( "AnimB2", typeID, sizeof(*this) );
	SAnimBase::ReportMetaInfo();

	uint8_t *pThis = (uint8_t*)this;
	NMetaInfo::ReportMetaInfo( "Type", (uint8_t*)&eType - pThis, sizeof(eType), NTypeDef::TYPE_TYPE_ENUM );
	NMetaInfo::ReportMetaInfo( "Action", (uint8_t*)&nAction - pThis, sizeof(nAction), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Length", (uint8_t*)&nLength - pThis, sizeof(nLength), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportMetaInfo( "Looped", (uint8_t*)&bLooped - pThis, sizeof(bLooped), NTypeDef::TYPE_TYPE_BOOL );
	NMetaInfo::ReportMetaInfo( "WeaponsToUseWith", (uint8_t*)&nWeaponsToUseWith - pThis, sizeof(nWeaponsToUseWith), NTypeDef::TYPE_TYPE_INT );
	NMetaInfo::ReportStructMetaInfo( "aabb_a", &aabb_a, pThis ); 
	NMetaInfo::ReportStructMetaInfo( "aabb_d", &aabb_d, pThis ); 
	NMetaInfo::ReportMetaInfo( "MoveSpeed", (uint8_t*)&fMoveSpeed - pThis, sizeof(fMoveSpeed), NTypeDef::TYPE_TYPE_FLOAT );
	NMetaInfo::FinishMetaInfoReport();
}

int SAnimB2::operator&( IXmlSaver &saver )
{
	NMetaInfo::STerminalClassReporter reporter( this, saver );
	saver.AddTypedSuper( (SAnimBase*)(this) );
	saver.Add( "Type", &eType );
	saver.Add( "Action", &nAction );
	saver.Add( "Length", &nLength );
	saver.Add( "Looped", &bLooped );
	saver.Add( "WeaponsToUseWith", &nWeaponsToUseWith );
	saver.Add( "aabb_a", &aabb_a );
	saver.Add( "aabb_d", &aabb_d );
	saver.Add( "MoveSpeed", &fMoveSpeed );

	return 0;
}

int SAnimB2::operator&( IBinSaver &saver )
{
	saver.Add( 1, (SAnimBase*)this );
	saver.Add( 3, &eType );
	saver.Add( 4, &nAction );
	saver.Add( 5, &nLength );
	saver.Add( 6, &bLooped );
	saver.Add( 7, &nWeaponsToUseWith );
	saver.Add( 8, &aabb_a );
	saver.Add( 9, &aabb_d );
	saver.Add( 10, &fMoveSpeed );

	return 0;
}

}
using namespace NDb;
REGISTER_DATABASE_CLASS( STATS_B2_M1, 0x10093480, SAnimB2 )

