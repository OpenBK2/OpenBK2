#include "stdafx.h"

#include "Turret.h"
#include "SerializeOwner.h"
#include <cstddef>
#include <cstring>

void CTurret::SerializeRotation( IBinSaver &saver, const int nChunk, SRotating &rotation )
{
	// Keep the legacy raw-chunk size and offsets, but never save/checksum padding
	// or the unused high bytes of SAIAngle. Reading copies only actual fields back.
	alignas(SRotating) unsigned char data[sizeof(SRotating)] = {};
	if ( saver.IsReading() )
	{
		saver.AddRawData( nChunk, data, sizeof(data) );
		rotation.wCurAngle.allign = 0;
		rotation.wFinalAngle.allign = 0;
	}
	const auto field = [&]( auto &value, const size_t offset )
	{
		if ( saver.IsReading() )
			std::memcpy( &value, data + offset, sizeof(value) );
		else
			std::memcpy( data + offset, &value, sizeof(value) );
	};
	field( rotation.wRotationSpeed, offsetof(SRotating, wRotationSpeed) );
	field( rotation.wCurAngle.wAngle, offsetof(SRotating, wCurAngle) );
	field( rotation.wFinalAngle.wAngle, offsetof(SRotating, wFinalAngle) );
	field( rotation.sign, offsetof(SRotating, sign) );
	field( rotation.startTime, offsetof(SRotating, startTime) );
	field( rotation.endTime, offsetof(SRotating, endTime) );
	field( rotation.bFinished, offsetof(SRotating, bFinished) );
	if ( !saver.IsReading() )
		saver.AddRawData( nChunk, data, sizeof(data) );
}

int CTurret::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CLinkObject*>(this) );
	SerializeRotation( saver, 2, hor );
	SerializeRotation( saver, 3, ver );
	saver.Add( 4, &bCanReturn );
	saver.Add( 5, &bVerAiming );
	saver.Add( 6, &pTracedUnit );
	saver.Add( 7, &pLockingGun );
	saver.Add( 8, &wDefaultHorAngle );
	saver.Add( 9, &bReturnToNULLVerAngle );
	return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

